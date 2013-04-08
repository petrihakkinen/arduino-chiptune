// MIDI Chiptune Synthesizer
//
// Pin 3:   audio out (PWM)
// Pin 8:   MIDI in
// Pin 12:  key down indicator led

#include <SoftwareSerial.h>

#define DEBUG

// MIDI Commands (nnnn = channel)
// NOTE_ON           1001nnnn
// CONTROL_CHANGE    1011nnnn
// PITCH_WHEEL       1110nnnn
//
// All commands have the highest bit set.
// Commands may be omitted from the stream if same command is repeated.

#define MIDI_NOTE_ON         0b10010000  //144
#define MIDI_CONTROL_CHANGE  0b10110000  //176
#define MIDI_PITCH_WHEEL     0b11100000  //224

#define SAMPLE_FREQUENCY     16000
#define OSCILLATORS          3
#define INSTRUMENTS          8
#define TRACK_LENGTH         32
#define TRACKS               1

const int beatLedPin = 12;

// use soft serial
SoftwareSerial MIDI(8, 9);

#ifdef DEBUG
SoftwareSerial debug(0, 1);
#endif

// use hardware serial
//#define MIDI Serial

byte command = 0; // previous midi command

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

// waveforms
#define TRIANGLE 0
#define PULSE    1
#define SAWTOOTH 2
#define NOISE    3

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

// Oscillator state, these are read by the timer routine at 16 Khz
struct Oscillator {
  // phase
  uint16_t  phase;
  uint16_t  frequency;
  
  // waveform
  uint8_t   waveform;
  uint8_t   pulseWidth;   // pulse width in range 0-255
  uint16_t  amp;          // hi8: amplitude in range 0-127, lo8: fractionals for envelope
  int8_t    noise;        // current noise value
  
  // envelope
  uint8_t   ctrl;         // 0=release, 1=attack, 2=decay
  uint8_t   attack;       // attack rate
  uint8_t   decay;        // decay rate
  uint8_t   sustain;      // sustain amplitude in range 0-127
  uint8_t   release;      // release rate
};

// Channel state, updated at 50hz by playroutine
struct Channel {
  uint8_t   instrument;   // instrument playing
  uint8_t   note;         // note playing
  uint16_t  frequency;    // base frequency of the note adjusted with slide up/down
  uint8_t   vibratoPhase;
};

// Track contains note and instrument data for one channel.
struct Track {
  uint8_t   note[TRACK_LENGTH];
  uint8_t   noteLength[TRACK_LENGTH];
  uint8_t   instrument[TRACK_LENGTH];
};

volatile Oscillator osc[OSCILLATORS];
Channel channel[OSCILLATORS];
Track track[TRACKS];

uint16_t  noise = 0xACE1;
uint8_t   nextVoice = 0;
uint8_t   instrument = 0;

uint16_t  trackpos = 0;
uint8_t   trackcounter = 0;
uint8_t   tempo = 10;    // 10 = 150bpm, smaller values = faster tempo

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

void playNote(byte note) {
  if(note >= 0 && note < sizeof(pitches)/2) {
    playNote(nextVoice, note, instrument);
    
    // rotate oscillators
#if OSCILLATORS > 1
    nextVoice = (nextVoice+1) % OSCILLATORS;    
#endif
  }
}
void stopNote(byte note) {
  for(int i=0; i<OSCILLATORS; i++) {
    if(channel[i].note == note) {
      osc[i].ctrl = 0;
    }
  }
}

void recordNote(byte note) {
  // round trackpos to nearest track slot index
  uint8_t tpos = trackpos;
  if(trackcounter >= tempo>>1)
    tpos = (tpos+1) & (TRACK_LENGTH-1);
    
  Track* tr = &track[0];
  tr->note[tpos] = note;
  tr->noteLength[tpos] = 4;
  tr->instrument[tpos] = instrument;
}

void readMIDI() {
  while(MIDI.available() > 0) {
    if(MIDI.peek() & 128) {
      command = MIDI.read();
      //Serial.println(command);
    }
    
    if((command & 0xf0) == MIDI_NOTE_ON) {
      byte channel = command & 0xf;
      byte note = MIDI.read();
      byte velocity = MIDI.read();
//      Serial.print("NOTE_ON: channel ");
//      Serial.print(channel);
//      Serial.print(" note ");
//      Serial.print(note);
//      Serial.print(" velocity ");
//      Serial.println(velocity);
            
      if(channel == 0) {
        note -= 24;
        if(velocity > 0) {
//          Serial.print("play ");
//          Serial.println(note);
          playNote(note);
          
          recordNote(note);
        } else {
//          Serial.print("stop ");
//          Serial.println(note);
          stopNote(note);
        }
      }
     
#ifdef DEBUG
      if(channel == 15 && note == 10 && velocity > 0) {
        // dump instruments
        debug.println("");
        for(int i=0; i<INSTRUMENTS; i++) {
          Instrument* instr = &instruments[i];
          debug.print("{ ");
          debug.print(instr->waveform);
          debug.print(",");
          debug.print(instr->attack);
          debug.print(",");
          debug.print(instr->decay);
          debug.print(",");
          debug.print(instr->sustain);
          debug.print(",");
          debug.print(instr->release);
          debug.print(",");
          debug.print(instr->pulseWidth);
          debug.print(",");
          debug.print(instr->pulseWidthSpeed);
          debug.print(",");
          debug.print(instr->vibratoDepth);
          debug.print(",");
          debug.print(instr->vibratoSpeed);
          debug.print(",");
          debug.print(instr->effect);
          debug.println(" },");
        }
      }
#endif
    } else if((command & 0xf0) == MIDI_CONTROL_CHANGE) {
      byte channel = command & 0xf;
      byte control = MIDI.read();
      byte value = MIDI.read();
//      Serial.print("CONTROL_CHANGE: channel ");
//      Serial.print(channel);
//      Serial.print(" control ");
//      Serial.print(control);
//      Serial.print(" value ");
//      Serial.println(value);

      // B5: tempo
      if(channel == 0 && control == 43)
        tempo = value;

      // B1: instrument
      if(channel == 0 && control == 39)
        instrument = min(value, INSTRUMENTS-1);

      Instrument* instr = &instruments[instrument];
      
      // B2: waveform
      if(channel == 0 && control == 40)
        instr->waveform = value;
        
      // B3: effect
      if(channel == 0 && control == 41)
        instr->effect = value;
      
      // D9-D12: ADSR
      if(channel == 0 && control == 8)
        instr->attack = max(value, 1);
      if(channel == 0 && control == 9)
        instr->decay = max(value, 1);
      if(channel == 0 && control == 10)
        instr->sustain = value;
      if(channel == 0 && control == 12)
        instr->release = max(value, 1);
                
      // D13-D14: vibrato depth & vibrato speed
      if(channel == 0 && control == 13)
        instr->vibratoDepth = value;
      if(channel == 0 && control == 14)
        instr->vibratoSpeed = value;
      
      // D15-D16: pulse width & pulse width speed
      if(channel == 0 && control == 15)
        instr->pulseWidth = value << 1;
      if(channel == 0 && control == 16)
        instr->pulseWidthSpeed = value;                
    } else if((command & 0xf0) == MIDI_PITCH_WHEEL) {
      byte channel = command & 0xf;
      byte lo = MIDI.read();
      byte hi = MIDI.read();
      int value = (hi<<7) | lo;
//      Serial.print("PITCH_WHEEL: channel ");
//      Serial.print(channel);
//      Serial.print(" value ");
//      Serial.println(value);
    } else {
//      Serial.print("UNKNOWN MIDI COMMAND ");
//      Serial.print(command);
//      Serial.print(" ");
//      Serial.println(command, BIN);
      MIDI.read();
    }
  }
}

inline uint8_t updateOscillators() {
  // Galois LFSR, see http://en.wikipedia.org/wiki/Linear_feedback_shift_register
  noise = (noise >> 1) ^ (-(noise & 1) & 0xB400u);

  int16_t output = 0;
  
  for(int i=0; i<OSCILLATORS; i++) {
    // update oscillator phase
    osc[i].phase += osc[i].frequency;
    int16_t phase = osc[i].phase >> 8; // [0,255]
    
    int16_t value = 0;  // [-32,31]
    
    switch(osc[i].waveform) {
    case TRIANGLE: 
      if(phase < 128)
        value = (phase>>1) - 32;
      else
        value = ((255 - phase)>>1) - 31;
      break;
    
    case PULSE:
      value = (phase < osc[i].pulseWidth ? -32 : 31);
      break;
  
    case SAWTOOTH:
      value = (phase>>2) - 32;
      break;
 
    case NOISE:
      if(phase > 64) {
        osc[i].noise = (int)(noise >> 10) - 32;
        osc[i].phase -= 16384;
      }
      value = osc[i].noise;
      break;
    }
       
    int16_t amp = osc[i].amp>>8;  // [0,127]
    output += value * amp;  // [-4096,4095]
  }
  
#if OSCILLATORS == 1
  return (output>>5) + 128;
#elif OSCILLATORS == 2
  return (output>>6) + 128;
#elif OSCILLATORS == 3
  // (x*10)>>5 is roughly same as x/3 but much faster
  return ((output>>5)*10>>5) + 128;
#elif OSCILLATORS == 4
  return (output>>7) + 128;
#else
#error "Invalid number of oscillators"
#endif
}

inline void updateEnvelopes() {
  for(int i=0; i<OSCILLATORS; i++) {
    if(osc[i].ctrl == 1) {
      // attack: fade to peak amplitude, start decay when peak amplitude reached
      uint16_t amp = osc[i].amp;
      amp += ((uint16_t)osc[i].attack)<<2;
      if(amp >= 0x7fff) {
        amp = 0x7fff;
        osc[i].ctrl = 2;
      }
      osc[i].amp = amp;
    } else if(osc[i].ctrl == 2) {
      // decay: fade to sustain amplitude
      int16_t amp = osc[i].amp;
      amp = max(amp - ((int16_t)osc[i].decay<<2), osc[i].sustain<<8);
      osc[i].amp = amp;
    } else {
      // release: fade to zero amplitude
      int16_t amp = osc[i].amp;
      amp = max(amp - osc[i].release, 0);
      osc[i].amp = amp;
    }
  }
}

uint8_t sample = 0;
int envelopeUpdateCounter = 0;

int tickCounter = 0;  // tick rate 16 Khz

ISR(TIMER0_COMPA_vect) // called at 16 KHz
{
  /*
  ATmega328 Datasheet p. 14:
  When an interrupt occurs, the Global Interrupt Enable I-bit is cleared and all interrupts are
  disabled. The user software can write logic one to the I-bit to enable nested interrupts. All
  enabled interrupts can then interrupt the current interrupt routine. The I-bit is automatically
  set when a Return from Interrupt instruction – RETI – is executed.
  */

  // enable nested interrupts
  // without this, MIDI serial input is randomly garbled
  // SoftwareSerial library uses Pin Change Interrupts to read serial data
  // but the interrupt is delayed until the end of this interrupt unless
  // nested interrupts are enabled
  asm("sei");
  
  // set Output Compare Register B for Timer/Counter2
  OCR2B = sample;
  sample = updateOscillators();
  
  if(envelopeUpdateCounter == 0) {
    updateEnvelopes();
    envelopeUpdateCounter = 16;
  }
  envelopeUpdateCounter--;
  
  tickCounter++;
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
  Track* tr = &track[0];  
  if(tr->noteLength[trackpos] > 0) {
    // TODO: voice number!
    playNote(0, tr->note[trackpos], tr->instrument[trackpos]);
  }
  
  // flash led 4 times per beat
  if((trackpos & 1) == 0)
    digitalWrite(beatLedPin, HIGH); //trackpos & 1 == 0);  
  else
    digitalWrite(beatLedPin, LOW); //trackpos & 1 == 0);  
}

uint8_t effectPhase = 0;
uint8_t arp = 0;

void updateEffects() {  // called at 50hz
  effectPhase++;
  
  for(int i=0; i<OSCILLATORS; i++) {
    Channel* chan = &channel[i];
    Instrument* instr = &instruments[chan->instrument];
    
    uint16_t f = chan->frequency;

    // vibrato
    // FIXME: this is pitch bend, not vibrato!
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

void setup() {
  // 440hz test tone
  /*
  pinMode(7, OUTPUT);
  for(;;) {
    digitalWrite(7, HIGH);
    delay(1000/220);
    digitalWrite(7, LOW);
    delay(1000/220);
  }
  */
  
  memset((void*)osc, 0, sizeof(osc));
  memset(channel, 0, sizeof(channel));
  memset(track, 0, sizeof(track));

  // global interrupt disable
  asm("cli");
  
  // set CPU clock prescaler to 1
  // isn't this the default setting?
  //CLKPR = 0x80;
  //CLKPR = 0x80;
  
  // DDRx Data Direction Register for Port x where x = C,D
  // each bit sets output mode for a pin
  DDRC = 0x12; 
  DDRD = 0xff;
    
  // this configures Timer/Counter0 to cause interrupts at 16 KHz
  // 16000000 Hz / 8 / 125 = 16000 Hz
  TCCR0A = 2;  // set Clear Timer on Compare Match (CTC) mode
  TCCR0B = 2;  // set Timer/Counter clock prescaler to 1/8
  OCR0A = 125; // set Output Compare Register for Timer/Counter0 Comparator A
  
  // Enable Fast PWM
  // ===============
  // Timer/Counter2 Control Register A
  // bits 7-6: Clear OC2A on Compare Match, set OC2A at BOTTOM (non-inverting mode)
  // bits 5-4: Clear OC2B on Compare Match, set OC2B at BOTTOM (non-inverting mode)
  // bits 1-0: enable Fast PWM
  TCCR2A=0b10100011;
  // Timer/Counter2 Control Register A
  // bits 2-0: No prescaling (full clock rate)
  TCCR2B=0b00000001;
  
  // enables interrupt on Timer0
  // enable Timer/Counter0 Compare Match A interrupt
  TIMSK0 = 2;
  
  // global interrupt enable
  asm("sei");
  
  /*
  for(int i=0; i<OSCILLATORS; i++) {
    osc[i].phase = 0;
    osc[i].frequency = 0;
    osc[i].waveform = 0;
    osc[i].amp = 0;
    osc[i].pulseWidth = 128;
    osc[i].attack = 16;
    osc[i].decay = 16;
    osc[i].sustain = 64;
    osc[i].release = 16;
    osc[i].ctrl = 0;
  }
  */ 
  
#ifdef DEBUG
  debug.begin(9600);
#endif

  MIDI.begin(31250);
  //Serial.begin(9600);
  
  pinMode(beatLedPin, OUTPUT);
}

void loop() {
    readMIDI();
 
    // update effects
    if(tickCounter >= 320) {
      playroutine();
      updateEffects();
      tickCounter = 0;
    }
    
    // debug keystate
//    int keystate = 0;
//    for(int i=0; i<OSCILLATORS; i++)
//      if(playingNote[i])
//        keystate += 1<<i;
//    digitalWrite(12, keystate != 0 ? HIGH : LOW);   
}
