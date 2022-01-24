#include "userosc.h"

#include <vector>
#include <array>

struct State {
  bool is_on = false;

  float disto_amount = 1.f;

  float vol0 = 1.f;
  size_t index0 = 0;
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
  const float* wave0 =
    state.index0 % 4 == 0 ? wavesA[0] :
    state.index0 % 4 == 1 ? wavesB[0] :
    state.index0 % 4 == 2 ? wavesC[0] :
    wavesD[0];

  const auto w0 = osc_w0f_for_note(
    (params->pitch)>>8,
    params->pitch & 0xFF);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++) {
    float sig = 0.f;
    if (state.is_on) sig += state.vol0 * osc_wave_scanf(wave0, state.phi0);

    sig = osc_softclipf(0.0f, state.disto_amount * sig);

    // sig = osc_sat_schetzenf(state.disto_amount * sig);
    *yy = f32_to_q31(sig);

    state.phi0 += w0;
    state.phi0 -= static_cast<uint32_t>(state.phi0);
  }

}

void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  state.is_on = true;
}

void OSC_NOTEOFF(
  const user_osc_param_t* const params)
{
  state.is_on = false;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  static const uint8_t k_a_thr = k_waves_a_cnt;

  switch (index) {
  case k_user_osc_param_id1:
    state.index0 = value % 4;
    break;
  case k_user_osc_param_id2:
    state.disto_amount = 1 + param_val_to_f32(value);
    break;
  case k_user_osc_param_shape:
    state.vol0 = clip01f(value * 0.01f);
    break;
  case k_user_osc_param_shiftshape:
    state.disto_amount = 1 + 4.f * param_val_to_f32(value);
    break;
  }
}
