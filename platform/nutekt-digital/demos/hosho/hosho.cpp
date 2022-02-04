#include "userosc.h"

#include <array>
#include <chrono>

using namespace std::literals::chrono_literals;

struct Item {
  using Top = std::chrono::duration<float>;
  float vol = 1.f;
  Top gate_hold = 500ms;
  Top gate_attack = 2ms;
  Top gate_delay = 0ms;
};

using Items = std::array<Item, 3>;

static Items hosho_items {
  Item{1.f, 20ms, .01ms, 0ms},
  Item{1.f, 250ms /* (B) */, 10ms, 50ms /* (A) */ },
  Item{1.f, 2ms, .30ms, 0ms},
};

constexpr float master_gain = 1.f;

struct State {
  float time = 0.f;
  uint32_t beat_index = 0;
  float noise_mix = .5f;
  uint32_t count = 0;
  uint32_t samplerate = 1;
  float phi0 = 0.f;
};

static State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
}

float attack_shape(const float time, const float tau)
{
  return time < 0.f ? 0.f : 1.f - expf(-time / tau);
}

float osc_shaped_noise() {
  const float aa = fabs(osc_white());
  const float bb = osc_white();
  const float cc = .6 * sqrtf(-2 * logf(aa)) * cosf(M_PI * bb);
  return cc;
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  const float lfo = q31_to_f32(params->shape_lfo);

  const auto get_hosho_signal = [&lfo](const State& state_) -> float {
    const Item& item = hosho_items[state_.beat_index % std::tuple_size<Items>::value];
    const bool is_on =
      state_.time >= item.gate_delay.count() &&
      state_.time < (item.gate_hold.count() + lfo);
    const float vol =
      is_on ? item.vol * attack_shape(state_.time - item.gate_delay.count(), item.gate_attack.count()) :
      0.f;
    const float current = osc_white() * state_.noise_mix + osc_shaped_noise() * (1.f - state_.noise_mix);
    return  vol * current;
  };

  const auto w0 = osc_w0f_for_note(
    (params->pitch)>>8,
    params->pitch & 0xFF);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  float sig_hosho = 0.f;
  for (; yy < yy_end; yy++) {
    sig_hosho = state.count % state.samplerate == 0 ? get_hosho_signal(state) : sig_hosho;

    const float sig_mbira = osc_sinf(state.phi0);
    const float sig_master = master_gain * (
      .8f * sig_hosho +
      .2f * sig_mbira);
    *yy = f32_to_q31(sig_master);

    state.time += k_samplerate_recipf;
    state.count ++;
    state.phi0 += w0;
    state.phi0 -= static_cast<uint32_t>(state.phi0);
  }
}

void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  state.time = 0;
  state.beat_index ++;
}

void OSC_NOTEOFF(
  const user_osc_param_t* const params)
{
  // state.vol0 = 0.f;
}

void OSC_PARAM(uint16_t beat_index, uint16_t value)
{
  switch (beat_index) {
  case k_user_osc_param_id1:
    state.noise_mix = value / 99.f;
    break;
  case k_user_osc_param_id2:
    state.samplerate = value;
    break;
  case k_user_osc_param_shape:
    std::get<1>(hosho_items).gate_delay = param_val_to_f32(value) * 200ms;
    break;
  case k_user_osc_param_shiftshape:
    std::get<1>(hosho_items).gate_hold = param_val_to_f32(value) * 500ms;
    break;
  }
}
