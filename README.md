Chiptune Tracker
Copyright (C) 2013 Petri Hakkinen

Max song length		64
Max patterns		64*
Max instruments		16

* Number of different patterns affects memory usage.

Global commands
---------------

Ctrl-O 			Load song
Ctrl-S			Save song
Ctrl-E			Export song
Tab				Switch between song editor, track editor and instrument editor
Arrow keys		Move cursor
<>				Change instrument
,.				Change octave
Numpad / *		Change tempo
Alt-F4			Quit

Song editor
-----------

0-9 a-f			Enter track numbers in hexadecimal format
Insert			Insert a new song row after current row
Delete			Delete current song row

Track editor
------------

-				Insert note-off
Insert			Insert empty row
Delete			Delete current row and move rows to fill space
Backspace		Erase current row
Home/end		Move cursor to start/end of pattern
Page up/down	Move to previous/next pattern in song
Ctrl-A			Select entire track
Shift-Up/down	Select block
Ctrl-C			Copy block
Ctrl-X			Cut block
Ctrl-V			Paste block
Space			Play current pattern only
Shift-Space		Play song from beginning
Enter			Toggle detail editing mode of row (also see effects below)

Effects
-------

--				No effect
sx				Slide pitch up by x per tick
Sx				Slide pitch down by x per tick
ax				Arpeggio x: 0=none, 1=major, 2=minor, 3=dominanth seventh, 4=octave
dx				Drum effect; Play noice x ticks, then switch to instrument waveform.
px				Portamento; Slide to new note with speed x
vx				Slide volume down by x per tick
Vx				Slide volume up by x per tick

Instrument editor
-----------------

Enter + 0-9 a-f	Enter instrument parameters in hexadecimal format
Numpad +/-		Adjust instrument parameter by 1 up/down

Instrument parameters
---------------------

Waveform		Triangle, Pulse, Sawtooth or Noise
Attack			ADSR envelope attack length; 1 = slowest possible attack, 7f = fastest
Decay			ADSR envelope decay length; 1 = slowest possible decay, 7f = fastest
Sustain			ADSR envelope sustain volume in range 0-7f
Release			ADSR envelope release length; 1 = slowest possible release, 7f = fastest
Pulse width		Pulse width for pulse waveform; 40 = 25% pulse, 7f = 50% pulse
Pulse speed		Pulse width modulation speed; 0 = no modulation, 7f = very rapid
Vibrato depth	Amplitude of the vibrato effect
Vibrato speed	Speed of the vibrato effect
