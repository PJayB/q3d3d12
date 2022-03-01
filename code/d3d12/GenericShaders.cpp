#include "pch.h"
#include "GenericShaders.h"

namespace QD3D12
{
	Shader GenericShaders::s_vertexST;
    Shader GenericShaders::s_vertexMT;
    Shader GenericShaders::s_vertexDepth;
    Shader GenericShaders::s_vertexDlight;
	Shader GenericShaders::s_pixelUnlitST;
	Shader GenericShaders::s_pixelST;
    Shader GenericShaders::s_pixelMT;
    Shader GenericShaders::s_pixelDepth;
    Shader GenericShaders::s_pixelDlight;

	void InitGenericUnlitRootSignatureSlots(
		_Out_ D3D12_ROOT_PARAMETER* baseParams,
		_In_ SIZE_T size)
	{
		assert(size >= sizeof(D3D12_ROOT_PARAMETER) * GENERIC_ROOT_SIGNATURE_SLOT_COUNT_UNLIT);
        auto params = reinterpret_cast<CD3DX12_ROOT_PARAMETER*>(baseParams);
		params[GENERIC_ROOT_SIGNATURE_SLOT_VS_COMMON_CB].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		params[GENERIC_ROOT_SIGNATURE_SLOT_PS_COMMON_CB].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	}

	void InitGenericLitRootSignatureSlots(
		_Out_ D3D12_ROOT_PARAMETER* baseParams,
		_In_ SIZE_T size)
	{
		assert(size >= sizeof(D3D12_ROOT_PARAMETER) * GENERIC_ROOT_SIGNATURE_SLOT_COUNT_LIT);
        auto params = reinterpret_cast<CD3DX12_ROOT_PARAMETER*>(baseParams);
		params[GENERIC_ROOT_SIGNATURE_SLOT_VS_COMMON_CB].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		params[GENERIC_ROOT_SIGNATURE_SLOT_PS_COMMON_CB].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		params[GENERIC_ROOT_SIGNATURE_SLOT_PS_DLIGHT_ARRAY_CB].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		params[GENERIC_ROOT_SIGNATURE_SLOT_PS_DLIGHT_INFO_CB].InitAsConstants(2, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	}

	void SetGenericUnlitRootConstants(
		_In_ ID3D12GraphicsCommandList* pList,
		_In_ const GENERIC_ROOT_SIGNATURE_UNLIT_CONSTANT_BUFFERS* pCBs)
	{
		SetGenericRootConstantBufferView<GENERIC_ROOT_SIGNATURE_SLOT_VS_COMMON_CB>(pList, pCBs->VSCommonCB);
		SetGenericRootConstantBufferView<GENERIC_ROOT_SIGNATURE_SLOT_PS_COMMON_CB>(pList, pCBs->PSCommonCB);
	}

	void SetGenericLitRootConstants(
		_In_ ID3D12GraphicsCommandList* pList,
		_In_ const GENERIC_ROOT_SIGNATURE_LIT_CONSTANT_BUFFERS* pCBs)
	{
		SetGenericRootConstantBufferView<GENERIC_ROOT_SIGNATURE_SLOT_VS_COMMON_CB>(pList, pCBs->VSCommonCB);
		SetGenericRootConstantBufferView<GENERIC_ROOT_SIGNATURE_SLOT_PS_COMMON_CB>(pList, pCBs->PSCommonCB);
		SetGenericRootConstantBufferView<GENERIC_ROOT_SIGNATURE_SLOT_PS_DLIGHT_ARRAY_CB>(pList, pCBs->PSDlightArrayCB);
		pList->SetGraphicsRoot32BitConstants(GENERIC_ROOT_SIGNATURE_SLOT_PS_DLIGHT_INFO_CB, 2, &pCBs->DlightCounts, 0);
	}

	void GenericShaders::Init()
	{
		s_vertexST.Init("genericst_vs");
        s_vertexMT.Init("genericmt_vs");
        s_vertexDepth.Init("depth_vs");
        s_vertexDlight.Init("dlight_vs");
		s_pixelUnlitST.Init("genericunlitst_ps");
		s_pixelST.Init("genericst_ps");
		s_pixelMT.Init("genericmt_ps");
        s_pixelDepth.Init("depth_ps");
        s_pixelDlight.Init("dlight_ps");
	}
	
	void GenericShaders::Destroy()
	{
		s_vertexST.Destroy();
        s_vertexMT.Destroy();
        s_vertexDepth.Destroy();
        s_vertexDlight.Destroy();
        s_pixelUnlitST.Destroy();
		s_pixelST.Destroy();
        s_pixelMT.Destroy();
        s_pixelDepth.Destroy();
        s_pixelDlight.Destroy();
    }
}