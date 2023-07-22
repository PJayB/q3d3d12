#pragma once

extern "C" {
#   include "../qshared/q_shared.h"
}

namespace XBO
{
    class UserInfoCache
    {
    public:
        UserInfoCache();
        ~UserInfoCache();

        void Update();
        BOOL HasUserId(UINT id);
        Windows::Xbox::System::User^ GetUserObject(UINT id);
        BOOL GetGamerTag(UINT id, LPSTR buf, UINT bufLen);
        Windows::Xbox::Storage::ConnectedStorageSpace^ GetStorageForUser(UINT id);
        void SetStorageForUser(UINT id, Windows::Xbox::Storage::ConnectedStorageSpace^ storage);

    private:
        struct USER_INFO
        {
            char DisplayName[256];
            UINT Id;
            accountSignInState_t SignInState;
            std::atomic<BOOL> IsValid;
            Windows::Xbox::System::User^ User;
            Windows::Xbox::Storage::ConnectedStorageSpace^ StorageSpace;
        };

        enum USER_UPDATE_STATE
        {
            USER_UPDATE_OK,
            USER_UPDATE_CURRENT_USER_SIGNED_OUT
        };

        CRITICAL_SECTION m_cs;
        USER_INFO m_cache[Q_MAX_USERS];
        USER_INFO* FindUserInfo_NotThreadSafe(UINT id);
        void Clear_NotThreadSafe();
        void Lock();
        void Unlock();
    };

    // Wrapping this encourages functions that use this to cache the value first 
    // so we don't get code referencing it multiple times in the same code block
    // without a lock
    class CurrentUser
    {
    public:

        CurrentUser() : m_id(c_NoUser) {}

        UINT Get() const { return m_id; }
        void Set(UINT id) { m_id = id; }

    private:
        std::atomic<UINT> m_id;
    };
}
