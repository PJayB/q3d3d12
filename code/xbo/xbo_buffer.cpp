#include "xbo_buffer.h"

using namespace Windows::Storage::Streams;

namespace XBO
{
    WinRtBuffer::WinRtBuffer(LPBYTE data, UINT32 dataSize)
        : m_data(data)
        , m_capacity(dataSize)
        , m_size(dataSize)
    {
    }

    WinRtBuffer::~WinRtBuffer()
    {
    }

    IFACEMETHODIMP WinRtBuffer::get_Capacity(UINT32* capacity)
    {
        *capacity = m_capacity;
        return S_OK;
    }

    IFACEMETHODIMP WinRtBuffer::get_Length(UINT32* length)
    {
        *length = m_size;
        return S_OK;
    }

    IFACEMETHODIMP WinRtBuffer::put_Length(UINT32 length)
    {
        if (length > m_capacity)
            return E_INVALIDARG;
        
        m_size = length;
        return S_OK;
    }

    IFACEMETHODIMP WinRtBuffer::Buffer(LPBYTE* buf) 
    {
        if (m_data != nullptr)
        {
            *buf = m_data;
            return S_OK;
        }
        
        return E_FAIL;
    }

    Windows::Storage::Streams::IBuffer^ WinRtWrapBuffer(LPBYTE data, UINT32 dataSize)
    {
        return reinterpret_cast<Windows::Storage::Streams::IBuffer^>(
            Microsoft::WRL::Make<WinRtBuffer>(data, dataSize).Get());
    }

    // Sorry.
    Windows::Storage::Streams::IBuffer^ WinRtWrapBuffer(const BYTE* data, UINT32 dataSize)
    {
        return reinterpret_cast<Windows::Storage::Streams::IBuffer^>(
            Microsoft::WRL::Make<WinRtBuffer>(const_cast<LPBYTE>(data), dataSize).Get());
    }

    void GetBufferBytes( Windows::Storage::Streams::IBuffer^ buffer, BYTE** ppOut )
    {
        if( ppOut == nullptr || buffer == nullptr )
        {
            throw ref new Platform::InvalidArgumentException();
        }
        *ppOut = nullptr;

        Microsoft::WRL::ComPtr<IInspectable> srcBufferInspectable(reinterpret_cast<IInspectable*>( buffer ));
        Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> srcBufferByteAccess;
        srcBufferInspectable.As(&srcBufferByteAccess);
        srcBufferByteAccess->Buffer(ppOut);
    }
}
