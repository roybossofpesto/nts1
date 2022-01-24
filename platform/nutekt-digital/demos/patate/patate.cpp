#include "userosc.h"

#include <vector>
#include <array>

struct State {
  bool is_on = false;

  const float* wave0 = wavesC[0];
  float phi0 = 0.f;
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

  const auto w0 = osc_w0f_for_note(
    (params->pitch)>>8,
    params->pitch & 0xFF);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++) {
    const float sig = !state.is_on ? osc_white() : osc_wave_scanf(state.wave0, state.phi0);
    // sig += ;


    *yy = f32_to_q31(sig);

    state.phi0 += w0;
    state.phi0 -= static_cast<uint32_t>(state.phi0);
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
