extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "winrt_utils.h"
#include "../gamethread/gt_shared.h"

//============================================
// Quake APIs

#if !defined(Q_WINRT_FORCE_WIN32_THREAD_API)
C_EXPORT void Win8_CreateRenderThread( void (* function)( void* ), void* data )
{
    concurrency::create_task( [function, data] () 
    {
        function(data);
    } );
}
#endif

/*
================
Sys_Milliseconds
================
*/
int			sys_timeBase;
C_EXPORT void  Sys_InitTimer (void)
{
	static qboolean	initialized = qfalse;

	if (!initialized) {
        WinRT::InitTimer();
		sys_timeBase = WinRT::GetTime();
		initialized = qtrue;
	}
}

C_EXPORT int Sys_Milliseconds (void)
{
	return WinRT::GetTime() - sys_timeBase;
}

// @pjb I hate this
int g_LastFrameEventTime = 0;
void Sys_SetFrameTime( int time )
{
    g_LastFrameEventTime = time;
}
C_EXPORT int Sys_FrameTime (void)
{
    return g_LastFrameEventTime;
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/

C_EXPORT qboolean Sys_LowPhysicalMemory() {
	// @pjb: TODO: I don't know what the equivalent is here
    return qfalse;
}

/*
==================
Sys_ShowConsole()
==================
*/
C_EXPORT void Sys_ShowConsole( int visLevel, qboolean quitOnClose )
{
    (void)( visLevel );
    (void)( quitOnClose );

    if (quitOnClose)
        GameThread::PostQuitMessage();
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
C_EXPORT void QDECL Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

    OutputDebugStringA( text );

    Platform::String^ textObj = WinRT::CopyString( text );
    WinRT::Throw( E_FAIL, textObj );
}

/*
==============
Sys_Quit
==============
*/
C_EXPORT void Sys_Quit( void ) {
    GameThread::PostQuitMessage();
}

/*
==============
Sys_Print
==============
*/
C_EXPORT void Sys_Print( const char *msg ) {
	OutputDebugStringA( msg );
}

/*
==============
Sys_Cwd

Win8: return the installation directory (read-only)
==============
*/
C_EXPORT char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	Windows::ApplicationModel::Package^ pkg = Windows::ApplicationModel::Package::Current;
    Windows::Storage::StorageFolder^ installDir = pkg->InstalledLocation;

    WinRT::CopyString( installDir->Path, cwd, sizeof(cwd) );

	return cwd;
}

/*
================
Sys_GetEvent

================
*/
C_EXPORT sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t	ev;
	msg_t		netmsg;
	netadr_t	adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = ( netadr_t* ) Z_Malloc( len );
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
C_EXPORT void Sys_Init( void ) {

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	Cvar_Set( "arch", "win8" );

    Sys_DetectCPU();

	// Cvar_Set( "username", Sys_GetCurrentUser() ); @pjb: no longer used

	IN_Init();		// FIXME: not in dedicated?
}

