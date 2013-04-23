#define _CRT_SECURE_NO_WARNINGS

#include "Windows.h"
#include "Console.h"
#include <stdio.h>

Console::Console(int width, int height)
{
	m_width = width;
	m_height = height;
	m_cursorX = 0;
	m_cursorY = 0;
	m_textAttributes = NormalText;

	m_pBuffer = new CHAR_INFO[width*height];
	memset(m_pBuffer, 0, sizeof(CHAR_INFO) * width * height);

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
	m_cursorX = x;
	m_cursorY = y;
}

void Console::setTextAttributes(WORD attributes)
{
	m_textAttributes = attributes;
}

void Console::writeText(const char* text)
{
	char ch;
	while(ch = *text++)
	{
		int i = m_cursorY * m_width + m_cursorX;

		if(i >= 0 && i < m_width*m_height)
		{
			m_pBuffer[i].Char.AsciiChar = ch;
			m_pBuffer[i].Attributes = m_textAttributes;
		}

		m_cursorX++;
		if(m_cursorX >= m_width)
		{
			m_cursorX = 0;
			m_cursorY++;
		}
	}
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
	int i = y * m_width + x;
	if(i >= 0 && i < m_width*m_height)
		m_pBuffer[i].Attributes = attributes;
}

void Console::refresh()
{
	COORD pos;
	pos.X = 0;
	pos.Y = 0;

	COORD size;
	size.X = m_width;
	size.Y = m_height;

	SMALL_RECT region = { 0, 0, m_width-1, m_height-1 };

	WriteConsoleOutput(m_output, m_pBuffer, size, pos, &region);
}
