#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>
#include <agile.h>

#include "win8_app.h"
#include "../gamethread/gt_shared.h"
#include "../winrt/winrt_utils.h"
#include "../d3d11/d3d_win.h"
#include "win8_msgs.h"

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

Win8::Quake3Win8::Quake3Win8()
{
}

void Win8::Quake3Win8::Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView)
{
	applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &Quake3Win8::OnActivated);
}

void Win8::Quake3Win8::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
	window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &Quake3Win8::OnWindowSizeChanged);

    window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Quake3Win8::OnPointerPressed);

	window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Quake3Win8::OnPointerReleased);

    window->PointerWheelChanged +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Quake3Win8::OnPointerWheelChanged);

	window->PointerCursor = nullptr;
}

void Win8::Quake3Win8::Uninitialize()
{
    ReleaseMouse();
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

void Win8::Quake3Win8::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = Win8::GAME_MSG_WIN8_MOUSE_DOWN;
    msg.Param0 = PackMouseButtons( args->CurrentPoint->Properties );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Win8::Quake3Win8::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = Win8::GAME_MSG_WIN8_MOUSE_UP;
    msg.Param0 = ~PackMouseButtons( args->CurrentPoint->Properties );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Win8::Quake3Win8::OnPointerMoved(MouseDevice^ mouse, MouseEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = Win8::GAME_MSG_WIN8_MOUSE_MOVE;
    msg.Param0 = args->MouseDelta.X;
    msg.Param1 = args->MouseDelta.Y;
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Win8::Quake3Win8::OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = Win8::GAME_MSG_WIN8_MOUSE_WHEEL;
    msg.Param0 = args->CurrentPoint->Properties->MouseWheelDelta;
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Win8::Quake3Win8::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    CaptureMouse();
}

void Win8::Quake3Win8::OnWindowSizeChanged(CoreWindow^ window, WindowSizeChangedEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = Win8::GAME_MSG_WIN8_VIDEO_CHANGE;
    msg.Param0 = WinRT::ConvertDipsToPixels( window->Bounds.Width );
    msg.Param1 = WinRT::ConvertDipsToPixels( window->Bounds.Height );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

BOOL Win8::Quake3Win8::HandleGameMessage( const GameThread::MSG* msg )
{
	switch (msg->Message)
	{
    case GAME_MSG_WIN8_VIDEO_CHANGE:
        D3D_WinRT_NotifyWindowResize( (int) msg->Param0, (int) msg->Param1 );
        return FALSE;
    case GAME_MSG_WIN8_MOUSE_MOVE:
        Sys_QueEvent( 0, SE_MOUSE, (int) msg->Param0, (int) msg->Param1, 0, NULL );
        return FALSE;
    case GAME_MSG_WIN8_MOUSE_WHEEL:
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
    case GAME_MSG_WIN8_MOUSE_DOWN: 
        for ( size_t i = 0; i < NUM_MOUSE_BUTTONS; ++i )
        {
            // Bit is set if the mouse button was pressed
            if ( msg->Param0 & (1LL << i) )
		        Sys_QueEvent( SYS_EVENT_FRAME_TIME, SE_KEY, K_MOUSE1 + (int) i, qtrue, 0, NULL );
        }
        return FALSE;
    case GAME_MSG_WIN8_MOUSE_UP: 
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

void Win8::Quake3Win8::CaptureMouse()
{
	m_mouseCaptureHandle = MouseDevice::GetForCurrentView()->MouseMoved +=
		ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &Quake3Win8::OnPointerMoved);
}

void Win8::Quake3Win8::ReleaseMouse()
{
	MouseDevice::GetForCurrentView()->MouseMoved -= m_mouseCaptureHandle;
}

BOOL Win8::Quake3Win8::HandleSystemMessage( const GameThread::MSG* msg )
{
    return TRUE;
}

IFrameworkView^ Win8::Quake3Win8ApplicationSource::CreateView()
{
    auto platform = ref new Quake3Win8();
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
    auto q3ApplicationSource = ref new Win8::Quake3Win8ApplicationSource();

    try
    {
        CoreApplication::Run(q3ApplicationSource);
    }
    catch ( Platform::Exception^ ex )
    {
    }

	return 0;
}
