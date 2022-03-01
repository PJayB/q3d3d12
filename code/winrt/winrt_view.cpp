extern "C" {
#   include "../client/client.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>

#include "winrt_view.h"
#include "winrt_d3d.h"
#include "winrt_utils.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Devices::Input;
using namespace concurrency;

namespace WinRT
{

int ConvertDipsToPixels(float dips)
{
#if defined(Q_WINRT_FORCE_DIPS_EQUAL_PIXELS)
	return (int) dips;
#elif defined(_WIN32_WINNT_WINBLUE)
	static const float dipsPerInch = 96.0f;
	return (int) floor(dips * Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi / dipsPerInch + 0.5f); // Round to nearest integer.
#else
	static const float dipsPerInch = 96.0f;
	return (int) floor(dips * Windows::Graphics::Display::DisplayProperties::LogicalDpi / dipsPerInch + 0.5f); // Round to nearest integer.
#endif
}

class Win8SystemThreadCallback
{
public:

    virtual void WaitForGameCompleted() = 0;
	virtual BOOL IsGameCompleted() = 0;
    virtual BOOL PopSystemMessage( GameThread::MSG* msg ) = 0;
    virtual void SetWindowReady() = 0;
};

class Win8GameThreadBridge :
    public GameThread::GameThreadCallback,
    public Win8SystemThreadCallback
{
public:
	Win8GameThreadBridge(Quake3Platform^ platform)
        : m_platform(platform)
	{
		// Create an event that we'll use to notify the game thread when our
		// window is ready.
		m_activatedEvent = CreateEventEx( nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS );

		// Create an event that will signal when the game thread is done
		m_completedEvent = CreateEventEx( nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS );
	}

	virtual ~Win8GameThreadBridge()
	{
		CloseHandle( m_activatedEvent );
		CloseHandle( m_completedEvent );
	}

    /*
    ------
    Game thread API
    ------
    */

	void WaitForWindowReady()
	{
		WaitForSingleObjectEx( m_activatedEvent, INFINITE, FALSE );
	}

    BOOL HandleGameMessage( const GameThread::MSG* msg )
    {
        return m_platform->HandleGameMessage( msg );
    }

    void PostSystemMessage( const GameThread::MSG* msg )
    {
        m_systemMessages.Post( msg );
    }

	void Completed()
	{
		SetEvent( m_completedEvent );
	}

    /*
    ------
    System thread API
    ------
    */

    void SetWindowReady()
    {
        SetEvent( m_activatedEvent );
    }

    void WaitForGameCompleted()
    {
        WaitForSingleObjectEx( m_completedEvent, INFINITE, FALSE );
    }

	BOOL IsGameCompleted()
	{
		return WaitForSingleObjectEx( m_completedEvent, 0, FALSE ) == WAIT_OBJECT_0;
	}

    BOOL PopSystemMessage( GameThread::MSG* msg )
    {
        return m_systemMessages.Pop( msg );
    }

private:

	HANDLE m_completedEvent;
	HANDLE m_activatedEvent;
	GameThread::MessageQueue m_systemMessages;
    Quake3Platform^ m_platform;

};

Quake3FrameworkView::Quake3FrameworkView(Quake3Platform^ platform) :
    m_platform(platform),
	m_windowClosed(false),
	m_windowVisible(true),
    m_currentMsgDlg(nullptr)
{
    // Force the graphics layer to wait for window bringup
    D3D_WinRT_InitDeferral();
    
    m_gameCallbacks = new Win8GameThreadBridge(platform);
}

Quake3FrameworkView::~Quake3FrameworkView()
{
    delete m_gameCallbacks;
}

void Quake3FrameworkView::Initialize(CoreApplicationView^ applicationView)
{
    // Initialize the platform object
    m_platform->Initialize(applicationView);

    // @pjb: Todo: on Blue this is way better: poll m_gameThread.is_done()
#ifdef Q_WINRT_FORCE_WIN32_THREAD_API
	m_gameThread = CreateThread( 
		NULL,
		0,
		(LPTHREAD_START_ROUTINE) GameThread::GameThread,
		m_gameCallbacks,
		0,
		NULL );

    SetThreadAffinityMask( m_gameThread, 1 ); // Put on core 0.
#else
    auto gameCallbacks = m_gameCallbacks;
    m_gameThread = Concurrency::create_task( [gameCallbacks] ()
    {
        GameThread::GameThread( gameCallbacks );
    } );
#endif

	applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &Quake3FrameworkView::OnActivated);

	CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &Quake3FrameworkView::OnSuspending);

	CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &Quake3FrameworkView::OnResuming);
}

void Quake3FrameworkView::SetWindow(CoreWindow^ window)
{
    m_platform->SetWindow(window);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &Quake3FrameworkView::OnVisibilityChanged);

	window->Closed += 
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &Quake3FrameworkView::OnWindowClosed);

	window->CharacterReceived +=
		ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &Quake3FrameworkView::OnCharacterReceived);

	window->KeyDown +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &Quake3FrameworkView::OnKeyDown);

	window->KeyUp +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &Quake3FrameworkView::OnKeyUp);

	m_logicalSize = Windows::Foundation::Size(window->Bounds.Width, window->Bounds.Height);

    IUnknown* windowPtr = GetPointer( window );
    D3D_WinRT_NotifyNewWindow( 
        windowPtr, 
        ConvertDipsToPixels( m_logicalSize.Width ), 
        ConvertDipsToPixels( m_logicalSize.Height ) );
}


void Quake3FrameworkView::Load(Platform::String^ entryPoint)
{
}

void Quake3FrameworkView::HandleExceptionMessage( const GameThread::MSG* msg )
{
    assert( msg->Message == GameThread::SYS_MSG_EXCEPTION );
    m_currentMsgDlg = DisplayException( WinRT::GetType<Platform::Exception>( (IUnknown*) msg->Param0 ) );
}

void Quake3FrameworkView::HandleMessagesFromGame()
{
    GameThread::MSG msg;

    // Get new messages
    while ( m_gameCallbacks->PopSystemMessage( &msg ) )
    {
        HandleMessageFromGame( &msg );
    }
}

void Quake3FrameworkView::HandleMessageFromGame( const GameThread::MSG* msg )
{
    if ( m_platform->HandleSystemMessage( msg ) )
    {
        switch ( msg->Message )
        {
        case GameThread::SYS_MSG_EXCEPTION:
            HandleExceptionMessage( msg );
            break;
        case GameThread::SYS_MSG_GAME_READY:
            // Yay, game is ready to go.
            break;
        default:
            assert(0);
            break;
        }
    }
}

void Quake3FrameworkView::Run()
{
	while ( !m_gameCallbacks->IsGameCompleted() )
	{
		if (m_windowVisible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			
            HandleMessagesFromGame();
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}

    // Consume any dangling messagse
    HandleMessagesFromGame();

    // If there's a dialog waiting to be dismissed, poll
    if ( m_currentMsgDlg != nullptr )
    {
        std::atomic<bool> dlgDismissed = false;
        Concurrency::create_task( m_currentMsgDlg ).then([&dlgDismissed] ( Windows::UI::Popups::IUICommand^ ) 
        {
            dlgDismissed = true;
        });

        // @pjb: Todo: on Blue this is way better: task.is_done()
        while ( !dlgDismissed )
        {
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }
}

void Quake3FrameworkView::Uninitialize()
{
    // Wait for the game to finish up
    m_gameCallbacks->WaitForGameCompleted();

    D3D_WinRT_CleanupDeferral();
}

void Quake3FrameworkView::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	// @pjb: todo: if not visible, show menu?
	m_windowVisible = args->Visible;
}

void Quake3FrameworkView::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;

    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = GameThread::GAME_MSG_QUIT;
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

// Pack the keycode information in the Win32 way (given here:
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms646280(v=vs.85).aspx)
static size_t PackKeyStatus( Windows::UI::Core::CorePhysicalKeyStatus status, bool keyDown )
{
    size_t packed;
    packed  = status.RepeatCount & 0xFFFF;
    packed |= (status.ScanCode & 0xFF) << 16;
    packed |= (status.IsExtendedKey & 1) << 24;
    packed |= (status.WasKeyDown & 1) << 30;
    packed |= ((keyDown & 1) ^ 1) << 31;
    return packed;
}

void Quake3FrameworkView::OnCharacterReceived(CoreWindow^ sender, CharacterReceivedEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = GameThread::GAME_MSG_KEY_CHAR;
    msg.Param0 = (size_t) args->KeyCode;
    msg.Param1 = PackKeyStatus( args->KeyStatus, true );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Quake3FrameworkView::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = GameThread::GAME_MSG_KEY_DOWN;
    msg.Param0 = (size_t) args->VirtualKey;
    msg.Param1 = PackKeyStatus( args->KeyStatus, true );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Quake3FrameworkView::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = GameThread::GAME_MSG_KEY_UP;
    msg.Param0 = (size_t) args->VirtualKey;
    msg.Param1 = PackKeyStatus( args->KeyStatus, false );
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

void Quake3FrameworkView::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    if ( args->Kind == ActivationKind::Launch )
    {
        LaunchActivatedEventArgs^ launchArgs = (LaunchActivatedEventArgs^) args;
        SetCommandLine( launchArgs->Arguments );
    }

    auto window = CoreWindow::GetForCurrentThread();
    window->Activate();

    // Let the game thead continue
    m_gameCallbacks->SetWindowReady();
}

void Quake3FrameworkView::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

    HANDLE deferralCompleteEvent = CreateEventEx( 
        nullptr,
        nullptr,
        CREATE_EVENT_MANUAL_RESET,
        EVENT_ALL_ACCESS );

    // Notify the game that we're going away for a while.
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = GameThread::GAME_MSG_SUSPENDED;
    msg.TimeStamp = Sys_Milliseconds();
    msg.Param0 = (size_t) deferralCompleteEvent;
    GameThread::PostGameMessage( &msg );

    // Wait for game to acknowledge that we're suspending. If the game locks up
    // for whatever reason the system may kill our app.
    WaitForSingleObjectEx( deferralCompleteEvent, INFINITE, FALSE );

    // Notify the system that we've acknowledged its suspend request
	deferral->Complete();
}
 
void Quake3FrameworkView::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // Notify the game that we're back.
    GameThread::MSG msg;
    ZeroMemory( &msg, sizeof(msg) );
    msg.Message = GameThread::GAME_MSG_RESUMED;
    msg.TimeStamp = Sys_Milliseconds();
    GameThread::PostGameMessage( &msg );
}

}
