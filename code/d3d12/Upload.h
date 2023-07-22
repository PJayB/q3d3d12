#pragma once

#include "D3D12Core.h"
#include "Fence.h"

namespace QD3D12
{
#define QD3D12_NUM_UPLOAD_COMMAND_LISTS 16

	// Has a command list of it's own so it can upload at any time.
	class Upload
	{
	private:
		static ID3D12CommandAllocator*	        s_alloc;
		static Fence							s_fence;
        static ID3D12GraphicsCommandList*       GetUploadCL(ID3D12Device* pDevice);
	
	public:
		static void Init();
		static void Destroy();

		static void SingleSubresourceDynamic(
			_In_ ID3D12GraphicsCommandList* pCmdList,
			_In_ ID3D12Resource* pResource,
			_In_ ID3D12Resource* pStagingResource,
			_In_ UINT subresourceIndex,
			_In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA* pSubRes,
			_In_ D3D12_RESOURCE_STATES stateBefore,
			_In_ D3D12_RESOURCE_STATES stateAfter);

		static void Subresources(
			_In_ ID3D12Resource* pResource,
			_In_ UINT subresourceIndexStart,
			_In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA* pSubRes,
			_In_ UINT numSubresources,
			_In_ D3D12_RESOURCE_STATES stateBefore,
			_In_ D3D12_RESOURCE_STATES stateAfter);
	};

	// TODO: 
	// We need to close and execute the upload command buffer, as well
	// as N-buffer it to avoid trampling it, AND we need to reset it
}
