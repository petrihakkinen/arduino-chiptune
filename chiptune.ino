// MIDI Chiptune Synthesizer
//
// Pin 3:   audio out (PWM)
// Pin 8:   MIDI in
// Pin 12:  key down indicator led

#include <SoftwareSerial.h>
#include "oscillators.h"
#include "playroutine.h"

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

const int beatLedPin = 12;

// use soft serial
SoftwareSerial MIDI(8, 9);

#ifdef DEBUG
SoftwareSerial debug(0, 1);
#endif

// use hardware serial
//#define MIDI Serial

byte command = 0; // previous midi command

uint8_t   nextVoice = 0;
uint8_t   instrument = 0;

void playNote(byte note) {
  if(note >= 0 && note < 12*8) {
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

/*
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
*/

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
          
          //recordNote(note);
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
      //if(channel == 0 && control == 43)
      //  tempo = value;

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

static uint8_t sample = 0;
static int tickCounter = 0;  // tick rate 16 Khz

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
    
  tickCounter++;
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
  
  initOscillators();
  initPlayroutine();
  
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
