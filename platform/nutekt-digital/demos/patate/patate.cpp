#include "userosc.h"

struct State {
  bool is_on = false;
};

State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
 // (void)platform;
  // (void)api;
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;

  for (size_t kk=0; kk<frames; kk++) {
    const float sig = state.is_on ? osc_white() : 0.f;
    yy[kk] = f32_to_q31(sig);
  }

}

void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  state.is_on = true;
  // s_waves.state.flags |= Waves::k_flag_reset;
}

void OSC_NOTEOFF(
  const user_osc_param_t* const params)
{
  state.is_on = false;
  // (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{

}
