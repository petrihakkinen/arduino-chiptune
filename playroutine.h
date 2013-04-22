#ifndef PLAYROUTINE_H
#define PLAYROUTINE_H

#include "defs.h"
#include "oscillators.h"

#define INSTRUMENTS     16
#define TRACK_LENGTH    32
#define TRACKS          32
#define CHANNELS		3
#define SONG_LENGTH		32

// effects
#define NOEFFECT        0
#define SLIDEUP         1
#define SLIDEDOWN       2
#define ARPEGGIO_MAJOR  3
#define ARPEGGIO_MINOR  4
#define ARPEGGIO_DOMINANT_SEVENTH 5
#define ARPEGGIO_OCTAVE 6
//#define DRUM          7    // use noise waveform for first 50hz cycle

// Instrument parameters
struct Instrument
{
	uint8_t  waveform;
	uint8_t  attack;
	uint8_t  decay;
	uint8_t  sustain;
	uint8_t  release;
	uint8_t  pulseWidth;
	uint8_t  pulseWidthSpeed;
	uint8_t  vibratoDepth;    // 0-127
	uint8_t  vibratoSpeed;    // 0-127
	uint8_t  effect;
	//uint8_t  param;           // effect parameter
};

struct TrackLine
{
	uint8_t note;
	uint8_t instrument;	
};

// Track contains note and instrument data for one channel.
struct Track
{
	TrackLine lines[TRACK_LENGTH];
};

// Channel state, updated at 50hz by playroutine
struct Channel
{
	uint8_t   instrument;   // instrument playing
	uint8_t   note;         // note playing
	uint16_t  frequency;    // base frequency of the note adjusted with slide up/down
	uint8_t   vibratoPhase;

	// TODO: rewrite arpeggio!
	uint8_t   arpPhase1;
	uint8_t   arpPhase2;
};

struct Song
{
	uint8_t   tracks[SONG_LENGTH][CHANNELS];
};

extern Channel channel[CHANNELS];

extern Track tracks[TRACKS];
extern uint8_t trackpos;
extern uint8_t tempo;

extern Instrument instruments[INSTRUMENTS];

extern Song song;
extern uint8_t songpos;

void initPlayroutine();

// Plays back the song. Should be called at 50hz.
void playroutine();

// Updates effects on channels. Should be called at 50hz.
void updateEffects();

void playNote(uint8_t voice, uint8_t note, uint8_t instrNum);
void noteOff(uint8_t voice);

#endif
