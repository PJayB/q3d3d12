#include <xaudio2.h>

extern "C" {
#   include "../client/snd_local.h"
}

#include "xaudio_public.h"
#include "xaudio_local.h"

#define STREAMING_BUFFER_SIZE 0x10000

//#define LOOPING_BUFFERS

static IXAudio2* g_pXAudio2 = NULL;
static IXAudio2MasteringVoice* g_pMasterVoice = NULL;
static IXAudio2SourceVoice* g_pSourceVoice = NULL;

#define STREAMING_BUFFER_COUNT 16
static BYTE g_StreamingBuffers[STREAMING_BUFFER_COUNT][STREAMING_BUFFER_SIZE];
static SIZE_T g_LeakDetect = 0;
static int g_StreamingBufferIndex = 0;
static StreamingVoiceContext g_Context( STREAMING_BUFFER_COUNT );
static double g_Freq;

cvar_t* xaudio_blockWarning = nullptr;
cvar_t* xaudio_lengthScale = nullptr;
cvar_t* xaudio_log = nullptr;
cvar_t* xaudio_allowDrop = nullptr;
cvar_t* xaudio_maxQueueSize = nullptr;

/*
==================
XAudio_Shutdown
==================
*/
void XAudio_Shutdown( void ) 
{
    if ( g_pSourceVoice ) 
    {
        g_pSourceVoice->Stop();
        g_pSourceVoice->DestroyVoice();
    }

    if ( g_pMasterVoice )
    {
        g_pMasterVoice->DestroyVoice();
    }

    if ( g_pXAudio2 )
    {
        g_pXAudio2->Release();
    }

    g_pSourceVoice = NULL;
    g_pMasterVoice = NULL;
    g_pXAudio2 = NULL;

    Com_Printf( "XAudio2 shut down.\n" );
}

/*
==================
XAudio_Init

Initialize direct sound
Returns false if failed
==================
*/
qboolean XAudio_Init(void) 
{
    Com_Printf( "XAudio2 initializing:\n" );

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    // For stats purposes
    LARGE_INTEGER pFreq;
    QueryPerformanceFrequency( &pFreq );
    g_Freq = 1000.0 / pFreq.QuadPart;

    // Create the audio objects
    HRESULT hr = XAudio2Create( &g_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR );
    if ( FAILED( hr ) ) 
    {
        Com_Printf( "... Warning: XAudio2 failed to initialize.\n" );
        goto fail;
    }

    xaudio_lengthScale = Cvar_Get( "xaudio_lengthScale", "0", CVAR_TEMP | CVAR_ARCHIVE );
    xaudio_blockWarning = Cvar_Get( "xaudio_blockWarning", "0", CVAR_TEMP | CVAR_ARCHIVE );
    xaudio_log = Cvar_Get( "xaudio_log", "0", CVAR_TEMP | CVAR_ARCHIVE );
    xaudio_allowDrop = Cvar_Get( "xaudio_allowDrop", "1", CVAR_TEMP | CVAR_ARCHIVE );
    xaudio_maxQueueSize = Cvar_Get( "xaudio_maxQueueSize", "3", CVAR_TEMP | CVAR_ARCHIVE );

    cvar_t* xaudio_debugLevel = Cvar_Get( "xaudio_debugLevel", "0", CVAR_TEMP | CVAR_LATCH );
    if ( xaudio_debugLevel->integer > 0 )
    {
        XAUDIO2_DEBUG_CONFIGURATION dbgCfg;
        ZeroMemory( &dbgCfg, sizeof( dbgCfg ) );
        dbgCfg.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
        
        if ( xaudio_debugLevel->integer > 1 )
            dbgCfg.TraceMask |= XAUDIO2_LOG_INFO;
        if ( xaudio_debugLevel->integer > 2 )
            dbgCfg.TraceMask |= XAUDIO2_LOG_DETAIL;
        if ( xaudio_debugLevel->integer > 3 )
            dbgCfg.TraceMask |= XAUDIO2_LOG_STREAMING;

        if ( IsDebuggerPresent() ) 
        {
            dbgCfg.BreakMask = XAUDIO2_LOG_ERRORS;
        }

        g_pXAudio2->SetDebugConfiguration( &dbgCfg );
    }

    hr = g_pXAudio2->CreateMasteringVoice( &g_pMasterVoice );
    if ( FAILED( hr ) )
    {
        Com_Printf( "... Warning: XAudio2 failed to initialize master voice.\n" );
        goto fail;
    }
    
    // Set up the DMA structure
    dma.channels = 2;
    dma.samplebits = 16;
    dma.speed = 22050;
    Com_Printf( "... Configuring: channels(%d) samplebits(%d) speed(%d).\n", 
        dma.channels, dma.samplebits, dma.speed);

    // Set up the wave format
    WAVEFORMATEX format;
	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = dma.channels;
    format.wBitsPerSample = dma.samplebits;
    format.nSamplesPerSec = dma.speed;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.cbSize = 0;
    format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign; 

    // Create the source voice
    hr = g_pXAudio2->CreateSourceVoice( &g_pSourceVoice, &format, 0, 2.0f, &g_Context );
    if ( FAILED( hr ) )
    {
        Com_Printf( "... Warning: XAudio2 failed to initialize source voice.\n" );
        goto fail;
    }

    // Start the voice (but there's no data yet)
    g_pSourceVoice->Start( 0, 0 );

	dma.channels = format.nChannels;
	dma.samplebits = format.wBitsPerSample;
	dma.speed = format.nSamplesPerSec;
	dma.samples = STREAMING_BUFFER_SIZE / (dma.samplebits/8);
	dma.submission_chunk = 1;
	dma.buffer = NULL;

    dma.manybuffered = qtrue;
    g_StreamingBufferIndex = 0;
    
    for ( int i = 0; i < STREAMING_BUFFER_COUNT; ++i )
    {
        Com_Memset( g_StreamingBuffers[i], 0, dma.samples * dma.samplebits/8 );
    }

    Com_Printf( "... OK.\n" );
    return qtrue;

fail:

    Com_Printf( "... Init Failed: 0x%08X\n", hr );

    XAudio_Shutdown();
    return qfalse;
}

/*
==============
XAudio_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
static int lastSample = 0;
static int bufferOffset = 0;

int XAudio_GetDMAPos( void ) 
{
    return lastSample;
}

/*
==============
XAudio_BeginPainting

Makes sure dma.buffer is valid
===============
*/

void XAudio_BeginPainting( int reserve ) 
{
    // Wait for a buffer to become available

    // Have we still got space in this buffer? Be conservative.
    int reserveBytes = (reserve << 1) * dma.samplebits / 8;
    if ( bufferOffset + reserveBytes > STREAMING_BUFFER_SIZE )
    {
        // Wait for a buffer to become available
        for (;;)
        {
            XAUDIO2_VOICE_STATE state;

            g_pSourceVoice->GetState( &state, XAUDIO2_VOICE_NOSAMPLESPLAYED );

            if (state.BuffersQueued < STREAMING_BUFFER_COUNT - 1)
                break;

            LARGE_INTEGER startTime, endTime;
            QueryPerformanceCounter( &startTime );
            WaitForSingleObjectEx( g_Context.hBufferEndEvent, INFINITE, FALSE );
            QueryPerformanceCounter( &endTime );

            int msBlock = (endTime.QuadPart - startTime.QuadPart) * g_Freq;
            if ( xaudio_blockWarning->integer > 0 && msBlock >= xaudio_blockWarning->integer )
            {
                Com_Printf( "(Prep) Blocked on XAudio buffers for %0dms!\n", msBlock );
            }
        }

        // Reset the buffer
        g_StreamingBufferIndex = (g_StreamingBufferIndex + 1) % STREAMING_BUFFER_COUNT;
        bufferOffset = 0;
    }

    // Pass back the pointer to this buffer
    dma.buffer = g_StreamingBuffers[g_StreamingBufferIndex] + bufferOffset;
}

/*
==============
XAudio_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void XAudio_Submit( int offset, int length ) 
{
    assert( g_LeakDetect == 0 );

    XAUDIO2_VOICE_STATE state;
    g_pSourceVoice->GetState( &state );

    if ( xaudio_log->integer > 0 )
        Com_Printf( "DMA offset %10d length %4d | XAudio2 played %10d | Lead %d\n", offset, length, state.SamplesPlayed, offset - state.SamplesPlayed );

    // Why? Because magic.
    if ( xaudio_lengthScale->integer > 0 )
        length /= xaudio_lengthScale->integer;

    // Get the length in bytes
    int byteLength = length * ( dma.samplebits / 8 );
    byteLength &= ~3; // Round to 4-byte block size

    // Wait for a buffer to become available
    for (;;)
    {
        if ( xaudio_allowDrop->integer > 0 && 
             state.BuffersQueued > xaudio_maxQueueSize->integer )
            return;

        if (state.BuffersQueued < STREAMING_BUFFER_COUNT - 1)
            break;

        LARGE_INTEGER startTime, endTime;
        QueryPerformanceCounter( &startTime );
        WaitForSingleObjectEx( g_Context.hBufferEndEvent, INFINITE, FALSE );
        QueryPerformanceCounter( &endTime );

        int msBlock = (endTime.QuadPart - startTime.QuadPart) * g_Freq;
        if ( xaudio_blockWarning->integer > 0 && msBlock >= xaudio_blockWarning->integer )
        {
            Com_Printf( "(Sub) Blocked on XAudio buffers for %dms!\n", msBlock );
        }

        // Re-query (don't need "samples played" any more.)
        g_pSourceVoice->GetState( &state, XAUDIO2_VOICE_NOSAMPLESPLAYED );
    }

    // Submit the current buffer
    XAUDIO2_BUFFER xbuf;
    ZeroMemory( &xbuf, sizeof( xbuf ) );
    xbuf.AudioBytes = byteLength;
    xbuf.pAudioData = dma.buffer;
    g_pSourceVoice->SubmitSourceBuffer( &xbuf );

    // Counters
    lastSample += length;
    bufferOffset += byteLength;
}

/*
=================
XAudio_Activate

When we change windows we need to do this
=================
*/
void XAudio_Activate( void ) 
{

    // Nothing required here.
}
