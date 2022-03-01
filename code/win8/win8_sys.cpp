extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "../winrt/winrt_utils.h"

/*
==============
Sys_UserDir

Win8: return the writable directory
==============
*/
C_EXPORT char *Sys_UserDir( void ) {
	static char cwd[MAX_OSPATH];

	auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;

    WinRT::CopyString( localFolder->Path, cwd, sizeof(cwd) );

	return cwd;
}

/*
================
Sys_GetClipboardData

================
*/
C_EXPORT char *Sys_GetClipboardData( void ) {

    static char clipBuf[4096];

    auto data = Windows::ApplicationModel::DataTransfer::Clipboard::GetContent();
    if ( data->Contains( Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text ) )
    {
        auto t = concurrency::create_task( data->GetTextAsync() );

        WinRT::CopyString( t.get(), clipBuf, sizeof( clipBuf ) );
        return clipBuf;
    }
    else
    {
        return NULL;
    }
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
C_EXPORT void * QDECL Sys_LoadDll( const char *name, char *fqpath , int (QDECL **entryPoint)(size_t, ...),
				  int (QDECL *systemcalls)(size_t, ...) ) {
	static int	lastWarning = 0;
	HINSTANCE	libHandle;
	void	(QDECL *dllEntry)( int (QDECL *syscallptr)(size_t, ...) );
	char	filename[MAX_QPATH];
    wchar_t wfilename[MAX_QPATH];

	*fqpath = 0 ;		// added 7/20/02 by T.Ray

#ifdef _M_X64
	Com_sprintf( filename, sizeof( filename ), "%sx64.dll", name );
#elif defined(_ARM_)
	Com_sprintf( filename, sizeof( filename ), "%sarm.dll", name );
#else
    Com_sprintf(filename, sizeof(filename), "%sx86.dll", name);
#endif

    WinRT::MultiByteToWide( filename, wfilename, _countof( wfilename ) );
    
    // There's a lot of logic here in the original game (dealing with overloaded
    // paths etc), but WinRT simplifies that for us. It can only ever be in the root 
    // folder.
	libHandle = LoadPackagedLibrary( wfilename, 0 );
    if (libHandle) {
        Com_Printf("LoadPackagedLibrary '%s' ok\n", filename);
    }
    else {
        Com_Printf("LoadPackagedLibrary '%s' failed\n", filename);
        return NULL;
    }

	dllEntry = ( void (QDECL *)( int (QDECL *)( size_t, ... ) ) )GetProcAddress( libHandle, "dllEntry" ); 
	*entryPoint = (int (QDECL *)(size_t,...))GetProcAddress( libHandle, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}
	dllEntry( systemcalls );

	if ( libHandle ) Q_strncpyz ( fqpath , filename , MAX_QPATH ) ;		// added 7/20/02 by T.Ray
	return libHandle;
}
