extern "C" {
#   include "../game/q_shared.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
#   include "../client/client.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "xbo_msgs.h"

cvar_t  *in_gamepadCheckDelay;
cvar_t  *in_gamepadLDeadZone;
cvar_t  *in_gamepadRDeadZone;
cvar_t  *in_gamepadXLookSensitivity;
cvar_t  *in_gamepadYLookSensitivity;
cvar_t  *in_gamepadInvertX;
cvar_t  *in_gamepadInvertY;
cvar_t  *in_gamepadTriggerThreshold;

using namespace Windows::Xbox::Input;

struct gamepadButtonMapping_t
{
    GamepadButtons xbo;
    int quake;
};

typedef struct {
    float nx, ny;               // normalized [-1,1]
    float x, y;                 // raw
    float xDeadZone, yDeadZone; // stripped of deadzone (shouldn't be used directly)
} gamepadThumbstickReading_t;

struct gamepadReading_t
{
    gamepadThumbstickReading_t lStick;
    gamepadThumbstickReading_t rStick;
    GamepadButtons buttons;
    FLOAT lTrigger;
    FLOAT rTrigger;
    BOOL bHoldingRTrigger;
    BOOL bHoldingLTrigger;
};

gamepadReading_t g_previousStates[MAX_GAMEPADS];
gamepadReading_t g_currentStates[MAX_GAMEPADS];



static const gamepadButtonMapping_t gamepadButtonMappings[] = { 
    { GamepadButtons::DPadUp         , K_GAMEPAD_DPAD_UP     },
    { GamepadButtons::DPadDown       , K_GAMEPAD_DPAD_DOWN   },
    { GamepadButtons::DPadLeft       , K_GAMEPAD_DPAD_LEFT   },
    { GamepadButtons::DPadRight      , K_GAMEPAD_DPAD_RIGHT  },
    { GamepadButtons::Menu           , K_GAMEPAD_START       },
    { GamepadButtons::View           , K_GAMEPAD_BACK        },
    { GamepadButtons::LeftThumbstick , K_GAMEPAD_LSTICK      },
    { GamepadButtons::RightThumbstick, K_GAMEPAD_RSTICK      },
    { GamepadButtons::LeftShoulder   , K_GAMEPAD_LBUMPER     },
    { GamepadButtons::RightShoulder  , K_GAMEPAD_RBUMPER     },
    { GamepadButtons::A              , K_GAMEPAD_A           },
    { GamepadButtons::B              , K_GAMEPAD_B           },
    { GamepadButtons::X              , K_GAMEPAD_X           },
    { GamepadButtons::Y              , K_GAMEPAD_Y           }
};


/*
=========
XBO Public API
=========
*/
namespace XBO
{
    struct GAMEPAD_INFO
    {
        IGamepad^ Gamepad;
        Windows::Xbox::System::User^ User;
        UINT UserId;
    };

    static GAMEPAD_INFO g_gamepads[Q_MAX_USERS];

    static void UpdateGamepadCache()
    {
        auto allGamepads = Gamepad::Gamepads;
        UINT numGamepads = allGamepads->Size;
        for (UINT gamepadIdx = 0; gamepadIdx < Q_MAX_USERS; ++gamepadIdx)
        {
            IGamepad^ gamepad = nullptr;
            if ( gamepadIdx < numGamepads )
                gamepad = allGamepads->GetAt(gamepadIdx);

            g_gamepads[gamepadIdx].Gamepad = gamepad;
            if (gamepad != nullptr)
            {
                g_gamepads[gamepadIdx].User = gamepad->User;
                if (g_gamepads[gamepadIdx].User != nullptr)
                {
                    g_gamepads[gamepadIdx].UserId = g_gamepads[gamepadIdx].User->Id;
                }
                else
                {
                    g_gamepads[gamepadIdx].UserId = c_NoUser;
                }
            }
            else
            {
                g_gamepads[gamepadIdx].User = nullptr;
                g_gamepads[gamepadIdx].UserId = c_NoUser;
            }
        }
    }

    IGamepad^ GetGamepad(UINT gamepadIndex)
    {
        return g_gamepads[gamepadIndex].Gamepad;
    }

    Windows::Xbox::System::User^ GetUserForGamepad(UINT gamepadIndex)
    {
        return g_gamepads[gamepadIndex].User;
    }

    UINT GetUserIdForGamepad(UINT gamepadIndex)
    {
        return g_gamepads[gamepadIndex].UserId;
    }
}

/*
=========
Support functions
=========
*/

static float GamepadStripDeadzone( float reading, float deadzone )
{
    if ( reading > deadzone )
        return (reading - deadzone);
    else if ( reading < -deadzone )
        return (reading + deadzone);
    else
        return 0;
}

static float GamepadNormalizeThumbstick( float reading, float deadzone )
{
    return reading / (1 - deadzone);
}

static void GetGamepadThumbstickReading( float x, float y, float deadzone, gamepadThumbstickReading_t* thumb )
{
    thumb->x = x;
    thumb->y = y;
    thumb->xDeadZone = GamepadStripDeadzone( x, deadzone );
    thumb->yDeadZone = GamepadStripDeadzone( y, deadzone );
    thumb->nx = GamepadNormalizeThumbstick( thumb->xDeadZone, deadzone );
    thumb->ny = GamepadNormalizeThumbstick( thumb->yDeadZone, deadzone );
}

static void GetGamepadCurrentState(IGamepad^ gamepad, gamepadReading_t* currentState)
{
    // Get the new reading
    IGamepadReading^ reading = nullptr;
    if ( gamepad != nullptr )
    {
        reading = gamepad->GetCurrentReading();
    }

    if ( reading != nullptr )
    {
        currentState->buttons = reading->Buttons;

        currentState->lTrigger = reading->LeftTrigger;
        currentState->rTrigger = reading->RightTrigger;

        GetGamepadThumbstickReading( 
            reading->LeftThumbstickX, 
            reading->LeftThumbstickY,
            in_gamepadLDeadZone->value,
            &currentState->lStick );

        GetGamepadThumbstickReading( 
            reading->RightThumbstickX, 
            reading->RightThumbstickY,
            in_gamepadRDeadZone->value,
            &currentState->rStick );

        currentState->bHoldingLTrigger = currentState->lTrigger > in_gamepadTriggerThreshold->value;
        currentState->bHoldingRTrigger = currentState->rTrigger > in_gamepadTriggerThreshold->value;
    }
    else
    {
        // No new reading, so reset the current state
        ZeroMemory( currentState, sizeof(gamepadReading_t) );
    }

}

static void ApplyGamepad(UINT gamepadIndex, const gamepadReading_t* previousState, const gamepadReading_t* currentState)
{
    // Find the delta between the new states
    UINT pressedButtons = ( (UINT)previousState->buttons ^ (UINT)currentState->buttons ) & (UINT)currentState->buttons;
    UINT releasedButtons = ( (UINT)previousState->buttons ^ (UINT)currentState->buttons ) & ~(UINT)currentState->buttons;
    BOOL pressedLTrigger = ( previousState->bHoldingLTrigger ^ currentState->bHoldingLTrigger ) & currentState->bHoldingLTrigger;
    BOOL releasedLTrigger = ( previousState->bHoldingLTrigger ^ currentState->bHoldingLTrigger ) & ~currentState->bHoldingLTrigger;
    BOOL pressedRTrigger = ( previousState->bHoldingRTrigger ^ currentState->bHoldingRTrigger ) & currentState->bHoldingRTrigger;
    BOOL releasedRTrigger = ( previousState->bHoldingRTrigger ^ currentState->bHoldingRTrigger ) & ~currentState->bHoldingRTrigger;

    //
    // Handle buttons
    //
    for ( int i = 0; i < _countof(gamepadButtonMappings); ++i )
    {
        const gamepadButtonMapping_t* button = &gamepadButtonMappings[i];

        if ( pressedButtons & (UINT)button->xbo )
            Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_KEY, button->quake, qtrue, gamepadIndex, 0, NULL );
        else if ( releasedButtons & (UINT)button->xbo )
            Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_KEY, button->quake, qfalse, gamepadIndex, 0, NULL );
    }
    
    //
    // Handle triggers
    //
    if ( pressedLTrigger )
        Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_LTRIGGER, qtrue, gamepadIndex, 0, NULL );
    else if ( releasedLTrigger )
        Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_LTRIGGER, qfalse, gamepadIndex, 0, NULL );
    
    if ( pressedRTrigger )
        Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_RTRIGGER, qtrue, gamepadIndex, 0, NULL );
    else if ( releasedRTrigger )
        Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_GAMEPAD_RTRIGGER, qfalse, gamepadIndex, 0, NULL );

    //
    // Handle left stick as keys
    //
    // If the values have changed ACCOUNTING FOR DEADZONE then we send messages, but only then
    if ( currentState->lStick.yDeadZone != previousState->lStick.yDeadZone ) {
        int movement = currentState->lStick.ny * 127;
        Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_GAMEPAD_AXIS, AXIS_FORWARD, movement, gamepadIndex, 0, NULL );
    }

    if ( currentState->lStick.xDeadZone != previousState->lStick.xDeadZone ) {
        int movement = currentState->lStick.nx * 127;
        Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_GAMEPAD_AXIS, AXIS_SIDE, movement, gamepadIndex, 0, NULL );
    }

    //
    // Handle right stick as a mouse
    //
    if ( currentState->rStick.nx != 0 || 
         currentState->rStick.ny != 0 ) 
    {
        int invertlookx = in_gamepadInvertX->integer ? -1 : 1;
        int invertlooky = in_gamepadInvertY->integer ? 1 : -1;

        float sensitivityX = in_gamepadXLookSensitivity->value;
        float sensitivityY = in_gamepadYLookSensitivity->value;

        float x = currentState->rStick.nx * invertlookx * sensitivityX;
        float y = currentState->rStick.ny * invertlooky * sensitivityY;

        Sys_QueUserEvent( SYS_EVENT_FRAME_TIME, SE_MOUSE, x, y, gamepadIndex, 0, NULL );
    }
}

// Register gamepad Cvars
static void RegisterGamepadCvars( void )
{
    in_gamepadLDeadZone         = Cvar_Get("in_gamepadLDeadZone",           "0.24",  CVAR_ARCHIVE | CVAR_SYSTEM_SET);
    in_gamepadRDeadZone         = Cvar_Get("in_gamepadRDeadZone",           "0.24",  CVAR_ARCHIVE | CVAR_SYSTEM_SET);
    in_gamepadXLookSensitivity  = Cvar_Get("in_gamepadXLookSensitivity",    "5",     CVAR_ARCHIVE);
    in_gamepadYLookSensitivity  = Cvar_Get("in_gamepadYLookSensitivity",    "5",     CVAR_ARCHIVE);
    in_gamepadInvertX           = Cvar_Get("in_gamepadInvertX",             "0",     CVAR_ARCHIVE);
    in_gamepadInvertY           = Cvar_Get("in_gamepadInvertY",             "0",     CVAR_ARCHIVE);
    in_gamepadTriggerThreshold  = Cvar_Get("in_gamepadTriggerThreshold",    "0.1",   CVAR_ARCHIVE | CVAR_SYSTEM_SET);
}

/*
===========
IN_Init
===========
*/
C_EXPORT void IN_Init( void ) {
    RegisterGamepadCvars();

    ZeroMemory( &g_previousStates, sizeof( g_previousStates ) );
    ZeroMemory( &g_currentStates,  sizeof( g_currentStates ) );

    XBO::UpdateGamepadCache();

	for (UINT gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; ++gamepadIndex)
	{
		IGamepad^ gamepad = XBO::GetGamepad( gamepadIndex );

		// Cache the previous state
		g_previousStates[gamepadIndex] = g_currentStates[gamepadIndex];

		// Fixes a bug when returning from the controls menu where events are fired twice
		// (namely the Go Back button fires spuriously)
		GetGamepadCurrentState( gamepad, &g_currentStates[gamepadIndex] );
	}
}

/*
===========
IN_Shutdown
===========
*/
C_EXPORT void IN_Shutdown( void ) {
    // Nothing.
}

/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
C_EXPORT void IN_Frame (void) {

    XBO::UpdateGamepadCache();

	for (UINT gamepadIndex = 0; gamepadIndex < MAX_GAMEPADS; ++gamepadIndex)
	{
		IGamepad^ gamepad = XBO::GetGamepad( gamepadIndex );
        UINT gamepadUserId = XBO::GetUserIdForGamepad( gamepadIndex );

		// Cache the previous state
		g_previousStates[gamepadIndex] = g_currentStates[gamepadIndex];

		// Get the new state for this frame
		GetGamepadCurrentState( gamepad, &g_currentStates[gamepadIndex] );

		// Send messages based on the new state
        // Only apply if it's the right user or there are no users signed in
        if ( !XBO::IsUserSignedIn() || gamepadUserId == XBO::GetCurrentUserId() )
        {
		    ApplyGamepad( gamepadIndex, &g_previousStates[gamepadIndex], &g_currentStates[gamepadIndex] );
        }
	}
}
