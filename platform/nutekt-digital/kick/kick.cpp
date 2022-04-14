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

  float attack = 10e-3;
  float release = 100e-3;
  float intensity = 48;
};

static State state;

void OSC_INIT(uint32_t /*platform*/, uint32_t /*api*/)
{
  state = State();
}

float get_pitch_env(
  const float time,
  const float attack,
  const float release,
  const float intensity)
{
  return intensity * (
    time < 0 ? -1 :
    time < attack ? 2 / attack * (time - attack / 2) :
    expf((time - attack) / -release));
}

float get_volume_env(
  const float time,
  const float sustain,
  const float release)
{
  return
    time < 0 ? 0 :
    time < sustain ? 1 :
    expf((time - sustain) / -release);
}

void OSC_CYCLE(
  const user_osc_param_t* const params,
  int32_t* yy_,
  const uint32_t frames)
{
  const Note in_note = (params->pitch >> 8) % 152;
  const Note in_bend = params->pitch & 0xFF;

  const auto w0 = osc_w0f_for_note(in_note - 24, in_bend);

  auto yy = static_cast<q31_t*>(yy_);
  auto yy_end = yy + frames;
  for (; yy < yy_end; yy++) {
    const float pitch_env = get_pitch_env(
      state.time,
      state.attack,
      state.release,
      state.intensity);
    const float volume_env = get_volume_env(
      state.time,
      state.attack + 3 * state.release,
      35e-3);
    const float ww = w0 * powf(2.f, pitch_env / 12.f);
    const float sig_master = volume_env * osc_sinf(state.osc0.phi);

    *yy = f32_to_q31(sig_master);

    state.time += k_samplerate_recipf;
    state.osc0.update(ww);
    // state.osc1.update(w1);
  }
}


void OSC_NOTEON(
  const user_osc_param_t* const /*params*/)
{
  state.index ++;
  state.index %= 4;

  if (state.index == 0) {
    // state.osc0 = Osc(); // uncomment to add click
    state.time = 0;
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
    case k_user_osc_param_shape: /* Intensity (A) */
      state.intensity = 24 + 72 * param_val_to_f32(value);
      break;
    case k_user_osc_param_shiftshape: /* Release (B) */
      state.release = param_val_to_f32(value) * 400e-3f + 20e-3f;
      break;
    case k_user_osc_param_id1:  /* Attack */
      state.attack = value * 1e-3;
      break;
  }
}
