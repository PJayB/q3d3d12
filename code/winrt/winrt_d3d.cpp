#include "../d3d/Direct3DCommon.h"

#include "../d3d11/D3D11Core.h"
#include "../d3d11/D3D11Driver.h"
#include "../d3d11/D3D11Device.h"

#include "../d3d12/D3D12Core.h"
#include "../d3d12/D3D12QAPI.h"
#include "../d3d12/Device.h"
#include "../d3d12/SwapChain.h"

using namespace Microsoft::WRL;

HANDLE g_WaitingForVideoEvent = nullptr;
int g_WindowWidth = 0;
int g_WindowHeight = 0;
IUnknown* g_Window = nullptr;

void D3D_WinRT_InitDeferral()
{
	g_WaitingForVideoEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
}

void D3D_WinRT_CleanupDeferral()
{
	SAFE_RELEASE(g_Window);
	CloseHandle(g_WaitingForVideoEvent);
}

#ifdef Q_HAS_D3D12
D3D_PUBLIC void D3D12_Window_Init()
{
	// Wait for video init before we do anything
	WaitForSingleObjectEx(g_WaitingForVideoEvent, INFINITE, FALSE);

	assert(g_Window);

	// Now go ahead :)
	ComPtr<ID3D12Device> device(QD3D12::Device::Init());

	ComPtr<QDXGIFactory> dxgiFactory;
	HRESULT hr = QD3D12::SwapChain::GetDxgiFactory(device.Get(), dxgiFactory.GetAddressOf());
	if (FAILED(hr))
	{
        hr = QD3D12::SwapChain::CreateDxgiFactory(dxgiFactory.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
    		RI_Error(ERR_FATAL, "Failed to get DXGI Factory: 0x%08x.\n", hr);
	    	return;
        }
	}

	// Prepare the swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	QD3D12::SwapChain::GetDescFromConfig(&swapChainDesc);

	// Set up win8 params. Force disable multisampling.
	swapChainDesc.Width = (UINT)g_WindowWidth;
	swapChainDesc.Height = (UINT)g_WindowHeight;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	ComPtr<IDXGISwapChain1> swapChain;
	hr = dxgiFactory->CreateSwapChainForCoreWindow(
		QD3D12::Device::GetQ(),
		g_Window,
		&swapChainDesc,
		nullptr,
		swapChain.GetAddressOf());

	if (FAILED(hr))
	{
		RI_Error(ERR_FATAL, "Failed to create Direct3D 12 swapchain: 0x%08x.\n", hr);
		return;
	}

	ComPtr<QDXGIDevice> dxgiDevice;
	hr = QD3D12::SwapChain::GetDxgiDevice(device.Get(), &dxgiDevice);
	if (SUCCEEDED(hr))
	{
		// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
		// ensures that the application will only render after each VSync, minimizing power consumption.
		dxgiDevice->SetMaximumFrameLatency(1);
	}

    ComPtr<QDXGISwapChain> swapChain2;
    COM_ERROR_TRAP(swapChain.As(&swapChain2));
	QD3D12::SwapChain::Init(swapChain2.Get());
}

D3D_PUBLIC void D3D12_Window_Shutdown()
{
	QD3D12::SwapChain::Destroy();
	QD3D12::Device::Destroy();
}
#endif

#ifdef Q_HAS_D3D11
D3D_PUBLIC void D3D11_Window_Init()
{
    // Wait for video init before we do anything
    WaitForSingleObjectEx( g_WaitingForVideoEvent, INFINITE, FALSE );

    assert(g_Window);

    // Now go ahead :)
    QD3D11Device* device = QD3D11::InitDevice();

    IDXGIFactory2* dxgiFactory = nullptr;
    HRESULT hr = QD3D11::GetDxgiFactory( device, &dxgiFactory );
    if ( FAILED( hr ) )
    {
        RI_Error( ERR_FATAL, "Failed to get DXGI Factory: 0x%08x.\n", hr );
        return;
    }

    // Prepare the swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    ZeroMemory( &swapChainDesc, sizeof(swapChainDesc) );

    QD3D11::GetSwapChainDescFromConfig( &swapChainDesc );

    // Set up win8 params. Force disable multisampling.
    swapChainDesc.Width = (UINT) g_WindowWidth;
    swapChainDesc.Height = (UINT) g_WindowHeight;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    IDXGISwapChain1* swapChain = nullptr;
    hr = dxgiFactory->CreateSwapChainForCoreWindow(
        device, 
        g_Window,
        &swapChainDesc, 
        nullptr, 
        &swapChain);

    if (FAILED(hr))
    {
        RI_Error( ERR_FATAL, "Failed to create Direct3D 11 swapchain: 0x%08x.\n", hr );
        return;
    }

    QDXGIDevice* dxgiDevice = nullptr;
    hr = QD3D11::GetDxgiDevice( device, &dxgiDevice );
    if ( SUCCEEDED( hr ) )
    {
		// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
		// ensures that the application will only render after each VSync, minimizing power consumption.
        dxgiDevice->SetMaximumFrameLatency(1);
    }

    QD3D11::InitSwapChain( swapChain );

    SAFE_RELEASE( swapChain );
    SAFE_RELEASE( device );
}

D3D_PUBLIC void D3D11_Window_Shutdown()
{
    QD3D11::DestroyDevice();
    QD3D11::DestroySwapChain();
}
#endif

void D3D_WinRT_NotifyNewWindow(IUnknown* window, int logicalSizeX, int logicalSizeY)
{
	// Re-enable the deferral of the video init
	ResetEvent(g_WaitingForVideoEvent);

	// Restart the video if we're already running
	if (g_Window != nullptr)
	{
		Cbuf_AddText("vid_restart\n");
		SAFE_RELEASE(g_Window);
	}

	// Swap out the window information
	g_Window = window;
	g_WindowWidth = logicalSizeX;
	g_WindowHeight = logicalSizeY;

	// Allow the game to run again
	SetEvent(g_WaitingForVideoEvent);
}

void D3D_WinRT_NotifyWindowResize(int logicalSizeX, int logicalSizeY)
{
	assert(g_Window);

	if (g_WindowWidth != logicalSizeX || g_WindowHeight != logicalSizeY)
	{
		g_WindowWidth = logicalSizeX;
		g_WindowHeight = logicalSizeY;

		// Allow the video driver to proceed now we have the right information
#if defined(Q_HAS_D3D12) && defined(Q_HAS_D3D11)
        if (QD3D12::Device::IsStarted() || QD3D11::DeviceStarted())
#elif defined(Q_HAS_D3D12)
        if (QD3D12::Device::IsStarted())
#elif defined(Q_HAS_D3D11)
        if (QD3D11::DeviceStarted())
#endif
		{
			Cbuf_AddText("vid_restart\n");
		}
	}
}
