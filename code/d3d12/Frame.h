#pragma once

#include "CommandState.h"
#include "BackBufferState.h"
#include "Fence.h"
#include "UploadScratch.h"
#include "TessBuffers.h"
#include "DlightBuffers.h"
#include "DescriptorBatch.h"

namespace QD3D12
{
	class Frame
	{
	private:
		static UINT s_frameCount;
		static UINT s_currentIndex;
		static BOOL s_open;
        static BOOL s_initialized;

		static CommandState s_commandRing[QD3D12_NUM_BUFFERS];
		static BackBufferState s_backBufferRing[QD3D12_NUM_BUFFERS];
		static UploadScratch s_scratchSpace[QD3D12_NUM_BUFFERS];
		static TessBuffers s_tess[QD3D12_NUM_BUFFERS];
		static DlightBuffers s_dlights[QD3D12_NUM_BUFFERS];
        static DescriptorBatch s_descriptors[QD3D12_NUM_BUFFERS][D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
		static Fence s_fences[QD3D12_NUM_BUFFERS];

		static Fence* CurrentFence() { return &s_fences[s_currentIndex]; }

        static void InitDescriptorBatch(UINT bufferIndex, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT size);

	public:		
		static void Init();
		static void Destroy();

		static void Begin(); // Reentrant.
		static void End(
			ID3D12Resource* pBackBuffer,
			DXGI_FORMAT bbFormat );

		static inline BOOL IsOpen() { return s_open; }
		static inline UINT CurrentIndex() { return s_currentIndex; }
		static inline UINT FrameCount() { return s_frameCount; }

		static CommandState* GetCommandState(UINT index);
		static ID3D12GraphicsCommandList* GetCommandList(UINT index);
		static BackBufferState* GetBackBufferState(UINT index);
		static UploadScratch* GetUploadScratchMemory(UINT index);
		static TessBuffers* GetTessBuffers(UINT index);
        static DlightBuffers* GetDlightBuffers(UINT index);
        static DescriptorBatch* GetDescriptorBatch(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index);

		static inline CommandState* CurrentCommandState() { assert(IsOpen());  return &s_commandRing[s_currentIndex]; }
		static inline ID3D12GraphicsCommandList* CurrentCommandList() { assert(IsOpen()); return s_commandRing[s_currentIndex].List(); }
		static inline BackBufferState* CurrentBackBufferState() { assert(IsOpen()); return &s_backBufferRing[s_currentIndex]; }
		static inline UploadScratch* CurrentUploadScratchMemory() { assert(IsOpen()); return &s_scratchSpace[s_currentIndex]; }
		static inline TessBuffers* CurrentTessBuffers() { assert(IsOpen()); return &s_tess[s_currentIndex]; }
		static inline DlightBuffers* CurrentDlightBuffers() { assert(IsOpen()); return &s_dlights[s_currentIndex]; }
        static inline DescriptorBatch* CurrentDescriptorBatch(D3D12_DESCRIPTOR_HEAP_TYPE type) { assert(IsOpen()); return &s_descriptors[s_currentIndex][type]; }
	};
}
