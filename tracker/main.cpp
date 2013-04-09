#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <Windows.h>
#include "Console.h"
#include "AudioStreamXA2.h"
#include "../oscillators.h"
#include "../playroutine.h"
#include <stdio.h>

const int width = 100;
const int height = 70;
const int trackEditorX = 5;
const int trackEditorY = 6;
const int instrumentEditorX = 40;
const int instrumentEditorY = 6;

Console* g_pConsole = 0;

const char* notes[] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "H-" };

#define EDIT_TRACK		0
#define EDIT_INSTRUMENT	1

int editor = 0;
int instrument = 0;
int instrumentParam = 0;
int track = 0;
int trackcursor = 0;
int transpose = 12;
bool done = false;

void drawTrack(const Track* tr, int x, int y)
{
	g_pConsole->writeText(x, y++, "Track %02x", track);
	y++;

	for(int yy=0; yy<TRACK_LENGTH; yy++)
	{
		if(tr->noteLength[yy] > 0)
		{
			const char* note = notes[tr->note[yy] % 12];
			int oct = tr->note[yy] / 12;
			g_pConsole->writeText(x, y+yy, " %s%d %02x", note, oct, tr->instrument[yy]);
		}
		else
		{
			g_pConsole->writeText(x, y+yy, " --- --");
		}
	}

	if(editor == EDIT_TRACK)
		g_pConsole->writeText(x, y+trackcursor, ">");
}

void drawTracks()
{
	drawTrack(&tracks[track], trackEditorX, trackEditorY);
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
	g_pConsole->writeText(x, y++, " Pulse Speed    %02x", instr->pulseWidthSpeed);
	g_pConsole->writeText(x, y++, " Vibrato Depth  %02x", instr->vibratoDepth);
	g_pConsole->writeText(x, y++, " Vibrato Speed  %02x", instr->vibratoSpeed);

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

void editTrack()
{
	int ch = g_pConsole->readChar();
	if(ch < 0)
		return;

	const int x = trackEditorX;
	const int y = trackEditorY;

	switch(ch)
	{
	case VK_TAB:
		editor = EDIT_INSTRUMENT;
		drawTracks();
		drawInstrument();
		break;

	case VK_ESCAPE:
		done = true;
		break;

	case VK_UP:
		trackcursor = max(trackcursor-1, 0);
		drawTracks();
		break;

	case VK_DOWN:
		trackcursor = min(trackcursor+1, TRACK_LENGTH-1);
		drawTracks();
		break;

	case VK_LEFT:
		break;

	case VK_RIGHT:
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
		tracks[track].noteLength[trackcursor] = 0;
		drawTracks();
		break;

	default:
		int note = keyToNote(ch);
		if(note >= 0)
		{
			Track* tr = &tracks[track];
			tr->note[trackcursor] = note;
			tr->noteLength[trackcursor] = 1;
			tr->noteLength[trackcursor] = 1;
			tr->instrument[trackcursor] = instrument;
			drawTracks();
		}
		break;
	}
}

void editInstrument()
{
	int ch = g_pConsole->readChar();
	if(ch < 0)
		return;

	const int x = instrumentEditorX;
	const int y = instrumentEditorY;

	switch(ch)
	{
	case VK_TAB:
		editor = EDIT_TRACK;
		drawTracks();
		drawInstrument();
		break;

	case VK_ESCAPE:
		done = true;
		break;

	case VK_UP:
		instrumentParam = max(instrumentParam-1, 0);
		drawInstrument();
		break;

	case VK_DOWN:
		instrumentParam = min(instrumentParam+1, 3);
		drawInstrument();
		break;

	case VK_LEFT:
		{
			uint8_t* params = &instruments[instrument].waveform;
			params[instrumentParam] = (params[instrumentParam]-1) & 0x7f;
			drawInstrument();
		}
		break;

	case VK_RIGHT:
		{
			uint8_t* params = &instruments[instrument].waveform;
			params[instrumentParam] = (params[instrumentParam]+1) & 0x7f;
			drawInstrument();
		}
		break;

	case 111:
		instrument = max(instrument - 1, 0);
		drawInstrument();
		break;

	case 106:
		instrument = min(instrument + 1, INSTRUMENTS-1);
		drawInstrument();
		break;
	}
}

static int playCounter = 0;

unsigned char audioHandler()
{
	playCounter++;
	if(playCounter == 320)
	{
		playroutine();
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

	drawTracks();
	drawInstrument();

	while(!done)
	{
		/*
		static int i = 0;
		i++;
		char text[256];
		sprintf(text, "%d", i);
		g_pConsole->setCursorPos(0, 0);
		g_pConsole->writeText(text);
		*/

		switch(editor)
		{
		case EDIT_TRACK:
			editTrack();
			break;

		case EDIT_INSTRUMENT:
			editInstrument();
			break;
		}

		Sleep(20);
	}

	return 0;
}
