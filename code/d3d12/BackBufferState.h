#pragma once

#include "DescriptorHeaps.h"

namespace QD3D12
{
	class BackBufferState
	{
		// --- Instance State ---
	private:
		D3D12_VIEWPORT m_viewport;
		DescriptorArray m_rtvDescriptorHeap;
		DXGI_SWAP_CHAIN_DESC m_scDesc;
        DXGI_SAMPLE_DESC m_sampleDesc;

		D3D12_CPU_DESCRIPTOR_HANDLE m_cbRTV;
		ID3D12Resource* m_colorBuffer;
        
		DescriptorArray m_dsvDescriptorHeap;
		ID3D12Resource* m_depthBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE m_bbDSV;

	public:
		BackBufferState();
		~BackBufferState();

		void Init(ID3D12GraphicsCommandList* cmdList, UINT bufferIndex);
		void Destroy();

		const D3D12_VIEWPORT& Viewport() const { return m_viewport; }
		D3D12_CPU_DESCRIPTOR_HANDLE ColorBufferRTV() const { return m_cbRTV; }
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferDSV() const { return m_bbDSV; }

		ID3D12Resource* ColorBufferResource() const { return m_colorBuffer; }
		ID3D12Resource* DepthBufferResource() const { return m_depthBuffer; }

#ifdef _DEBUG
        enum DEBUG_CURRENT_STATE
        {
            BUFFER_FRAME_NEXT,
            BUFFER_RESOLVE_NEXT,
            BUFFER_PRESENT_NEXT
        } m_currentState;
#endif

		void PrepareForFrame(ID3D12GraphicsCommandList* pCmdList);
		void PrepareForResolve(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pBackBuffer);
		void PrepareForPresent(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pBackBuffer);

		// For filling in PSOs
		const DXGI_SWAP_CHAIN_DESC* SwapChainDesc() const { return &m_scDesc; }
        const DXGI_SAMPLE_DESC* SampleDesc() const { return &m_sampleDesc; }
		UINT GetRenderTargetFormats(DXGI_FORMAT formats[], UINT count);

		// TODO: change this?
		static const DXGI_FORMAT DEPTH_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;
	};
}
