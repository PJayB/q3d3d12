#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>
#include <agile.h>

#include "xbo_app.h"
#include "xbo_common.h"
#include "xbo_msgs.h"

extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Devices::Input;
using namespace Windows::Xbox::System;
using namespace concurrency;

namespace XBO
{

Quake3XboxOne::Quake3XboxOne()
{
}

void Quake3XboxOne::Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView)
{
	// Additional hooks into suspending and resuming (Quake3BaseApp also hooks these)
	CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &Quake3XboxOne::OnSuspending);

	CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &Quake3XboxOne::OnResuming);

	// Hook into constrained/unconstrained event
    CoreApplication::ResourceAvailabilityChanged += 
        ref new EventHandler<Platform::Object^>(this, &Quake3XboxOne::OnResourceAvailabilityChanged);

    // User event callbacks
    User::UserAdded += ref new EventHandler<UserAddedEventArgs^ >(this, &Quake3XboxOne::OnUserAdded);
    User::UserRemoved += ref new EventHandler<UserRemovedEventArgs^ >(this, &Quake3XboxOne::OnUserRemoved);
    User::UserDisplayInfoChanged += ref new EventHandler<UserDisplayInfoChangedEventArgs^>(this, &Quake3XboxOne::OnUserDisplayInfoChanged);
    User::SignInCompleted += ref new EventHandler<SignInCompletedEventArgs^>(this, &Quake3XboxOne::OnSignInCompleted);
    User::SignOutCompleted += ref new EventHandler<SignOutCompletedEventArgs^>(this, &Quake3XboxOne::OnSignOutCompleted);
    User::SignOutStarted += ref new EventHandler<SignOutStartedEventArgs^>(this, &Quake3XboxOne::OnSignOutStarted);
    // @pjb: TODO: User::OnlineStateChanged
}

void Quake3XboxOne::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
}

void Quake3XboxOne::Uninitialize()
{
    // @pjb: todo: remove event hooks
}

void Quake3XboxOne::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
    // TODO: QD3D11::SuspendImmediateContext();
}
 
void Quake3XboxOne::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // TODO: QD3D11::ResumeImmediateContext();
}

void Quake3XboxOne::OnResourceAvailabilityChanged(Platform::Object^ sender, Platform::Object^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = (CoreApplication::ResourceAvailability == ResourceAvailability::Constrained) 
        ? GAME_MSG_XBO_CONSTRAINED
        : GAME_MSG_XBO_UNCONSTRAINED;
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Quake3XboxOne::OnUserAdded(Platform::Object^ sender, Windows::Xbox::System::UserAddedEventArgs^ args)
{
    OnUsersListUpdated();
}

void Quake3XboxOne::OnUserRemoved(Platform::Object^ sender, Windows::Xbox::System::UserRemovedEventArgs^ args)
{
    OnUsersListUpdated();
}

void Quake3XboxOne::OnUserDisplayInfoChanged(Platform::Object^ sender, Windows::Xbox::System::UserDisplayInfoChangedEventArgs^ args)
{
    OnUsersListUpdated();
}

void Quake3XboxOne::OnSignInCompleted(Platform::Object^ sender, Windows::Xbox::System::SignInCompletedEventArgs^ args)
{
    OnUsersListUpdated();
}

void Quake3XboxOne::OnSignOutCompleted(Platform::Object^ sender, Windows::Xbox::System::SignOutCompletedEventArgs^ args)
{
    OnUsersListUpdated();
}

void Quake3XboxOne::OnSignOutStarted(Platform::Object^ sender, Windows::Xbox::System::SignOutStartedEventArgs^ args)
{
    OnUsersListUpdated();
}

BOOL Quake3XboxOne::HandleGameMessage( const GameThread::MSG* msg )
{
	switch (msg->Message)
	{
    case GAME_MSG_XBO_USER_LIST_UPDATED:
        XBO::OnUsersListUpdated();
        return FALSE;
    case GAME_MSG_XBO_CONSTRAINED:
        GameThread::SignalGameConstrained();
        return FALSE;
    case GAME_MSG_XBO_UNCONSTRAINED:
        // Re-cache the user list in case sign-in changes were made.
        XBO::OnUsersListUpdated();
		GameThread::SignalGameUnconstrained();
        return FALSE;
    case GameThread::GAME_MSG_RESUMED:
        // Re-cache the user list in case sign-in changes were made.
        XBO::OnUsersListUpdated();
        break;
	default:
		break;
	}
	return TRUE;
}

BOOL Quake3XboxOne::HandleSystemMessage( const GameThread::MSG* msg )
{
    return TRUE;
}

IFrameworkView^ Quake3XboxOneApplicationSource::CreateView()
{
    auto platform = ref new Quake3XboxOne();
    return ref new WinRT::Quake3FrameworkView(platform);
}

} // namespace XBO

/*
==================
main

==================
*/
[Platform::MTAThread]
int main( Platform::Array<Platform::String^>^ args )
{
    auto q3ApplicationSource = ref new XBO::Quake3XboxOneApplicationSource();

    try
    {
        CoreApplication::Run(q3ApplicationSource);
    }
    catch ( Platform::Exception^ ex )
    {
    }

	return 0;
}
