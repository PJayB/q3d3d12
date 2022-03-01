#pragma once

#include <robuffer.h>
#include <wrl.h>
#include <Windows.Storage.Streams.h>

namespace XBO
{
    class WinRtBuffer : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<
            Microsoft::WRL::WinRtClassicComMix>,
        ABI::Windows::Storage::Streams::IBuffer,
        Windows::Storage::Streams::IBufferByteAccess,
        Microsoft::WRL::FtmBase>
    {
    public:

        WinRtBuffer(LPBYTE data, UINT32 dataSize);
        virtual ~WinRtBuffer();

        IFACEMETHODIMP get_Capacity(UINT32* capacity) override;
        IFACEMETHODIMP get_Length(UINT32* length) override;
        IFACEMETHODIMP put_Length(UINT32 length) override;
        IFACEMETHODIMP Buffer(LPBYTE* buffer) override;

    private:
        LPBYTE m_data;
        UINT32 m_size;
        UINT32 m_capacity;
    };

    Windows::Storage::Streams::IBuffer^ WinRtWrapBuffer(LPBYTE data, UINT32 dataSize);
    Windows::Storage::Streams::IBuffer^ WinRtWrapBuffer(const BYTE* data, UINT32 dataSize);
    void GetBufferBytes( Windows::Storage::Streams::IBuffer^ buffer, BYTE** ppOut );
}

