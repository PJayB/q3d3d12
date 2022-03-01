#include "pch.h"
#include "DescriptorBatch.h"

namespace QD3D12
{
    void DescriptorBatch::Init(
        _In_ D3D12_DESCRIPTOR_HEAP_TYPE type,
        _In_ UINT maxDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_FLAGS flags;
        switch (type)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            break;
        default:
            flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            break;
        }

        m_array.Init(
            type,
            flags,
            maxDescriptors);
        m_type = type;
        m_count = 0;
        m_size = maxDescriptors;
    }

    void DescriptorBatch::Destroy()
    {
        m_array.Destroy();
    }

	DESCRIPTOR_HANDLE_TUPLE DescriptorBatch::AllocateDescriptors(
		_In_ UINT count )
	{
        assert(m_count + count <= m_size);

		DESCRIPTOR_HANDLE_TUPLE t;
		t.Gpu = m_array.GetGpuHandle(m_count);
		t.Cpu = m_array.GetCpuHandle(m_count);

        m_count += count;
        return t;
	}

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorBatch::WriteDescriptors(
        _In_reads_(descriptorRangeCount) const D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorRangeStarts,
        _In_reads_(descriptorRangeCount) const UINT* pDescriptorRangeSizes,
        _In_ UINT descriptorRangeCount,
        _In_ UINT totalDescriptorCount)
    {
        assert(m_count + totalDescriptorCount <= m_size);

        ID3D12Device* pDevice = Device::Get();
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_array.GetCpuHandle(m_count);

        pDevice->CopyDescriptors(
            1,
            &cpuHandle,
            &totalDescriptorCount,
            descriptorRangeCount,
            pDescriptorRangeStarts,
            pDescriptorRangeSizes,
            m_type);

        auto gpuHandle = m_array.GetGpuHandle(m_count);
     
        m_count += totalDescriptorCount;
        return gpuHandle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorBatch::WriteDescriptors(
        _In_reads_(descriptorRangeCount) const D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorRangeStarts,
        _In_reads_(descriptorRangeCount) const UINT* pDescriptorRangeSizes,
        _In_ UINT descriptorRangeCount)
    {
        UINT totalDescriptorCount = 0;
        for (UINT i = 0; i < descriptorRangeCount; ++i)
            totalDescriptorCount += pDescriptorRangeSizes[i];
        
        return WriteDescriptors(
            pDescriptorRangeStarts,
            pDescriptorRangeSizes,
            descriptorRangeCount,
            totalDescriptorCount);
    }

    void DescriptorBatch::Reset()
    {
        m_count = 0;
 
    }
}
