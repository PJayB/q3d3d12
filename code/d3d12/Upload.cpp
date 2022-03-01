#include "pch.h"
#include "Upload.h"
#include "Fence.h"

namespace QD3D12
{
	ID3D12CommandAllocator* Upload::s_alloc = nullptr;
	Fence Upload::s_fence;

    ID3D12GraphicsCommandList* Upload::GetUploadCL(ID3D12Device* pDevice)
    {
		ID3D12GraphicsCommandList* pList = nullptr;
		COM_ERROR_TRAP(pDevice->CreateCommandList(
            QD3D12_NODE_MASK,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			s_alloc,
			nullptr,
			__uuidof(ID3D12GraphicsCommandList),
			(void**) &pList));
		return pList;
    }

    void Upload::Init()
	{
		ID3D12Device* pDevice = Device::Get();
		ID3D12CommandQueue* pCmdQ = Device::GetQ();

		s_fence.Init(pCmdQ);

		COM_ERROR_TRAP(pDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
            __uuidof(ID3D12CommandAllocator),
			(void**) &s_alloc));
	}

	void Upload::Destroy()
	{
		s_fence.Destroy();
		SAFE_RELEASE(s_alloc);
	}

	void Upload::SingleSubresourceDynamic(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ ID3D12Resource* pResource,
		_In_ ID3D12Resource* pStagingResource,
		_In_ UINT subresourceIndex,
		_In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA* pSubRes,
		_In_ D3D12_RESOURCE_STATES stateBefore,
		_In_ D3D12_RESOURCE_STATES stateAfter)
	{
		TransitionResource(pCmdList, pResource, stateBefore, D3D12_RESOURCE_STATE_COPY_DEST);
		UpdateSubresources<1>(pCmdList, pResource, pStagingResource, 0, subresourceIndex, 1, pSubRes);
		TransitionResource(pCmdList, pResource, D3D12_RESOURCE_STATE_COPY_DEST, stateAfter);
	}

	void Upload::Subresources(
		_In_ ID3D12Resource* pResource,
		_In_ UINT subresourceIndexStart,
		_In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA* pSubRes,
		_In_ UINT numSubresources,
		_In_ D3D12_RESOURCE_STATES stateBefore,
		_In_ D3D12_RESOURCE_STATES stateAfter)
	{
		UINT64 uploadSize = GetRequiredIntermediateSize(
			pResource,
			subresourceIndexStart,
			numSubresources);

		// Get the device
		ID3D12Device* pDevice = nullptr;
		pResource->GetDevice(__uuidof(ID3D12Device), (void**) &pDevice);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

		// Create a temporary buffer
		ID3D12Resource* pScratchResource = nullptr;
		COM_ERROR_TRAP(pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, // D3D12_CLEAR_VALUE* pOptimizedClearValue
			__uuidof(ID3D12Resource),
			(void**) &pScratchResource));

        // Fetch a new command list
        ID3D12GraphicsCommandList* list = GetUploadCL(pDevice);

		// Transition the resource to a copy target, copy and transition back
		TransitionResource(list, pResource, stateBefore, D3D12_RESOURCE_STATE_COPY_DEST);
		UpdateSubresources(list, pResource, pScratchResource, 0, subresourceIndexStart, numSubresources, pSubRes);
		TransitionResource(list, pResource, D3D12_RESOURCE_STATE_COPY_DEST, stateAfter);

        list->Close();

		// Submit the job to the GPU
		// TODO: remove me, thread this or SOMETHING
		ID3D12CommandQueue* pCmdQ = Device::GetQ();
		pCmdQ->ExecuteCommandLists(1, (ID3D12CommandList**) &list);

		// Wait for the GPU to complete all its work
		s_fence.Advance();
		s_fence.Wait();

		// Free the scratch resource
		SAFE_RELEASE(pScratchResource);

		// Free the list
		SAFE_RELEASE(list);
	}
}

