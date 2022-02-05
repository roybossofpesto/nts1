#include "userosc.h"

#include <array>
#include <chrono>
#include <vector>
#include <cstdlib>

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
  Top gate_hold = 500ms;
  Top gate_attack = 2ms;
  Top gate_delay = 0ms;
};

using Items = std::array<Item, 3>;

static Items hosho_items {
  Item{1.f, 20ms, .01ms, 0ms},
  Item{1.f, 250ms /* (B) */, 50ms, 50ms /* (A) */ },
  Item{1.f, 2ms, .30ms, 0ms},
};

constexpr float master_gain = 1.f;
constexpr float master_hosho_versus_mbira = 0.f;

struct State {
  float time = 0.f;
  float prev_time = -1.f;
  size_t index = 0;
  float noise_mix = .5f;
  uint32_t count = 0;
  uint32_t samplerate = 1;
  float phi0 = 0.f;
  Top mbira_hold = 100ms;
};

static State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
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

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  const float lfo = q31_to_f32(params->shape_lfo);

  const auto get_hosho_signal = [&lfo](const State& state_) -> float {
    const Item& item = hosho_items[state_.index % std::tuple_size<Items>::value];
    const bool is_on =
      state_.time >= item.gate_delay.count() &&
      state_.time < (item.gate_hold.count() + lfo);
    const float vol =
      is_on ? item.vol * attack_shape(state_.time - item.gate_delay.count(), item.gate_attack.count()) :
      0.f;
    const float current = osc_white() * state_.noise_mix + osc_shaped_noise() * (1.f - state_.noise_mix);
    return  vol * current;
  };

  const Note in_note = (params->pitch >> 8) % 152;
  const auto in_chord_note = in_note % 12;
  // const auto w0 = osc_w0f_for_note(
  //   midi_note_in,
  //   params->pitch & 0xFF); // midi in
  // const auto w0 = 440.f * k_samplerate_recipf; // A 440Hz
  const auto mbira_index = state.index / 4;
  const auto current_chord = mbira_pattern[mbira_index % std::tuple_size<Pattern>::value];
  const auto chord_notes = mbira_midi_chords[current_chord % std::tuple_size<Chords>::value];
  const auto aa_chord_note = std::get<0>(chord_notes);
  const auto bb_chord_note = std::get<1>(chord_notes);

  const auto dist_in_aa = note_distance(in_chord_note, aa_chord_note);
  const auto dist_in_bb = note_distance(in_chord_note, bb_chord_note);

  const auto chord_note = dist_in_aa < dist_in_bb ? aa_chord_note : bb_chord_note;
  const auto midi_note = 60 + chord_note;
  const auto w0 = osc_w0f_for_note(midi_note, 0);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  float sig_hosho = 0.f;
  for (; yy < yy_end; yy++) {
    sig_hosho = state.count % state.samplerate == 0 ? get_hosho_signal(state) : sig_hosho;

    const float sig_mbira = state.time < state.mbira_hold.count() ? osc_sinf(state.phi0) : 0.f;

    const float sig_master = master_gain * (
      master_hosho_versus_mbira * sig_hosho +
      (1.f - master_hosho_versus_mbira) * sig_mbira);

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
  const Top tempo = state.prev_time >= 0 ? (state.prev_time * 1s) : 0s;
  switch (index) {
  case k_user_osc_param_id1:
    state.noise_mix = value / 99.f;
    break;
  case k_user_osc_param_id2:
    state.samplerate = value;
    break;
  case k_user_osc_param_shape: /* (A) */
    std::get<1>(hosho_items).gate_delay = param_val_to_f32(value) * tempo;
    break;
  case k_user_osc_param_shiftshape: /* (B) */
    std::get<1>(hosho_items).gate_hold = param_val_to_f32(value) * tempo;
    break;
  }
}
