#include "userosc.h"

#include <vector>
#include <array>

struct State {
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

const float* get_wavetable(size_t index)
{
  static const uint8_t k_a_thr = k_waves_a_cnt;
  static const uint8_t k_b_thr = k_a_thr + k_waves_b_cnt;
  static const uint8_t k_c_thr = k_b_thr + k_waves_c_cnt;
  static const uint8_t k_d_thr = k_c_thr + k_waves_d_cnt;

  const float * const * table;

  if (index < k_a_thr) {
    table = wavesA;
  }
  else if (index < k_b_thr) {
    table = wavesB;
    index -= k_a_thr;
  }
  else if (index < k_c_thr) {
    table = wavesC;
    index -= k_b_thr;
  }
  else if (index < k_d_thr) {
    table = wavesD;
    index -= k_c_thr;
  }
  else {
    table = wavesE;
    index -= k_d_thr;
  }

  return table[index];
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  const float* wave0 = get_wavetable(state.index0);

  const auto w0 = osc_w0f_for_note(
    (params->pitch)>>8,
    params->pitch & 0xFF);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++) {
    float sig = 0.f;
    sig += state.vol0 * osc_wave_scanf(wave0, state.phi0);

    sig = osc_softclipf(0.0f, state.disto_amount * sig);
    sig = osc_softclipf(0.1f, state.disto_amount * sig);

    // sig = osc_sat_schetzenf(state.disto_amount * sig);
    *yy = f32_to_q31(sig);

    state.phi0 += w0;
    state.phi0 -= static_cast<uint32_t>(state.phi0);
  }

}

void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  state.vol0 = 1.f;
}

void OSC_NOTEOFF(
  const user_osc_param_t* const params)
{
  state.vol0 = 0.f;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  static const uint8_t k_a_thr = k_waves_a_cnt;

  switch (index) {
  case k_user_osc_param_id1:
    state.index0 = value % 4;
    break;
  case k_user_osc_param_shape:
    state.vol0 = clip01f(value * 0.01f);
    break;
  case k_user_osc_param_shiftshape:
    state.disto_amount = pow(10.f, 12.f * param_val_to_f32(value));
    break;
  }
}
