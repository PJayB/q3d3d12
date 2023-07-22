#include "Device.h"
#include "DescriptorHeaps.h"

namespace QD3D12
{
	// Q: Why not use D3D12_DESCRIPTOR_HEAP_DESC instead of making your own?
	// A: Because if the D3D12 alter the structure in a way that compiles then this will explode.
	struct DESCRIPTOR_HEAP_DESC
	{
		D3D12_DESCRIPTOR_HEAP_TYPE Type;
        D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
	};

	static const DESCRIPTOR_HEAP_DESC c_DescriptorHeapDescs[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = 
	{
		// TODO: default count for descriptors
		{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,	D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV,			D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV,			D3D12_DESCRIPTOR_HEAP_FLAG_NONE }
	};

    void DefaultDescriptorHeapDesc(
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        D3D12_DESCRIPTOR_HEAP_DESC* pDesc,
        UINT count)
    {
        assert(c_DescriptorHeapDescs[type].Type == type);
        pDesc->Flags = c_DescriptorHeapDescs[type].Flags;
        pDesc->NumDescriptors = 0;
        pDesc->Type = type;
    }

    DescriptorArray::DescriptorArray()
    {
        ZeroMemory(this, sizeof(*this));
    }

    DescriptorArray::~DescriptorArray()
    {
        Destroy();
    }

    void DescriptorArray::Init(
        _In_ const D3D12_DESCRIPTOR_HEAP_DESC* pDesc)
	{
        assert(pDesc != nullptr);
        assert(pDesc->NumDescriptors > 0);

		ID3D12Device* pDevice = Device::Get();

        m_desc = *pDesc;

		COM_ERROR_TRAP(pDevice->CreateDescriptorHeap(
			pDesc, 
			__uuidof(ID3D12DescriptorHeap), 
			(void**) &m_pHeap));

        m_hCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();

		if (pDesc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
			m_hGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();

		m_increment = pDevice->GetDescriptorHandleIncrementSize(pDesc->Type);
	}

    void DescriptorArray::Init(
        _In_ D3D12_DESCRIPTOR_HEAP_TYPE type,
        _In_ D3D12_DESCRIPTOR_HEAP_FLAGS flags,
        _In_ UINT count)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Flags = flags;
        desc.NumDescriptors = count;
        desc.Type = type;
        desc.NodeMask = QD3D12_NODE_MASK;
        Init(&desc);
    }

	void DescriptorArray::Destroy()
	{
		SAFE_RELEASE(m_pHeap);
		ZeroMemory(this, sizeof(*this));
	}
}
