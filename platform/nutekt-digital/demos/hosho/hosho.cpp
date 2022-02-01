#include "userosc.h"

#include <array>
#include <chrono>

using namespace std::literals::chrono_literals;

struct Item {
  using Top = std::chrono::duration<float>;
  float vol = 1.f;
  Top gate_hold = 40ms;
  Top gate_attack = 2ms;
};

using Items = std::array<Item, 3>;

static Items hosho_items {
  Item{.1f, 20ms, .1ms},
  Item{1.f, 80ms, .5s},
  Item{.05f, 10ms, .1ms},
};

constexpr float master_gain = 10.f;

struct State {
  float time = 0.f;
  uint32_t beat_index = 0;
  float noise_mix = .5f;
  uint32_t count = 0;
};

static State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
}

float attack_shape(const float time, const float tau)
{
  return 1.f - expf(-time / tau);
}

float osc_shaped_noise() {
  const float aa = fabs(osc_white());
  const float bb = osc_white();
  const float cc = sqrtf(-2 * logf(aa)) * cosf(M_PI * bb);
  return cc;
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  const float lfo = q31_to_f32(params->shape_lfo);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  float sig = 0.f;
  for (; yy < yy_end; yy++) {

    const auto get_signal = [&lfo](const State& state_) -> float {
      const Item& item = hosho_items[state_.beat_index % std::tuple_size<Items>::value];
      const bool is_on = state_.time < (item.gate_hold.count() + lfo);
      const float vol =
        is_on ? item.vol * attack_shape(state_.time, item.gate_attack.count()) :
        0.f;
      const float current = osc_white() * state_.noise_mix + osc_shaped_noise() * (1.f - state_.noise_mix);
      return  vol * current;
    };

    sig = state.count % 10 == 0 ? get_signal(state) : sig;
    sig *= master_gain;

    *yy = f32_to_q31(sig);

    state.time += k_samplerate_recipf;
    state.count ++;
  }
}

void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  state.time = 0;
  state.beat_index ++;
  state.beat_index %= 3;
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
  case k_user_osc_param_shape:
    std::get<1>(hosho_items).gate_attack = 200ms + param_val_to_f32(value) * 400ms;
    break;
  case k_user_osc_param_shiftshape:
    std::get<1>(hosho_items).gate_hold = param_val_to_f32(value) * 500ms;
    break;
  }
}
