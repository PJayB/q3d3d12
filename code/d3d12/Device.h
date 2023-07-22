#pragma once

#include "D3D12Core.h"
#include "Fence.h"

namespace QD3D12
{
	class Device
	{
	private:
		static D3D_FEATURE_LEVEL s_featureLevel;
		static ID3D12Device* s_pDevice;
		static ID3D12CommandQueue* s_pCmdQ;
        static ID3D12Debug* s_pDebug;
		static Fence s_frameFence;

	public:
		static ID3D12Device* Init();
		static ID3D12Device* Get();
		static ID3D12CommandQueue* GetQ();
        static ID3D12Debug* GetDebug();
		static void Destroy();
		static bool IsStarted();
		static void WaitForGpu();
        static bool IsSupported();

		static D3D_FEATURE_LEVEL FeatureLevel() { return s_featureLevel; }
	};
}
