#include "pch.h"
#include "D3D12QAPI.h"
#include "Device.h"
#include "GlobalCommandList.h"

namespace QD3D12
{
	D3D_FEATURE_LEVEL Device::s_featureLevel;
	ID3D12Device* Device::s_pDevice = nullptr;
	ID3D12CommandQueue* Device::s_pCmdQ = nullptr;
    ID3D12Debug* Device::s_pDebug = nullptr;
	Fence Device::s_frameFence;

    #ifdef _ARM_
	static const D3D_FEATURE_LEVEL c_targetFeatureLevel = D3D_FEATURE_LEVEL_9_1;
#else
	static const D3D_FEATURE_LEVEL c_targetFeatureLevel = D3D_FEATURE_LEVEL_11_0;
#endif

    static HRESULT CreateQuake3Device(ID3D12Device** ppDevice, D3D_FEATURE_LEVEL featureLevel)
    {
        using Microsoft::WRL::ComPtr;

        HRESULT hr = S_OK;

#ifndef _XBOX_ONE
        BOOL bWARP = Cvar_Get("dx12_warp", "0", CVAR_TEMP | CVAR_LATCH)->integer != 0;
        if (bWARP) 
        {
            ComPtr<QDXGIFactory> dxgiFactory;
            COM_ERROR_TRAP(CreateDXGIFactory2(0, __uuidof(QDXGIFactory), (void**) &dxgiFactory));

            ComPtr<IDXGIAdapter> pWarpAdapter;
            COM_ERROR_TRAP(dxgiFactory->EnumWarpAdapter(__uuidof(IDXGIAdapter), (void**) &pWarpAdapter));

            // Use WARP device
            hr = D3D12CreateDevice(
			    pWarpAdapter.Get(),
			    featureLevel,
			    IID_PPV_ARGS(ppDevice));
        }
        else
#endif
        {
            hr = D3D12CreateDevice(
			    nullptr, // pAdapter
			    featureLevel,
			    IID_PPV_ARGS(ppDevice));
        }

        return hr;
    }
    
    bool Device::IsSupported()
    {
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        HRESULT hr = CreateQuake3Device(device.ReleaseAndGetAddressOf(), c_targetFeatureLevel);

        return SUCCEEDED(hr);
    }

	ID3D12Device* Device::Init()
	{
        const D3D_FEATURE_LEVEL featureLevel = c_targetFeatureLevel;

#ifdef _DEBUG
		static const char* d3dDefaultDebugSetting = "1";
#else
		static const char* d3dDefaultDebugSetting = "0";
#endif

        // Get the debug layer
		BOOL bDebug = Cvar_Get("dx12_debug", d3dDefaultDebugSetting, CVAR_TEMP | CVAR_LATCH)->integer != 0;

#ifdef _XBOX_ONE
		D3D12XBOX_PROCESS_DEBUG_FLAGS xboFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_NONE;
#if defined(PROFILE) || defined(_DEBUG)
        xboFlags |= D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
#endif
        if (bDebug) xboFlags |= D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED | D3D12XBOX_PROCESS_DEBUG_FLAG_VALIDATED | D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
        COM_ERROR_TRAP(D3D12XboxSetProcessDebugFlags(xboFlags));
#else // _XBOX_ONE
        if (bDebug)
        {
            HRESULT hr = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**) &s_pDebug);
            if ( FAILED(hr) )
            {
                RI_Printf( PRINT_WARNING, "Debug Layer Query failed: %s\n", QD3D::HResultToString(hr) );
            }
            else
            {
                s_pDebug->EnableDebugLayer();
            }
        }
#endif

        COM_ERROR_TRAP(CreateQuake3Device(&s_pDevice, featureLevel));

		s_featureLevel = featureLevel;

		// Get default command queue
        D3D12_COMMAND_QUEUE_DESC qDesc;
        ZeroMemory(&qDesc, sizeof(qDesc));
        qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
#ifdef _XBOX_ONE
        qDesc.NodeMask = QD3D12_NODE_MASK;
#endif
        qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		COM_ERROR_TRAP( s_pDevice->CreateCommandQueue(&qDesc, __uuidof(ID3D12CommandQueue), (void**) &s_pCmdQ) );

		s_frameFence.Init();

		GlobalCommandList::Init( s_pDevice, s_pCmdQ );

		return s_pDevice;
	}

	ID3D12Device* Device::Get()
	{
		assert(s_pDevice != nullptr);
		return s_pDevice;
	}

	ID3D12CommandQueue* Device::GetQ()
	{
		assert(s_pCmdQ != nullptr);
		return s_pCmdQ;
	}

	bool Device::IsStarted()
	{
		return s_pDevice != nullptr;
	}

	void Device::WaitForGpu()
	{
        if (Device::IsStarted())
        {
			s_frameFence.Advance();
			s_frameFence.Wait();
        }
	}

	void Device::Destroy()
	{
		WaitForGpu();

		GlobalCommandList::Destroy();

		s_frameFence.Destroy();
        
		SAFE_RELEASE(s_pCmdQ);

        if (s_pDebug != nullptr)
        {
            // TODO
            // s_pDebug->ReportLiveDeviceObjects( D3D12_RLDO_DETAIL );
            SAFE_RELEASE( s_pDebug );
        }

		while (SAFE_RELEASE(s_pDevice)) {}
	}
}