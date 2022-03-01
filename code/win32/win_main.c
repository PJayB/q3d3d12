/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// win_main.c

#include "../client/client.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include "../win/win_shared.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <direct.h>
#include <conio.h>

#define	CD_BASEDIR	"quake3"
#define	CD_EXE		"quake3.exe"
#define	CD_BASEDIR_LINUX	"bin\\x86\\glibc-2.1"
#define	CD_EXE_LINUX "quake3"
#define MEM_THRESHOLD 96*1024*1024

//============================================

/*
================
Sys_Milliseconds
================
*/
int			sys_timeBase;
int Sys_Milliseconds (void)
{
	int			sys_curtime;
	static qboolean	initialized = qfalse;

	if (!initialized) {
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/

qboolean Sys_LowPhysicalMemory() {
	MEMORYSTATUS stat;
  GlobalMemoryStatus (&stat);
	return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
    MSG        msg;

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	timeEndPeriod( 1 );

	IN_Shutdown();

	// wait for the user to quit
	while ( 1 ) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit_f ();
		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sys_DestroyConsole();

	exit (1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void ) {
	timeEndPeriod( 1 );
	IN_Shutdown();
	Sys_DestroyConsole();

	exit (0);
}

/*
==============
Sys_Print
==============
*/
void Sys_Print( const char *msg ) {
	Conbuf_AppendText( msg );
}


/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

//========================================================


/*
================
Sys_ScanForCD

Search all the drives to see if there is a valid CD to grab
the cddir from
================
*/
qboolean Sys_ScanForCD( void ) {
	static char	cddir[MAX_OSPATH];
	char		drive[4];
	FILE		*f;
	char		test[MAX_OSPATH];
#if 0
	// don't override a cdpath on the command line
	if ( strstr( sys_cmdline, "cdpath" ) ) {
		return;
	}
#endif

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	// scan the drives
	for ( drive[0] = 'c' ; drive[0] <= 'z' ; drive[0]++ ) {
		if ( GetDriveType (drive) != DRIVE_CDROM ) {
			continue;
		}

		sprintf (cddir, "%s%s", drive, CD_BASEDIR);
		sprintf (test, "%s\\%s", cddir, CD_EXE);
		f = fopen( test, "r" );
		if ( f ) {
			fclose (f);
			return qtrue;
    } else {
      sprintf(cddir, "%s%s", drive, CD_BASEDIR_LINUX);
      sprintf(test, "%s\\%s", cddir, CD_EXE_LINUX);
  		f = fopen( test, "r" );
	  	if ( f ) {
		  	fclose (f);
			  return qtrue;
      }
    }
	}

	return qfalse;
}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) {
				data = Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData ) );
				GlobalUnlock( hClipboardData );
				
				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}


/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( !FreeLibrary( dllHandle ) ) {
		Com_Error (ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine

TTimo: added some verbosity in debug
=================
*/
extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

// fqpath param added 7/20/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
// fqpath will be empty if dll not loaded, otherwise will hold fully qualified path of dll module loaded
// fqpath buffersize must be at least MAX_QPATH+1 bytes long
void * QDECL Sys_LoadDll( const char *name, char *fqpath , vmEntryPoint_t* entryPoint, vmSystemCall_t systemCall ) {
	static int	lastWarning = 0;
	HINSTANCE	libHandle;
	void	(QDECL *dllEntry)( vmSystemCall_t );
	char	*basepath;
	char	*cdpath;
	char	*gamedir;
	char	*fn;
#ifdef NDEBUG
	int		timestamp;
  int   ret;
#endif
	char	filename[MAX_QPATH];

	*fqpath = 0 ;		// added 7/20/02 by T.Ray

#ifdef _M_X64
	Com_sprintf( filename, sizeof( filename ), "%sx64.dll", name );
#elif defined(_ARM_)
	Com_sprintf( filename, sizeof( filename ), "%sarm.dll", name );
#else
    Com_sprintf(filename, sizeof(filename), "%sx86.dll", name);
#endif

#ifdef NDEBUG
	timestamp = Sys_Milliseconds();
	if( ((timestamp - lastWarning) > (5 * 60000)) && !Cvar_VariableIntegerValue( "dedicated" )
		&& !Cvar_VariableIntegerValue( "com_blindlyLoadDLLs" ) ) {
		if (FS_FileExists(filename)) {
			lastWarning = timestamp;
			ret = MessageBoxEx( NULL, "You are about to load a .DLL executable that\n"
				  "has not been verified for use with Quake III Arena.\n"
				  "This type of file can compromise the security of\n"
				  "your computer.\n\n"
				  "Select 'OK' if you choose to load it anyway.",
				  "Security Warning", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND,
				  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ) );
			if( ret != IDOK ) {
				return NULL;
			}
		}
	}
#endif

    // @pjb: allowing use of DLLs in different paths for this build

	libHandle = LoadLibrary( filename );
  if (libHandle)
    Com_Printf("LoadLibrary '%s' ok\n", filename);
  else
    Com_Printf("LoadLibrary '%s' failed\n", filename);
	if ( !libHandle ) {
	basepath = Cvar_VariableString( "fs_basepath" );
	cdpath = Cvar_VariableString( "fs_cdpath" );
	gamedir = Cvar_VariableString( "fs_game" );

	fn = FS_BuildOSPath( basepath, gamedir, filename );
	libHandle = LoadLibrary( fn );
  if (libHandle)
    Com_Printf("LoadLibrary '%s' ok\n", fn);
  else
    Com_Printf("LoadLibrary '%s' failed\n", fn);

	if ( !libHandle ) {
		if( cdpath[0] ) {
			fn = FS_BuildOSPath( cdpath, gamedir, filename );
			libHandle = LoadLibrary( fn );
      if (libHandle)
        Com_Printf("LoadLibrary '%s' ok\n", fn);
      else
        Com_Printf("LoadLibrary '%s' failed\n", fn);
		}

		if ( !libHandle ) {
			return NULL;
		}
	}
	}

	dllEntry = ( void (QDECL *)( vmSystemCall_t ) )GetProcAddress( libHandle, "dllEntry" ); 
	if ( !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}

	*entryPoint = (vmCall_t)GetProcAddress( libHandle, "vmMainA" );
	if ( !*entryPoint ) {
		FreeLibrary( libHandle );
		return NULL;
	}

	dllEntry( systemCall );

	if ( libHandle ) Q_strncpyz ( fqpath , filename , MAX_QPATH ) ;		// added 7/20/02 by T.Ray
	return libHandle;
}

/*
Sys_FrameTime

@pjb: replaces g_wv.sysMsgTime
*/
int g_frameTime = 0;
int Sys_FrameTime( void )
{
    return g_frameTime;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
    MSG			msg;
	sysEvent_t	ev;
	char		*s;
	msg_t		netmsg;
	netadr_t	adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if ( !GetMessage (&msg, NULL, 0, 0) ) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		g_frameTime = msg.time;

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = (int) strlen( s ) + 1;
		b = Z_Malloc( len );
		Q_strncpyz( b, s, len-1 );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = Z_Malloc( len );
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
	ev.evTime = timeGetTime();

	return ev;
}

//================================================================

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

void Sys_Init( void ) {

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );

#pragma warning( disable : 4996 )
	if (!GetVersionEx (&g_wv.osversion))
		Sys_Error ("Couldn't get OS info");
#pragma warning( default : 4996 )

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error ("Quake3 requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Quake3 doesn't run on Win32s");

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		Cvar_Set( "arch", "winnt" );
	}
	else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= WIN98_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win98" );
		}
		else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win95 osr2.x" );
		}
		else
		{
			Cvar_Set( "arch", "win95" );
		}
	}
	else
	{
		Cvar_Set( "arch", "unknown Windows variant" );
	}

	// save out a couple things in rom cvars for the renderer to access
    /* 
        @pjb: ugghh

	Cvar_Get( "win_hinstance", va("%i", (int)g_wv.hInstance), CVAR_ROM );
	Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );
    */

    Sys_DetectCPU();

	IN_Init();		// FIXME: not in dedicated?
}


//=======================================================================

int	totalMsec, countMsec;

/*
==================
WinMain

==================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	char		cwd[MAX_OSPATH];
	int			startTime, endTime;

    // should never get a previous instance in Win32
    if ( hPrevInstance ) {
        return 0;
	}

    g_wv.hPrimaryWnd = NULL;
	g_wv.hInstance = hInstance;
	Q_strncpyz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();
#if 0
	// if we find the CD, add a +set cddir xxx command line
	Sys_ScanForCD();
#endif

	Sys_InitStreamThread();

	Com_Init( sys_cmdline );
	NET_Init();

	_getcwd (cwd, sizeof(cwd));
	Com_Printf("Working directory: %s\n", cwd);

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

    // main game loop
	while( 1 ) {
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
			Sleep( 5 );
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );
//    _controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
                                // syscall turns them back on!

		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}


