#ifndef AUDIOSTREAMXA2_H
#define AUDIOSTREAMXA2_H

#include <XAudio2.h>

#define BUFFERS		3
#define BUFFER_SIZE	256

// XAudio2 audio output stream
class AudioStreamXA2 : public IXAudio2VoiceCallback
{
public:
	AudioStreamXA2();
	~AudioStreamXA2();

	void start();

	void __stdcall OnBufferEnd(void* pBufferContext);
	void __stdcall OnBufferStart(void* pBufferContext) {}
	void __stdcall OnLoopEnd(void* pBufferContext) {}
	void __stdcall OnStreamEnd() {}
	void __stdcall OnVoiceError(void* pBufferContext, HRESULT error) {}
	void __stdcall OnVoiceProcessingPassEnd() {}
	void __stdcall OnVoiceProcessingPassStart(UINT32 samplesRequired) {}

private:
	IXAudio2*					m_pXAudio2;
	IXAudio2MasteringVoice*		m_pMasteringVoice;
	IXAudio2SourceVoice*		m_pSource;
	XAUDIO2_BUFFER				m_buffers[BUFFERS];
	int							m_currentBuffer;
};

#endif
