#include "ConstantBuffer.h"
#include "Frame.h"

namespace QD3D12
{
	D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::Write(
		_In_reads_(size) LPCVOID data,
		_In_ UINT64 size)
	{
		return m_addr = GlobalWrite(data, size);
	}

	D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GlobalWrite(
		_In_reads_(size) LPCVOID data,
		_In_ UINT64 size)
	{
		UploadScratch* pScratch = Frame::CurrentUploadScratchMemory();

		return pScratch->Write(
			data, 
			QD3D12_CONSTANT_BUFFER_ALIGNMENT, 
			size);
	}
}
