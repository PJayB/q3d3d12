#include "pch.h"
#include "Resources.h"
#include "Upload.h"

namespace QD3D12
{
    void Resources::CreateInitializedResource(
        _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        _In_ D3D12_HEAP_FLAGS heapMiscFlags,
        _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        _In_ D3D12_SUBRESOURCE_DATA* pSubresourceData,
		_In_opt_ const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		_In_ D3D12_RESOURCE_STATES stateBefore,
		_In_ D3D12_RESOURCE_STATES stateAfter,
        _COM_Outptr_ ID3D12Resource** ppResource)
    {
        ID3D12Device* pDevice = Device::Get();

        COM_ERROR_TRAP(pDevice->CreateCommittedResource(
            pHeapProperties,
            heapMiscFlags,
            pResourceDesc,
            QD3D12_RESOURCE_STATE_INITIAL,
            pOptimizedClearValue,
            __uuidof(ID3D12Resource),
            (void**) ppResource));

		Upload::Subresources(
			*ppResource,
			0, 
            pSubresourceData,
			pResourceDesc->MipLevels * pResourceDesc->DepthOrArraySize,
			stateBefore,
			stateAfter);
    }

	void Resources::CreateImmutableBuffer(
		_In_ D3D12_RESOURCE_STATES usage,
		_In_reads_(bufferSize) LPCVOID bufferData,
		_In_ UINT64 bufferSize,
		_COM_Outptr_ ID3D12Resource** ppResource)
	{
		D3D12_SUBRESOURCE_DATA subres;
		subres.pData = bufferData;
		subres.RowPitch = bufferSize;
		subres.SlicePitch = bufferSize;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		CreateInitializedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			&subres,
			nullptr,
			QD3D12_RESOURCE_STATE_INITIAL,
			usage,
			ppResource);
	}

	void Resources::CreateDynamicBuffer(
		_In_ D3D12_RESOURCE_STATES usage,
		_In_ UINT64 bufferSize,
		_COM_Outptr_ ID3D12Resource** ppResource)
	{
		ID3D12Device* pDevice = Device::Get();

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		COM_ERROR_TRAP(pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			__uuidof(ID3D12Resource),
			(void**)ppResource));
	}

    void Resources::ResolveTarget(
        ID3D12GraphicsCommandList* pCmdList,
        ID3D12Resource* pDest,
        ID3D12Resource* pSrc,
        DXGI_FORMAT format,
        BOOL msaa )
    {
        if ( msaa ) 
        {
            pCmdList->ResolveSubresource(
                pDest,
                0,
                pSrc,
                0,
                format );
        }
        else
        {
            D3D12_TEXTURE_COPY_LOCATION from;
            from.pResource = pSrc;
            from.SubresourceIndex = 0;
            from.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                
            D3D12_TEXTURE_COPY_LOCATION to;
            to.pResource = pDest;
            to.SubresourceIndex = 0;
            to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

            pCmdList->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);                    
        }
    }
}

