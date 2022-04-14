#include "userosc.h"

#include <array>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <tuple>

#include "mersenne.h"

using namespace std::literals::chrono_literals;


void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();

  { // reseed rng_buffers
    for (auto& rng_buffer : rng_buffers)
      for (auto& rng_value : rng_buffer)
        rng_value = rng.uniform();
  }
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
}


void OSC_NOTEON(
  const user_osc_param_t* const /*params*/)
{
  state.index ++;

  if (state.index >= 4) {
    state.index = 0;
    state.time = 0;
    state.phi0 = 0;
    state.phi1 = 0;
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
