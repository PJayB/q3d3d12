#pragma once

#include "../winrt/winrt_view.h"

namespace Win8
{
    struct MSG;

    ref class Quake3Win8 sealed : public WinRT::Quake3Platform
    {
    public:
	    Quake3Win8();
	
	    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView) override;
	    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window) override;
        virtual void Uninitialize() override;

    internal: 

        virtual BOOL HandleGameMessage( const GameThread::MSG* msg ) override;
        virtual BOOL HandleSystemMessage( const GameThread::MSG* msg ) override;

    protected:
	    // Event Handlers.
	    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	    void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
	    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	    void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	    void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	    void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	    void OnPointerMoved(Windows::Devices::Input::MouseDevice^ sender, Windows::Devices::Input::MouseEventArgs^ args);
	    void OnPointerWheelChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);

    private:

        void CaptureMouse();
        void ReleaseMouse();

        Windows::Foundation::EventRegistrationToken m_mouseCaptureHandle;
    };

    ref class Quake3Win8ApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
    {
    public:
	    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
    };
}
