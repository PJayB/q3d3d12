#pragma once

#include <xdk.h>
#include <wrl.h>
#include <d3d11_x.h>
#include <DirectXMath.h>
#include <pix.h>

#include "../winrt/winrt_shared.h"

namespace XBO
{
    // Input:
    Windows::Xbox::Input::IGamepad^ GetGamepad(UINT gamepadIndex);
    Windows::Xbox::System::User^ GetUserForGamepad(UINT gamepadIndex);
    UINT GetUserIdForGamepad(UINT gamepadIndex);

    // Users:
    static const UINT c_NoUser = ~0U;

    void OnUsersListUpdated();
    BOOL IsUserSignedIn();
    UINT GetCurrentUserId();
    Windows::Xbox::System::User^ GetCurrentUser();
    Windows::Xbox::Storage::ConnectedStorageSpace^ GetCurrentUserStorage();

    // Storage:
    BOOL WriteDataToStorage(
        Windows::Xbox::Storage::ConnectedStorageSpace^ storage,
        LPCWSTR filename,
        const BYTE* buffer,
        UINT bufferSize);
    Windows::Storage::Streams::IBuffer^ ReadDataFromStorage(
        Windows::Xbox::Storage::ConnectedStorageSpace^ storage,
        LPCWSTR filename);
}
