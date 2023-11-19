#include "userosc.h"

#include <array>
// #include <vector>
// #include <map>
// #include <cstdlib>

// void *__dso_handle = 0;
// void *_sbrk = 0;

// void * operator new(std::size_t n)
// {
//   void * const p = std::malloc(n);
//   // handle p == 0
//   return p;
// }

// void operator delete(void * p) // or delete(void *, std::size_t)
// {
//   std::free(p);
// }

struct State
{
  static constexpr const size_t max_channels = 3;

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

  state.channels[0].dphi = 1.0 * dphi;
  state.channels[1].dphi = .5f * dphi;
  state.channels[2].dphi = .666f * dphi;

  auto yy = static_cast<q31_t *>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++)
  {

    // accumulate channel signals
    float sig = 0.f;
    for (const auto &channel : state.channels)
      sig += channel.volume * osc_wave_scanf(wave, channel.phi);
    sig /= State::max_channels;

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
    float aa = param_val_to_f32(value);
    state.channels[1].volume = (aa < .5) ? (2 * aa) : 1;
    state.channels[2].volume = (aa < .5) ? 0 : (2 * aa - 1);
  }
  break;
  case k_user_osc_param_shiftshape:
    state.disto_amount = pow(10.f, 4 * param_val_to_f32(value));
    break;
  }
}
