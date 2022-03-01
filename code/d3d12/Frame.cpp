#include "pch.h"
#include "Frame.h"
#include "TessBuffers.h"
#include "Resources.h"
#include "GlobalCommandList.h"

namespace QD3D12
{
	CommandState Frame::s_commandRing[];
	BackBufferState Frame::s_backBufferRing[];
	UploadScratch Frame::s_scratchSpace[];
	TessBuffers Frame::s_tess[];
    DlightBuffers Frame::s_dlights[];
    DescriptorBatch Frame::s_descriptors[][D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	Fence Frame::s_fences[];
	UINT Frame::s_frameCount = 0;
	UINT Frame::s_currentIndex = 0;
	BOOL Frame::s_open = FALSE;
    BOOL Frame::s_initialized = FALSE;

	// --- statics ---

	void Frame::Begin()
	{
		if (!s_open)
		{
			s_open = TRUE;

			CurrentFence()->Wait();
			CurrentCommandState()->Reset();
			CurrentBackBufferState()->PrepareForFrame(
				CurrentCommandList());
		}
	}

	void Frame::End(
		ID3D12Resource* pBackBuffer,
		DXGI_FORMAT bbFormat )
	{
		if (IsOpen())
		{
            auto pCmdList = CurrentCommandList();
            auto pBackBufferState = CurrentBackBufferState();

            // Transition the color buffer and render it into the back buffer
            pBackBufferState->PrepareForResolve(pCmdList, pBackBuffer);
            
            // Copy to the back buffer
            Resources::ResolveTarget(
                pCmdList,
                pBackBuffer,
                pBackBufferState->ColorBufferResource(),
				bbFormat,
                pBackBufferState->SampleDesc()->Count > 1 );

			// Transition the back buffer to Present
			pBackBufferState->PrepareForPresent(pCmdList, pBackBuffer);

			// Close the command queue
			CurrentCommandState()->CloseAndExecute();

			// Mark the end of the current frame
			CurrentFence()->Advance();

			// Reset the current scratch memory
			CurrentUploadScratchMemory()->Reset();

			// Reset the other buffers
			CurrentTessBuffers()->Reset();
			CurrentDlightBuffers()->Reset();

            // Reset the descriptor heaps
            for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
            {
                CurrentDescriptorBatch((D3D12_DESCRIPTOR_HEAP_TYPE)i)->Reset();
            }

			// Set the new index
			s_frameCount++;
			s_currentIndex = (s_currentIndex + 1) % QD3D12_NUM_BUFFERS;
			s_open = FALSE;
		}
	}

    void Frame::InitDescriptorBatch(
        UINT bufferIndex,
        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        UINT count)
    {
        s_descriptors[bufferIndex][heapType].Init(heapType, count);
    }

	void Frame::Init()
	{
		// Initialize the frame data
		s_currentIndex = 0;
		s_open = FALSE;
		s_frameCount = 0;

		for (int i = 0; i < QD3D12_NUM_BUFFERS; ++i)
		{
			//
			// Initialize resources
			//
            s_fences[i].Init();
			s_commandRing[i].Init();
			s_backBufferRing[i].Init(GlobalCommandList::Get(), i);
			s_scratchSpace[i].Init(QD3D12_UPLOAD_SCRATCH_SIZE);
			s_tess[i].Init();
			s_dlights[i].Init();

            InitDescriptorBatch(i, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, QD3D12_NUM_CBV_SRV_UAV_DESCRIPTORS);
            InitDescriptorBatch(i, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, QD3D12_NUM_SAMPLER_DESCRIPTORS);
            InitDescriptorBatch(i, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, QD3D12_NUM_RTV_DESCRIPTORS);
            InitDescriptorBatch(i, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, QD3D12_NUM_DSV_DESCRIPTORS);

			//
			// Set names
			//
			s_tess[i].SetResourceName(L"F%d Tess", i);
			SetResourceName(s_dlights[i].DlightScratch()->Resource(), L"F%d DlightScratch", i);
			SetResourceName(s_backBufferRing[i].ColorBufferResource(), L"F%d Color Buffer", i);
			SetResourceName(s_backBufferRing[i].DepthBufferResource(), L"F%d Depth Buffer", i);
			SetResourceName(s_scratchSpace[i].Resource(), L"F%d Upload Scratch", i);
		}

        GlobalCommandList::KickOff();

        s_initialized = TRUE;
	}

	void Frame::Destroy()
	{
        if (s_initialized)
        {
		    for (int i = 0; i < QD3D12_NUM_BUFFERS; ++i)
		    {
			    s_fences[i].Wait();
			    s_commandRing[i].Destroy();
			    s_backBufferRing[i].Destroy();
				s_scratchSpace[i].Destroy();
				s_tess[i].Destroy();
				s_dlights[i].Destroy();

                for (UINT j = 0; j < _countof(s_descriptors[i]); ++j)
                {
                    s_descriptors[i][j].Destroy();
                }

                s_fences[i].Destroy();
		    }

		    s_currentIndex = 0;
		    s_frameCount = 0;
		    s_open = FALSE;
            s_initialized = FALSE;
        }
	}

	CommandState* Frame::GetCommandState(UINT index)
	{
		assert(index < _countof(s_commandRing));
		return &s_commandRing[index];
	}

    ID3D12GraphicsCommandList* Frame::GetCommandList(UINT index)
	{
		return GetCommandState(index)->List();
	}

	BackBufferState* Frame::GetBackBufferState(UINT index)
	{
		assert(index < _countof(s_backBufferRing));
		return &s_backBufferRing[index];
	}

	UploadScratch* Frame::GetUploadScratchMemory(UINT index)
	{
		assert(index < _countof(s_scratchSpace));
		return &s_scratchSpace[index];
	}

	TessBuffers* Frame::GetTessBuffers(UINT index)
	{
		assert(index < _countof(s_tess));
		return &s_tess[index];
	}

	DlightBuffers* Frame::GetDlightBuffers(UINT index)
	{
		assert(index < _countof(s_dlights));
		return &s_dlights[index];
	}

    DescriptorBatch* Frame::GetDescriptorBatch(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index)
	{
		assert(index < _countof(s_descriptors));
		return &s_descriptors[index][type];
	}
}
