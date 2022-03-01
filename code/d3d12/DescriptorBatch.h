#pragma once

#include "DescriptorHeaps.h"

namespace QD3D12
{
	struct DESCRIPTOR_HANDLE_TUPLE 
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Gpu;
		D3D12_CPU_DESCRIPTOR_HANDLE Cpu;
	};

	class DescriptorBatch
	{
	private:
        DescriptorArray m_array;
        D3D12_DESCRIPTOR_HEAP_TYPE m_type;
        UINT m_size;
        UINT m_count;
	public:
        void Init(
            _In_ D3D12_DESCRIPTOR_HEAP_TYPE type,
            _In_ UINT maxDescriptors);
        void Destroy();
        void Reset();

		DESCRIPTOR_HANDLE_TUPLE AllocateDescriptors(
			_In_ UINT count );

        D3D12_GPU_DESCRIPTOR_HANDLE WriteDescriptors(
            _In_reads_(descriptorRangeCount) const D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorRangeStarts,
            _In_reads_(descriptorRangeCount) const UINT* pDescriptorRangeSizes,
            _In_ UINT descriptorRangeCount,
            _In_ UINT totalDescriptorCount);

        D3D12_GPU_DESCRIPTOR_HANDLE WriteDescriptors(
            _In_reads_(descriptorRangeCount) const D3D12_CPU_DESCRIPTOR_HANDLE* pDescriptorRangeStarts,
            _In_reads_(descriptorRangeCount) const UINT* pDescriptorRangeSizes,
            _In_ UINT descriptorRangeCount);

        ID3D12DescriptorHeap* Heap() const { return m_array.Heap(); }
	};
}
