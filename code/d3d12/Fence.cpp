#include "pch.h"
#include "Fence.h"

namespace QD3D12
{
	Fence::Fence()
		: m_fence(0)
		, m_hEv(nullptr)
        , m_ifence(nullptr)
		, m_cmdQ(nullptr)
	{
	}

	Fence::~Fence()
	{
        assert(m_hEv == nullptr);
	}

    void Fence::Init(ID3D12CommandQueue* pCmdQ)
    {
		if (pCmdQ == nullptr)
			pCmdQ = Device::GetQ();

		m_hEv = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_fence = 0;
		m_cmdQ = SAFE_ADDREF(pCmdQ);

        if (pCmdQ != nullptr)
        {
            Device::Get()->CreateFence(m_fence, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**) &m_ifence);
        }
    }

    void Fence::Destroy()
    {
        CloseHandle(m_hEv);
        m_hEv = nullptr;

        SAFE_RELEASE(m_ifence);
		SAFE_RELEASE(m_cmdQ);
    }

	void Fence::Advance()
	{
        if (m_cmdQ)
        {
            UINT64 waitFor = ++m_fence;
            COM_ERROR_TRAP(m_cmdQ->Signal(m_ifence, waitFor));
        }
    }

	void Fence::Advance(UINT64 v)
	{
        if (m_cmdQ)
        {
		    assert(v > m_fence);
		    m_fence = v;
		    COM_ERROR_TRAP(m_cmdQ->Signal(m_ifence, v));
        }
	}

	void Fence::Wait()
	{
		if (m_cmdQ && m_ifence && m_ifence->GetCompletedValue() < m_fence)
		{
            COM_ERROR_TRAP(m_ifence->SetEventOnCompletion(m_fence, m_hEv));
			WaitForSingleObject(m_hEv, INFINITE);
		}
	}

	void Fence::WaitUntil(UINT64 value)
	{
		if (value > m_fence)
		{
			Advance(value);
		}
		Wait();
	}

	bool Fence::HasExpired() const
	{
        return m_ifence ? m_ifence->GetCompletedValue() >= m_fence : true;
	}

	UINT64 Fence::CompletedValue() const
	{
		return m_ifence ? m_ifence->GetCompletedValue() : 0;
	}
}
