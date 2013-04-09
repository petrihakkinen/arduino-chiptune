#include "playroutine.h"
#include "oscillators.h"

// frequencies for first 8 octaves (notes C0-B7)
uint16_t pitches[] = {
  0x0043,0x0046,0x004b,0x004f,0x0054,0x0059,0x005e,0x0064,0x006a,0x0070,0x0077,0x007e,
  0x0086,0x008d,0x0096,0x009f,0x00a8,0x00b2,0x00bd,0x00c8,0x00d4,0x00e1,0x00ee,0x00fc,
  0x010b,0x011b,0x012c,0x013e,0x0151,0x0165,0x017a,0x0191,0x01a9,0x01c2,0x01dd,0x01f9,
  0x0217,0x0237,0x0259,0x027d,0x02a3,0x02cb,0x02f5,0x0322,0x0352,0x0385,0x03ba,0x03f3,
  0x042f,0x046f,0x04b2,0x04fa,0x0546,0x0596,0x05eb,0x0645,0x06a5,0x070a,0x0775,0x07e6,
  0x085f,0x08de,0x0965,0x09f4,0x0a8c,0x0b2c,0x0bd6,0x0c8a,0x0d49,0x0e14,0x0eea,0x0fcd,
  0x10bd,0x11bc,0x12ca,0x13e8,0x1517,0x1658,0x17ac,0x1915,0x1a93,0x1c27,0x1dd4,0x1f9a,
  0x217b,0x2378,0x2594,0x27d0,0x2a2e,0x2cb0,0x2f58,0x3229,0x3524,0x384d,0x3ba6,0x3f32,
};

// sinetable for vibrato effect
int8_t sintab[] = {
  0,6,12,17,22,26,29,31,31,31,29,26,22,17,12,6,0,-7,-13,-18,-23,-27,-30,-32,-32,-32,
  -30,-27,-23,-18,-13,-7
};

Instrument instruments[INSTRUMENTS] = {
  { 1,113,89,20,16,225,21,0,0,0 },  // bass
  { 3,127,40,0,127,0,0,81,76,0 },   // snare
  { 1,113,89,20,16,208,21,0,0,3 },  // lead
  { 1,22,65,0,127,76,29,31,98,1 },  // fx
  { 0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0 },
};

// Channel state, updated at 50hz by playroutine
struct Channel {
  uint8_t   instrument;   // instrument playing
  uint8_t   note;         // note playing
  uint16_t  frequency;    // base frequency of the note adjusted with slide up/down
  uint8_t   vibratoPhase;
};

Channel channel[OSCILLATORS];
Track tracks[TRACKS];

uint8_t trackpos = 0;
uint8_t trackcounter = 0;
uint8_t tempo = 10;    // 10 = 150bpm, smaller values = faster tempo

uint8_t effectPhase = 0;
uint8_t arp = 0;

void initPlayroutine() {
  memset(channel, 0, sizeof(channel));
  memset(tracks, 0, sizeof(tracks));
}

void playNote(uint8_t voice, uint8_t note, uint8_t instrNum) {
  Instrument* instr = &instruments[instrNum];
  uint16_t freq = pitches[note];
  osc[voice].frequency = freq;
  osc[voice].ctrl = 1;
  osc[voice].waveform = instr->waveform;
  osc[voice].attack = instr->attack;
  osc[voice].decay = instr->decay;
  osc[voice].sustain = instr->sustain;
  osc[voice].release = instr->release;
  osc[voice].pulseWidth = instr->pulseWidth;
  channel[voice].instrument = instrNum;
  channel[voice].note = note;
  channel[voice].frequency = freq;
  channel[voice].vibratoPhase = 0;
  
  // reseed noise
  if(instr->waveform == NOISE)
    noise = 0xACE1;
}

void stopNote(uint8_t note) {
  for(int i=0; i<OSCILLATORS; i++) {
    if(channel[i].note == note) {
      osc[i].ctrl = 0;
    }
  }
}

void recordNote(uint8_t voice, uint8_t note, uint8_t instrNum) {
  // round trackpos to nearest track slot index
  uint8_t tpos = trackpos;
  if(trackcounter >= tempo>>1)
    tpos = (tpos+1) & (TRACK_LENGTH-1);
    
  Track* tr = &tracks[voice];
  tr->note[tpos] = note;
  tr->noteLength[tpos] = 4;
  tr->instrument[tpos] = instrNum;
}

void updateEffects() {  // called at 50hz
  effectPhase++;
  
  for(int i=0; i<OSCILLATORS; i++) {
    Channel* chan = &channel[i];
    Instrument* instr = &instruments[chan->instrument];
    
    uint16_t f = chan->frequency;

    // vibrato
    uint8_t vibscaler = f>>4; 
    f += ((sintab[chan->vibratoPhase>>3] * instr->vibratoDepth) >> 5) * vibscaler >> 3;
    chan->vibratoPhase += instr->vibratoSpeed;
    
    // pulse width
    osc[i].pulseWidth = instr->pulseWidth;
    instr->pulseWidth += instr->pulseWidthSpeed >> 4;
    
    // effect
    switch(instr->effect) {
    case SLIDEUP:
      chan->frequency += 32;
      break;
      
    case SLIDEDOWN:
      chan->frequency = max((int16_t)chan->frequency - 8, 0);
      break;

    case ARPEGGIO_MAJOR:
      // note +0,+4,+7
      if((effectPhase & 1) == 0)
        arp = (arp+1) % 3;
      if(arp == 0)
        f = pitches[chan->note];
      if(arp == 1)
        f = pitches[chan->note+4];
      if(arp == 2)
        f = pitches[chan->note+7];
      break;

    case ARPEGGIO_MINOR:
      // note +0,+3,+7
      if((effectPhase & 1) == 0)
        arp = (arp+1) % 3;
      if(arp == 0)
        f = pitches[chan->note];
      if(arp == 1)
        f = pitches[chan->note+3];
      if(arp == 2)
        f = pitches[chan->note+7];
      break;

    case ARPEGGIO_DOMINANT_SEVENTH:
      // note +0,+4,+7,10
      if((effectPhase & 1) == 0)
        arp = (arp+1) % 4;
      if(arp == 0)
        f = pitches[chan->note];
      if(arp == 1)
        f = pitches[chan->note+4];
      if(arp == 2)
        f = pitches[chan->note+7];
      if(arp == 3)
        f = pitches[chan->note+10];
      break;

    case ARPEGGIO_OCTAVE:
      if(effectPhase & 2)
        f *= 2;  // +1 octave
      break;      
    }
    
    osc[i].frequency = f;
  }
}

// Updates track playback. Called at 50hz.
void playroutine() {
  updateEffects();

  trackcounter++;
  if(trackcounter < tempo)
    return;
    
  // we get here 8 times per beat (1/8th notes)
  
  // increment track pos
  trackpos = (trackpos+1);
  trackcounter = 0;
  
  // track finished?
  if(trackpos == TRACK_LENGTH)
     trackpos = 0; // TODO: start next track

  // play note?
  Track* tr = &tracks[0];  
  if(tr->noteLength[trackpos] > 0) {
    // TODO: voice number!
    playNote(0, tr->note[trackpos], tr->instrument[trackpos]);
  }
  
#ifdef ARDUINO
  // flash led 4 times per beat
  if((trackpos & 1) == 0)
    digitalWrite(beatLedPin, HIGH); //trackpos & 1 == 0);  
  else
    digitalWrite(beatLedPin, LOW); //trackpos & 1 == 0);  
#endif
}
