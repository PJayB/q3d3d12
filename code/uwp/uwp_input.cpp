extern "C" {
#   include "../game/q_shared.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
#   include "../xinput/xinput_public.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "uwp_msgs.h"

/*
================
IN_DeactivateMouse
================
*/
C_EXPORT void IN_DeactivateMouse( void ) 
{
    // @pjb: handled automatically by Win8 (alt-tab)
}

/*
===========
IN_Init
===========
*/
C_EXPORT void IN_Init( void ) {
    IN_StartupGamepad();
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
    IN_GamepadMove();
}
