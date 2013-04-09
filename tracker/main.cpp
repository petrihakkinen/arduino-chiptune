#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <Windows.h>
#include "Console.h"
#include "AudioStreamXA2.h"
#include "../oscillators.h"
#include "../playroutine.h"
#include <stdio.h>

#define INSTRUMENT_PARAMS	10

const int width = 100;
const int height = 70;
const int songEditorX = 2;
const int songEditorY = 6;
const int trackEditorX = 15;
const int trackEditorY = 6;
const int instrumentEditorX = 50;
const int instrumentEditorY = 6;

Console* g_pConsole = 0;

const char* notes[] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "H-" };

#define EDIT_TRACK		0
#define EDIT_INSTRUMENT	1

int editor = 0;
int instrument = 0;
int instrumentParam = 0;
int trackcol = 0;
int trackcursor = 0;
int transpose = 24;
bool done = false;
bool playing = false;

void drawSong()
{
	int x = songEditorX;
	int y = songEditorY;

	g_pConsole->writeText(x, y++, "Song");
	y++;

	for(int i = 0; i < SONG_LENGTH; i++)
		g_pConsole->writeText(x, y++, "%02x %02x %02x", song.tracks[i][0], song.tracks[i][1], song.tracks[i][2]);
}

void drawTracks()
{
	int x = trackEditorX;

	for(int i = 0; i < CHANNELS; i++)
	{
		Track* tr = &tracks[song.tracks[songpos][i]];

		int y = trackEditorY;

		g_pConsole->writeText(x, y++, "Track %02x", song.tracks[songpos][i]);
		y++;

		for(int yy=0; yy<TRACK_LENGTH; yy++)
		{
			uint8_t note = tr->lines[yy].note;
			if(note > 0 && note < 0xff)
			{
				note--;
				const char* name = notes[note % 12];
				int oct = note / 12;
				g_pConsole->writeText(x, y+yy, " %s%d %02x", name, oct, tr->lines[yy].instrument);
			}
			else if(note == 0xff)
			{
				g_pConsole->writeText(x, y+yy, " off --");
			}
			else
			{
				g_pConsole->writeText(x, y+yy, " --- --");
			}
		}

		if(editor == EDIT_TRACK && trackcol == i)
			g_pConsole->writeText(x, y+trackcursor, ">");

		x += 10;
	}
}

void drawInstrument()
{
	int x = instrumentEditorX;
	int y = instrumentEditorY;

	Instrument* instr = &instruments[instrument];

	const char* forms[] = { "tri  ", "pulse", "saw  ", "noise" };
	instr->waveform &= 3;

	g_pConsole->writeText(x, y++, " Instrument     %02x [/*]", instrument);
	y++;
	g_pConsole->writeText(x, y++, " Waveform       %02x %s", instr->waveform, forms[instr->waveform]);
	g_pConsole->writeText(x, y++, " Attack         %02x", instr->attack);
	g_pConsole->writeText(x, y++, " Decay          %02x", instr->decay);
	g_pConsole->writeText(x, y++, " Sustain        %02x", instr->sustain);
	g_pConsole->writeText(x, y++, " Release        %02x", instr->release);
	g_pConsole->writeText(x, y++, " Pulse Width    %02x", instr->pulseWidth);
    g_pConsole->writeText(x, y++, " Pulse Speed    %02x", instr->pulseWidthSpeed);
	g_pConsole->writeText(x, y++, " Vibrato Depth  %02x", instr->vibratoDepth);
	g_pConsole->writeText(x, y++, " Vibrato Speed  %02x", instr->vibratoSpeed);
	g_pConsole->writeText(x, y++, " Effect         %02x", instr->effect);

	if(editor == EDIT_INSTRUMENT)
		g_pConsole->writeText(x, instrumentEditorY+instrumentParam+2, ">");
}

int keyToNote(int key)
{
	const char* keys = "ZSXDCVGBHNJMQ2W3ER5T6Y7UI9O0P";
	for(size_t i=0; i<strlen(keys); i++)
		if(keys[i] == key)
			return i + transpose;
	return -1;
}

void processInput()
{
	InputEvent ev;
	if(!g_pConsole->readInput(&ev))
		return;

	Track* tr = &tracks[song.tracks[songpos][trackcol]];

	if(ev.keyDown)
	{
		switch(ev.key)
		{
		case VK_TAB:
			editor = (editor + 1) % 2;
			drawTracks();
			drawInstrument();
			break;

		case VK_ESCAPE:
			done = true;
			break;

		case VK_UP:
			if(editor == EDIT_TRACK)
			{
				trackcursor = max(trackcursor-1, 0);
				drawTracks();
			}
			else if(editor == EDIT_INSTRUMENT)
			{
				instrumentParam = max(instrumentParam-1, 0);
				drawInstrument();
			}
			break;

		case VK_DOWN:
			if(editor == EDIT_TRACK)
			{
				trackcursor = min(trackcursor+1, TRACK_LENGTH-1);
				drawTracks();
			}
			else if(editor == EDIT_INSTRUMENT)
			{
				instrumentParam = min(instrumentParam+1, INSTRUMENT_PARAMS-1);
				drawInstrument();
			}
			break;

		case VK_LEFT:
			if(editor == EDIT_TRACK)
			{
				trackcol = max(trackcol-1, 0);
				drawTracks();
			}
			else if(editor == EDIT_INSTRUMENT)
			{
				uint8_t* params = &instruments[instrument].waveform;
				params[instrumentParam] = max(params[instrumentParam]-1, 0);
				drawInstrument();
			}
			break;

		case VK_RIGHT:
			if(editor == EDIT_TRACK)
			{
				trackcol = min(trackcol+1, CHANNELS-1);
				drawTracks();
			}
			else if(editor == EDIT_INSTRUMENT)
			{
				uint8_t* params = &instruments[instrument].waveform;
				params[instrumentParam] = min(params[instrumentParam]+1, 0x7f);
				drawInstrument();
			}
			break;

		case ' ':
			playing = !playing;
			resetOscillators();
			if(playing)
				trackpos = TRACK_LENGTH-1;
			break;

		case 111:
			instrument = max(instrument - 1, 0);
			drawInstrument();
			break;

		case 106:
			instrument = min(instrument + 1, INSTRUMENTS-1);
			drawInstrument();
			break;

		case VK_DELETE:
		case 220:
			tr->lines[trackcursor].note = 0;
			drawTracks();
			break;

		case 189: //'-'
			// note off
			if(editor == EDIT_TRACK)
				tr->lines[trackcursor].note = 0xff;
			drawTracks();
			break;

		default:
			int note = keyToNote(ev.key);
			if(note >= 0)
			{
				if(channel[0].note != note || osc[0].ctrl == 0)
					playNote(0, note, instrument);
				if(editor == EDIT_TRACK)
				{
					tr->lines[trackcursor].note = note;
					tr->lines[trackcursor].instrument = instrument;
					drawTracks();
				}
		}
			break;
		}
	}

	// stop note when key is released
	if(!ev.keyDown)
	{
		int note = keyToNote(ev.key);
		if(note >= 0)
			noteOff(0);
	}
}

unsigned char audioHandler()
{
	static int playCounter = 0;
	playCounter++;
	if(playCounter == 320)
	{
		if(playing)
			playroutine();
		updateEffects();
		playCounter = 0;
	}
	return updateOscillators();
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// initialize COM (required by XAudio2)
	CoInitializeEx( NULL, COINIT_MULTITHREADED );

	Console console(width, height);
	g_pConsole = &console;

	initOscillators();
	initPlayroutine();

	AudioStreamXA2 audioStream;
	audioStream.start();

	drawSong();
	drawTracks();
	drawInstrument();

	while(!done)
	{
		processInput();
		Sleep(20);

		if(playing)
		{
			trackcursor = trackpos;
			drawTracks();
		}
	}
	return 0;
}
