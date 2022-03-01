#pragma once

#include "winrt_d3d.h"
#include "winrt_utils.h"
#include "../gamethread/gt_shared.h"

namespace WinRT
{
    class Win8GameThreadBridge;

    ref class Quake3Platform abstract
    {
    internal:

	    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView) abstract;
	    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window) abstract;
        virtual void Uninitialize() abstract;

        virtual BOOL HandleGameMessage( const GameThread::MSG* msg ) abstract;
        virtual BOOL HandleSystemMessage( const GameThread::MSG* msg ) abstract;
    };

	ref class Quake3FrameworkView sealed : 
        public Windows::ApplicationModel::Core::IFrameworkView
	{
    public:
	    Quake3FrameworkView(Quake3Platform^ platform);
        virtual ~Quake3FrameworkView();

	    // IFrameworkView Methods.
	    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	    virtual void Load(Platform::String^ entryPoint);
	    virtual void Run();
	    virtual void Uninitialize();

    private:

	    // Event Handlers.
	    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	    void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	    void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
	    void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
        void OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args);
        void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);

        // Interaction with the native game layer.
        void HandleExceptionMessage( const GameThread::MSG* msg );
        void WaitForGameReady();
        void HandleMessagesFromGame();
        void HandleMessageFromGame( const GameThread::MSG* msg );

        Win8GameThreadBridge* m_gameCallbacks;
        Quake3Platform^ m_platform;
        std::atomic<bool> m_windowClosed;
	    std::atomic<bool> m_windowVisible;
        Windows::Foundation::Size m_logicalSize;
#ifdef Q_WINRT_FORCE_WIN32_THREAD_API
		HANDLE m_gameThread;
#else
        Concurrency::task<void> m_gameThread;
#endif
        Windows::Foundation::IAsyncOperation<Windows::UI::Popups::IUICommand^>^ m_currentMsgDlg;
	};

    int ConvertDipsToPixels(float dips);
}
