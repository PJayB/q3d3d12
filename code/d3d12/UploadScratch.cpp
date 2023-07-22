#include "UploadScratch.h"

namespace QD3D12
{
	UploadScratch::UploadScratch()
		: m_uploadResource(nullptr)
		, m_buffer(nullptr)
		, m_size(0)
		, m_cpuTotal(0)
	{
	}

	UploadScratch::~UploadScratch()
	{
		Destroy();
	}

	void UploadScratch::Init(UINT64 sizeBytes)
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

		SetResourceName(m_uploadResource, "UploadScratch");

		m_uploadResource->Map(0, nullptr, (void**)&m_buffer);
		ZeroMemory(m_buffer, sizeBytes);

		m_gpuAddress = m_uploadResource->GetGPUVirtualAddress();
		m_size = sizeBytes;
		m_cpuTotal = 0;
	}

	void UploadScratch::Destroy()
	{
		if (m_uploadResource != nullptr)
		{
			m_uploadResource->Unmap(0, nullptr);
		}

		SAFE_RELEASE(m_uploadResource);
		m_buffer = nullptr;
		m_size = 0;
		m_cpuTotal = 0;
	}

	void UploadScratch::Reset()
	{
		m_cpuTotal = 0;
	}

	D3D12_GPU_VIRTUAL_ADDRESS UploadScratch::Reserve(
		_In_ UINT64 alignment,
		_In_ UINT64 size,
		_Out_ void** ppData)
	{
		m_cpuTotal = QD3D::Align<UINT64>(m_cpuTotal, alignment);

		// Wait for the resource to become available if the GPU is using it
		// Set the new write location
		UINT64 offset = m_cpuTotal;
		assert(m_cpuTotal + size <= m_size);

		// Pass out the memory
		*ppData = m_buffer + offset;

		m_cpuTotal += size;

		// Return the location of the memory
		return m_gpuAddress + offset;
	}

	D3D12_GPU_VIRTUAL_ADDRESS UploadScratch::Write(
		_In_reads_(size) LPCVOID data,
		_In_ UINT64 alignment,
		_In_ UINT64 size)
	{
		m_cpuTotal = QD3D::Align<UINT64>(m_cpuTotal, alignment);
		
		// Wait for the resource to become available if the GPU is using it
		// Set the new write location
		UINT64 offset = m_cpuTotal;
		assert(m_cpuTotal + size <= m_size);

		// Write the memory in
		memcpy(m_buffer + offset, data, size);

		m_cpuTotal += size;

		// Return the location of the memory
		return m_gpuAddress + offset;
	}
}

