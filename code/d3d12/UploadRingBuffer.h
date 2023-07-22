#pragma once

#include "Fence.h"

namespace QD3D12
{
	class UploadRingBuffer
	{
	private:
		Fence m_fence;
		ID3D12Resource* m_uploadResource;
		BYTE* m_buffer;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress;
		UINT64 m_size;
		UINT64 m_cpuTotal; // what's been written so far

		inline UINT64 RingOffset(UINT64 total) const
		{
			return total % m_size;
		}

	public:
		UploadRingBuffer();
		~UploadRingBuffer();

		void Init(
			_In_ UINT64 sizeBytes);
		void Destroy();

		D3D12_GPU_VIRTUAL_ADDRESS Write(
			_In_reads_(size) LPCVOID data,
			_In_ UINT64 size);

		ID3D12Resource* Resource() const { return m_uploadResource; }
	};
}
