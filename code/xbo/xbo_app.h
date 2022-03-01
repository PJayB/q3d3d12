#pragma once

#include <atomic>
#include "xbo_common.h"
#include "../winrt/winrt_shared.h"

namespace XBO
{
    ref class Quake3XboxOne sealed : public WinRT::Quake3Platform
    {
    public:
	    Quake3XboxOne();
	
	    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView) override;
	    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window) override;
        virtual void Uninitialize() override;

    internal: 

        virtual BOOL HandleGameMessage( const GameThread::MSG* msg ) override;
        virtual BOOL HandleSystemMessage( const GameThread::MSG* msg ) override;

    private:

        // Event Handlers.
	    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	    void OnResuming(Platform::Object^ sender, Platform::Object^ args);
        void OnResourceAvailabilityChanged(Platform::Object^ sender, Platform::Object^ args);
        void OnUserAdded(Platform::Object^ sender, Windows::Xbox::System::UserAddedEventArgs^ args);
        void OnUserRemoved(Platform::Object^ sender, Windows::Xbox::System::UserRemovedEventArgs^ args);
        void OnUserDisplayInfoChanged(Platform::Object^ sender, Windows::Xbox::System::UserDisplayInfoChangedEventArgs^ args);
        void OnSignInCompleted(Platform::Object^ sender, Windows::Xbox::System::SignInCompletedEventArgs^ args);
        void OnSignOutCompleted(Platform::Object^ sender, Windows::Xbox::System::SignOutCompletedEventArgs^ args);
        void OnSignOutStarted(Platform::Object^ sender, Windows::Xbox::System::SignOutStartedEventArgs^ args);
    };

    ref class Quake3XboxOneApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
    {
    public:
	    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
    };
}
