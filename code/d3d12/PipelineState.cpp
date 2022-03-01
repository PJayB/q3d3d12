#include "pch.h"
#include "PipelineState.h"
#include "BackBufferState.h" // TODO: move?
#include "Frame.h" // TODO: move?
#include "PSODB.h"

using namespace Microsoft::WRL;

namespace QD3D12
{
	static D3D12_BLEND GetSrcBlendConstant(int qConstant);
	static D3D12_BLEND GetDestBlendConstant(int qConstant);
	static D3D12_BLEND GetSrcBlendAlphaConstant(int qConstant);
	static D3D12_BLEND GetDestBlendAlphaConstant(int qConstant);

    cvar_t* PipelineState::dx12_psoWarn = nullptr;

	const PIPELINE_DRAW_STATE_DESC PipelineState::ALL_BITS = {
		GLS_ALLBITS,
		CT_ALL_CULL_BITS,
		TRUE,
		TRUE
	};

    PipelineState::PipelineState() {}
    PipelineState::~PipelineState() {}

	void PipelineState::Init(
        _In_z_ const char* dbname,
		_In_ PIPELINE_STATE_DESC* pPipelineState,
		_In_ PIPELINE_TARGET_STATE_DESC* pTargetState)
	{
        m_db.reset(new PSODB(dbname));
		m_pipelineDesc = *pPipelineState;
		m_targetDesc = *pTargetState;

        LoadCache();
	}

	void PipelineState::Destroy()
	{
		for (auto kvp : m_stateMatrix)
		{
			SAFE_RELEASE(kvp.second);
		}
		m_stateMatrix.clear();
	}

    void PipelineState::LoadCache()
    {
        if (!Device::IsStarted())
        {
            return;
        }

        ID3D12Device* device = Device::Get();

        // Enumerate the cache and see if we can load files
        std::vector<uint64_t> hashes = m_db->EnumerateHashes();
        for (auto hash : hashes)
        {
            // Skip already loaded hashes
            auto i = m_stateMatrix.find(hash);
            if (i != std::end(m_stateMatrix))
                continue;

            CachedPSO* cachedPSO = nullptr;
            if (m_db->LoadCachedBlob(hash, &cachedPSO))
            {
                // Compute the pipeline state
                D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
                ZeroMemory(&desc, sizeof(desc));

                ComputeGraphicsPipelineStateDesc(
                    &m_pipelineDesc,
                    &m_targetDesc,
                    &cachedPSO->Desc,
                    &desc);

                ID3D12PipelineState* pso = nullptr;                
                HRESULT hr = device->CreateGraphicsPipelineState(
                    &desc,
                    __uuidof(ID3D12PipelineState),
                    (void**) &pso);
                if (SUCCEEDED(hr))
                {
                    // Add this PSO to the cache
                    m_stateMatrix[hash] = pso;
                }

                m_db->FreeCachedBlob(cachedPSO);
            }
        }
    }

	ID3D12PipelineState * PipelineState::CreateAndCachePipelineState(
        const char* caller,
        const PIPELINE_DRAW_STATE_DESC * pDrawState,
        UINT64 hash,
        BOOL bLateCache)
	{
        static DWORD totalUncachedPSOCount = 0;
        static LARGE_INTEGER totalTimeSpendCreatingPSOs = { 0, 0 };

        LARGE_INTEGER start, end, freq;
        QueryPerformanceCounter( &start );

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		ComputeGraphicsPipelineStateDesc(
			&m_pipelineDesc,
			&m_targetDesc,
			pDrawState,
			&desc);

        // Let's try and load the cached blob first
        CachedPSO* cachedPSO = nullptr;
        ID3D12PipelineState* pState = nullptr;
        if (m_db->LoadCachedBlob(hash, &cachedPSO))
        {
            if (memcmp(pDrawState, &cachedPSO->Desc, sizeof(*pDrawState)) != 0)
            {
                RI_Error(ERR_FATAL, "The cached PSO %llx doesn't match the cached state.", hash);
            }

            desc.CachedPSO.CachedBlobSizeInBytes = cachedPSO->BlobSize;
            desc.CachedPSO.pCachedBlob = cachedPSO->BlobData;

            HRESULT hr = Device::Get()->CreateGraphicsPipelineState(
                &desc,
                __uuidof(ID3D12PipelineState),
                (void**) &pState);

            switch (hr)
            {
            case S_OK:
                break;
            case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
            case D3D12_ERROR_ADAPTER_NOT_FOUND:
                // Try again without the cache
                desc.CachedPSO.CachedBlobSizeInBytes = 0;
                desc.CachedPSO.pCachedBlob = nullptr;

                COM_ERROR_TRAP(Device::Get()->CreateGraphicsPipelineState(
                    &desc,
                    __uuidof(ID3D12PipelineState),
                    (void**) &pState));
                break;
            default:
                QD3D::D3DError(hr, "PSO Create", __FILE__, __LINE__);
                break;
            }

            m_db->FreeCachedBlob(cachedPSO);
        }
        else
        {
            COM_ERROR_TRAP(Device::Get()->CreateGraphicsPipelineState(
                &desc,
                __uuidof(ID3D12PipelineState),
                (void**) &pState));

            // Cache the PSO
            if (!m_db->CacheBlob(hash, pDrawState, pState))
            {
                Com_Printf("^3WARNING: Failed to cache PSO blob %x\n", hash);
            }
        }

        m_stateMatrix[hash] = pState;

        QueryPerformanceCounter( &end );
        QueryPerformanceFrequency( &freq );

        if (bLateCache || dx12_psoWarn->integer)
        {
            totalUncachedPSOCount++;
            totalTimeSpendCreatingPSOs.QuadPart += end.QuadPart - start.QuadPart;

            if (!bLateCache && dx12_psoWarn->integer) 
            {
                // This was the result of a precache outside of Init()
                Com_Printf("^1ERROR: %s precached a PSO outside of Init zone\n", caller);
            }
            else if (bLateCache && !dx12_psoWarn->integer)
            {
                // This was generated on the fly but inside the Init zone
                Com_Printf("^3WARNING: %s used Get to generate a PSO inside Init zone. Use Precache instead.\n", caller);
            }
            else
            {
                // This was generated on the fly outside of the Init zone
				Com_Printf("^3WARNING: %s generated a PSO during run-time (hash 0x%llx)\n", caller, hash);

				char tmp[512] = { 0 };
				R_GetGLStateMaskString(tmp, sizeof(tmp), pDrawState->StateBits);
				Com_Printf("^3   StateBits: %s\n", tmp);
				Com_Printf("^3   CullMode: 0x%u, PolyOffset: 0x%u, Outline: 0x%u\n",
					pDrawState->CullMode, pDrawState->PolyOffset, pDrawState->Outline);
            }

#ifdef _DEBUG
            char stateCode[2048] = {0};
            R_GetGLStateMaskString(stateCode, _countof(stateCode), pDrawState->StateBits);
            char cullMode[256];
            switch (pDrawState->CullMode)
            {
            case CT_FRONT_SIDED: Com_sprintf(cullMode, _countof(cullMode), "CT_FRONT_SIDED"); break;
            case CT_BACK_SIDED: Com_sprintf(cullMode, _countof(cullMode), "CT_BACK_SIDED"); break;
            case CT_TWO_SIDED: Com_sprintf(cullMode, _countof(cullMode), "CT_TWO_SIDED"); break;
            default: Com_sprintf(cullMode, _countof(cullMode), "0x%X", pDrawState->CullMode); break;
            };

            RI_Printf(PRINT_DEVELOPER, "^5PIPELINE_DRAW_STATE_DESC psoDesc = {\n");
            RI_Printf(PRINT_DEVELOPER, "^5    %s, // StateBits\n", stateCode);
            RI_Printf(PRINT_DEVELOPER, "^5    %s, // CullMode\n", cullMode);
            RI_Printf(PRINT_DEVELOPER, "^5    %d, // PolyOffset\n", pDrawState->PolyOffset);
            RI_Printf(PRINT_DEVELOPER, "^5    %d // Outline\n", pDrawState->Outline);
            RI_Printf(PRINT_DEVELOPER, "^5};\n");
#endif

            double totalS = totalTimeSpendCreatingPSOs.QuadPart / (double) freq.QuadPart;
            Com_Printf("^3WARNING: Total time for all %d uncached PSOs: %dms.\n",
                totalUncachedPSOCount,
                (int) (totalS * 1000));
        }

		return pState;
	}

	ID3D12PipelineState * PipelineState::PrecachePipelineState(
        const char* caller,
        const PIPELINE_DRAW_STATE_DESC * pDrawState)
	{
		auto hash = HashPipelineDrawStateDesc(pDrawState);

		auto i = m_stateMatrix.find(hash);
		if (i != std::end(m_stateMatrix))
			return i->second;

        return CreateAndCachePipelineState(
            caller,
            pDrawState,
            hash,
            FALSE );
	}

	ID3D12PipelineState* PipelineState::GetPipelineState(
        _In_z_ const char* caller,
		_In_ const PIPELINE_DRAW_STATE_DESC* pDrawState)
	{
		auto hash = HashPipelineDrawStateDesc(pDrawState);

		auto i = m_stateMatrix.find(hash);
		if (i != std::end(m_stateMatrix))
			return i->second;

        return CreateAndCachePipelineState(
            caller,
            pDrawState,
            hash,
            TRUE );
	}

	static void ComputeDepthStencilState(
		_Out_ D3D12_DEPTH_STENCIL_DESC* pState,
		_In_ BOOL depthDisable,
		_In_ BOOL depthFuncEqual,
		_In_ BOOL depthMask
		)
	{
		*pState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pState->DepthEnable = !depthDisable;
		pState->DepthWriteMask = depthMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		pState->DepthFunc = depthFuncEqual ? D3D12_COMPARISON_FUNC_EQUAL : D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	static void ComputeRasterizerState(
		_Out_ D3D12_RASTERIZER_DESC* pState, 
		_In_ UINT cullMode,
		_In_ BOOL polyOffset,
		_In_ BOOL outline)
	{
		*pState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		pState->FillMode = outline ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
		
		switch (cullMode)
		{
		case CT_BACK_SIDED:
			pState->CullMode = D3D12_CULL_MODE_BACK;
			break;
		case CT_FRONT_SIDED:
			pState->CullMode = D3D12_CULL_MODE_FRONT;
			break;
		case CT_TWO_SIDED:
			pState->CullMode = D3D12_CULL_MODE_NONE;
			break;
		default:
			assert(0);
		}

		if (polyOffset)
		{
			pState->DepthBias = r_offsetFactor->value;
			pState->SlopeScaledDepthBias = r_offsetUnits->value;
		}

		pState->FrontCounterClockwise = TRUE;
		pState->DepthClipEnable = TRUE;
		pState->DepthBiasClamp = 0;
		// TODO: pState->MultisampleEnable?
		// TODO: pState->AntialiasedLineEnable?
	}

	static void ComputeBlendState(
		_Out_ D3D12_BLEND_DESC* pState,
		_In_ UINT srcBlend,
		_In_ UINT dstBlend)
	{
		*pState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		if ((srcBlend == 0 && dstBlend == 0) ||
			(srcBlend == GLS_SRCBLEND_ONE && dstBlend == GLS_DSTBLEND_ZERO))
		{
			// No blending.
			return;
		}

		pState->RenderTarget[0].BlendEnable = TRUE;

		if (srcBlend == GLS_SRCBLEND_ZERO && dstBlend == GLS_DSTBLEND_ZERO)
			pState->RenderTarget[0].RenderTargetWriteMask = 0;
		else
			pState->RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		pState->RenderTarget[0].SrcBlend = GetSrcBlendConstant(srcBlend);
		pState->RenderTarget[0].DestBlend = GetDestBlendConstant(dstBlend);
		pState->RenderTarget[0].SrcBlendAlpha = GetSrcBlendAlphaConstant(srcBlend);
		pState->RenderTarget[0].DestBlendAlpha = GetDestBlendAlphaConstant(dstBlend);
	}

	void PipelineState::ComputeGraphicsPipelineStateDesc(
		const PIPELINE_STATE_DESC* pPipelineState, 
		const PIPELINE_TARGET_STATE_DESC* pTargetState, 
		const PIPELINE_DRAW_STATE_DESC* pDrawState, 
		D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc)
	{
		// Set up the pipeline desc
		pDesc->pRootSignature		= pPipelineState->pRootSignature;
		pDesc->VS					= pPipelineState->VS;
		pDesc->PS					= pPipelineState->PS;
		pDesc->InputLayout			= pPipelineState->InputLayout;
		pDesc->IBStripCutValue      = pPipelineState->IBStripCutValue;

		// Set up the target desc
		memcpy(pDesc->RTVFormats, pTargetState->RTVFormats, sizeof(pDesc->RTVFormats));
		pDesc->NumRenderTargets		= pTargetState->NumRenderTargets;
		pDesc->DSVFormat			= pTargetState->DSVFormat;
		pDesc->SampleDesc			= pTargetState->SampleDesc;

		// Set up the draw desc
		pDesc->SampleMask = ~0U;
		pDesc->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		ComputeDepthStencilState(
			&pDesc->DepthStencilState,
			(pDrawState->StateBits & GLS_DEPTHTEST_DISABLE),
			(pDrawState->StateBits & GLS_DEPTHFUNC_EQUAL),
			(pDrawState->StateBits & GLS_DEPTHMASK_TRUE));

		ComputeRasterizerState(
			&pDesc->RasterizerState,
			pDrawState->CullMode,
			pDrawState->PolyOffset,
			(pDrawState->StateBits & GLS_POLYMODE_LINE));

		ComputeBlendState(
			&pDesc->BlendState,
			(pDrawState->StateBits & GLS_SRCBLEND_BITS),
			(pDrawState->StateBits & GLS_DSTBLEND_BITS));
	}

    cvar_t* PipelineState::GetPrecacheWarningCvar()
    {
        if (dx12_psoWarn == nullptr)
            dx12_psoWarn = Cvar_Get("dx12_psoWarn", "1", CVAR_ROM );
        return dx12_psoWarn;
    }

    BOOL PipelineState::GetPrecacheWarning()
    {
        return GetPrecacheWarningCvar()->integer != 0;
    }

    void PipelineState::SetPrecacheWarning(BOOL bEnabled)
    {
        GetPrecacheWarningCvar()->integer = bEnabled;
    }


	// TODO: should this be here?
	void InitializeDefaultTargetDesc(
		_Out_ PIPELINE_TARGET_STATE_DESC* pDesc)
	{
		ZeroMemory(pDesc, sizeof(*pDesc));

		BackBufferState* bbState = Frame::GetBackBufferState(0);

		// Get the render target and depth formats
		pDesc->NumRenderTargets = bbState->GetRenderTargetFormats(
			pDesc->RTVFormats,
			1); // TODO: we should fix the number of RTs

		pDesc->DSVFormat = BackBufferState::DEPTH_FORMAT;
		pDesc->SampleDesc = *bbState->SampleDesc();
	}

    // TODO: should we also cache the outline and poly offset modes?
    void CachePipelineStateBits(
        _In_z_ const char* Caller,
        _In_ PipelineState* pPSO,
		_In_ UINT32 StateBits, // GLS_* flags
		_In_ UINT32 CullMode,
        _In_ UINT32 PolyOffset,
        _In_ UINT32 Outline )
    {
        PIPELINE_DRAW_STATE_DESC desc;
        desc.StateBits = StateBits;
        desc.CullMode = CullMode;
        desc.PolyOffset = PolyOffset;
        desc.Outline = Outline;
        pPSO->PrecachePipelineState( Caller, &desc );
    }

    void CacheGenericPipelineStateBits(
        _In_z_ const char* Caller,
        _In_ PipelineState* pPSO,
		_In_ UINT32 StateBits )
    {
        CachePipelineStateBits( Caller, pPSO, StateBits, CT_FRONT_SIDED, FALSE, FALSE );
        CachePipelineStateBits( Caller, pPSO, StateBits, CT_BACK_SIDED, FALSE, FALSE );
        CachePipelineStateBits( Caller, pPSO, StateBits, CT_TWO_SIDED, FALSE, FALSE );
    }

    void CacheShaderStagePipelineState(
        _In_z_ const char* Caller,
        _In_ const shader_t* pShader,
        _In_ const shaderStage_t* pStage,
        _In_ PipelineState* pPSO )
    {
        char tmp[1024];
        sprintf_s( 
            tmp, 
            _countof(tmp), 
            "%s (%s)", 
            pShader->name,
            Caller );

        // If it's single-sided we need to generate both versions because it
        // might end up being mirrored.
        if ( pShader->cullType == CT_TWO_SIDED )
        {
            CachePipelineStateBits( 
                tmp,
                pPSO,
                pStage->stateBits,
                CT_TWO_SIDED,
                pShader->polygonOffset,
                FALSE );
        }
        else
        {
            CachePipelineStateBits( 
                tmp,
                pPSO,
                pStage->stateBits,
                CT_BACK_SIDED,
                pShader->polygonOffset,
                FALSE );
            CachePipelineStateBits( 
                tmp,
                pPSO,
                pStage->stateBits,
                CT_FRONT_SIDED,
                pShader->polygonOffset,
                FALSE );
        }
    }

    void CacheShaderPipelineState(
        _In_z_ const char* Caller,
        _In_ const shader_t* pShader,
        _In_ PipelineState* pPSO )
    {
        for ( int i = 0; i < MAX_SHADER_STAGES; ++i )
        {
            const shaderStage_t* pStage = pShader->stages[i];
            if ( pStage )
            {
                CacheShaderStagePipelineState( Caller, pShader, pStage, pPSO );
            }
        }
    }

	//----------------------------------------------------------------------------
	// Get the blend constants (TODO: move me!)
	//----------------------------------------------------------------------------
	static D3D12_BLEND GetSrcBlendConstant(int qConstant)
	{
		switch (qConstant)
		{
		case GLS_SRCBLEND_ZERO:
			return D3D12_BLEND_ZERO;
		case GLS_SRCBLEND_ONE:
			return D3D12_BLEND_ONE;
		case GLS_SRCBLEND_DST_COLOR:
			return D3D12_BLEND_DEST_COLOR;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			return D3D12_BLEND_INV_DEST_COLOR;
		case GLS_SRCBLEND_SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case GLS_SRCBLEND_DST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			return D3D12_BLEND_SRC_ALPHA_SAT;
		default:
			ASSERT(0);
			return D3D12_BLEND_ONE;
		}
	}

	static D3D12_BLEND GetDestBlendConstant(int qConstant)
	{
		switch (qConstant)
		{
		case GLS_DSTBLEND_ZERO:
			return D3D12_BLEND_ZERO;
		case GLS_DSTBLEND_ONE:
			return D3D12_BLEND_ONE;
		case GLS_DSTBLEND_SRC_COLOR:
			return D3D12_BLEND_SRC_COLOR;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_COLOR;
		case GLS_DSTBLEND_SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case GLS_DSTBLEND_DST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
		default:
			ASSERT(0);
			return D3D12_BLEND_ONE;
		}
	}

	static D3D12_BLEND GetSrcBlendAlphaConstant(int qConstant)
	{
		switch (qConstant)
		{
		case GLS_SRCBLEND_ZERO:
			return D3D12_BLEND_ZERO;
		case GLS_SRCBLEND_ONE:
			return D3D12_BLEND_ONE;
		case GLS_SRCBLEND_DST_COLOR:
			return D3D12_BLEND_DEST_ALPHA;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			return D3D12_BLEND_INV_DEST_ALPHA;
		case GLS_SRCBLEND_SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case GLS_SRCBLEND_DST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			return D3D12_BLEND_SRC_ALPHA_SAT;
		default:
			ASSERT(0);
			return D3D12_BLEND_ONE;
		}
	}

	static D3D12_BLEND GetDestBlendAlphaConstant(int qConstant)
	{
		switch (qConstant)
		{
		case GLS_DSTBLEND_ZERO:
			return D3D12_BLEND_ZERO;
		case GLS_DSTBLEND_ONE:
			return D3D12_BLEND_ONE;
		case GLS_DSTBLEND_SRC_COLOR:
			return D3D12_BLEND_SRC_ALPHA;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case GLS_DSTBLEND_SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case GLS_DSTBLEND_DST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
		default:
			ASSERT(0);
			return D3D12_BLEND_ONE;
		}
	}

}
