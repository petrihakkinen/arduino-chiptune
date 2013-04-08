#ifndef CONSOLE_H
#define CONSOLE_H

class Console
{
public:
	Console(int width, int height);
	~Console();

	// input
	int readChar();

	// output
	void setCursorPos(int x, int y);
	void writeText(const char* text);
	void writeText(int x, int y, const char* fmt, ...);

private:
	HANDLE	m_input;
	HANDLE	m_output;
	int		m_width;
	int		m_height;
};

#endif
