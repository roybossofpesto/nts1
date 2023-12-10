#include "userosc.h"
// #include <custom/mersenne.h>
#include <custom/interp.h>
#include <array>

struct State
{
  static constexpr const size_t max_channels = 4;

  float shape_amount = 1.f;
  float shiftshape_amount = 1.f;
  size_t wave_index = 0;

  size_t top_now = 0;
  size_t top_last_noteon = 0;

  struct Channel
  {
    float volume = 1.f;
    float frequency = 0.f;
    float position = 0.f;
  };

  std::array<Channel, max_channels> channels;

  // struct Drone
  // {
  //   float frequency = 0.f;
  //   float position = 0.f;
  //   size_t top_next_rng = 0;
  // };

  // Drone drone;
};

static State state = State();
// static custom::MersenneTwister rng = custom::MersenneTwister();

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
  // rng = custom::MersenneTwister();
}

// const float *get_wavetable(size_t index)
// {
//   switch (index % 6)
//   {
//   case 0:
//     return wavesA[0];
//   case 1:
//     return wavesB[0];
//   case 2:
//     return wavesC[0];
//   case 3:
//     return wavesD[0];
//   case 4:
//     return wavesE[0];
//   case 5:
//     return wavesF[0];
//   }
//   return wavesA[0];
// }

void OSC_CYCLE(
    const user_osc_param_t *const params,
    int32_t *yy_,
    const uint32_t frames)
{
  // const float *wave = get_wavetable(state.wave_index);

  const auto frequency_main = osc_w0f_for_note(
      params->pitch >> 8,
      params->pitch & 0xFF);

  state.channels[0].frequency = 1.f / 1.f * frequency_main;
  state.channels[1].frequency = 1.f / 2.f * frequency_main;
  state.channels[2].frequency = 3.f / 2.f * frequency_main;
  state.channels[3].frequency = 5.f / 4.f * frequency_main;

  // if (state.drone.top_next_rng < state.top_now)
  // {
  //   const auto note_rng = rng.uniform_uchar(120, 150);
  //   state.drone.frequency = osc_w0f_for_note(note_rng, 0);
  //   state.drone.top_next_rng = state.top_now + k_samplerate * 1.0f;
  // }

  auto yy = static_cast<q31_t *>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++)
  {
    // const float delta = static_cast<float>(state.top_now - state.top_last_noteon) * k_samplerate_recipf;
    // const float release = exp(-delta / (state.shape_amount * .075f + .015f));
    // state.channels[0].volume = release;
    // state.channels[1].volume = release;
    // state.channels[2].volume = .5f * release;
    // state.channels[3].volume = .25f * release;

    const float foo = clip01f(state.shape_amount);
    const float bar = clip01f(state.shiftshape_amount);
    state.channels[0].volume = 1;
    state.channels[1].volume = custom::smoothstep(.1f, .3f, foo);
    state.channels[2].volume = custom::smoothstep(.4f, .6f, foo);
    state.channels[3].volume = custom::smoothstep(.7f, .9f, foo);

    // accumulate channel signals
    float sig = 0.f;
    float vol = 0.f;
    for (const auto &channel : state.channels)
    {
      const float aa = osc_sinf(channel.position);
      const float bb = osc_bl_sawf(channel.position, state.wave_index);
      const float cc = linintf(clip01f(state.shiftshape_amount), aa, bb);
      sig += channel.volume * cc;
      vol += channel.volume;
    }
    if (vol)
      sig /= vol;

    // sig += channel.volume * osc_wave_scanf(wave, channel.position);
    // sig += state.shiftshape_amount * ;

    // // apply disto
    // state.disto_amount = std::max(state.disto_amount, 1.f);
    // sig = osc_softclipf(0.1f, state.disto_amount * sig);

    *yy = f32_to_q31(sig);

    for (auto &channel : state.channels)
      channel.position += channel.frequency;

    // state.drone.position += state.drone.frequency;

    state.top_now += 1;
  }
}

void OSC_NOTEON(
    const user_osc_param_t *const params)
{
  state.channels[0].position = 0.f;
  state.channels[1].position = 0.f;
  state.channels[2].position = 0.f;
  state.channels[3].position = 0.f;
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
    break;
  }
}
