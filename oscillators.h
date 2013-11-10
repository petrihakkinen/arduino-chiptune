#ifndef OSCILLATORS_H
#define OSCILLATORS_H

#include "defs.h"

#define OSCILLATORS          4

// waveforms
#define TRIANGLE 0
#define PULSE    1
#define SAWTOOTH 2
#define NOISE    3

// Oscillator state, these are read by the timer routine at 16 Khz
struct Oscillator
{
	// phase
	uint16_t  phase;
	uint16_t  frequency;

	// waveform
	uint8_t   waveform;
	uint8_t   pulseWidth;   // pulse width in range 0-255
	uint16_t  amp;          // hi8: amplitude in range 0-127, lo8: fractionals for envelope
	int8_t    noise;        // current noise value

	// envelope
	uint8_t   ctrl;         // 0=release, 1=attack, 2=decay, 3=manual volume control
	uint8_t   attack;       // attack rate
	uint8_t   decay;        // decay rate
	uint8_t   sustain;      // sustain amplitude in range 0-127
	uint8_t   release;      // release rate
};

extern volatile Oscillator osc[OSCILLATORS];
extern uint16_t noise;	// global noise state

void initOscillators();
uint8_t updateOscillators();
void updateEnvelopes();
void resetOscillators();

#endif
