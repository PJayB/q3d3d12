#include "pch.h"
#include "GenericDrawStageMT.h"
#include "RootSignature.h"
#include "Image.h"
#include "DrawStages.h"
#include "Frame.h"
#include "DescriptorBatch.h"

namespace QD3D12
{
	const D3D12_INPUT_ELEMENT_DESC GenericMultiTextureDrawStage::INPUT_LAYOUT[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	void GenericMultiTextureDrawStage::Init()
	{
		// Root signature
		CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);   // Pixel shader reads two SRVs.
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);   // Pixel shader reads one Sampler.

        CD3DX12_ROOT_PARAMETER params[GMT_SLOT_COUNT];
		InitGenericLitRootSignatureSlots(params, sizeof(params));
        params[GMT_SRV_SLOT].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        params[GMT_SAMPLER_SLOT].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		m_pRSig = RootSignature::Create(&rsig);
		
		// Pipeline state
		PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(psoDesc));
		psoDesc.pRootSignature = m_pRSig;
		psoDesc.VS = GenericShaders::VertexMT()->ByteCode();
		psoDesc.PS = GenericShaders::PixelMT()->ByteCode();
		psoDesc.InputLayout.pInputElementDescs = INPUT_LAYOUT;
		psoDesc.InputLayout.NumElements = _countof(INPUT_LAYOUT);
		PIPELINE_TARGET_STATE_DESC targetDesc;
		InitializeDefaultTargetDesc(&targetDesc);
		m_pso.Init(
            "GenericDrawStageMT",
			&psoDesc,
			&targetDesc);

        // Cache common states
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, 0 );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_DEFAULT );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_DEPTHMASK_TRUE );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_ATEST_GE_80|GLS_DEPTHMASK_TRUE );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_ONE|GLS_DSTBLEND_SRC_ALPHA );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_ZERO|GLS_DSTBLEND_SRC_ALPHA );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_DST_COLOR|GLS_DSTBLEND_ZERO );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_DST_COLOR|GLS_DSTBLEND_SRC_COLOR );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_DST_COLOR|GLS_DSTBLEND_SRC_ALPHA );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_ZERO|GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA|GLS_DSTBLEND_SRC_ALPHA );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_DST_COLOR|GLS_DSTBLEND_ZERO|GLS_DEPTHFUNC_EQUAL );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA|GLS_DEPTHMASK_TRUE );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA|GLS_DEPTHFUNC_EQUAL );
        CacheGenericPipelineStateBits( __FUNCTION__, &m_pso, GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA|GLS_DEPTHTEST_DISABLE );
	}

	void GenericMultiTextureDrawStage::Destroy()
	{
		m_pso.Destroy();
		SAFE_RELEASE(m_pRSig);
	}
    
    struct GENERIC_MULTI_TEXTURE_GPU_DESCRIPTOR_HANDLES
    {
        union {
            D3D12_GPU_DESCRIPTOR_HANDLE Array[4];
            struct {
                D3D12_GPU_DESCRIPTOR_HANDLE ShaderResource0;
                D3D12_GPU_DESCRIPTOR_HANDLE ShaderResource1;
                D3D12_GPU_DESCRIPTOR_HANDLE Sampler0;
                D3D12_GPU_DESCRIPTOR_HANDLE Sampler1;
            };
        };
    };

    static void GatherGpuDescriptorHandlesForMultiTexture(
        DescriptorBatch* pSRVDescriptorBatch,
        DescriptorBatch* pSamplerDescriptorBatch,
        const Image* tex0,
        const Image* tex1,
        D3D12_GPU_DESCRIPTOR_HANDLE* pHandles)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE srvSourceHandles[] = { 
            tex0->CopySourceSRV(), 
            tex1->CopySourceSRV()
        };

        D3D12_CPU_DESCRIPTOR_HANDLE samplerSourceHandles[] = {
            tex0->CopySourceSampler(), 
            tex1->CopySourceSampler()
        };

        UINT handleCounts[] = { 
            1, 1
        };

        pHandles[0] = pSRVDescriptorBatch->WriteDescriptors(srvSourceHandles, handleCounts, _countof(srvSourceHandles), _countof(srvSourceHandles));
        pHandles[1] = pSamplerDescriptorBatch->WriteDescriptors(samplerSourceHandles, handleCounts, _countof(samplerSourceHandles), _countof(samplerSourceHandles));
    }
    
	void GenericMultiTextureDrawStage::Draw(
		ID3D12GraphicsCommandList* cmdList,
        const Image* tex0,
        const Image* tex1,
		const GENERIC_MULTI_TEXTURE_DRAW_DESC* desc)
	{
        assert(tex0);
        assert(tex1);

        DescriptorBatch* pSRVDescriptorBatch = Frame::CurrentDescriptorBatch(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        DescriptorBatch* pSamplerDescriptorBatch = Frame::CurrentDescriptorBatch(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		ID3D12DescriptorHeap* pDescriptorHeaps[] = {
            pSRVDescriptorBatch->Heap(),
            pSamplerDescriptorBatch->Heap()
		};

        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandles[2];
        GatherGpuDescriptorHandlesForMultiTexture(
            pSRVDescriptorBatch, pSamplerDescriptorBatch,
            tex0, tex1, 
            gpuDescriptorHandles);

		cmdList->SetGraphicsRootSignature(m_pRSig);
		cmdList->SetPipelineState(m_pso.GetPipelineState(__FUNCTION__, &desc->DrawStateDesc));
		cmdList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

		SetTessArbVertexBuffers<_countof(desc->VertexBufferViews.Array)>(
			cmdList,
			desc->VertexBufferViews.Array);

        // Bind resources & descriptors
		SetGenericLitRootDescriptorTables<
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

    void GenericMultiTextureDrawStage::CacheShaderPipelineState( const shader_t* shader )
    {
        for ( int i = 0; i < MAX_SHADER_STAGES; ++i )
        {
            const shaderStage_t* stage = shader->stages[i];
            if ( stage && ( stage->bundle[1].image[0] != nullptr ) )
            {
                CacheShaderStagePipelineState( __FUNCTION__, shader, stage, &m_pso );
            }
        }
    }
}
