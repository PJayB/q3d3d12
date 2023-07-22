#include "GenericDrawStageUnlitST.h"
#include "RootSignature.h"
#include "Image.h"
#include "DrawStages.h"

namespace QD3D12
{
	const D3D12_INPUT_ELEMENT_DESC GenericUnlitSingleTextureDrawStage::INPUT_LAYOUT[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	void GenericUnlitSingleTextureDrawStage::Init()
	{
		// Root signature
		CD3DX12_DESCRIPTOR_RANGE ranges[2];
		// TODO: make these constants
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);   // Pixel shader reads one SRV.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);   // Pixel shader reads one Sampler.

		CD3DX12_ROOT_PARAMETER params[GUST_SLOT_COUNT];
		InitGenericUnlitRootSignatureSlots(params, sizeof(params));
		params[GUST_SRV_SLOT].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		params[GUST_SAMPLER_SLOT].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		m_pRSig = RootSignature::Create(&rsig);
		
		// Pipeline state
		PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(psoDesc));
		psoDesc.pRootSignature = m_pRSig;
		psoDesc.VS = GenericShaders::VertexST()->ByteCode();
		psoDesc.PS = GenericShaders::PixelUnlitST()->ByteCode();
		psoDesc.InputLayout.pInputElementDescs = INPUT_LAYOUT;
		psoDesc.InputLayout.NumElements = _countof(INPUT_LAYOUT);
		PIPELINE_TARGET_STATE_DESC targetDesc;
		InitializeDefaultTargetDesc(&targetDesc);
		m_pso.Init(
            "GenericDrawStageUnlitST",
			&psoDesc,
			&targetDesc);

        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, 0 );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_DEFAULT );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA|GLS_DEPTHFUNC_EQUAL );
	}

	void GenericUnlitSingleTextureDrawStage::Destroy()
	{
		m_pso.Destroy();
		SAFE_RELEASE(m_pRSig);
	}

	void GenericUnlitSingleTextureDrawStage::Draw(
		ID3D12GraphicsCommandList* cmdList,
		const Image* tex,
		const GENERIC_UNLIT_SINGLE_TEXTURE_DRAW_DESC* desc)
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

		SetTessArbVertexBuffers<_countof(desc->VertexBufferViews.Array)>(
			cmdList,
			desc->VertexBufferViews.Array);

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

    void GenericUnlitSingleTextureDrawStage::CacheShaderPipelineState( const shader_t* shader )
    {
        if ( shader->fogPass != 0 || shader->isSky )
        {
            for ( int i = 0; i < MAX_SHADER_STAGES; ++i )
            {
                const shaderStage_t* stage = shader->stages[i];
                if ( stage )
                {
                    CacheShaderStagePipelineState( __FUNCTION__, shader, stage, &m_pso );
                }
            }
        }
    }
}
