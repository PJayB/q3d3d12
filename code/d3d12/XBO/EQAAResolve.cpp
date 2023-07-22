#include "EQAAResolve.h"
#include "RootSignature.h"
#include "Frame.h"

#ifndef _XBOX_ONE
#   error EQAAResolve is only supported on Xbox One
#endif

namespace QD3D12
{
    Shader EQAAResolve::s_shader;
    ID3D12PipelineState* EQAAResolve::s_pso = nullptr;
    ID3D12RootSignature* EQAAResolve::s_rsig = nullptr;

    struct ConstantBufferFMask
    {
        BOOL COLOR_EXPANDED;
        UINT LOG_NUM_FRAGMENTS;
        UINT LOG_NUM_SAMPLES;
        UINT NUM_FRAGMENTS;
        UINT NUM_SAMPLES;
        UINT INDEX_BITS;
        UINT INDEX_MASK;
        UINT VALID_INDEX_MASK;
        UINT INVALID_INDEX_MASK;
    };

    static const size_t c_NumFMaskConstantDwords = (sizeof(ConstantBufferFMask) + 3) / sizeof(uint32_t);

    static UINT CalcLogFragmentFromSampleCount(UINT sampleCount)
    {
        switch (sampleCount)
        {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
        case 16: return 4;
        default: assert(sampleCount < 16); return 0;
        }
    }
    
    static ConstantBufferFMask CalcFMaskParams(UINT sampleCount, UINT quality, BOOL useNativeFMask)
    {
        static const UINT c_iMaxLogFragments = 4;
        static const UINT c_MaxQuality = 5;   // 16? Upper bound for the number of quality levels we will consider
        assert(quality < c_MaxQuality);

        // Hard-coded table
        static const UINT c_LogSamples[c_iMaxLogFragments][c_MaxQuality] =
        {
            {0, 1, 2, 3, 4},
            {1, 1, 2, 3, 4},
            {2, 2, 2, 3, 4},
            {3, 3, 3, 3, 4},
        };

        // Work out the log2 of sampleCount
        UINT logFragments = CalcLogFragmentFromSampleCount(sampleCount);
        assert(logFragments < c_iMaxLogFragments);

        UINT logSamples = c_LogSamples[logFragments][quality];

        // EQAA has "unknown" as an extra code
        UINT bitsPerSample = (logSamples == logFragments) ? logFragments : (logFragments + 1);
        bitsPerSample = (bitsPerSample == 3) ? 4 : bitsPerSample;  // hardware rounds up to power-of-two
        bitsPerSample = useNativeFMask ? 4 : bitsPerSample;
        assert(bitsPerSample <= 4);

        UINT indexMask = useNativeFMask ? 0xf : ((1 << bitsPerSample) - 1);
        UINT validIndexMask = (1 << logFragments) - 1;
        UINT invalidIndexMask = indexMask & ~validIndexMask;

        ConstantBufferFMask constantBufferFMask =
        {
            quality == 0,                   // BOOL COLOR_EXPANDED;
            logFragments,                   // UINT LOG_NUM_FRAGMENTS;
            logSamples,                     // UINT LOG_NUM_SAMPLES;
            1U << logFragments,             // UINT NUM_FRAGMENTS;
            1U << logSamples,               // UINT NUM_SAMPLES;
            bitsPerSample,                  // UINT INDEX_BITS;
            indexMask,                      // UINT INDEX_MASK;
            validIndexMask,                 // UINT VALID_INDEX_MASK;
            invalidIndexMask,               // UINT INVALID_INDEX_MASK;
        };

        return constantBufferFMask;
    }

    static DXGI_FORMAT CalcFMaskRawFormat(const ConstantBufferFMask& constantBufferFMask)
    {
        UINT bitsPerPixel = constantBufferFMask.INDEX_BITS * constantBufferFMask.NUM_SAMPLES;

        assert(bitsPerPixel > 0 && bitsPerPixel <= 64);

        if (bitsPerPixel <= 8)
        {
            return DXGI_FORMAT_R8_UINT;
        }
        else if (bitsPerPixel <= 16)
        {
            return DXGI_FORMAT_R16_UINT;
        }
        else if (bitsPerPixel <= 32)
        {
            return DXGI_FORMAT_R32_UINT;
        }
        else //if( iBitsPerPixel <= 64 )
        {
            return DXGI_FORMAT_R32G32_UINT;
        }
    }

    void EQAAResolve::Init()
    {
        s_shader.Init("eqaa_resolve_cs");

		CD3DX12_DESCRIPTOR_RANGE dt[2];
		dt[0].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0 );
		dt[1].Init( D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0 );


		CD3DX12_ROOT_PARAMETER params[2];
		params[0].InitAsDescriptorTable(_countof(dt), dt, D3D12_SHADER_VISIBILITY_ALL);
        params[1].InitAsConstants(c_NumFMaskConstantDwords, 0);

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        s_rsig = RootSignature::Create( &rsig );

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
        ZeroMemory( &psoDesc, sizeof(psoDesc) );
        psoDesc.CS = s_shader.ByteCode();
        psoDesc.NodeMask = QD3D12_NODE_MASK;
        psoDesc.pRootSignature = s_rsig;

        COM_ERROR_TRAP( Device::Get()->CreateComputePipelineState(
            &psoDesc,
            __uuidof(ID3D12PipelineState),
            (void**) &s_pso ) );
    }

    void EQAAResolve::Destroy()
    {
        s_shader.Destroy();

        SAFE_RELEASE( s_rsig );
        SAFE_RELEASE( s_pso );
    }

    void EQAAResolve::Apply(
        ID3D12GraphicsCommandList* pCmdList,
        ID3D12Resource* pSource,
        ID3D12Resource* pFMask,
        ID3D12Resource* pTarget )
    {
		// TODO: remove all of this
        D3D12_RESOURCE_DESC srcDesc = pSource->GetDesc();
        D3D12_RESOURCE_DESC dstDesc = pTarget->GetDesc();

		DescriptorBatch* pDescriptors = Frame::CurrentDescriptorBatch(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DESCRIPTOR_HANDLE_TUPLE hDescriptors = pDescriptors->AllocateDescriptors(3);

		ID3D12Device* pDevice = Device::Get();

		UINT increment = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        ConstantBufferFMask constantBufferFMask = CalcFMaskParams(srcDesc.SampleDesc.Count, srcDesc.SampleDesc.Quality, FALSE);

        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuHandle(hDescriptors.Cpu);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof(srvDesc) );
		srvDesc.Format = srcDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		pDevice->CreateShaderResourceView(pSource, &srvDesc, hCpuHandle);
        hCpuHandle.Offset(increment);

        srvDesc.Format = CalcFMaskRawFormat(constantBufferFMask);
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        pDevice->CreateShaderResourceView(pFMask, &srvDesc, hCpuHandle);
        hCpuHandle.Offset(increment);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory( &uavDesc, sizeof(uavDesc) );
		uavDesc.Format = dstDesc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		pDevice->CreateUnorderedAccessView(pTarget, nullptr, &uavDesc, hCpuHandle);
        hCpuHandle.Offset(increment);

        pCmdList->SetPipelineState( s_pso );
        pCmdList->SetComputeRootSignature( s_rsig );
        pCmdList->SetComputeRootDescriptorTable( 0, hDescriptors.Gpu );
        pCmdList->SetComputeRoot32BitConstants( 1, c_NumFMaskConstantDwords, &constantBufferFMask, 0 );
        pCmdList->Dispatch( dstDesc.Width / 8, dstDesc.Height / 8, 1 );
    }
}
