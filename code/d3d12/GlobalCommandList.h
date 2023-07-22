#pragma once

#include "D3D12Core.h"

namespace QD3D12
{
    // For loading and stuff
	class GlobalCommandList
	{
	private:
		static ID3D12CommandQueue* s_pCmdQ;
        static ID3D12CommandAllocator* s_pCmdAlloc;
		static ID3D12GraphicsCommandList* s_pCmdList;

	public:
		static void Init(
			_In_ ID3D12Device* pDevice,
			_In_ ID3D12CommandQueue* pCmdQ );
		static void Destroy();

        static ID3D12GraphicsCommandList* Get();
        static void KickOff();
	};
}
