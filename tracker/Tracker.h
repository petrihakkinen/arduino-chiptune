#ifndef TRACKER_H
#define TRACKER_H

#define TRACK_LENGTH	32
#define TRACKS			1
#define INSTRUMENTS		8

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;

struct Track
{
	uint8_t		note[TRACK_LENGTH];
	uint8_t		noteLength[TRACK_LENGTH];
	uint8_t		instrument[TRACK_LENGTH];
};

struct Instrument
{
	uint8_t		waveform;
	uint8_t		pulseWidthSpeed;
	uint8_t		vibratoDepth;
	uint8_t		vibratoSpeed;
};

Track tracks[TRACKS];
Instrument instruments[INSTRUMENTS];

#endif
