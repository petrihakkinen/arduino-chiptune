This is a work in progress chiptune music player/tracker for Arduino.

Instruments:
- waveform
- ADSR
- pulse width
- pulse width speed (ping pong effect between min and max pulse width)
- vibrato depth
- vibrato speed
- effect

Effects:
- 0=no effect
- 1=Drum: play noise for one 50hz tick, then use the instrument waveform. Fast frequency slide down.
- 2=Slide down X: slide down frequency by amount X
- 3=Slide up X: slide up frequency by amount X
- 4=Rob Hubbard Arpeggio: switch between normal note and +1 octave at 50 hz
- 5=Custom Arpeggio: switch between note, note +X and note +Y at 50 hz

References

Rob Hubbard's Music: Disassembled, Commented and Explained
http://www.1xn.org/text/C64/rob_hubbards_music.txt

klystrack Instrument Editor
http://code.google.com/p/klystrack/wiki/InstrumentEditor