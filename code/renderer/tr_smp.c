/*
===========================================================

SMP acceleration

===========================================================
*/

// @pjb: needed to rip this out of GLimp

#include "tr_local.h"
#include "tr_layer.h"

#include <Windows.h>

#if (!defined(WIN8) && !defined(WIN10)) || defined(Q_WINRT_FORCE_WIN32_THREAD_API)
#   define NATIVE_THREAD_API 1
#endif

HANDLE	rtHaveWorkEvent;            // The render thread has work to do
HANDLE	rtFrameCompleteEvent;       // The renderer just completed a frame

#if NATIVE_THREAD_API
HANDLE	renderThreadHandle;
int  	renderThreadId = 0;
#else
extern void Win8_CreateRenderThread( void (* function)( void* ), void* data );
#endif

static	void	*smpData;

static void RenderThreadWrapper( void (* function)( void ) ) {
	function();

	// unbind the context before we die
    GFX_MakeCurrent( qfalse );
}

/*
=======================
GLimp_SpawnRenderThread
=======================
*/
qboolean RSMP_SpawnRenderThread( void (*function)( void ) ) {

	rtHaveWorkEvent = CreateEventEx( NULL, NULL, 0, EVENT_ALL_ACCESS );
	rtFrameCompleteEvent = CreateEventEx( NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS ); // @pjb: no manual reset on this one

#if NATIVE_THREAD_API
	renderThreadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)RenderThreadWrapper,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   function,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   (LPDWORD) &renderThreadId );

	if ( !renderThreadHandle ) {
		return qfalse;
	}
#else
    Win8_CreateRenderThread( RenderThreadWrapper, function );
#endif

	return qtrue;
}

void* RSMP_RendererSleep( void ) {
	void	*data;

    // We've looped around from a previous frame. Let's mark that as complete.
    GFX_MakeCurrent( qfalse );
    SetEvent( rtFrameCompleteEvent );

    // Wait for work
	WaitForSingleObjectEx( rtHaveWorkEvent, INFINITE, FALSE );
    GFX_MakeCurrent( qtrue );

    // We do this here too in case of a race condition with WakeRenderer
    ResetEvent( rtFrameCompleteEvent );

	data = smpData;

	return data;
}


void RSMP_FrontEndSleep( void ) {
    // Wait for the render thread to complete its current job
	WaitForSingleObjectEx( rtFrameCompleteEvent, INFINITE, FALSE );
    GFX_MakeCurrent( qtrue );
}


void RSMP_WakeRenderer( void *data ) {
    smpData = data;

    // Pass control back to the render thread
    GFX_MakeCurrent( qfalse );

	// Poke the bear
    ResetEvent( rtFrameCompleteEvent ); // The frame is no longer "complete" (new frame)
	SetEvent( rtHaveWorkEvent ); // Wake the render thread.
}

#if NATIVE_THREAD_API
int RSMP_GetRenderThreadId(void) {
    return renderThreadId;
}
#endif

