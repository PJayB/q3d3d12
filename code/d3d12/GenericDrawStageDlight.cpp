#include "pch.h"
#include "GenericDrawStageDlight.h"
#include "RootSignature.h"
#include "Image.h"
#include "DrawStages.h"

namespace QD3D12
{
	const D3D12_INPUT_ELEMENT_DESC GenericDlightDrawStage::INPUT_LAYOUT[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	void GenericDlightDrawStage::Init()
	{
		D3D12_ROOT_PARAMETER params[GENERIC_ROOT_SIGNATURE_SLOT_COUNT_LIT];
		InitGenericLitRootSignatureSlots(params, sizeof(params));

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		m_pRSig = RootSignature::Create(&rsig);
		
		// Pipeline state
		PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(psoDesc));
		psoDesc.pRootSignature = m_pRSig;
		psoDesc.VS = GenericShaders::VertexDlight()->ByteCode();
		psoDesc.PS = GenericShaders::PixelDlight()->ByteCode();
		psoDesc.InputLayout.pInputElementDescs = INPUT_LAYOUT;
		psoDesc.InputLayout.NumElements = _countof(INPUT_LAYOUT);
		PIPELINE_TARGET_STATE_DESC targetDesc;
		InitializeDefaultTargetDesc(&targetDesc);
		m_pso.Init(
            "GenericDrawStageDlight",
			&psoDesc,
			&targetDesc);
	}

	void GenericDlightDrawStage::Destroy()
	{
		m_pso.Destroy();
		SAFE_RELEASE(m_pRSig);
	}

	void GenericDlightDrawStage::Draw(
		ID3D12GraphicsCommandList* cmdList,
		const GENERIC_DLIGHT_DRAW_DESC* desc)
	{
		cmdList->SetGraphicsRootSignature(m_pRSig);
		cmdList->SetPipelineState(m_pso.GetPipelineState(__FUNCTION__, &desc->DrawStateDesc));

        SetGenericLitRootConstants(
            cmdList, 
            &desc->CommonCBs);
		
		cmdList->DrawIndexedInstanced(
			desc->NumIndices, 
			1, // instance count 
			0, // start index location
			0, // base vertex location
			0); // start index location
	}

    void GenericDlightDrawStage::CacheShaderPipelineState( const shader_t* shader )
    {
        if ( ( shader->sort <= SS_OPAQUE ) && 
             !( shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) )
        {
            QD3D12::CacheShaderPipelineState( __FUNCTION__, shader, &m_pso );
        }
    }
}
