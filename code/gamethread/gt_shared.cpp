extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <windows.h>

#include "gt_shared.h"

void Sys_SetFrameTime( int time );

namespace GameThread
{
    MessageQueue g_gameMsgs;

    void SignalGameConstrained()
    {
        // If in single-player and in-game, pause it
        if ( !( cls.keyCatchers & KEYCATCH_UI ) && cls.state == CA_ACTIVE && !clc.demoplaying )
        {
			if ( SV_IsMultiplayerSession() ) {
				// In multiplayer we just show the bubble above the player's head
				cls.keyCatchers = KEYCATCH_VOID;
			} else  {
				// In singleplayer we go to the menu to pause the game
				 UIVM_SetActiveMenu( UIMENU_INGAME );
			}
        }
    }

	void SignalGameUnconstrained()
	{
		// Undo our keycatch if we set it
		cls.keyCatchers &= ~KEYCATCH_VOID;
	}

	// @pjb: todo: handle platform specific messages here!
    void HandleMessage( const MSG* msg )
    {
        switch (msg->Message)
        {
        case GAME_MSG_KEY_CHAR:
		    Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_CHAR, (int) msg->Param0, 0, 0, NULL );
            break;
        case GAME_MSG_KEY_DOWN:
    		Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, Sys_MapKey( (int) msg->Param1 ), qtrue, 0, NULL );
            break;
        case GAME_MSG_KEY_UP:
    		Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, Sys_MapKey( (int) msg->Param1 ), qfalse, 0, NULL );
            break;
        case GAME_MSG_SUSPENDED:
            // Disconnect from an MP session if in MP. If in SP, throw up the menu.
            if ( com_sv_running && com_sv_running->integer ) {
                SignalGameConstrained();
            } else {
                CL_Disconnect_f();
            }

            // Acknowledge that we recieved this message.
            SetEvent( (HANDLE) msg->Param0 );
            break;
        case GAME_MSG_RESUMED:
			SignalGameUnconstrained();
            break;
        default:
            // Don't know what this is. You should return FALSE from
			// GameThreadCallback::HandleGameMessage.
			assert(0);
			break;
        }
    }

    void GameThreadLoop( GameThreadCallback* platformCallback )
    {
        MSG msg;

        for ( ; ; )
	    {
            // Wait for a new frame
            // @pjb: todo

            // Process any messages from the queue
            while ( g_gameMsgs.Pop( &msg ) )
            {
                if ( msg.Message == GAME_MSG_QUIT )
                    return;

                Sys_SetFrameTime( (int) msg.TimeStamp );

				if ( platformCallback->HandleGameMessage( &msg ) )
				{
					HandleMessage( &msg );
				}
            }

		    // make sure mouse and joystick are only called once a frame
		    IN_Frame();

		    // run the game
		    Com_Frame();
	    };
    }

    void PostGameReadyMsg( GameThreadCallback* callback )
    {
        MSG msg;
        ZeroMemory( &msg, sizeof( msg ) );
        msg.Message = (size_t) GameThread::SYS_MSG_GAME_READY;
        msg.TimeStamp = Sys_Milliseconds();
        callback->PostSystemMessage( &msg );
    }

	void PostGameMessage( const MSG* msg )
	{
		g_gameMsgs.Post(msg);
	}

    void GameThread( GameThreadCallback* platformCallback )
    {
        // Init quake stuff
	    Sys_InitTimer();
        Sys_InitStreamThread();

        // We need to wait for window activation before we can continue.
        //WaitForSingleObjectEx( windowReadyEvent, INFINITE, FALSE );
		platformCallback->WaitForWindowReady();

        // Sys commandline should be valid at this stage
	    Com_Init( sys_cmdline );
	    NET_Init();
        IN_Init();

	    Com_Printf( "Working directory: %s\n", Sys_Cwd() );

        PostGameReadyMsg( platformCallback );

        // Loop until done
        GameThreadLoop( platformCallback );

        Com_Quit_f();

        IN_Shutdown();
        NET_Shutdown();

		// Tell the main thread we're done with this object
		platformCallback->Completed();
    }

	void PostQuitMessage()
	{
		MSG msg;
		ZeroMemory( &msg, sizeof(msg) );
		msg.Message = GAME_MSG_QUIT;
		msg.TimeStamp = Sys_Milliseconds();
		g_gameMsgs.Post(&msg);
	}
}
