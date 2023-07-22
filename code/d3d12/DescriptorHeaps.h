#pragma once

#include "D3D12Core.h"

namespace QD3D12
{
    // A contiguous linear random-access descriptor heap
    class DescriptorArray
    {
    public:

        DescriptorArray();
        ~DescriptorArray();

        void Init(
            _In_ const D3D12_DESCRIPTOR_HEAP_DESC* pDesc);
        void Init(
            _In_ D3D12_DESCRIPTOR_HEAP_TYPE type,
            _In_ D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            _In_ UINT count);
        void Destroy();

        inline D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGpuHandle() const;
        inline D3D12_CPU_DESCRIPTOR_HANDLE GetFirstCpuHandle() const;

        inline D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(_In_ UINT index) const;
        inline D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(_In_ UINT index) const;

		inline UINT Count() const { return m_desc.NumDescriptors; }
		inline UINT Flags() const { return m_desc.Flags; }
		inline D3D12_DESCRIPTOR_HEAP_TYPE Type() const { return m_desc.Type; }
		inline UINT Increment() const { return m_increment; }
		inline ID3D12DescriptorHeap* Heap() const { return m_pHeap; }

    private:

        ID3D12DescriptorHeap* m_pHeap;
        D3D12_DESCRIPTOR_HEAP_DESC m_desc;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCPU;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGPU;
		UINT m_increment;
    };

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorArray::GetFirstCpuHandle() const
	{
		assert(m_pHeap != nullptr);
		return m_hCPU;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorArray::GetFirstGpuHandle() const
	{
		assert(m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		assert(m_pHeap != nullptr);
		return m_hGPU;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorArray::GetCpuHandle(_In_ UINT index) const
	{
		assert(index < m_desc.NumDescriptors);
		assert(m_pHeap != nullptr);
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_hCPU, (INT)index, m_increment);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorArray::GetGpuHandle(_In_ UINT index) const
	{
		assert(index < m_desc.NumDescriptors);
		assert(m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		assert(m_pHeap != nullptr);
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_hGPU, (INT)index, m_increment);
	}

    void DefaultDescriptorHeapDesc(
        D3D12_DESCRIPTOR_HEAP_TYPE type, 
        D3D12_DESCRIPTOR_HEAP_DESC* pDesc,
        UINT count);
}
