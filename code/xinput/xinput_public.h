#ifndef __XINPUT_H__
#define __XINPUT_H__

#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <xinput.h>

// @pjb: gamepad stuff
typedef struct {
    float nx, ny;               // normalized [-1,1]
    short x, y;                 // raw
    short xDeadZone, yDeadZone; // stripped of deadzone (shouldn't be used directly)
} gamepadThumbstickReading_t;

typedef struct {

    // device information
    XINPUT_CAPABILITIES caps;

    // Xinput state
    XINPUT_GAMEPAD lastState;
    XINPUT_GAMEPAD state;

    // thumbstick values
    gamepadThumbstickReading_t leftThumb;
    gamepadThumbstickReading_t rightThumb;

    // old thumbstick values
    gamepadThumbstickReading_t oldLeftThumb;
    gamepadThumbstickReading_t oldRightThumb;

    // timer that counts down to the next controller check
    unsigned short checkTimer;

    // bitfield indicating which buttons were pressed in previous frames
    unsigned short heldButtons;

    // bitfield indicating which buttons were *just* pressed this frame
    unsigned short pressedButtons;

    // bitfield indicating which buttons were *just* released this frame
    unsigned short releasedButtons;

    // bitfield for controller info: userindex and connectivity state
    int userIndex : 3;
    int connected : 1;
    int inserted : 1;
    int removed : 1;

    // bitfield indicating which triggers were held, pressed or released this frame
    int heldLeftTrigger : 1;
    int heldRightTrigger : 1;
    int pressedLeftTrigger : 1;
    int pressedRightTrigger : 1;
    int releasedLeftTrigger : 1;
    int releasedRightTrigger : 1;

} gamepadInfo_t;

void IN_StartupGamepad( void );
void IN_GamepadMove( void );





#endif
