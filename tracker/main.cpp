#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <Windows.h>
#include "Console.h"
#include "AudioStreamXA2.h"
#include "../oscillators.h"
#include "../playroutine.h"
#include <stdio.h>

#define INSTRUMENT_PARAMS	10

const int width = 80;
const int height = 45;
const int infoX = 2;
const int infoY = 2;
const int songEditorX = 2;
const int songEditorY = 6;
const int trackEditorX = 15;
const int trackEditorY = 6;
const int instrumentEditorX = 50;
const int instrumentEditorY = 6;

Console* g_pConsole = 0;

const char* notes[] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "H-" };

#define EDIT_SONG		0
#define EDIT_TRACK		1
#define EDIT_INSTRUMENT	2

int editor = EDIT_TRACK;
int trackcol = 0;
int trackcursor = 0;
int transpose = 24;
bool done = false;
bool playing = false;

// song editor
static int songcursor = 0;
static int songcol = 0;

// instrument editor
int instrument = 0;
int instrumentParam = 0;
int instrumentcol = 0;

void drawInfo()
{
	int x = infoX;
	int y = infoY;
	g_pConsole->writeText(x, y, "Tempo %02x /*", tempo);
}

void drawSong()
{
	int x = songEditorX;
	int y = songEditorY;

	g_pConsole->writeText(x, y++, "Song");
	y++;

	for(int i = 0; i < SONG_LENGTH; i++)
	{
		if(playing)
			g_pConsole->setTextAttributes(i == songpos ? Console::SelectedText : Console::NormalText);
		g_pConsole->writeText(x, y+i, "%02x %02x %02x", song.tracks[i][0], song.tracks[i][1], song.tracks[i][2]);
	}

	// cursor pos
	if(editor == EDIT_SONG)
	{
		int col = songcol>>1;
		int nibble = songcol & 1;
		g_pConsole->writeTextAttributes(x + col*3 + nibble, y+songcursor, Console::InvertedText);
	}

	g_pConsole->setTextAttributes(Console::NormalText);
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
			WORD attributes = Console::NormalText;
			if(playing && trackpos == yy)
				attributes = Console::SelectedText;
			if(editor == EDIT_TRACK && trackcol == i && trackcursor == yy)
				attributes = Console::InvertedText;
			g_pConsole->setTextAttributes(attributes);

			uint8_t note = tr->lines[yy].note;
			if(note > 0 && note < 0xff)
			{
				note--;
				const char* name = notes[note % 12];
				int oct = note / 12;
				g_pConsole->writeText(x, y+yy, "%s%d %02x", name, oct, tr->lines[yy].instrument);
			}
			else if(note == 0xff)
			{
				g_pConsole->writeText(x, y+yy, "off --");
			}
			else
			{
				g_pConsole->writeText(x, y+yy, "--- --");
			}
			g_pConsole->setTextAttributes(Console::NormalText);
		}

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

	g_pConsole->writeText(x, y++, "Instrument     %02x <>", instrument);
	y++;

	for(int i = 0; i < 10; i++)
	{
		//g_pConsole->setTextAttributes(editor == EDIT_INSTRUMENT && instrumentParam == i ? Console::SelectedText : Console::NormalText);
		switch(i)
		{
		case 0: g_pConsole->writeText(x, y+i, "Waveform       %02x %s", instr->waveform, forms[instr->waveform]); break;
		case 1: g_pConsole->writeText(x, y+i, "Attack         %02x", instr->attack); break;
		case 2: g_pConsole->writeText(x, y+i, "Decay          %02x", instr->decay); break;
		case 3: g_pConsole->writeText(x, y+i, "Sustain        %02x", instr->sustain); break;
		case 4: g_pConsole->writeText(x, y+i, "Release        %02x", instr->release); break;
		case 5: g_pConsole->writeText(x, y+i, "Pulse Width    %02x", instr->pulseWidth); break;
		case 6: g_pConsole->writeText(x, y+i, "Pulse Speed    %02x", instr->pulseWidthSpeed); break;
		case 7: g_pConsole->writeText(x, y+i, "Vibrato Depth  %02x", instr->vibratoDepth); break;
		case 8: g_pConsole->writeText(x, y+i, "Vibrato Speed  %02x", instr->vibratoSpeed); break;
		case 9: g_pConsole->writeText(x, y+i, "Effect         %02x", instr->effect); break;
		}
	}

	// cursor pos
	if(editor == EDIT_INSTRUMENT)
	{
		int col = songcol>>1;
		int nibble = songcol & 1;
		g_pConsole->writeTextAttributes(x + 15 + instrumentcol, y+instrumentParam, BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE);
	}

	g_pConsole->setTextAttributes(Console::NormalText);
}

int keyToNote(int key)
{
	const char* keys = "zsxdcvgbhnjmq2w3er5t6y7ui9o0p";
	for(size_t i = 0; i < strlen(keys); i++)
		if(keys[i] == key)
			return i + transpose;
	return -1;
}

int keyToNibble(int key)
{
	const char* keys = "0123456789abcdef";
	for(int i = 0; i < 16; i++)
		if(keys[i] == key)
			return i;
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
		switch(ev.charCode)
		{
		case '\t':
			editor = (editor + 1) % 3;
			drawSong();
			drawTracks();
			drawInstrument();
			break;

		case ' ':
			playing = !playing;
			resetOscillators();
			if(playing)
			{
				// play song
				songpos = 0;
				trackpos = 0;
			}
			drawSong();
			drawTracks();
			break;

		case '/':
			tempo = max(tempo-1, 1);
			drawInfo();
			break;

		case '*':
			tempo = tempo+1;
			drawInfo();
			break;

		case '<':
			instrument = max(instrument - 1, 0);
			drawInstrument();
			break;

		case '>':
			instrument = min(instrument + 1, INSTRUMENTS-1);
			drawInstrument();
			break;

		case '+':
			if(editor == EDIT_INSTRUMENT)
			{
				uint8_t* params = &instruments[instrument].waveform;
				params[instrumentParam] = min(params[instrumentParam]+1, 0x7f);
				drawInstrument();
			}
			break;

		case '-':
			if(editor == EDIT_INSTRUMENT)
			{
				uint8_t* params = &instruments[instrument].waveform;
				params[instrumentParam] = max(params[instrumentParam]-1, 0);
				drawInstrument();
			}
			else if(editor == EDIT_TRACK)
			{
				tr->lines[trackcursor].note = 0xff;			// note off
				drawTracks();
			}
			break;

		default:
			if(editor == EDIT_SONG)
			{
				int n = keyToNibble(ev.charCode);
				if(n >= 0)
				{
					uint8_t& t = song.tracks[songcursor][songcol>>1];
					if((songcol & 1) == 0)
						t = (t & 0x0f) | (n<<4);
					else
						t = (t & 0xf0) | n;
					t = min(t, TRACKS - 1);
					songcol = min(songcol + 1, CHANNELS*2-1);
					drawSong();
				}
			}
			else if(editor == EDIT_INSTRUMENT)
			{
				int n = keyToNibble(ev.charCode);
				if(n >= 0)
				{
					uint8_t* params = &instruments[instrument].waveform;
					uint8_t& t = params[instrumentParam];
					if((instrumentcol & 1) == 0)
						t = (t & 0x0f) | (n<<4);
					else
						t = (t & 0xf0) | n;
					t = min(t, 0x7f);
					instrumentcol = min(instrumentcol + 1, 1);
					drawInstrument();
				}
			}
			else
			{
				int note = keyToNote(ev.charCode);
				if(note >= 0)
				{
					if(channel[0].note != note || osc[0].ctrl == 0)
						playNote(0, note, instrument);
					if(editor == EDIT_TRACK)
					{
						tr->lines[trackcursor].note = note;
						tr->lines[trackcursor].instrument = instrument;
						trackcursor = min(trackcursor + 1, TRACK_LENGTH-1);
						drawTracks();
					}
				}
			}
			break;
		}

		switch(ev.key)
		{
		case VK_ESCAPE:
			done = true;
			break;

		case VK_DELETE:
			if(editor == EDIT_TRACK)
			{
				tr->lines[trackcursor].note = 0;
				trackcursor = min(trackcursor + 1, TRACK_LENGTH-1);
				drawTracks();
			}
			break;

		case VK_UP:
			if(editor == EDIT_SONG)
			{
				songcursor = max(songcursor-1, 0);
				drawSong();
			}
			else if(editor == EDIT_TRACK)
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
			if(editor == EDIT_SONG)
			{
				songcursor = min(songcursor+1, SONG_LENGTH-1);
				drawSong();
			}
			else if(editor == EDIT_TRACK)
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
			if(editor == EDIT_SONG)
			{
				songcol = max(songcol - 1, 0);
				drawSong();
			}
			else if(editor == EDIT_TRACK)
			{
				trackcol = max(trackcol-1, 0);
				drawTracks();
			}
			else if(editor == EDIT_INSTRUMENT)
			{
				instrumentcol = max(instrumentcol - 1, 0);
				drawInstrument();
			}
			break;

		case VK_RIGHT:
			if(editor == EDIT_SONG)
			{
				songcol = min(songcol + 1, CHANNELS*2 - 1);
				drawSong();
			}
			else if(editor == EDIT_TRACK)
			{
				trackcol = min(trackcol+1, CHANNELS-1);
				drawTracks();
			}
			else if(editor == EDIT_INSTRUMENT)
			{
				instrumentcol = min(instrumentcol + 1, 1);
				drawInstrument();
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

	drawInfo();
	drawSong();
	drawTracks();
	drawInstrument();

	while(!done)
	{
		processInput();
		Sleep(20);

		if(playing)
		{
			drawSong();
			drawTracks();
		}

		g_pConsole->refresh();
	}
	return 0;
}
