/*
=========================================================================

@pjb: GAMEPAD

=========================================================================
*/
#include "../client/client.h"
#include "../xinput/xinput_public.h"
#include "../win/win_shared.h"

static gamepadInfo_t gamepads[ XUSER_MAX_COUNT ];

cvar_t  *in_gamepad;
cvar_t  *in_gamepadCheckDelay;
cvar_t  *in_gamepadLDeadZone;
cvar_t  *in_gamepadRDeadZone;
cvar_t  *in_gamepadXLookSensitivity;
cvar_t  *in_gamepadYLookSensitivity;
cvar_t  *in_gamepadInvertX;
cvar_t  *in_gamepadInvertY;

void IN_GetGamepadReading( gamepadInfo_t* gamepad, int userIndex );

// Register gamepad Cvars
void IN_RegisterGamepadCvars( void )
{
    in_gamepad              = Cvar_Get ("in_gamepad",               "1",        CVAR_ARCHIVE|CVAR_LATCH);
    in_gamepadCheckDelay    = Cvar_Get ("in_gamepadCheckDelay",     "200",      CVAR_ARCHIVE);
    in_gamepadLDeadZone     = Cvar_Get ("in_gamepadLDeadZone",      "7849",     CVAR_ARCHIVE);
    in_gamepadRDeadZone     = Cvar_Get ("in_gamepadRDeadZone",      "8689",     CVAR_ARCHIVE);
    in_gamepadXLookSensitivity = Cvar_Get ("in_gamepadXLookSensitivity",      "10",     CVAR_ARCHIVE);
    in_gamepadYLookSensitivity = Cvar_Get ("in_gamepadYLookSensitivity",      "10",     CVAR_ARCHIVE);
    in_gamepadInvertX       = Cvar_Get ("in_gamepadInvertX",        "0",     CVAR_ARCHIVE);
    in_gamepadInvertY       = Cvar_Get ("in_gamepadInvertY",        "0",     CVAR_ARCHIVE);
}

void IN_StartupGamepad(void) {
    int checkTime;
    int i;

    IN_RegisterGamepadCvars();
    
    if (! in_gamepad->integer ) {
        Com_Printf("Gamepad is not active.\n");
        return;
    }

    ZeroMemory( &gamepads, sizeof(gamepads) );

    // Delay before we check for connected gamepads
    checkTime = in_gamepadCheckDelay->integer;
    for ( i = 0; i < XUSER_MAX_COUNT; ++i )
    {
        gamepads[i].checkTimer = i * checkTime / XUSER_MAX_COUNT + 1;

        // Get the base readings (fixes a bug where dupe events are fired on input restart)
        IN_GetGamepadReading( &gamepads[i], i );
    }

    in_gamepad->modified = qfalse;
}

int IN_GamepadStripDeadzone( int reading, int deadzone )
{
    if ( reading > deadzone )
        return (reading - deadzone);
    else if ( reading < -deadzone )
        return (reading + deadzone + 1);
    else
        return 0;
}

#define THUMBSTICK_MAGIC 32767.0f
float IN_GamepadNormalizeThumbstick( int reading, int deadzone )
{
    return reading / (THUMBSTICK_MAGIC - deadzone);
}

void IN_GetGamepadThumbstickReading( short x, short y, int deadzone, gamepadThumbstickReading_t* thumb )
{
    thumb->x = x;
    thumb->y = y;
    thumb->xDeadZone = IN_GamepadStripDeadzone( x, deadzone );
    thumb->yDeadZone = IN_GamepadStripDeadzone( y, deadzone );
    thumb->nx = IN_GamepadNormalizeThumbstick( thumb->xDeadZone, deadzone );
    thumb->ny = IN_GamepadNormalizeThumbstick( thumb->yDeadZone, deadzone );
}

void IN_GetGamepadReading( gamepadInfo_t* gamepad, int userIndex )
{
    XINPUT_STATE xinputState;
    byte wasConnected;
    DWORD getStateR;
    int ldeadzone;
    int rdeadzone;
    qboolean triggerPressed;

    // Query the gamepad state, and if that succeeds the gamepad is connected
    wasConnected = gamepad->connected;
    getStateR = XInputGetState( userIndex, &xinputState );
    gamepad->connected = ( getStateR == ERROR_SUCCESS ) ? qtrue : qfalse;

    // Track insertions and removals
    gamepad->removed = ( wasConnected && !gamepad->connected );
    gamepad->inserted = ( !wasConnected && gamepad->connected );

    // If it's no longer connected, reset the check timer
    if (! gamepad->connected ) {
        gamepad->checkTimer = in_gamepadCheckDelay->integer;
        return;
    }

    // If we've just plugged it in, cache the capabilities of this gamepad 
    if ( gamepad->inserted ) {
        ZeroMemory( gamepad, sizeof( gamepadInfo_t ) );
        gamepad->inserted = qtrue;
        gamepad->connected = qtrue;
        gamepad->userIndex = userIndex;
        XInputGetCapabilities( userIndex, XINPUT_FLAG_GAMEPAD, &gamepad->caps );
    }

    // Get some old state
    ldeadzone = in_gamepadLDeadZone->integer;
    rdeadzone = in_gamepadRDeadZone->integer;

    // Cache the old state
    memcpy( &gamepad->lastState, &gamepad->state, sizeof( XINPUT_GAMEPAD ) );

    // Copy the state
    memcpy( &gamepad->state, &xinputState.Gamepad, sizeof( XINPUT_GAMEPAD ) );

    gamepad->oldLeftThumb = gamepad->leftThumb;
    gamepad->oldRightThumb = gamepad->rightThumb;

    //Get thumbstick values
    IN_GetGamepadThumbstickReading( 
        gamepad->state.sThumbLX, 
        gamepad->state.sThumbLY,
        ldeadzone,
        &gamepad->leftThumb );

    IN_GetGamepadThumbstickReading(
        gamepad->state.sThumbRX,
        gamepad->state.sThumbRY,
        rdeadzone,
        &gamepad->rightThumb );

    // Get the buttons that have been pressed or released since the last update
    gamepad->pressedButtons = ( gamepad->heldButtons ^ gamepad->state.wButtons ) & gamepad->state.wButtons;
    gamepad->releasedButtons = ( gamepad->heldButtons ^ gamepad->state.wButtons ) & ~gamepad->state.wButtons;

    // Cache the current set of held buttons
    gamepad->heldButtons = gamepad->state.wButtons;

    // Figure out whether the left trigger has been pulled or not
    triggerPressed = gamepad->state.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    if ( triggerPressed ) {
        gamepad->pressedLeftTrigger = !gamepad->heldLeftTrigger;
        gamepad->releasedLeftTrigger = qfalse;
    }
    else {
        gamepad->pressedLeftTrigger = qfalse;
        gamepad->releasedLeftTrigger = gamepad->heldLeftTrigger;
    }

    gamepad->heldLeftTrigger = triggerPressed;

    // Figure out whether the left trigger has been pulled or not
    triggerPressed = gamepad->state.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    if ( triggerPressed ) {
        gamepad->pressedRightTrigger = !gamepad->heldRightTrigger;
        gamepad->releasedRightTrigger = qfalse;
    }
    else {
        gamepad->pressedRightTrigger = qfalse;
        gamepad->releasedRightTrigger = gamepad->heldRightTrigger;
    }

    gamepad->heldRightTrigger = triggerPressed;
}

typedef struct 
{
    DWORD xinput;
    int quake;
} gamepadButtonMapping_t;

static const gamepadButtonMapping_t gamepadButtonMappings[] = { 
    { XINPUT_GAMEPAD_DPAD_UP       , K_GAMEPAD_DPAD_UP     },
    { XINPUT_GAMEPAD_DPAD_DOWN     , K_GAMEPAD_DPAD_DOWN   },
    { XINPUT_GAMEPAD_DPAD_LEFT     , K_GAMEPAD_DPAD_LEFT   },
    { XINPUT_GAMEPAD_DPAD_RIGHT    , K_GAMEPAD_DPAD_RIGHT  },
    { XINPUT_GAMEPAD_START         , K_GAMEPAD_START       },
    { XINPUT_GAMEPAD_BACK          , K_GAMEPAD_BACK        },
    { XINPUT_GAMEPAD_LEFT_THUMB    , K_GAMEPAD_LSTICK      },
    { XINPUT_GAMEPAD_RIGHT_THUMB   , K_GAMEPAD_RSTICK      },
    { XINPUT_GAMEPAD_LEFT_SHOULDER , K_GAMEPAD_LBUMPER     },
    { XINPUT_GAMEPAD_RIGHT_SHOULDER, K_GAMEPAD_RBUMPER     },
    { XINPUT_GAMEPAD_A             , K_GAMEPAD_A           },
    { XINPUT_GAMEPAD_B             , K_GAMEPAD_B           },
    { XINPUT_GAMEPAD_X             , K_GAMEPAD_X           },
    { XINPUT_GAMEPAD_Y             , K_GAMEPAD_Y           }
};

void IN_ApplyGamepad( const gamepadInfo_t* gamepad )
{
    int i;

    //
    // Handle buttons
    //
    for ( i = 0; i < _countof(gamepadButtonMappings); ++i )
    {
        const gamepadButtonMapping_t* button = &gamepadButtonMappings[i];

        if ( gamepad->pressedButtons & button->xinput )
            Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, button->quake, qtrue, 0, NULL );
        else if ( gamepad->releasedButtons & button->xinput )
            Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, button->quake, qfalse, 0, NULL );
    }
    
    //
    // Handle triggers
    //
    if ( gamepad->pressedRightTrigger )
        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_RTRIGGER, qtrue, 0, NULL );
    else if ( gamepad->releasedRightTrigger )
        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_RTRIGGER, qfalse, 0, NULL );

    if ( gamepad->pressedLeftTrigger )
        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_LTRIGGER, qtrue, 0, NULL );
    else if ( gamepad->releasedLeftTrigger )
        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_LTRIGGER, qfalse, 0, NULL );
    
    //
    // Handle left stick as keys
    //
    // If the values have changed ACCOUNTING FOR DEADZONE then we send messages, but only then
    if ( gamepad->leftThumb.yDeadZone != gamepad->oldLeftThumb.yDeadZone ) {
        int movement = gamepad->leftThumb.ny * 127;
        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_GAMEPAD_AXIS, AXIS_FORWARD, movement, 0, NULL );
    }

    if ( gamepad->leftThumb.xDeadZone != gamepad->oldLeftThumb.xDeadZone ) {
        int movement = gamepad->leftThumb.nx * 127;
        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_GAMEPAD_AXIS, AXIS_SIDE, movement, 0, NULL );
    }

    //
    // Handle right stick as a mouse
    //
    if ( gamepad->rightThumb.nx != 0 || 
         gamepad->rightThumb.ny != 0 ) 
    {
        float invertlookx = in_gamepadInvertX->integer ? -1.f :  1.f;
        float invertlooky = in_gamepadInvertY->integer ?  1.f : -1.f;

        float sensitivityX = in_gamepadXLookSensitivity->value;
        float sensitivityY = in_gamepadYLookSensitivity->value;

        float x = gamepad->rightThumb.nx * invertlookx * sensitivityX;
        float y = gamepad->rightThumb.ny * invertlooky * sensitivityY;

        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_MOUSE, x, y, 0, NULL );
    }
}

void IN_GamepadMove(void)
{
    int i;
    int firstConnected = -1;
    for ( i = 0; i < XUSER_MAX_COUNT; ++i )
    {
        gamepadInfo_t* gamepad = &gamepads[i];

        // If this isn't connected, we only want to poll it periodically
        if ( !gamepad->connected && gamepad->checkTimer > 0 ) {
            --gamepad->checkTimer;
            continue;
        }

        IN_GetGamepadReading( gamepad, i );

        if (firstConnected == -1 && gamepad->connected)
            firstConnected = i;
    }

    if (firstConnected == -1)
        return;

    // Convert the first connected gamepad to input to Quake
    IN_ApplyGamepad( &gamepads[ firstConnected ] );
}


