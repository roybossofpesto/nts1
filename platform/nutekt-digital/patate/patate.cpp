#include "userosc.h"
#include "custom.h"

#include <array>
// #include <random>

struct State
{
  static constexpr const size_t max_channels = 5;

  // using Rng = std::mt19937;
  // using UniformUInt8 = std::uniform_int_distribution<uint8_t>;

  float shape_amount = 1.f;
  float shiftshape_amount = 1.f;
  size_t wave_index = 0;
  // Rng rng = Rng(0x1234abcd);
  // UniformUInt8 dist_note = UniformUInt8(64, 96);

  struct Channel
  {
    float volume = 1.f;
    float frequency = 0.f;
    float position = 0.f;
  };

  std::array<Channel, max_channels> channels;
  size_t top_now = 0;
  size_t top_last_noteon = 0;
};

static State state = State();

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
}

const float *get_wavetable(size_t index)
{
  switch (index % 6)
  {
  case 0:
    return wavesA[0];
  case 1:
    return wavesB[0];
  case 2:
    return wavesC[0];
  case 3:
    return wavesD[0];
  case 4:
    return wavesE[0];
  case 5:
    return wavesF[0];
  }
  return wavesA[0];
}

void OSC_CYCLE(
    const user_osc_param_t *const params,
    int32_t *yy_,
    const uint32_t frames)
{
  // const float *wave = get_wavetable(state.wave_index);

  const auto frequency_main = osc_w0f_for_note(
      params->pitch >> 8,
      params->pitch & 0xFF);

  // const auto note_rng = state.dist_note(state.rng);
  // const auto frequency_rng = osc_notehzf(note_rng);
  const auto frequency_rng = frequency_main; // osc_notehzf(note_rng);

  // const float mask = state.shiftshape_amount < .5f ? 1.f : 0.f;

  state.channels[0].frequency = 1.f / 1.f * frequency_main;
  state.channels[1].frequency = 1.f / 1.f * frequency_main;
  state.channels[2].frequency = 3.f / 1.f * frequency_main;
  state.channels[3].frequency = 5.f / 4.f * frequency_main;
  state.channels[4].frequency = frequency_rng;

  auto yy = static_cast<q31_t *>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++)
  {
    const float delta = static_cast<float>(state.top_now - state.top_last_noteon) * k_samplerate_recipf;
    const float release = exp(-delta / (state.shape_amount * .1 + .025f));

    state.channels[0].volume = release;
    state.channels[1].volume = release;
    state.channels[2].volume = release;
    state.channels[3].volume = release;
    state.channels[4].volume = state.shiftshape_amount;

    // accumulate channel signals
    float sig = 0.f;
    float vol = 0.f;
    for (const auto &channel : state.channels)
    {
      // sig += channel.volume * osc_wave_scanf(wave, channel.position);
      sig += channel.volume * osc_bl_sawf(channel.position, state.wave_index);
      vol += channel.volume;
    }
    sig /= state.channels.size();

    // // apply disto
    // state.disto_amount = std::max(state.disto_amount, 1.f);
    // sig = osc_softclipf(0.1f, state.disto_amount * sig);

    *yy = f32_to_q31(sig);

    for (auto &channel : state.channels)
      channel.position += channel.frequency;
    state.top_now += 1;
  }
}

void OSC_NOTEON(
    const user_osc_param_t *const params)
{
  state.channels[0].position = 0.f;
  // state.channels[1].position = 0.f;
  // state.channels[2].position = 0.f;
  // state.channels[3].position = 0.f;
  state.top_last_noteon = state.top_now;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  // state.channels[0].volume = 0.f;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case k_user_osc_param_id1:
    state.wave_index = value % 7;
    break;
  case k_user_osc_param_shape:
    state.shape_amount = param_val_to_f32(value);
    break;
  case k_user_osc_param_shiftshape:
    state.shiftshape_amount = param_val_to_f32(value);
    // state.wave_index = value % 7;
    break;
  }
}
