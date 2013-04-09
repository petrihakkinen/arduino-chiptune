#include "playroutine.h"

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


Channel channel[OSCILLATORS];
Track tracks[TRACKS];

Song song;
uint8_t songpos = 0;

uint8_t trackpos = 0;
uint8_t trackcounter = 0;
uint8_t tempo = 5;    // 10 = 150bpm, smaller values = faster tempo

void initPlayroutine() {
  memset(channel, 0, sizeof(channel));
  memset(tracks, 0, sizeof(tracks));
  song.tracks[0][0] = 0;
  song.tracks[0][1] = 1;
  song.tracks[0][2] = 2;
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
  channel[voice].arpPhase1 = 0;
  channel[voice].arpPhase2 = 0;
  
  // reseed noise
  if(instr->waveform == NOISE)
    noise = 0xACE1;
}

void noteOff(uint8_t voice) {
  osc[voice].ctrl = 0;

  // in case the oscillator was in attack phase, the amplitude may have been higher than sustain amplitude
  // with low sustain and long release, this produces odd behavior so we'll clamp oscillator amplitude
  // to sustain level when note is released
  osc[voice].amp = min(osc[voice].amp, osc[voice].sustain<<8);
}

void recordNote(uint8_t voice, uint8_t note, uint8_t instrNum) {
  // round trackpos to nearest track slot index
  uint8_t tpos = trackpos;
  if(trackcounter >= tempo>>1)
    tpos = (tpos+1) & (TRACK_LENGTH-1);
    
  Track* tr = &tracks[voice];
  tr->lines[tpos].note = note;
  tr->lines[tpos].instrument = instrNum;
}

// Updates track playback. Called at 50hz.
void playroutine() {
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
  for(int i = 0; i < CHANNELS; i++) {
    Track* tr = &tracks[song.tracks[songpos][i]];
    uint8_t note = tr->lines[trackpos].note;
    if(note > 0 && note < 0xff) {
      playNote(i, note-1, tr->lines[trackpos].instrument);
    } else if(note == 0xff) {
      noteOff(i);
    }
  }  
#ifdef ARDUINO
  // flash led 4 times per beat
  if((trackpos & 1) == 0)
    digitalWrite(beatLedPin, HIGH); //trackpos & 1 == 0);  
  else
    digitalWrite(beatLedPin, LOW); //trackpos & 1 == 0);  
#endif
}

void updateEffects() {  // called at 50hz
  for(int i=0; i<OSCILLATORS; i++) {
    Channel* chan = &channel[i];
    Instrument* instr = &instruments[chan->instrument];
    
    uint16_t f = chan->frequency;

    // vibrato
    uint8_t vibscaler = f>>4; 
    f += ((sintab[chan->vibratoPhase>>3] * instr->vibratoDepth) >> 5) * vibscaler >> 3;
    chan->vibratoPhase += instr->vibratoSpeed;
    
	// pulse width
    osc[i].pulseWidth += instr->pulseWidthSpeed >> 4;

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
	  chan->arpPhase1++;
	  if(chan->arpPhase1 >= 2) {
		  chan->arpPhase2++;
		  if(chan->arpPhase2 >= 3)
			  chan->arpPhase2 = 0;
		  chan->arpPhase1 = 0;
	  }
      if(chan->arpPhase2 == 0)
        f = pitches[chan->note];
      if(chan->arpPhase2 == 1)
        f = pitches[chan->note+4];
      if(chan->arpPhase2 == 2)
        f = pitches[chan->note+7];
      break;

    case ARPEGGIO_MINOR:
      // note +0,+3,+7
	  chan->arpPhase1++;
	  if(chan->arpPhase1 >= 2) {
		  chan->arpPhase2++;
		  if(chan->arpPhase2 >= 3)
			  chan->arpPhase2 = 0;
		  chan->arpPhase1 = 0;
	  }
      if(chan->arpPhase2 == 0)
        f = pitches[chan->note];
      if(chan->arpPhase2 == 1)
        f = pitches[chan->note+3];
      if(chan->arpPhase2 == 2)
        f = pitches[chan->note+7];
      break;

    case ARPEGGIO_DOMINANT_SEVENTH:
      // note +0,+4,+7,10
	  chan->arpPhase1++;
	  if(chan->arpPhase1 >= 2) {
		  chan->arpPhase2++;
		  if(chan->arpPhase2 >= 4)
			  chan->arpPhase2 = 0;
		  chan->arpPhase1 = 0;
	  }
      if(chan->arpPhase2 == 0)
        f = pitches[chan->note];
      if(chan->arpPhase2 == 1)
        f = pitches[chan->note+4];
      if(chan->arpPhase2 == 2)
        f = pitches[chan->note+7];
      if(chan->arpPhase2 == 3)
        f = pitches[chan->note+10];
      break;

    case ARPEGGIO_OCTAVE:
      if(chan->arpPhase1++ & 2)
        f *= 2;  // +1 octave
      break;      
    }
    
    osc[i].frequency = f;
  }
}
