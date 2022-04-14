#include "userosc.h"

#include <array>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <tuple>

using namespace std::literals::chrono_literals;

using Note = uint8_t;

struct Osc {
  float phi = 0.f;
  void update(const float ww);
};

void Osc::update(const float ww)
{
  phi += ww;
  phi -= static_cast<uint32_t>(phi);
}

struct State {
  size_t index = 0;
  float time = 0.f;
  Osc osc0;
  // Osc osc1;
};

static State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  const Note in_note = (params->pitch >> 8) % 152;

  const auto w0 = osc_w0f_for_note(in_note, 0);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  float sig_hosho = 0.f;
  for (; yy < yy_end; yy++) {
    const float sig_master = osc_sinf(state.osc0.phi);

    *yy = f32_to_q31(sig_master);

    state.time += k_samplerate_recipf;
    state.osc0.update(w0);
    // state.osc1.update(w1);
  }
}


void OSC_NOTEON(
  const user_osc_param_t* const /*params*/)
{
  state.index ++;

  if (state.index >= 4) {
    state = State();
  }
}

void OSC_NOTEOFF(
  const user_osc_param_t* const /*params*/)
{
  // state.vol0 = 0.f;
}


void OSC_PARAM(uint16_t index, uint16_t value)
{
  switch (index) {
    case k_user_osc_param_id1:
      // state.master_hosho_mbira_mix = value / 99.f;
      break;
  }
}
