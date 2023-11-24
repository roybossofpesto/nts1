#include "userosc.h"

#include <array>

struct State
{
  static constexpr const size_t max_channels = 4;

  float disto_amount = 1.f;
  size_t wave_index = 0;

  struct Channel
  {
    float volume = 1.f;
    float dphi = 0.f;
    float phi = 0.f;
  };

  std::array<Channel, max_channels> channels;
};

State state;

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
  const float *wave = get_wavetable(state.wave_index);

  const auto dphi = osc_w0f_for_note(
      params->pitch >> 8,
      params->pitch & 0xFF);

  state.channels[0].dphi = 1.f / 1.f * dphi;
  state.channels[1].dphi = 1.f / 2.f * dphi;
  state.channels[2].dphi = 3.f / 4.f * dphi;
  state.channels[3].dphi = 5.f / 4.f * dphi;

  auto yy = static_cast<q31_t *>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++)
  {

    // accumulate channel signals
    float sig = 0.f;
    float vol = 0.f;
    for (const auto &channel : state.channels)
    {
      sig += channel.volume * osc_wave_scanf(wave, channel.phi);
      vol += channel.volume;
    }
    if (vol)
      sig /= vol;

    // apply disto
    state.disto_amount = std::max(state.disto_amount, 1.f);
    sig = osc_softclipf(0.1f, state.disto_amount * sig);

    *yy = f32_to_q31(sig);

    for (auto &channel : state.channels)
      channel.phi += channel.dphi;
  }
}

void OSC_NOTEON(
    const user_osc_param_t *const params)
{
  state.channels[0].phi = 0.f;
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
    state.wave_index = value % 4;
    break;
  case k_user_osc_param_shape:
  {
    float aa = param_val_to_f32(value) * 3;
    state.channels[0].volume = 1;
    state.channels[1].volume = (aa < 0) ? 0 : (aa < 1) ? (aa - 0) : 1;
    state.channels[2].volume = (aa < 1) ? 0 : (aa < 2) ? (aa - 1) : 1;
    state.channels[3].volume = (aa < 2) ? 0 : (aa < 3) ? (aa - 2) : 1;
  }
  break;
  case k_user_osc_param_shiftshape:
    state.disto_amount = pow(10.f, 4 * param_val_to_f32(value));
    break;
  }
}
