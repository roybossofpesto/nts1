#include "userosc.h"
#include "dsp/biquad.hpp"
#include "utils/float_math.h"

struct State
{
	float phase0 = 0.f;
	float phase1 = 0.f;
	float vol0 = 1.f;
	float vol1 = 1.f;
	float softclipAmt = 0.f;
	float cutoff = 4.f;
	uint8_t octaveShift = -1;
	uint8_t waveIndex = 0;
};

static State state;
static dsp::BiQuad filter;

void OSC_INIT(
	const uint32_t /*platform*/,
	const uint32_t /*api*/)
{
	state = State{};
	filter.flush();
}

void OSC_CYCLE(
	const user_osc_param_t* const params,
  int32_t* yn,
  const uint32_t frames)
{
	// Calcul de l'incrément de phase correspondant au pitch de l'oscillateur
	const uint8_t notePitch = 12 * state.octaveShift + (params->pitch >> 8);
	const uint8_t modWheel = params->pitch & 0xFF;

	float w0 = 0.f;
	float w1 = 0.f;
	switch (notePitch % 12) {
	case 0:
	case 5:
	case 7:
		w0 = osc_w0f_for_note(notePitch, modWheel);
		w1 = osc_w0f_for_note(notePitch + 16, modWheel);
		break;
	case 2:
	case 4:
	case 9:
		w0 = osc_w0f_for_note(notePitch, modWheel);
		w1 = osc_w0f_for_note(notePitch + 15, modWheel);
		break;
	case 11:
		w0 = osc_w0f_for_note(notePitch, modWheel);
		w1 = osc_w0f_for_note(notePitch + 15, modWheel);
		break;
	case 1:
	case 6:
	case 8:
		w0 = osc_w0f_for_note(notePitch - 1, modWheel);
		w1 = osc_w0f_for_note(notePitch - 1 + 19, modWheel);
		break;
	case 3:
		w0 = osc_w0f_for_note(notePitch - 1, modWheel);
		w1 = osc_w0f_for_note(notePitch - 1 + 19, modWheel);
		break;
	case 10:
		w0 = osc_w0f_for_note(notePitch - 1, modWheel);
		w1 = osc_w0f_for_note(notePitch - 1 + 19, modWheel);
		break;
	}

	const float lfo = q31_to_f32(params->shape_lfo);
	const float tan_cutoff = tanf(M_PI * (state.cutoff * .2f + lfo * .1f));
	// const float lfo = q31_to_f32(params->shape_lfo);
	for (uint32_t i = 0; i < frames; i++)	{
		/****** Forme d'onde ******/
		const float signal =
			osc_bl_sqrf(state.phase0, state.waveIndex) * state.vol0 +
			osc_bl_sawf(state.phase1, state.waveIndex) * state.vol1;

		filter.mCoeffs.setSOLP(tan_cutoff, 1.3f);
		const float signal_ = filter.process(signal);

		const float signal__ = osc_softclipf(
			.2f,
			state.softclipAmt * signal_);

		/**************************/

		// Conversion de float à int
		yn[i] = f32_to_q31(signal__);

		// Incrémentation et modulo de la phase
		state.phase0 += w0;
		state.phase0 -= static_cast<uint32_t>(state.phase0);
		state.phase1 += w1;
		state.phase1 -= static_cast<uint32_t>(state.phase1);
	}
}

void OSC_NOTEON(const user_osc_param_t* const /*params*/)
{
}

void OSC_NOTEOFF(const user_osc_param_t* const /*params*/)
{
}

void OSC_PARAM(const uint16_t index, const uint16_t value)
{
	switch (index) {
	case k_user_osc_param_shape:
		state.vol0 = param_val_to_f32(value);
		break;
	case k_user_osc_param_shiftshape:
		state.vol1 = param_val_to_f32(value);
		break;
	case k_user_osc_param_id1:
		state.softclipAmt = .5f + powf(value / 99.f, 1.8f) * 10.f;
		break;
	case k_user_osc_param_id2:
		state.octaveShift = value;
		break;
	case k_user_osc_param_id3:
		state.waveIndex = value;
		break;
	case k_user_osc_param_id4:
		state.cutoff = powf(value / 99.f, 1.8f);
		break;
	default:
		break;
	}
}
