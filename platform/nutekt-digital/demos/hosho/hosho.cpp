#include "userosc.h"

#include <vector>
#include <array>

struct State {
  float time = 0.f;
  uint32_t index = 0;
};

State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  // const auto w0 = osc_w0f_for_note(
  //   (params->pitch)>>8,
  //   params->pitch & 0xFF);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++) {
    float sig = 0.f;
    float vol = state.time < .5f ? 1.f : 0.f;
    switch (state.index) {
      default:
      case 0:
        vol *= 0.f;
        break;
      case 1:
        vol *= .5f;
        break;
      case 2:
        vol *= 1.f;
        break;
    }
    sig += vol * osc_white();
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
  // switch (index) {
  // case k_user_osc_param_id1:
  //   state.index0 = value % 4;
  //   break;
  // case k_user_osc_param_shape:
  //   state.vol0 = clip01f(value * 0.01f);
  //   break;
  // case k_user_osc_param_shiftshape:
  //   state.disto_amount = pow(10.f, 12.f * param_val_to_f32(value));
  //   break;
  // }
}
