extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "xbo_common.h"

/*
==============
Sys_UserDir

Win8: return the writable directory
==============
*/
C_EXPORT char *Sys_UserDir( void ) {
	static char cwd[MAX_OSPATH];

    // @pjb: todo!
	auto localFolder = ref new Platform::String( L"D:\\" );

    WinRT::CopyString( localFolder, cwd, sizeof(cwd) );

	return cwd;
}

/*
================
Sys_GetClipboardData

================
*/
C_EXPORT char *Sys_GetClipboardData( void ) {

    // @pjb: no clipboard on XBO
    return NULL;
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
C_EXPORT void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( !FreeLibrary( (HMODULE) dllHandle ) ) {
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
C_EXPORT extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

// fqpath param added 7/20/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
// fqpath will be empty if dll not loaded, otherwise will hold fully qualified path of dll module loaded
// fqpath buffersize must be at least MAX_QPATH+1 bytes long
C_EXPORT void * QDECL Sys_LoadDll( const char *name, char *fqpath , vmEntryPoint_t* entryPoint, vmSystemCall_t systemCall ) {
	static int	lastWarning = 0;
	HINSTANCE	libHandle;
	void	(QDECL *dllEntry)( vmSystemCall_t );
	char	filename[MAX_QPATH];
	char	*basepath;
	char	*gamedir;
    char	*cdpath;
	char	*fn;

	*fqpath = 0 ;		// added 7/20/02 by T.Ray

    wchar_t wfilename[MAX_QPATH];

	*fqpath = 0 ;		// added 7/20/02 by T.Ray

	Com_sprintf( filename, sizeof( filename ), "%sxbo.dll", name );
    WinRT::MultiByteToWide( filename, wfilename, _countof(wfilename) );

    basepath = Cvar_VariableString( "fs_basepath" );
    cdpath = Cvar_VariableString( "fs_cdpath" );
    gamedir = Cvar_VariableString( "fs_game" );

    fn = FS_BuildOSPath( basepath, gamedir, filename );
    WinRT::MultiByteToWide( fn, wfilename, _countof(wfilename) );

    libHandle = LoadLibrary( wfilename );
    if (libHandle)
        Com_Printf("LoadLibrary '%s' ok\n", fn);
    else
        Com_Printf("LoadLibrary '%s' failed\n", fn);

    if ( !libHandle ) {
        if( cdpath[0] ) {
            fn = FS_BuildOSPath( cdpath, gamedir, filename );
            WinRT::MultiByteToWide( fn, wfilename, _countof(wfilename) );

            libHandle = LoadLibrary( wfilename );
            if (libHandle)
                Com_Printf("LoadLibrary '%s' ok\n", fn);
            else
                Com_Printf("LoadLibrary '%s' failed\n", fn);
        }

        if ( !libHandle ) {
            return NULL;
        }
    }

	dllEntry = ( void (QDECL *)( vmSystemCall_t ) )GetProcAddress( libHandle, "dllEntry" ); 
	if ( !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}

	*entryPoint = (vmEntryPoint_t)GetProcAddress( libHandle, "vmMainA" );
	if ( !*entryPoint ) {
		FreeLibrary( libHandle );
		return NULL;
	}
	dllEntry( systemCall );

	if ( libHandle ) Q_strncpyz ( fqpath , filename , MAX_QPATH ) ;		// added 7/20/02 by T.Ray
	return libHandle;
}
