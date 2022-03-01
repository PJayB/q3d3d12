#pragma once

#include "Shader.h"
#include "PipelineState.h"
#include "DlightBuffers.h"

namespace QD3D12 
{
    enum GENERIC_ROOT_SIGNATURE_SLOTS
    {
        GENERIC_ROOT_SIGNATURE_SLOT_VS_COMMON_CB,
        GENERIC_ROOT_SIGNATURE_SLOT_PS_COMMON_CB,
        GENERIC_ROOT_SIGNATURE_SLOT_COUNT_UNLIT,
        GENERIC_ROOT_SIGNATURE_SLOT_UNLIT_CUSTOM_START = GENERIC_ROOT_SIGNATURE_SLOT_COUNT_UNLIT,
        GENERIC_ROOT_SIGNATURE_SLOT_PS_DLIGHT_ARRAY_CB = GENERIC_ROOT_SIGNATURE_SLOT_UNLIT_CUSTOM_START,
        GENERIC_ROOT_SIGNATURE_SLOT_PS_DLIGHT_INFO_CB,
        GENERIC_ROOT_SIGNATURE_SLOT_COUNT_LIT,
        GENERIC_ROOT_SIGNATURE_SLOT_LIT_CUSTOM_START = GENERIC_ROOT_SIGNATURE_SLOT_COUNT_LIT
    };

    struct GENERIC_ROOT_SIGNATURE_UNLIT_CONSTANT_BUFFERS
    {
        D3D12_GPU_VIRTUAL_ADDRESS VSCommonCB;
        D3D12_GPU_VIRTUAL_ADDRESS PSCommonCB;
    };

    struct GENERIC_ROOT_SIGNATURE_LIT_CONSTANT_BUFFERS
    {
		DLIGHT_INFO DlightCounts;

        D3D12_GPU_VIRTUAL_ADDRESS VSCommonCB;
        D3D12_GPU_VIRTUAL_ADDRESS PSCommonCB;
        D3D12_GPU_VIRTUAL_ADDRESS PSDlightArrayCB;
    };

    void InitGenericUnlitRootSignatureSlots(
        _Out_ D3D12_ROOT_PARAMETER* params,
        _In_ SIZE_T size);

    void InitGenericLitRootSignatureSlots(
        _Out_ D3D12_ROOT_PARAMETER* params,
		_In_ SIZE_T size);

	void SetGenericUnlitRootConstants(
		_In_ ID3D12GraphicsCommandList* pList,
		_In_ const GENERIC_ROOT_SIGNATURE_UNLIT_CONSTANT_BUFFERS* pCBs);

	void SetGenericLitRootConstants(
		_In_ ID3D12GraphicsCommandList* pList,
		_In_ const GENERIC_ROOT_SIGNATURE_LIT_CONSTANT_BUFFERS* pCBs);

	template<UINT DescriptorCount> void SetGenericUnlitRootDescriptorTables(
		_In_ ID3D12GraphicsCommandList* pList,
		_In_ const GENERIC_ROOT_SIGNATURE_UNLIT_CONSTANT_BUFFERS* pCBs,
		_In_ const D3D12_GPU_DESCRIPTOR_HANDLE* pHandles)
	{
		SetGenericUnlitRootConstants(pList, pCBs);
		SetGraphicsRootDescriptorTables<GENERIC_ROOT_SIGNATURE_SLOT_UNLIT_CUSTOM_START, DescriptorCount>(pList, pHandles);
	}

	template<UINT DescriptorCount> void SetGenericLitRootDescriptorTables(
		_In_ ID3D12GraphicsCommandList* pList,
		_In_ const GENERIC_ROOT_SIGNATURE_LIT_CONSTANT_BUFFERS* pCBs,
		_In_ const D3D12_GPU_DESCRIPTOR_HANDLE* pHandles)
	{
		SetGenericLitRootConstants(pList, pCBs);
		SetGraphicsRootDescriptorTables<GENERIC_ROOT_SIGNATURE_SLOT_LIT_CUSTOM_START, DescriptorCount>(pList, pHandles);
	}


    class GenericShaders
    {
    private:
        static Shader s_vertexST;
        static Shader s_vertexMT;
        static Shader s_vertexDepth;
        static Shader s_vertexDlight;
        static Shader s_pixelUnlitST;
        static Shader s_pixelST;
        static Shader s_pixelMT;
        static Shader s_pixelDepth;
        static Shader s_pixelDlight;
    public:
        static void Init();
        static void Destroy();

        static Shader* VertexST() { return &s_vertexST; }
        static Shader* VertexMT() { return &s_vertexMT; }
        static Shader* VertexDepth() { return &s_vertexDepth; }
        static Shader* VertexDlight() { return &s_vertexDlight; }
        static Shader* PixelUnlitST() { return &s_pixelUnlitST; }
        static Shader* PixelST() { return &s_pixelST; }
        static Shader* PixelMT() { return &s_pixelMT; }
        static Shader* PixelDepth() { return &s_pixelDepth; }
        static Shader* PixelDlight() { return &s_pixelDlight; }
    };
}
