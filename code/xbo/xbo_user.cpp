extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "xbo_common.h"
#include "xbo_user.h"
#include "xbo_buffer.h"

// for constructing a config string
#include <sstream>

using namespace Windows::Xbox::System;
using namespace Windows::Xbox::Input;
using namespace Windows::Xbox::Storage;
using namespace Windows::Xbox::UI;
using namespace Windows::Storage::Streams;

// Internal API
namespace XBO
{
    UserInfoCache::UserInfoCache()
    {
        InitializeCriticalSection(&m_cs);
        Lock();
            Clear_NotThreadSafe();
        Unlock();
    }

    UserInfoCache::~UserInfoCache()
    {
        DeleteCriticalSection(&m_cs);
    }

    void UserInfoCache::Lock()
    {
        EnterCriticalSection(&m_cs);
    }

    void UserInfoCache::Unlock()
    {
        LeaveCriticalSection(&m_cs);
    }

    UserInfoCache::USER_INFO* UserInfoCache::FindUserInfo_NotThreadSafe(UINT id)
    {
        for (UINT i = 0; i < Q_MAX_USERS; ++i)
        {
            if (m_cache[i].Id == id)
                return &m_cache[i];
        }

        return nullptr;
    }

    void UserInfoCache::Clear_NotThreadSafe()
    {
        // Free the COM pointers
        for ( UINT i = 0; i < Q_MAX_USERS; ++i )
        {
            m_cache[i].User = nullptr;
            m_cache[i].StorageSpace = nullptr;
        }

        ZeroMemory( &m_cache, sizeof(USER_INFO) * Q_MAX_USERS );
    }

    static accountSignInState_t GetUserSignInState( User^ user )
    {
        if (user == nullptr || user->IsGuest)
            return ACCOUNT_IS_GUEST;
        else if (user->IsSignedIn)
            return ACCOUNT_SIGNED_IN;
        else
            return ACCOUNT_PENDING; // @pjb: todo: not sure about this
    }

    void UserInfoCache::Update()
    {
        Lock();
        Clear_NotThreadSafe();

        auto users = User::Users;
        UINT userListSize = users->Size;
        for ( UINT i = 0; i < userListSize && i < Q_MAX_USERS; ++i )
        {
            auto user = users->GetAt( i );

            m_cache[i].Id = user->Id;
            m_cache[i].SignInState = GetUserSignInState(user);
            WinRT::CopyString( user->DisplayInfo->Gamertag, m_cache[i].DisplayName, sizeof( m_cache[i].DisplayName ) );
            m_cache[i].IsValid = TRUE;
            m_cache[i].User = user;
        }

        Unlock();
    }

    BOOL UserInfoCache::HasUserId(UINT id)
    {
        if (id == c_NoUser)
            return FALSE;

        Lock();

        BOOL found = FALSE;
        for (UINT i = 0; !found && i < Q_MAX_USERS; ++i)
        {
            if (m_cache[i].IsValid && 
                m_cache[i].Id == id &&
                m_cache[i].SignInState == ACCOUNT_SIGNED_IN)
            {
                found = TRUE;
            }
        }

        Unlock();
        return found;
    }

    User^ UserInfoCache::GetUserObject(UINT userId)
    {
        User^ user = nullptr;
        
        if (userId != c_NoUser)
        {
            Lock();
            const USER_INFO* info = FindUserInfo_NotThreadSafe(userId);
            if (info != nullptr && info->IsValid)
            {
                user = info->User;
            }
            Unlock();
        }

        return user;
    }

    BOOL UserInfoCache::GetGamerTag(UINT userId, LPSTR buf, UINT bufLen)
    {
        BOOL found = FALSE;
        if (userId != c_NoUser)
        {
            Lock();
            const USER_INFO* info = FindUserInfo_NotThreadSafe(userId);
            if (info != nullptr && info->IsValid)
            {
                strcpy_s(buf, bufLen, info->DisplayName);
                found = TRUE;
            }
            Unlock();
        }
        return found;
    }

    ConnectedStorageSpace^ UserInfoCache::GetStorageForUser(UINT userId)
    {
        ConnectedStorageSpace^ storage = nullptr;
        if (userId != c_NoUser)
        {
            Lock();
            const USER_INFO* info = FindUserInfo_NotThreadSafe(userId);
            if (info != nullptr && info->IsValid)
            {
                storage = info->StorageSpace;
            }
            Unlock();
        }
        return storage;
    }

    void UserInfoCache::SetStorageForUser(UINT userId, ConnectedStorageSpace^ storage)
    {
        if (userId != c_NoUser)
        {
            Lock();
            USER_INFO* info = FindUserInfo_NotThreadSafe(userId);
            if (info != nullptr && info->IsValid)
            {
                info->StorageSpace = storage;
            }
            Unlock();
        }
    }



    static UserInfoCache g_userCache;
    static CurrentUser g_currentUserID;
    static std::atomic<qboolean> g_systemDialogOpen = qfalse;

    static void DisconnectIfCurrentUserSignedOut()
    {
        UINT userId = g_currentUserID.Get();
        if (userId != c_NoUser && 
            !g_userCache.HasUserId(userId)) 
        {
            // Reset the signed in user
            g_currentUserID.Set(c_NoUser);

            // Let the user know what happened.
            Cvar_Set("com_errorMessage", "You were signed out.");

            // Drop to splash screen
            CL_Disconnect_f();
		    S_StopAllSounds();
            UIVM_SetActiveMenu( UIMENU_START );
        }
    }

    void OnUsersListUpdated()
    {
        g_userCache.Update();

        // If the current user is now gone, we need to abort
        DisconnectIfCurrentUserSignedOut();
    }
    
    BOOL IsUserSignedIn()
    {
        return g_currentUserID.Get() != c_NoUser ? qtrue : qfalse;
    }

    UINT GetCurrentUserId()
    {
        return g_currentUserID.Get();
    }

    User^ GetCurrentUser()
    {
        UINT userId = GetCurrentUserId();
        if (userId != c_NoUser)
        {
            return g_userCache.GetUserObject(userId);
        }
        return nullptr;
    }

    ConnectedStorageSpace^ GetCurrentUserStorage()
    {
        UINT userId = GetCurrentUserId();
        if (userId != c_NoUser)
        {
            return g_userCache.GetStorageForUser(userId);
        }
        return nullptr;
    }

    // Async denotes this is run on a thread that isn't the main thread
    void PrepareConnectedStorage_Async(User^ user)
    {
        UINT userId = user->Id;
        auto storageTask = concurrency::create_task(ConnectedStorageSpace::GetForUserAsync(user))
            .then([userId] (ConnectedStorageSpace^ storage)
        {
            XBO::g_userCache.SetStorageForUser(userId, storage);
        });

        storageTask.wait();
    }

    // Async denotes this is run on a thread that isn't the main thread
    void EnsureUserIsSignedInAndUpToDate_Async(UINT id)
    {
        // Get the user object
        User^ user = g_userCache.GetUserObject(id);
        if (user == nullptr)
            return;

        // Update their connected storage profile
        PrepareConnectedStorage_Async(user);

        // If the user is different from the currently signed in user, replace them
        g_currentUserID.Set(id);
    }
}

C_EXPORT qboolean Account_IsUserSignedIn( void ) {

    return XBO::IsUserSignedIn() ? qtrue : qfalse;
}

C_EXPORT void Account_SignIn( int controllerIndex )
{
	// If the controller index is invalid we should ignore it
	if ( controllerIndex < 0 || controllerIndex > Q_MAX_USERS )
		return;

    // Update the cache
    XBO::g_userCache.Update();

    // Don't try and show if it's already showing
    if ( XBO::g_systemDialogOpen )
        return;

    // Mark the system dialog as being open
    XBO::g_systemDialogOpen = qtrue;

    // There must be a gamepad associated with the user
    IGamepad^ gamepad = XBO::GetGamepad( controllerIndex );
    if ( gamepad == nullptr )
        return;

    // Get the user of the gamepad
    User^ user = gamepad->User;

    // If the user isn't signed in, we want to throw up the "who are you" system UI, 
    // then do the connected storage sync.
    // However, if the user DOES exist, we just want to sync
    if ( user == nullptr || user->IsSignedIn == FALSE )
    {
        // Kick off the UI
        auto asyncTask = SystemUI::ShowAccountPickerAsync(
            gamepad,
            Windows::Xbox::UI::AccountPickerOptions::None);

        concurrency::create_task( asyncTask ).then([] (concurrency::task<AccountPickerResult^> t) 
        {
            auto result = t.get();

            // If the user is valid then ensure they're signed in and their connected storage is 
            // up to date
            if (result->User != nullptr)
                XBO::EnsureUserIsSignedInAndUpToDate_Async(result->User->Id);
            else
                XBO::g_currentUserID.Set(XBO::c_NoUser);

            XBO::g_systemDialogOpen = qfalse;
        });
    }
    else
    {
        concurrency::create_task([user] ()
        {
            // Sign in the user
            XBO::EnsureUserIsSignedInAndUpToDate_Async(user->Id);
            XBO::g_systemDialogOpen = qfalse;
        });
    }
}

C_EXPORT void Account_SignOut( void )
{
    // If the system dialog is open then we can't really do this without fucking something up
    if ( XBO::g_systemDialogOpen )
        return;

    XBO::g_currentUserID.Set(XBO::c_NoUser);
}

C_EXPORT qboolean Account_GetPlayerName( char* buf, int bufLen )
{
    if (!XBO::IsUserSignedIn())
        return qfalse;

    return XBO::g_userCache.GetGamerTag( XBO::g_currentUserID.Get(), buf, bufLen )
        ? qtrue : qfalse;
}

// @pjb: TODO: query OnlineState for the user

static void Key_WriteBindings( std::stringstream& config ) {
	int		i;

    config << "unbindall\n";

	for (i=0 ; i<256 ; i++) {
		if (keys[i].binding && keys[i].binding[0] ) {
			config << "bind " << Key_KeynumToString(i) << " \"" << keys[i].binding << "\"\n";
		}
	}
}

extern "C" {
    extern cvar_t *cvar_vars;
}

static void Cvar_WriteVariables( std::stringstream& config ) {
	cvar_t	*var;
	char	buffer[1024];

	for (var = cvar_vars ; var ; var = var->next) {
		if( Q_stricmp( var->name, "cl_cdkey" ) == 0 ) {
			continue;
		}
        // Only archive or userinfo settings can be cached.

        // Ignore the serverinfo and ROM flags
        int flags = var->flags & ~(CVAR_SERVERINFO|CVAR_SYSTEMINFO|CVAR_ROM);

        // Only archive archived or archived+usersettings
		if( (flags == CVAR_ARCHIVE) || (flags == (CVAR_USERINFO | CVAR_ARCHIVE)) ) {
			// write the latched value, even if it hasn't taken effect yet
			if ( var->latchedString ) {
				Com_sprintf (buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->latchedString);
			} else {
				Com_sprintf (buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
			}
			config << buffer;
		}
	}
}

C_EXPORT void Account_SaveConfiguration( void )
{
    ConnectedStorageSpace^ storage = XBO::GetCurrentUserStorage();
    if ( storage != nullptr )
    {
        std::stringstream config;

        Key_WriteBindings (config);
	    Cvar_WriteVariables (config);

        std::string configString = std::move(config.str());
   
        XBO::WriteDataToStorage( storage, L"q3config", (LPBYTE) configString.c_str(), (UINT) configString.size() );
    }
}

C_EXPORT void Account_LoadConfiguration( void )
{
    ConnectedStorageSpace^ storage = XBO::GetCurrentUserStorage();
    if ( storage != nullptr )
    {
        // Fetch the buffer from storage
        IBuffer^ buf = XBO::ReadDataFromStorage( storage, L"q3config");
        if (buf == nullptr)
            return;

        // Crack it open and regard it as ASCII
        char* configTextNoTerm = nullptr;
        XBO::GetBufferBytes(buf, (BYTE**) &configTextNoTerm);

        if (configTextNoTerm)
        {
            // Annoyingly we need to null terminate it, so copy that sucka
            char* configText = new char[buf->Length + 1];
            memcpy(configText, configTextNoTerm, buf->Length);
            configText[buf->Length] = 0;

            // Execute the config as a text file
            Cbuf_ExecuteText(EXEC_APPEND, configText);
        }
    }
}

void Account_Init(void) {
	Cvar_Set("ua_enabled", "1");
}

void Account_Shutdown(void) {
	// No need to do anything here.
}

qboolean Account_IsServiceEnabled(void) {
	return qtrue;
}
