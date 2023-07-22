#include "SwapChain.h"

#include <xutility>

namespace QD3D12
{
	QDXGISwapChain* SwapChain::s_pSwapChain = nullptr;
	ID3D12Resource* SwapChain::s_pBackBuffers[DXGI_MAX_SWAP_CHAIN_BUFFERS];
	DXGI_SWAP_CHAIN_DESC1 SwapChain::s_desc;
	UINT SwapChain::s_presentIndex = 0;

	//--------------------------------------------------------------------

	void SwapChain::Init(QDXGISwapChain* swapChain)
	{
		s_pSwapChain = SAFE_ADDREF(swapChain);
		s_presentIndex = 0;
	
		for (UINT i = 0; i < QD3D12_NUM_BUFFERS; ++i)
		{
			// Get back buffer target
			COM_ERROR_TRAP(swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**) &s_pBackBuffers[i]));

			SetResourceName(s_pBackBuffers[i], "Back Buffer %d", i);
		}

		s_pSwapChain->GetDesc1( &s_desc );
	}

	void SwapChain::Destroy()
	{
		for (UINT i = 0; i < QD3D12_NUM_BUFFERS; ++i)
		{
			// Get back buffer target
			SAFE_RELEASE( s_pBackBuffers[i] );
		}
		SAFE_RELEASE(s_pSwapChain);
	}

	QDXGISwapChain* SwapChain::Get()
	{
		return s_pSwapChain;
	}

	ID3D12Resource* SwapChain::CurrentBackBuffer()
	{
		return s_pBackBuffers[s_presentIndex];
	}

	void SwapChain::GetDescFromConfig(
        DXGI_SWAP_CHAIN_DESC1* scDesc)
	{
        // Get best possible swapchain first
	    scDesc->BufferCount = QD3D12_NUM_BUFFERS;
	    scDesc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	    scDesc->SampleDesc.Count = 1;
	    scDesc->SampleDesc.Quality = 0;
	    scDesc->Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// @pjb: todo: tweaks from config
	}

	void SwapChain::GetMsaaDescFromConfig(
        ID3D12Device* pDevice, 
        DXGI_FORMAT format,
        DXGI_SAMPLE_DESC* scDesc)
	{
	    scDesc->Count = 1;
	    scDesc->Quality = 0;

		cvar_t* d3d_multisamples = Cvar_Get("dx12_msaa", "32", CVAR_ARCHIVE | CVAR_LATCH);
		if (d3d_multisamples->integer == 0) 
			return;

        // Check the multisample support
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaa;
        ZeroMemory(&msaa, sizeof(msaa));
        msaa.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        msaa.Format = format;

		const UINT multiSampleLevels[] = {16, 8, 4, 2, 0};
		UINT multiSampleIndex = 0;
		UINT msQuality = 0;
		while (multiSampleLevels[multiSampleIndex] != 0)
		{
			msaa.NumQualityLevels = 1;
			msaa.SampleCount = multiSampleLevels[multiSampleIndex];
			if (SUCCEEDED(pDevice->CheckFeatureSupport(
                D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                &msaa,
                sizeof(msaa))))
			{
				if (msaa.NumQualityLevels > 0)
				{
					msQuality = msaa.NumQualityLevels - 1;
					break;
				}
			}
			multiSampleIndex++;
		}

		if (multiSampleLevels[multiSampleIndex] > 1 && msQuality > 0)
		{
            scDesc->Count = multiSampleLevels[multiSampleIndex];
            scDesc->Quality = msQuality;
        }

		// Clamp the max MSAA to user settings
		if (d3d_multisamples->integer > 0 && scDesc->Count >= d3d_multisamples->integer)
			scDesc->Count = d3d_multisamples->integer;
	}

#ifdef _XBOX_ONE
	HRESULT SwapChain::Present(
		_In_ const XBOX_ONE_PRESENT_PARAMETERS* pParams)
	{
		s_presentIndex = (s_presentIndex + 1) % QD3D12_NUM_BUFFERS;

		QDXGISwapChain* pSwapChain = SwapChain::Get();

		/*
		TODO

		DXGIX_PRESENTARRAY_PARAMETERS pparms;
		ZeroMemory( &pparms, sizeof( pparms ) );

		pparms.SourceRect.bottom = pParams->BackBufferHeight;
		pparms.SourceRect.right = pParams->BackBufferWidth;
		pparms.ScaleFactorHorz = 1.0f;
		pparms.ScaleFactorVert = 1.0f;

		return DXGIXPresentArray(
			pParams->Interval, // Presentation interval
			pParams->PresentThreshold, // Present threshold
			0, // Flags
			1, // Number of swap chains
			&pSwapChain, // The swap chain
			&pparms ); // Present parameters
		*/
		return pSwapChain->Present1(pParams->Interval, 0, nullptr);
	}
#else
	HRESULT SwapChain::Present(INT interval)
	{
		s_presentIndex = (s_presentIndex + 1) % QD3D12_NUM_BUFFERS;

		DXGI_PRESENT_PARAMETERS parameters = { 0 };
		parameters.DirtyRectsCount = 0;
		parameters.pDirtyRects = nullptr;
		parameters.pScrollRect = nullptr;
		parameters.pScrollOffset = nullptr;

		return s_pSwapChain->Present1(interval, 0, &parameters);
	}
#endif

	//----------------------------------------------------------------------------
	// Gets the DXGI device
	//----------------------------------------------------------------------------
	HRESULT SwapChain::GetDxgiDevice(
		_In_ ID3D12Device* device,
		_Out_ QDXGIDevice** dxgiDevice)
	{
		return device->QueryInterface(__uuidof(QDXGIDevice), (void **)dxgiDevice);
	}

	//----------------------------------------------------------------------------
	// Gets the DXGI adapter
	//----------------------------------------------------------------------------
	HRESULT SwapChain::GetDxgiAdapter(
		_In_ ID3D12Device* device,
		_Out_ QDXGIAdapter** dxgiAdapter)
	{
		QDXGIDevice* dxgiDevice = nullptr;
		HRESULT hr = GetDxgiDevice(device, &dxgiDevice);
		if (FAILED(hr))
			return hr;

		hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)dxgiAdapter);
		SAFE_RELEASE(dxgiDevice);
		return hr;
	}

	//----------------------------------------------------------------------------
	// Gets the DXGI factory
	//----------------------------------------------------------------------------
	HRESULT SwapChain::GetDxgiFactory(
		_In_ ID3D12Device* device,
		_Out_ QDXGIFactory** dxgiFactory)
	{
		QDXGIAdapter* dxgiAdapter;
		HRESULT hr = GetDxgiAdapter(device, &dxgiAdapter);
		if (FAILED(hr))
			return hr;

		hr = dxgiAdapter->GetParent(__uuidof(QDXGIFactory), (void **)dxgiFactory);
		SAFE_RELEASE(dxgiAdapter);
		return hr;
	}

#ifndef _XBOX_ONE
    //----------------------------------------------------------------------------
    // Create a DXGI factory
    //----------------------------------------------------------------------------
    HRESULT SwapChain::CreateDxgiFactory(
        _Out_ QDXGIFactory** dxgiFactory)
    {
        return CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory));
    }
#endif

	//----------------------------------------------------------------------------
	// Creates a swap chain
	//----------------------------------------------------------------------------
#ifdef _XBOX_ONE
	HRESULT	SwapChain::CreateSwapChain(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ HWND hWnd,
		_In_ const DXGI_SWAP_CHAIN_DESC1* scd,
		_In_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fsd,
		_Out_ QDXGISwapChain** swapChain)
	{
		// TODO: remove this when it becomes irrelevant
		ID3D12Device* pDevice = nullptr;
		pCmdList->GetDevice( __uuidof(ID3D12Device), (void**) &pDevice );

		QDXGIFactory* dxgiFactory = nullptr;
		HRESULT hr = GetDxgiFactory( pDevice, &dxgiFactory );
		if (FAILED(hr))
			return hr;

		// Create the swap chain
		hr = dxgiFactory->CreateSwapChainForHwnd(pDevice, hWnd, scd, fsd, nullptr, (IDXGISwapChain1**) swapChain);

		SAFE_RELEASE(pDevice);

		SAFE_RELEASE(dxgiFactory);

		return hr;
	}
#else
	HRESULT	SwapChain::CreateSwapChain(
		_In_ ID3D12CommandQueue* pCmdQ,
		_In_ HWND hWnd,
		_In_ const DXGI_SWAP_CHAIN_DESC1* scd,
		_In_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fsd,
		_Out_ QDXGISwapChain** swapChain)
	{
        QDXGIFactory* dxgiFactory = nullptr;
        COM_ERROR_TRAP( CreateDXGIFactory2(0, __uuidof(QDXGIFactory), (void**) &dxgiFactory) );

		// Create the swap chain
		HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(pCmdQ, hWnd, scd, fsd, nullptr, (IDXGISwapChain1**) swapChain);

		SAFE_RELEASE(dxgiFactory);

		return hr;
	}
#endif
}

