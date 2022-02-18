#include "userosc.h"

#include <array>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <tuple>

#include "mersenne.h"

using namespace std::literals::chrono_literals;

using Note = uint8_t;
using Chord = std::array<Note, 2>;
using Chords = std::array<Chord, 7>;

constexpr Chords mbira_midi_chords {
  Chord{0, 7} /* do */,
  {2, 9} /* re */,
  {4, 11} /* mi */,
  {5, 0} /* fa */,
  {7, 2} /* sol */,
  {9, 4} /* la */,
  {11, 5} /* si */,
};

using Pattern = std::array<size_t, 12>;

constexpr Pattern mbira_pattern {
  0, 2, 4,
  0, 2, 5,
  0, 3, 5,
  1, 3, 5,
};

using Top = std::chrono::duration<float>;

struct Item {
  float vol = 1.f;
  float gate_hold_ratio = .5f;
  Top gate_attack = 2ms;
  float gate_delay_ratio = 0.f;
};

using Items = std::array<Item, 3>;

static Items hosho_items {
  Item{1.f, .1, 1ms, 0.f},
  {1.f, .25 /* (B) */, 50ms, .2f /* (A) */ },
  {1.f, .02, .30ms, 0.f},
};

constexpr float master_gain = 1.f;

struct State {
  float time = 0.f;
  float prev_time = -1.f;
  size_t index = 0;
  static constexpr float noise_mix = .95f;
  uint32_t count = 0;
  uint32_t samplerate = 1;
  float phi0 = 0.f;
  static constexpr float mbira_hold_ratio = .95f;
  float master_hosho_mbira_mix = .5f;
  size_t mbira_song = 0;
  float mbira_current_vol = 1.f;
  uint8_t mbira_wave = 0;
  float mbira_random_vol = .5f;
  size_t rng_buffer_index = 0;
};

static State state;
static MersenneTwister rng(103424);

using RngBuffers = std::array<std::array<float, 12>, 100>;
static RngBuffers rng_buffers;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();

  { // reseed rng_buffers
    for (auto& rng_buffer : rng_buffers)
      for (auto& rng_value : rng_buffer)
        rng_value = rng.uniform();
  }
}

float attack_shape(const float time, const float tau)
{
  return time < 0.f ? 0.f : 1.f - expf(-time / tau);
}

float osc_shaped_noise() {
  const float aa = fabs(osc_white());
  const float bb = osc_white();
  const float cc = .6 * sqrtf(-2 * logf(aa)) * cosf(M_PI * bb);
  return cc;
}

size_t note_distance(const Note aa_, const Note bb_)
{
  const auto aa = static_cast<int32_t>(aa_);
  const auto bb = static_cast<int32_t>(bb_);
  return std::min(
    abs(aa - bb),
    abs(bb - aa));
}


auto crossfade(const float aa, const float bb, const float mix_) -> float
{
  const auto mix = clip01f(mix_);
  return
    mix < .5f ? aa + mix * bb * 2.f :
    2.f * (1 - mix) * aa + bb;
  // const auto mix_ = 1.f - mix;
  // return aa * mix_ + bb * mix;
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  const Top tempo = state.prev_time >= 0 ? (state.prev_time * 1s) : 0s;
  const float lfo = q31_to_f32(params->shape_lfo);

  const auto get_hosho_signal = [&lfo, &tempo](const State& state_) -> float {
    const Item& item = hosho_items[state_.index % std::tuple_size<Items>::value];
    const Top gate_delay = item.gate_delay_ratio * tempo;
    const Top gate_hold = item.gate_hold_ratio * tempo;
    const bool is_on =
      state_.time >= gate_delay.count() &&
      state_.time < (gate_hold.count() + lfo);
    const float vol =
      is_on ? item.vol * attack_shape(state_.time - gate_delay.count(), item.gate_attack.count()) :
      0.f;
    const float current = osc_white() * state_.noise_mix + osc_shaped_noise() * (1.f - state_.noise_mix);
    return  vol * current;
  };

  const Note in_note = (params->pitch >> 8) % 152;
  const auto in_chord_note = in_note % 12;
  const auto in_octave_note = in_note / 12;
  // const auto w0 = osc_w0f_for_note(
  //   midi_note_in,
  //   params->pitch & 0xFF); // midi in
  // const auto w0 = 440.f * k_samplerate_recipf; // A 440Hz
  const auto mbira_index = state.index / 4;
  const auto current_chord =
    mbira_pattern[mbira_index % std::tuple_size<Pattern>::value] +
    state.mbira_song;
  const auto chord_notes = mbira_midi_chords[current_chord % std::tuple_size<Chords>::value];
  const auto aa_chord_note = std::get<0>(chord_notes);
  const auto bb_chord_note = std::get<1>(chord_notes);

  const auto dist_in_aa = note_distance(in_chord_note, aa_chord_note);
  const auto dist_in_bb = note_distance(in_chord_note, bb_chord_note);

  const auto chord_note = dist_in_aa < dist_in_bb ? aa_chord_note : bb_chord_note;

  const auto midi_note = in_octave_note * 12 + chord_note;
  const auto w0 = osc_w0f_for_note(midi_note, 0);


  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  float sig_hosho = 0.f;
  for (; yy < yy_end; yy++) {
    sig_hosho = state.count % state.samplerate == 0 ? get_hosho_signal(state) : sig_hosho;

    const float aa = attack_shape(state.time, 10e-3f);
    const float delta_mbira = state.time - state.mbira_hold_ratio * tempo.count();
    const float bb = delta_mbira < 0 ? 1.f : expf(-delta_mbira / 1e-2f);
    const float sig_mbira = state.mbira_current_vol * osc_bl_sawf(state.phi0, state.mbira_wave % 7) * bb * aa;

    const float sig_master = master_gain * crossfade(
      sig_hosho,
      sig_mbira,
      state.master_hosho_mbira_mix);

    *yy = f32_to_q31(sig_master);

    state.time += k_samplerate_recipf;
    state.count ++;
    state.phi0 += w0;
    state.phi0 -= static_cast<uint32_t>(state.phi0);
  }
}

void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  constexpr auto rng_buffers_max = std::tuple_size<RngBuffers>::value;
  constexpr auto rng_buffer_max = std::tuple_size<RngBuffers::value_type>::value;
  const auto& rng_buffer = rng_buffers[state.rng_buffer_index % rng_buffers_max];
  const float rng_value = rng_buffer[state.index % rng_buffer_max];

  const float foo = state.mbira_random_vol;
  const float low_bound = foo < .5f ? 0.f : (2.f * foo - 1.f);
  const float high_bound = foo < .5f ? (2.f * foo) : 1.f;
  float mbira_volume = (1.f - rng_value) * low_bound + rng_value * high_bound;
  if (mbira_volume < .2f) mbira_volume = 0.f;
  state.mbira_current_vol = mbira_volume;
  state.prev_time = state.time;
  state.time = 0;
  state.phi0 = 0;
  state.index ++;
}

void OSC_NOTEOFF(
  const user_osc_param_t* const params)
{
  // state.vol0 = 0.f;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index) {
    case k_user_osc_param_id1: /* Fade */
      state.master_hosho_mbira_mix = value / 99.f;
      break;
    case k_user_osc_param_id2: /* Song */
      state.mbira_song = value;
      break;
    // case k_user_osc_param_id3: /* Nois */
    //   state.noise_mix = value / 99.f;
    //   break;
    case k_user_osc_param_id3: /* GDel */
      std::get<1>(hosho_items).gate_delay_ratio = value / 99.;
      break;
    case k_user_osc_param_id4: /* Splr */
      state.samplerate = value;
      break;
    case k_user_osc_param_id5: /* Wtbl */
      state.mbira_wave = value;
      break;
    case k_user_osc_param_id6: /* GHol */
      std::get<1>(hosho_items).gate_hold_ratio = value / 99.;
      break;

    case k_user_osc_param_shape: /* Seed (A) */
      state.rng_buffer_index = value;
      break;
    case k_user_osc_param_shiftshape: /* VRng (B) */
      state.mbira_random_vol = param_val_to_f32(value);
      break;
  }
}
