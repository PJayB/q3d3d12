#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>
#include <agile.h>

#include "holo_app.h"
#include "holo_common.h"
#include "holo_msgs.h"

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
using namespace Windows::Graphics::Display;
using namespace Windows::Devices::Input;
using namespace concurrency;

#define NUM_MOUSE_BUTTONS 3

UWP::Quake3UWP::Quake3UWP()
{
}

void UWP::Quake3UWP::Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView)
{
	// Additional hooks into suspending and resuming (Quake3BaseApp also hooks these)
	CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &Quake3UWP::OnSuspending);

	CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &Quake3UWP::OnResuming);

	applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &Quake3UWP::OnActivated);
}

void UWP::Quake3UWP::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
	window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &Quake3UWP::OnWindowSizeChanged);

    window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Quake3UWP::OnPointerPressed);

	window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Quake3UWP::OnPointerReleased);

    window->PointerWheelChanged +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Quake3UWP::OnPointerWheelChanged);

	window->PointerCursor = nullptr;
}

void UWP::Quake3UWP::Uninitialize()
{
    ReleaseMouse();
}

void UWP::Quake3UWP::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
    // Save app state asynchronously after requesting a deferral. Holding a deferral
    // indicates that the application is busy performing suspending operations. Be
    // aware that a deferral may not be held indefinitely. After about five seconds,
    // the app will be forced to exit.
    SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

    // TODO: QD3D12::SuspendImmediateContext();
        
    deferral->Complete();
}

void UWP::Quake3UWP::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // TODO: QD3D12::ResumeImmediateContext();
}

static size_t PackMouseButtons( Windows::UI::Input::PointerPointProperties^ properties )
{
    // @pjb: todo: support for more buttons?
    size_t packed = 0;
    packed |= (properties->IsLeftButtonPressed & 1);
    packed |= (properties->IsRightButtonPressed & 1) << 1;
    packed |= (properties->IsMiddleButtonPressed & 1) << 2;
    return packed;
}

void UWP::Quake3UWP::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = UWP::GAME_MSG_UWP_MOUSE_DOWN;
    msg.Param0 = PackMouseButtons( args->CurrentPoint->Properties );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void UWP::Quake3UWP::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = UWP::GAME_MSG_UWP_MOUSE_UP;
    msg.Param0 = ~PackMouseButtons( args->CurrentPoint->Properties );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void UWP::Quake3UWP::OnPointerMoved(MouseDevice^ mouse, MouseEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = UWP::GAME_MSG_UWP_MOUSE_MOVE;
    msg.Param0 = args->MouseDelta.X;
    msg.Param1 = args->MouseDelta.Y;
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void UWP::Quake3UWP::OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = UWP::GAME_MSG_UWP_MOUSE_WHEEL;
    msg.Param0 = args->CurrentPoint->Properties->MouseWheelDelta;
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void UWP::Quake3UWP::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    CaptureMouse();
}

void UWP::Quake3UWP::OnWindowSizeChanged(CoreWindow^ window, WindowSizeChangedEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = UWP::GAME_MSG_UWP_VIDEO_CHANGE;
    msg.Param0 = WinRT::ConvertDipsToPixels( window->Bounds.Width );
    msg.Param1 = WinRT::ConvertDipsToPixels( window->Bounds.Height );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

BOOL UWP::Quake3UWP::HandleGameMessage( const GameThread::MSG* msg )
{
	switch (msg->Message)
	{
    case GAME_MSG_UWP_VIDEO_CHANGE:
        D3D_WinRT_NotifyWindowResize( (int) msg->Param0, (int) msg->Param1 );
        return FALSE;
    case GAME_MSG_UWP_MOUSE_MOVE:
        Sys_QueEvent( 0, SE_MOUSE, (int) msg->Param0, (int) msg->Param1, 0, NULL );
        return FALSE;
    case GAME_MSG_UWP_MOUSE_WHEEL:
        {
            int delta = (int) msg->Param0;
            if ( delta > 0 )
            {
				Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
				Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
            }
            else
            {
				Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
				Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
            }
        }
        return FALSE;
    case GAME_MSG_UWP_MOUSE_DOWN: 
        for ( size_t i = 0; i < NUM_MOUSE_BUTTONS; ++i )
        {
            // Bit is set if the mouse button was pressed
            if ( msg->Param0 & (1LL << i) )
		        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_MOUSE1 + (int) i, qtrue, 0, NULL );
        }
        return FALSE;
    case GAME_MSG_UWP_MOUSE_UP: 
        for ( size_t i = 0; i < NUM_MOUSE_BUTTONS; ++i )
        {
            // Bit is set if the mouse button was released
            if ( msg->Param0 & (1LL << i) )
		        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_MOUSE1 + (int) i, qfalse, 0, NULL );
        }
        return FALSE;
    default:
		break;
	}
	return TRUE;
}

void UWP::Quake3UWP::CaptureMouse()
{
	m_mouseCaptureHandle = MouseDevice::GetForCurrentView()->MouseMoved +=
		ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &Quake3UWP::OnPointerMoved);
}

void UWP::Quake3UWP::ReleaseMouse()
{
	MouseDevice::GetForCurrentView()->MouseMoved -= m_mouseCaptureHandle;
}

BOOL UWP::Quake3UWP::HandleSystemMessage( const GameThread::MSG* msg )
{
    return TRUE;
}

IFrameworkView^ UWP::Quake3UWPApplicationSource::CreateView()
{
    auto platform = ref new Quake3UWP();
    return ref new WinRT::Quake3FrameworkView(platform);
}

/*
==================
main

==================
*/
[Platform::MTAThread]
int main( Platform::Array<Platform::String^>^ args )
{
    auto q3ApplicationSource = ref new UWP::Quake3UWPApplicationSource();

    try
    {
        CoreApplication::Run(q3ApplicationSource);
    }
    catch ( Platform::Exception^ ex )
    {
    }

	return 0;
}
