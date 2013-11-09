#ifndef CONSOLE_H
#define CONSOLE_H

#define KEYMOD_SHIFT	1
#define KEYMOD_CTRL		2
#define KEYMOD_ALT		4

struct InputEvent
{
	bool keyDown;
	int repeatCount;
	int  key;
	char charCode;
	int keyModifiers;
};

class Console
{
public:
	enum
	{
		NormalText		= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE,
		IntenseText		= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY,
		SelectedText	= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY|BACKGROUND_BLUE,
		InvertedText	= BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE,
	};

	Console(int width, int height);
	~Console();

	// input
	bool readInput(InputEvent* pEvent);

	// output
	void showCursor(bool show);
	void setCursorPos(int x, int y);
	void setTextAttributes(WORD attributes);
	void writeText(const char* text);
	void writeText(int x, int y, const char* fmt, ...);
	void writeTextAttributes(int x, int y, WORD attributes);

	void refresh();

private:
	HANDLE		m_input;
	HANDLE		m_output;
	int			m_width;
	int			m_height;
	CHAR_INFO*	m_pBuffer;
	int			m_cursorX;
	int			m_cursorY;
	int			m_textAttributes;
};

#endif
