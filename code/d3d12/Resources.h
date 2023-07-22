#pragma once

namespace QD3D12
{
	class Resources
	{
	public:
        static void CreateInitializedResource(
            _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
            _In_ D3D12_HEAP_FLAGS heapMiscFlags,
            _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
            _In_ D3D12_SUBRESOURCE_DATA* pSubresourceData,
			_In_opt_ const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			_In_ D3D12_RESOURCE_STATES stateBefore,
			_In_ D3D12_RESOURCE_STATES stateAfter,
            _COM_Outptr_ ID3D12Resource** ppResource);

		// A read-only buffer that will never be changed.
		static void CreateImmutableBuffer(
			_In_ D3D12_RESOURCE_STATES usage,
			_In_reads_(bufferSize) LPCVOID bufferData,
			_In_ UINT64 bufferSize,
			_COM_Outptr_ ID3D12Resource** ppResource);

		// A writeable buffer that will constantly be changed.
		static void CreateDynamicBuffer(
			_In_ D3D12_RESOURCE_STATES usage,
			_In_ UINT64 bufferSize,
			_COM_Outptr_ ID3D12Resource** ppResource);

        static void ResolveTarget(
            _In_ ID3D12GraphicsCommandList* pCmdList,
            _In_ ID3D12Resource* pDest,
            _In_ ID3D12Resource* pSrc,
            _In_ DXGI_FORMAT format,
            _In_ BOOL multisampled );
    };
}
