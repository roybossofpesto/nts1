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

constexpr Items hosho_items {
  Item{.1f, 20ms, .1ms},
  Item{1.f, 80ms, .5s},
  Item{.05f, 10ms, .1ms},
};

struct State {
  float time = 0.f;
  uint32_t index = 0;
};

State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
}

float attack_shape(const float time, const float tau)
{
  return 1.f - expf(-time / tau);
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++) {
    float sig = 0.f;

    const Item& item = hosho_items[state.index % 3];
    const bool is_on = state.time < item.gate_hold.count();
    const float vol =
      is_on ? item.vol * attack_shape(state.time, item.gate_attack.count()) :
      0.f;

    const float current = 10.f * osc_white();
    sig += vol * current;

    *yy = f32_to_q31(sig);

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
    // state.second_tau = param_val_to_f32(value) * .2f;
    break;
  // case k_user_osc_param_shiftshape:
  //   state.disto_amount = pow(10.f, 12.f * param_val_to_f32(value));
  //   break;
  }
}
