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
  static constexpr const size_t max_channels = 16;

  float volume = 0.f;
  float disto_amount = 1.f;
  size_t wave_index = 0;

  struct Channel
  {
    uint16_t pitch = 0;
    float dphi = 0.f;
    float phi = 0.f;
  };

  // size_t num_channels = 0;
  // size_t kk = 0;
  std::array<Channel, max_channels> channels;
};

State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
  // state.num_channels = 1;
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
  const uint16_t pitch = params->pitch;

  // bool found_any = false;
  // for (const auto channel : state.channels)
  //   if (pitch == channel.pitch)
  //     found_any = true;

  // if (!found_any)
  {
    const auto dphi = osc_w0f_for_note(
        pitch >> 8,
        pitch & 0xFF);
    // state.num_channels %= State::max_channels;
    // state.channels[0] = State::Channel{
    //     pitch,
    //     dphi,
    //     0,
    // };

    for (auto &channel : state.channels) {
      state.channels[0].dphi = dphi;
    }

    // state.num_channels++;
  }

  auto yy = static_cast<q31_t *>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++)
  {

    // accumulate channel signals
    float sig = 0.f;
    size_t kk = 0;
    for (const auto &channel : state.channels) {
      // if (kk++ >= state.num_channels)
      //   continue;
      sig += state.volume * osc_wave_scanf(wave, channel.phi);
    }
    sig /= State::max_channels;

    // apply disto
    sig = osc_softclipf(0.0f, state.disto_amount * sig);
    sig = osc_softclipf(0.1f, state.disto_amount * sig);
    // sig = osc_sat_schetzenf(state.disto_amount * sig);

    *yy = f32_to_q31(sig);

    // kk = 0;
    for (auto &channel : state.channels) {
      // if (kk++ >= state.num_channels)
      //   continue;
      channel.phi += channel.dphi;
    }
  }
}

void OSC_NOTEON(
    const user_osc_param_t *const params)
{
  // auto &channel = state.channels[state.channel_index];
  // channel.vol = 1.f;
  // channel.phi = 0.f;
  state.volume = 1.f;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  // auto &channel = state.channels[state.channel_index];
  state.volume = 0.f;
  // state.num_channels = 0;

  // channel.vol = 0.f;
  // state.channel_index++;
  // state.channel_index %= State::num_channels;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index)
  {
  case k_user_osc_param_id1:
    state.wave_index = value % 4;
    break;
  case k_user_osc_param_shape:
    // state.volume = clip01f(value * 0.01f);
    // state.wave_index = value % 4;
    break;
  case k_user_osc_param_shiftshape:
    state.disto_amount = pow(10.f, 12.f * param_val_to_f32(value));
    break;
  }
}
