#pragma once

#include <atomic>
#include "D3D12Core.h"

namespace QD3D12
{
	class Fence
	{
	private:
        ID3D12Fence* m_ifence;
		ID3D12CommandQueue* m_cmdQ;
		std::atomic<UINT64> m_fence;
		HANDLE m_hEv;

	public:
		Fence();
		~Fence();

        void Init(ID3D12CommandQueue* pCmdQ = nullptr);
        void Destroy();

		void Advance();
		void Advance(UINT64 value);
		void Wait();
		void WaitUntil(UINT64 value);
		bool HasExpired() const;
		UINT64 CompletedValue() const;
	};
}
