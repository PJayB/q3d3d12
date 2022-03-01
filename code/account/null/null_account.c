/*
=========================================================================

@pjb: NULL ACCOUNT

=========================================================================
*/
#include "../../client/client.h"

// @pjb: enable the user accounts service?
cvar_t  *ua_enabled;

static void Account_InvalidOperation() {
    Com_Error( ERR_FATAL, "Account service was accessed without being checked for status first." );
}

void Account_Init( void ) {
    // @pjb: enable the user account service?
    ua_enabled = Cvar_Get( "ua_enabled", "0", CVAR_ROM ); // CVAR_INIT for valid platforms
}

void Account_Shutdown( void ) {
}

qboolean Account_IsServiceEnabled( void ) {
    return qfalse;
}

void Account_SignIn( int controllerIndex ) { Account_InvalidOperation(); }
void Account_SignOut( void ) { Account_InvalidOperation(); }
qboolean Account_GetPlayerName( char* buf, int bufLen) { Account_InvalidOperation(); return qfalse; }
void Account_SaveConfiguration( void ) { Account_InvalidOperation(); }
void Account_LoadConfiguration( void ) { Account_InvalidOperation(); }
qboolean Account_IsUserSignedIn( void ) { Account_InvalidOperation(); return qfalse; }
