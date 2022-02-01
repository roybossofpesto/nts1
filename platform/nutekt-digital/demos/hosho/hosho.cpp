#include "userosc.h"

#include <vector>
#include <array>
#include <array>
// #include <cassert>

struct State {
  float time = 0.f;
  uint32_t index = 0;
  // bool use_mean = true;

  // using Buffer = std::array<float, 32>;
  // Buffer samples;
  // Buffer::iterator current_sample = samples.begin();
  // float accum = 0.f;
};

State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
  // for (auto& sample : state.samples) sample = 0.f;
}

float attach_shape(const float time, const float tau) {
  return 1.f - expf(-time / tau);
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  constexpr float gate_decay = .05f; // seconds
  // const auto w0 = osc_w0f_for_note(
  //   (params->pitch)>>8,
  //   params->pitch & 0xFF);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++) {
    float sig = 0.f;
    float vol = state.time < gate_decay ? 1.f : 0.f;
    switch (state.index) {
      default:
      case 0:
        vol *= 1.f;
        break;
      case 1:
        vol *= attach_shape(state.time, .1f);
        break;
      case 2:
        vol *= .3f;
        break;
    }

    // const float prev = *state.current_sample;

    // state.current_sample ++;
    // if (state.current_sample >= state.samples.end())
      // state.current_sample = state.samples.begin();

    const float current = osc_white();
    // *state.current_sample = current;
    // state.accum += current;

    // constexpr auto buffer_size = 1;//std::tuple_size<State::Buffer>::value;
    const float shaped_noise = current;

    // state.accum -= prev;

    sig += vol * shaped_noise;
    //osc_sinf(state.phi0);
    //
    // sig = osc_softclipf(0.0f, state.disto_amount * sig);
    // sig = osc_softclipf(0.1f, state.disto_amount * sig);
    //
    // sig = osc_sat_schetzenf(state.disto_amount * sig);
    *yy = f32_to_q31(sig);

    // state.phi0 += w0;
    // state.phi0 -= static_cast<uint32_t>(state.phi0);

    state.time += k_samplerate_recipf;
  }

}

void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  state.time = 0;
  state.index ++;
  state.index %= 3;
}

void OSC_NOTEOFF(
  const user_osc_param_t* const params)
{
  // state.vol0 = 0.f;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index) {
  // case k_user_osc_param_id1:
  //   state.index0 = value % 4;
  //   break;
  case k_user_osc_param_shape:
    // state.use_mean = clip01f(value * 0.01f) > .5f ? true : false;
    break;
  // case k_user_osc_param_shiftshape:
  //   state.disto_amount = pow(10.f, 12.f * param_val_to_f32(value));
  //   break;
  }
}
