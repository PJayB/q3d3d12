#include "GenericDrawStageDepth.h"
#include "RootSignature.h"
#include "Image.h"
#include "DrawStages.h"

namespace QD3D12
{
	const D3D12_INPUT_ELEMENT_DESC GenericDepthDrawStage::INPUT_LAYOUT[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	void GenericDepthDrawStage::Init()
	{
		// Root signature
		CD3DX12_DESCRIPTOR_RANGE ranges[2];
		// TODO: make these constants
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);   // Pixel shader reads one SRV.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);   // Pixel shader reads one Sampler.

		CD3DX12_ROOT_PARAMETER params[GD_SLOT_COUNT];
		InitGenericUnlitRootSignatureSlots(params, sizeof(params));
		params[GD_SRV_SLOT].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		params[GD_SAMPLER_SLOT].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		m_pRSig = RootSignature::Create(&rsig);
		
		// Pipeline state
		PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(psoDesc));
		psoDesc.pRootSignature = m_pRSig;
		psoDesc.VS = GenericShaders::VertexDepth()->ByteCode();
		psoDesc.PS = GenericShaders::PixelDepth()->ByteCode();
		psoDesc.InputLayout.pInputElementDescs = INPUT_LAYOUT;
		psoDesc.InputLayout.NumElements = _countof(INPUT_LAYOUT);
		PIPELINE_TARGET_STATE_DESC targetDesc;
		InitializeDefaultTargetDesc(&targetDesc);
		m_pso.Init(
            "GenericDrawStageDepth",
			&psoDesc,
			&targetDesc);
	}

	void GenericDepthDrawStage::Destroy()
	{
		m_pso.Destroy();
		SAFE_RELEASE(m_pRSig);
	}

	void GenericDepthDrawStage::Draw(
		ID3D12GraphicsCommandList* cmdList,
		const Image* tex,
		const GENERIC_DEPTH_DRAW_DESC* desc)
	{
		assert(tex);

		ID3D12DescriptorHeap* pDescriptorHeaps[] = {
			Image::ShaderVisibleResourceHeap(),
			Image::ShaderVisibleSamplerHeap(),
		};

		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandles[] = {
			tex->ShaderVisibleSRV(),
			tex->ShaderVisibleSampler()
		};

		cmdList->SetGraphicsRootSignature(m_pRSig);
		cmdList->SetPipelineState(m_pso.GetPipelineState(__FUNCTION__, &desc->DrawStateDesc));
		cmdList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

        SetTessArbVertexBuffers<1>(
            cmdList,
            &desc->TexCoordVertexBufferView);

		SetGenericUnlitRootDescriptorTables<
			_countof(gpuDescriptorHandles)>(
			cmdList,
			&desc->CommonCBs,
			gpuDescriptorHandles);

		cmdList->DrawIndexedInstanced(
			desc->NumIndices, 
			1, // instance count 
			0, // start index location
			0, // base vertex location
			0); // start index location
	}

    void GenericDepthDrawStage::CacheShaderPipelineState( const shader_t* shader )
    {
        // Sky never writes to depth
        if ( shader->isSky )
            return;

        for ( int i = 0; i < MAX_SHADER_STAGES; ++i )
        {
            const shaderStage_t* stage = shader->stages[i];
            if ( stage && ( stage->stateBits & GLS_DEPTHMASK_TRUE ) )
            {
                CacheShaderStagePipelineState( __FUNCTION__, shader, stage, &m_pso );
            }
        }
    }
}
