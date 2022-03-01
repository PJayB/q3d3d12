//
/*
=======================================================================

START SCREEN

=======================================================================
*/


#include "ui_local.h"


#define ID_PRESSSTART			10

#define MAIN_BANNER_MODEL				"models/mapobjects/banner/banner5.md3"
#define MAIN_MENU_VERTICAL_SPACING		34


typedef struct {
	menuframework_s	menu;
	qhandle_t		bannerModel;
    qboolean        loggingIn;
    int             loggingInUserIndex;
} startscreen_t;

static startscreen_t s_start;

typedef struct {
	menuframework_s menu;	
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_loginErrorMessage;

/*
===============
StartScreen_Cache
===============
*/
void StartScreen_Cache( void ) {
	s_start.bannerModel = trap_R_RegisterModel( MAIN_BANNER_MODEL );
}

/*
===============
LoginErrorMessage_Key

Handle key presses on the error screen
===============
*/
sfxHandle_t LoginErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_StartScreen();
	return (menu_null_sound);
}

/*
===============
PressStart_Key

Handle key presses on the press start screen.
When a key is pressed, it will pass control to the system to show the sign in UI (if applicable).
===============
*/
sfxHandle_t PressStart_Key(int key, int userIndex)
{
    if (s_start.loggingIn) {
        return 0;
    }

	if (userIndex < 0 || userIndex > Q_MAX_USERS) {
		return 0;
	}

    s_start.loggingIn = qtrue;
    s_start.loggingInUserIndex = userIndex;

	trap_Cvar_Set( "com_errorMessage", "" );

    if (UI_AccountEnabled()) {
        trap_Account_SignIn( userIndex );
    } else {
        UI_MainMenu();
    }

	return (menu_in_sound);
}

/*
===============
Start_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Start_MenuDraw( void ) {
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	float			adjust;
	float			x, y, w, h;
	vec4_t			color = {0.5, 0, 0, 1};

    // Did we finish logging in?
    if (UI_AccountEnabled() && 
        s_start.loggingIn && 
        trap_Account_IsUserSignedIn() ) {
        // Load the user's configuration (bindings, preferences)
        trap_Account_LoadConfiguration();

        // Reset the start screen
        s_start.loggingIn = qfalse;
        s_start.loggingInUserIndex = Q_USER_ALL;

        // Switch to the main menu
        UI_MainMenu();
        return;
    }

	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	x = 0;
	y = 0;
	w = 640;
	h = 120;
	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	adjust = 0; // JDC: Kenneth asked me to stop this 1.0 * sin( (float)uis.realtime / 1000 );
	refdef.fov_x = 60 + adjust;
	refdef.fov_y = 19.6875 + adjust;

	refdef.time = uis.realtime;

	origin[0] = 300;
	origin[1] = 0;
	origin[2] = -32;

	trap_R_ClearScene();

	// add the model

	memset( &ent, 0, sizeof(ent) );

	adjust = 5.0 * sin( (float)uis.realtime / 5000 );
	VectorSet( angles, 0, 180 + adjust, 0 );
	AnglesToAxis( angles, ent.axis );
	ent.hModel = s_start.bannerModel;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );
	
	if (strlen(s_loginErrorMessage.errorMessage))
	{
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_loginErrorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	else if (s_start.loggingIn)
    {
		UI_DrawProportionalString_AutoWrapped( 320, 290, 600, 20, "LOGGING IN...", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
    }
    else
	{
		UI_DrawProportionalString_AutoWrapped( 320, 290, 600, 20, "PRESS ANY BUTTON", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}

	if (uis.demoversion) {
		UI_DrawProportionalString( 320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER|UI_SMALLFONT, color );
		UI_DrawString( 320, 400, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	} else {
		UI_DrawString( 320, 450, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	}
}

/*
===============
UI_StartScreen

This is before any user has logged in. If we're dumped here we need to tell the system
that the user has logged out.
===============
*/
void UI_StartScreen( void ) {
	trap_Cvar_Set( "sv_killserver", "1" );

    // @pjb: clean the system logged in user
    if (UI_AccountEnabled()) {
        trap_Account_SignOut();
    }

	memset( &s_start, 0 ,sizeof(s_start) );
	memset( &s_loginErrorMessage, 0 ,sizeof(errorMessage_t) );

	// com_errorMessage would need that too
	StartScreen_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_loginErrorMessage.errorMessage, sizeof(s_loginErrorMessage.errorMessage) );
	if (strlen(s_loginErrorMessage.errorMessage))
	{	
		s_loginErrorMessage.menu.draw = Start_MenuDraw;
		s_loginErrorMessage.menu.key = LoginErrorMessage_Key;
		s_loginErrorMessage.menu.fullscreen = qtrue;
		s_loginErrorMessage.menu.wrapAround = qtrue;
		s_loginErrorMessage.menu.showlogo = qtrue;		

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_loginErrorMessage.menu );
		
		return;
	}

	s_start.menu.draw = Start_MenuDraw;
    s_start.menu.userKey = PressStart_Key;
	s_start.menu.fullscreen = qtrue;
	s_start.menu.wrapAround = qtrue;
	s_start.menu.showlogo = qtrue;      

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu( &s_start.menu );
}
