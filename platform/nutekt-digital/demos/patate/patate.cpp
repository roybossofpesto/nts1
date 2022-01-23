#include "userosc.h"

uint8_t patate(uint8_t xx)
{
  return 42;
}


void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
 // (void)platform;
  // (void)api;
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t *yn,
  const uint32_t frames)
{


}


void OSC_NOTEON(
  const user_osc_param_t* const params)
{
  // s_waves.state.flags |= Waves::k_flag_reset;
}

void OSC_NOTEOFF(
  const user_osc_param_t* const params)
{
  // (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{

}
