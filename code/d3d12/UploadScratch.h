#pragma once

namespace QD3D12
{
	// YOU MUST FENCE THIS YOURSELF.
	// TODO: Make this thread safe
	class UploadScratch
	{
	private:
		ID3D12Resource* m_uploadResource;
		BYTE* m_buffer;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress;
		UINT64 m_size;
		UINT64 m_cpuTotal; // what's been written so far
	
	public:
		UploadScratch();
		~UploadScratch();

		void Init(
			_In_ UINT64 sizeBytes);
		void Destroy();

		void Reset();

		D3D12_GPU_VIRTUAL_ADDRESS Reserve(
			_In_ UINT64 alignment,
			_In_ UINT64 size,
			_Out_ void** pData);

		D3D12_GPU_VIRTUAL_ADDRESS Write(
			_In_reads_(size) LPCVOID data,
			_In_ UINT64 alignment,
			_In_ UINT64 size);

		ID3D12Resource* Resource() const { return m_uploadResource; }
	};
}
