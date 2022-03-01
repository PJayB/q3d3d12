#pragma once

#include "gt_msgq.h"

namespace GameThread
{
	// Common to all platforms
	enum GAME_MSG
	{
		GAME_MSG_QUIT,
		GAME_MSG_SUSPENDED,
		GAME_MSG_RESUMED,
		GAME_MSG_KEY_DOWN,
		GAME_MSG_KEY_UP,
		GAME_MSG_KEY_CHAR,
		GAME_MSG_CUSTOM
	};

	enum SYS_MSG
	{
		SYS_MSG_EXCEPTION		= -1,
		SYS_MSG_GAME_READY		= -2,
		_SYS_MSG_MAX
	};

	static_assert(_SYS_MSG_MAX < 0, "_SYS_MSG_MAX must be < 0");

	class GameThreadCallback
	{
	public:

		// This function must block until the system has a valid window
		virtual void WaitForWindowReady() = 0;

		// This function must handle game events. Return TRUE if you
		// want the default handler to also process the event.
		// You will never receive a GAME_MSG_QUIT notification.
		virtual BOOL HandleGameMessage(const MSG* msg) = 0;

		// The game wishes to notify the system of an event. You could push
		// these messages to a queue and handle them on the main thread.
		virtual void PostSystemMessage(const MSG* msg) = 0;

		// Notifies you when it's safe to delete this object
		virtual void Completed() = 0; 
	};

	void GameThread( GameThreadCallback* callback );
	void PostGameMessage( const MSG* msg );
	void PostQuitMessage();
	void SignalGameConstrained();
	void SignalGameUnconstrained();
}
