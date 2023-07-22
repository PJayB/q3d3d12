#include "BackBufferState.h"
#include "SwapChain.h"
#include "BackBufferState.h"

namespace QD3D12
{
	BackBufferState::BackBufferState()
		: m_depthBuffer(nullptr)
	{

	}

	BackBufferState::~BackBufferState()
	{
		Destroy();
	}

	void BackBufferState::Init(ID3D12GraphicsCommandList* cmdList, UINT bufferIndex)
	{
		ID3D12Device* pDevice = Device::Get();
		QDXGISwapChain* pSwapChain = SwapChain::Get();

		COM_ERROR_TRAP(pSwapChain->GetDesc(&m_scDesc));

		// Initialize the descriptor heap for the RTVs
		m_rtvDescriptorHeap.Init(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
		m_dsvDescriptorHeap.Init(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

		// Get the viewport from the back buffer
		const DXGI_MODE_DESC& mode = m_scDesc.BufferDesc;
		D3D12_VIEWPORT vp = 
		{ 
			0.0f, 0.0f, 
			static_cast<float>(mode.Width), static_cast<float>(mode.Height),
			0.0f, 1.0f 
		};
		m_viewport = vp;

        // Get the best possible sample desc
        SwapChain::GetMsaaDescFromConfig( pDevice, m_scDesc.BufferDesc.Format, &m_sampleDesc );

		D3D12_RESOURCE_STATES resourceType = (m_sampleDesc.Count > 1) ? D3D12_RESOURCE_STATE_RESOLVE_SOURCE : D3D12_RESOURCE_STATE_COPY_SOURCE;

		// Color target
		D3D12_RESOURCE_DESC rtrd;
		ZeroMemory(&rtrd, sizeof(rtrd));
		rtrd.DepthOrArraySize = 1;
		rtrd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		rtrd.Format = m_scDesc.BufferDesc.Format;
		rtrd.Height = m_scDesc.BufferDesc.Height;
		rtrd.Width = m_scDesc.BufferDesc.Width;
		rtrd.MipLevels = 1;
		rtrd.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		rtrd.SampleDesc = m_sampleDesc;

		const float debugClearColor[4] = { 0, 0, 0, 0 };
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_CLEAR_VALUE cClearVal(m_scDesc.BufferDesc.Format, debugClearColor);
		CD3DX12_CLEAR_VALUE dClearVal(DEPTH_FORMAT, 1.0f, 0);

		COM_ERROR_TRAP(pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&rtrd,
			resourceType,
			&cClearVal,
			__uuidof(ID3D12Resource),
			(void**)&m_colorBuffer));

		SetResourceName(m_colorBuffer, "Color Buffer %d", bufferIndex);

		// Create render target view
		m_cbRTV = m_rtvDescriptorHeap.GetCpuHandle(0);

        D3D12_RENDER_TARGET_VIEW_DESC rtvd;
        ZeroMemory( &rtvd, sizeof(rtvd) );
        rtvd.Format = rtrd.Format;
        rtvd.ViewDimension = (m_sampleDesc.Count > 1)
            ? D3D12_RTV_DIMENSION_TEXTURE2DMS 
            : D3D12_RTV_DIMENSION_TEXTURE2D;
		pDevice->CreateRenderTargetView(m_colorBuffer, &rtvd, m_cbRTV);

		// Depth target
		D3D12_RESOURCE_DESC dsrd;
		ZeroMemory(&dsrd, sizeof(dsrd));
		dsrd.DepthOrArraySize = 1;
		dsrd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		dsrd.Format = DEPTH_FORMAT;
		dsrd.Height = m_scDesc.BufferDesc.Height;
		dsrd.Width = m_scDesc.BufferDesc.Width;
		dsrd.MipLevels = 1;
		dsrd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		dsrd.SampleDesc = m_sampleDesc;

		COM_ERROR_TRAP(pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&dsrd,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&dClearVal,
			__uuidof(ID3D12Resource),
			(void**)&m_depthBuffer));

		SetResourceName(m_depthBuffer, "Depth Buffer %d", bufferIndex);

		m_bbDSV = m_dsvDescriptorHeap.GetCpuHandle(0);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvd;
		ZeroMemory(&dsvd, sizeof(dsvd));
		dsvd.Format = dsrd.Format;
		dsvd.Texture2D.MipSlice = 0;
        dsvd.ViewDimension = (m_sampleDesc.Count > 1)
            ? D3D12_DSV_DIMENSION_TEXTURE2DMS 
            : D3D12_DSV_DIMENSION_TEXTURE2D;
		pDevice->CreateDepthStencilView(
			m_depthBuffer,
			&dsvd,
			m_bbDSV);

#ifdef _DEBUG
        m_currentState = BUFFER_FRAME_NEXT;
#endif
    }

	void BackBufferState::Destroy()
	{
		SAFE_RELEASE(m_colorBuffer);
		SAFE_RELEASE(m_depthBuffer);
		m_rtvDescriptorHeap.Destroy();
	}

	void BackBufferState::PrepareForResolve(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* pBackBuffer)
	{
#ifdef _DEBUG
        ASSERT(m_currentState == BUFFER_RESOLVE_NEXT);
        m_currentState = BUFFER_PRESENT_NEXT;
#endif

        if ( m_sampleDesc.Count > 1 )
        {
		    TransitionResource(cmdList, m_colorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
		    TransitionResource(cmdList, pBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST);
        }
        else
        {
		    TransitionResource(cmdList, m_colorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		    TransitionResource(cmdList, pBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
        }
	}

	void BackBufferState::PrepareForPresent(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* pBackBuffer)
	{
#ifdef _DEBUG
        ASSERT(m_currentState == BUFFER_PRESENT_NEXT);
        m_currentState = BUFFER_FRAME_NEXT;
#endif

        if ( m_sampleDesc.Count > 1 )
        {
		    TransitionResource(cmdList, pBackBuffer, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT );
        }
        else
        {
		    TransitionResource(cmdList, pBackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT );
        }
	}

	void BackBufferState::PrepareForFrame(ID3D12GraphicsCommandList* cmdList)
	{
#ifdef _DEBUG
        ASSERT(m_currentState == BUFFER_FRAME_NEXT);
        m_currentState = BUFFER_RESOLVE_NEXT;
#endif

        if ( m_sampleDesc.Count > 1 )
        {
			TransitionResource(cmdList, m_colorBuffer, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
		else
		{
			TransitionResource(cmdList, m_colorBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

        D3D12_RECT srect = { 0, 0, (LONG) m_viewport.Width, (LONG) m_viewport.Height };

        cmdList->RSSetViewports(1, &m_viewport);
        cmdList->RSSetScissorRects(1, &srect);
	}

	UINT BackBufferState::GetRenderTargetFormats(DXGI_FORMAT formats[], UINT count)
	{
		count = min(count, m_scDesc.BufferCount);

		for (UINT i = 0; i < count; ++i)
		{
			formats[i] = m_scDesc.BufferDesc.Format;
		}

		return count;
	}
}
