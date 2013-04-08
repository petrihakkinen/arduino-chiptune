#ifndef AUDIOSTREAMXA2_H
#define AUDIOSTREAMXA2_H

#include <XAudio2.h>

#define BUFFERS		3
#define BUFFER_SIZE	4096*8

// XAudio2 audio output stream
class AudioStreamXA2
{
public:
	AudioStreamXA2();
	~AudioStreamXA2();

	void test();

private:
	IXAudio2*					m_pXAudio2;
	IXAudio2MasteringVoice*		m_pMasteringVoice;
	//IXAudio2SubmixVoice*		m_pSubmixVoice;
	IXAudio2SourceVoice*		m_pSource;
	XAUDIO2_BUFFER				m_buffers[BUFFERS];
};

#endif
