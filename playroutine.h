#ifndef PLAYROUTINE_H
#define PLAYROUTINE_H

#include "defs.h"

#define INSTRUMENTS     8
#define TRACK_LENGTH    32
#define TRACKS          1

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
struct Instrument {
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

// Track contains note and instrument data for one channel.
struct Track {
  uint8_t   note[TRACK_LENGTH];
  uint8_t   noteLength[TRACK_LENGTH];
  uint8_t   instrument[TRACK_LENGTH];
};

extern Track tracks[TRACKS];
extern Instrument instruments[INSTRUMENTS];

void initPlayroutine();

// Plays the music. Should be called at 50hz.
void playroutine();

void playNote(uint8_t voice, uint8_t note, uint8_t instrNum);
void stopNote(uint8_t note);

#endif
