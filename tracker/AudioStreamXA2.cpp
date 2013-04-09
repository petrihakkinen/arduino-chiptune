#include "AudioStreamXA2.h"
#include <stdio.h>

extern unsigned char audioHandler();

static void XA2error(const char* err)
{
	printf("XAudio2 error: %s\n", err);
	exit(-1);
}

AudioStreamXA2::AudioStreamXA2()
{
	UINT32 flags = 0;
#ifndef NDEBUG
	flags |= XAUDIO2_DEBUG_ENGINE;
#endif

	if(FAILED(XAudio2Create(&m_pXAudio2, flags, XAUDIO2_DEFAULT_PROCESSOR)))
		XA2error("XAudio2Create failed");

	if(FAILED(m_pXAudio2->CreateMasteringVoice( &m_pMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, NULL)))
		XA2error("CreateMasteringVoice failed");
	m_pMasteringVoice->SetVolume( 1.0f, 1 );

	// create source
	WAVEFORMATEX waveFormatEx;
	memset(&waveFormatEx, 0, sizeof(WAVEFORMATEX));
	waveFormatEx.wFormatTag      = WAVE_FORMAT_PCM;
	waveFormatEx.nChannels       = 1;
	waveFormatEx.nSamplesPerSec  = 16000;
	waveFormatEx.nAvgBytesPerSec = 16000;
	waveFormatEx.nBlockAlign     = 1;
	waveFormatEx.wBitsPerSample  = 8;
	waveFormatEx.cbSize          = 0;

	XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, m_pMasteringVoice };
	XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };
	if(FAILED(m_pXAudio2->CreateSourceVoice(&m_pSource, &waveFormatEx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this, &sendList, NULL)))
		XA2error("CreateSourceVoice");

	// init buffers
	for( int i = 0; i < BUFFERS; ++i )
	{
		m_buffers[i].Flags = 0;
		m_buffers[i].AudioBytes = BUFFER_SIZE;
		m_buffers[i].pAudioData = new BYTE[BUFFER_SIZE];
		m_buffers[i].PlayBegin = 0;
		m_buffers[i].PlayLength = BUFFER_SIZE;
		m_buffers[i].LoopBegin = 0;
		m_buffers[i].LoopLength = 0;
		m_buffers[i].LoopCount = 0;
		m_buffers[i].pContext = this;	// will be passed as the argument for the voice callback
	}

	m_currentBuffer = 0;
}

AudioStreamXA2::~AudioStreamXA2()
{
	m_pSource->DestroyVoice();
	m_pMasteringVoice->DestroyVoice();
	m_pXAudio2->Release();
}

void AudioStreamXA2::start()
{
	OnBufferEnd(this);

	UINT32 operationSet = 1;
	if(FAILED(m_pSource->Start( 0, operationSet)))
		XA2error("Start failed");

	// commit changes
	if(FAILED(m_pXAudio2->CommitChanges( operationSet )))
		XA2error("CommitChanges failed");
}

void AudioStreamXA2::OnBufferEnd(void* pBufferContext)
{
	// called when XAudio2 is done processing an audio buffer	
	AudioStreamXA2* pStream = (AudioStreamXA2*)pBufferContext;

	// fill buffer
	XAUDIO2_BUFFER* pBuffer = &pStream->m_buffers[(++pStream->m_currentBuffer) % BUFFERS] ;
	unsigned char* pData = (unsigned char*)pBuffer->pAudioData;
	for(int i = 0; i < BUFFER_SIZE; i++)
		*pData++ = audioHandler();

	// submit buffer
	m_pSource->SubmitSourceBuffer( pBuffer );
}
