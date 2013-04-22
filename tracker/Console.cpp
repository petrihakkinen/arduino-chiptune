#define _CRT_SECURE_NO_WARNINGS

#include "Windows.h"
#include "Console.h"
#include <stdio.h>

Console::Console(int width, int height)
{
	m_width = width;
	m_height = height;

	AllocConsole();
	m_input = GetStdHandle(STD_INPUT_HANDLE);
	m_output = GetStdHandle(STD_OUTPUT_HANDLE);

	// disable line input mode
	SetConsoleMode(m_input, 0);

	// resize console
	COORD size;
	size.X = width;
	size.Y = height;
	SetConsoleScreenBufferSize(m_output, size);
    MoveWindow(GetConsoleWindow(), 100, 100, width*9, height*13, TRUE);

	showCursor(false);
}

Console::~Console()
{
	FreeConsole();
}

bool Console::readInput(InputEvent* pEvent)
{
	INPUT_RECORD rec;
	DWORD cnt;
	PeekConsoleInput(m_input, &rec, 1, &cnt);
	if(cnt == 0)
		return false;

	ReadConsoleInput(m_input, &rec, 1, &cnt);
	if( rec.EventType == KEY_EVENT )
	{
		pEvent->keyDown = rec.Event.KeyEvent.bKeyDown != 0;
		pEvent->key = rec.Event.KeyEvent.wVirtualKeyCode;
		pEvent->repeatCount = rec.Event.KeyEvent.wRepeatCount;
		pEvent->charCode = rec.Event.KeyEvent.uChar.AsciiChar;
		return true;
	}

	return false;
}

void Console::showCursor(bool show)
{
	CONSOLE_CURSOR_INFO cursInfo;
	cursInfo.dwSize = 3;
	cursInfo.bVisible = show;
	SetConsoleCursorInfo(m_output, &cursInfo);
}

void Console::setCursorPos(int x, int y)
{
	COORD pos;
	pos.X = x;
	pos.Y = y;
	SetConsoleCursorPosition(m_output, pos);
}

void Console::setTextAttributes(WORD attributes)
{
	SetConsoleTextAttribute(m_output, attributes);
}

void Console::writeText(const char* text)
{
	DWORD cnt;
	WriteConsole(m_output, text, strlen(text), &cnt, 0);
}

void Console::writeText(int x, int y, const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	setCursorPos(x, y);
	writeText(buf);
}

void Console::writeTextAttributes(int x, int y, WORD attributes)
{
	COORD coord;
	coord.X = x;
	coord.Y = y;
	DWORD count; 
	WriteConsoleOutputAttribute(m_output, &attributes, 1, coord, &count);
}
