#ifndef PLAYROUTINE_H
#define PLAYROUTINE_H

#include "defs.h"
#include "oscillators.h"

#define INSTRUMENTS     16
#define TRACK_LENGTH    64
#define TRACKS          64
#define CHANNELS		OSCILLATORS
#define SONG_LENGTH		64

// effects
#define NOEFFECT        0
#define SLIDEUP         1
#define SLIDEDOWN       2
#define ARPEGGIO		3
#define DRUM            4		// use noise waveform for first 50hz cycle
#define SLIDE_NOTE		5		// slide to new note
#define VOLUME_UP		6		// slide volume up by param
#define VOLUME_DOWN		7		// slide volume down by param

// arpeggios
#define ARPEGGIO_NONE				0
#define ARPEGGIO_MAJOR				1
#define ARPEGGIO_MINOR				2
#define ARPEGGIO_DOMINANT_SEVENTH	3
#define ARPEGGIO_OCTAVE				4

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
	uint16_t  pulseWidthPhase;
};

struct TrackLine
{
	uint8_t note;
	uint8_t instrument;
	uint8_t	effect;				// upper nibble = effect, lower nibble = param
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

	// effects
	uint8_t	  effect;
	uint16_t  targetFrequency;	// for slide to effect
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
extern bool loopPattern;	// if set loop current pattern (do not advance in song)

void initPlayroutine();

void resetPlayroutine();

// Plays back the song. Should be called at 50hz.
void playroutine();

// Updates effects on channels. Should be called at 50hz.
void updateEffects();

void playNote(uint8_t voice, uint8_t note, uint8_t instrNum);
void noteOff(uint8_t voice);

#endif
