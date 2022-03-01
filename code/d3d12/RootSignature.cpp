#include "pch.h"
#include "RootSignature.h"

namespace QD3D12
{
	ID3D12RootSignature* RootSignature::s_pEmpty = nullptr;

	ID3D12RootSignature* RootSignature::Create(const D3D12_ROOT_SIGNATURE_DESC* rsig)
	{
		ID3DBlob* pOutBlob = nullptr, *pErrorBlob = nullptr;

		COM_ERROR_TRAP(D3D12SerializeRootSignature(
			rsig, 
			D3D_ROOT_SIGNATURE_VERSION_1, 
			&pOutBlob, 
			&pErrorBlob));

		ID3D12RootSignature* rootSignature = nullptr;
		COM_ERROR_TRAP(Device::Get()->CreateRootSignature(
            QD3D12_NODE_MASK,
			pOutBlob->GetBufferPointer(), 
			pOutBlob->GetBufferSize(), 
			__uuidof(ID3D12RootSignature), 
			(void**) &rootSignature ));

		SAFE_RELEASE(pOutBlob);
		SAFE_RELEASE(pErrorBlob);

		return rootSignature;
	}

	void RootSignature::Init()
	{
        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature(D3D12_DEFAULT);
		s_pEmpty = Create(&descRootSignature);
	}

	void RootSignature::Destroy()
	{
		SAFE_RELEASE(s_pEmpty);
	}

	ID3D12RootSignature* RootSignature::Empty()
	{
		assert(s_pEmpty != nullptr);
		return s_pEmpty;
	}
}
