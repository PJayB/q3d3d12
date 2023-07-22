#include "D3D11Device.h"

QD3D11Device* g_pDevice = nullptr;
ID3D11DeviceContext1* g_pImmediateContext = nullptr;
IDXGISwapChain1* g_pSwapChain = nullptr;

bool QD3D11::DeviceStarted()
{
    return g_pDevice != nullptr;
}

QD3D11Device* QD3D11::InitDevice()
{
    Com_Memset( &g_BufferState, 0, sizeof( g_BufferState ) );

#ifdef Q_WINRT_PLATFORM
    g_BufferState.featureLevel = D3D_FEATURE_LEVEL_9_1;
#else
    g_BufferState.featureLevel = D3D_FEATURE_LEVEL_11_1; 
#endif
	HRESULT hr = QD3D11::CreateDefaultDevice(
		D3D_DRIVER_TYPE_HARDWARE, 
		&g_pDevice, 
		&g_pImmediateContext, 
		&g_BufferState.featureLevel);
    if (FAILED(hr) || !g_pDevice || !g_pImmediateContext)
	{
        RI_Error( ERR_FATAL, "Failed to create Direct3D 11 device: 0x%08x.\n", hr );
        return nullptr;
	}

    RI_Printf( PRINT_ALL, "... feature level %d\n", g_BufferState.featureLevel );

    g_pDevice->AddRef();
    return g_pDevice;
}

void QD3D11::InitSwapChain( IDXGISwapChain1* swapChain )
{
    swapChain->AddRef();
    g_pSwapChain = swapChain;
    g_pSwapChain->GetDesc1( &g_BufferState.swapChainDesc );
    
    // Create D3D objects
    DestroyBuffers();
    CreateBuffers();

    // Clear the targets
    FLOAT clearCol[4] = { 0, 0, 0, 0 };
    g_pImmediateContext->ClearRenderTargetView( g_BufferState.backBufferView, clearCol );
    g_pImmediateContext->ClearDepthStencilView( g_BufferState.depthBufferView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0 );

    // Clear the screen immediately
    g_pSwapChain->Present( 0, 0 );
}

void QD3D11::DestroyDevice()
{
    SAFE_RELEASE(g_pImmediateContext);
    SAFE_RELEASE(g_pDevice);
}

void QD3D11::DestroySwapChain()
{
    SAFE_RELEASE(g_pSwapChain);
    Com_Memset( &g_BufferState, 0, sizeof( g_BufferState ) );
}

void QD3D11::GetSwapChainDescFromConfig( DXGI_SWAP_CHAIN_DESC1* scDesc )
{
    // Get best possible swapchain first
    QD3D11::GetBestQualitySwapChainDesc( g_pDevice, scDesc );

#ifndef _ARM_
    // Clamp the max MSAA to user settings
    cvar_t* d3d_multisamples = Cvar_Get( "d3d_multisamples", "32", CVAR_ARCHIVE | CVAR_LATCH );
    if ( d3d_multisamples->integer > 0 && scDesc->SampleDesc.Count > d3d_multisamples->integer )
        scDesc->SampleDesc.Count = d3d_multisamples->integer;
#endif
}



