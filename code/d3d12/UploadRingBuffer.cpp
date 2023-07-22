#include "UploadRingBuffer.h"

namespace QD3D12
{
	UploadRingBuffer::UploadRingBuffer()
		: m_uploadResource(nullptr)
		, m_buffer(nullptr)
		, m_size(0)
		, m_cpuTotal(0)
	{
	}

	UploadRingBuffer::~UploadRingBuffer()
	{
		Destroy();
	}

	void UploadRingBuffer::Init(UINT64 sizeBytes)
	{
		ID3D12Device* pDevice = Device::Get();

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeBytes);

		COM_ERROR_TRAP(pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			__uuidof(ID3D12Resource),
			(void**)&m_uploadResource));

		SetResourceName(m_uploadResource, "UploadRingBuffer");

		m_uploadResource->Map(0, nullptr, (void**)&m_buffer);
		ZeroMemory(m_buffer, sizeBytes);

		m_fence.Init();
		m_gpuAddress = m_uploadResource->GetGPUVirtualAddress();
		m_size = sizeBytes;
		m_cpuTotal = 0;
	}

	void UploadRingBuffer::Destroy()
	{
		if (m_uploadResource != nullptr)
		{
			m_uploadResource->Unmap(0, nullptr);
		}

		SAFE_RELEASE(m_uploadResource);
		m_fence.Destroy();
		m_buffer = nullptr;
		m_size = 0;
		m_cpuTotal = 0;
	}

	D3D12_GPU_VIRTUAL_ADDRESS UploadRingBuffer::Write(
		_In_reads_(size) LPCVOID data,
		_In_ UINT64 size)
	{
		// Wait for the resource to become available if the GPU is using it
		UINT64 offset = m_cpuTotal;
		UINT64 end = offset + size;
		if (end >= m_size)
			m_fence.WaitUntil(end - m_size);

		// Write the memory in
		offset = RingOffset(offset);
		memcpy(m_buffer + offset, data, size);

		// Set the new fence
		m_fence.Advance(end);
		m_cpuTotal = end;

		// Return the location of the memory
		return m_gpuAddress + offset;
	}
}

