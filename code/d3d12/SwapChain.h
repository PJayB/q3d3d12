#pragma once

#include "pch.h"

namespace QD3D12
{
	class SwapChain
	{
	private:
		static QDXGISwapChain* s_pSwapChain;
		static ID3D12Resource* s_pBackBuffers[];
		static DXGI_SWAP_CHAIN_DESC1 s_desc;
		static UINT s_presentIndex;

	public:
		static void Init(QDXGISwapChain* swapChain);
		static void Destroy();
		static QDXGISwapChain* Get();

	    static void GetDescFromConfig(
          _Out_ DXGI_SWAP_CHAIN_DESC1* desc);

	    static void GetMsaaDescFromConfig(
            _In_ ID3D12Device* pDevice, 
            DXGI_FORMAT format,
            DXGI_SAMPLE_DESC* scDesc);
    
        static HRESULT GetDxgiDevice(
          _In_ ID3D12Device* device,
          _Out_ QDXGIDevice** dxgiDevice);
      
        static HRESULT GetDxgiAdapter(
          _In_ ID3D12Device* device,
          _Out_ QDXGIAdapter** dxgiAdapter);

        static HRESULT GetDxgiFactory(
          _In_ ID3D12Device* device,
          _Out_ QDXGIFactory** dxgiFactory);

        static HRESULT CreateDxgiFactory(
          _Out_ QDXGIFactory** dxgiFactory);

        static HRESULT	CreateSwapChain(
          _In_ ID3D12CommandQueue* pCmdQ,
          _In_ HWND hWnd,
          _In_ const DXGI_SWAP_CHAIN_DESC1* scd,
          _In_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fsd,
          _Out_ QDXGISwapChain** swapChain);

		static ID3D12Resource* CurrentBackBuffer();
		static const DXGI_SWAP_CHAIN_DESC1* Desc() { return &s_desc; }

		static UINT GetSwapIndex() { return s_presentIndex; }

	    static HRESULT Present(
		      _In_ INT interval);
	};
}
