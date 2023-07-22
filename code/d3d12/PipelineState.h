#pragma once

#include <map>
#include <memory>
#include "D3D12Core.h"

namespace QD3D12
{
	/*
		Pipeline state consists of three types of data:

		System configuration constants, fetched from the back buffer state.
			UINT NumRenderTargets;
			DXGI_FORMAT RTVFormats[ 8 ];
			DXGI_FORMAT DSVFormat;
			DXGI_SAMPLE_DESC SampleDesc;

		Pipeline specific, set on creation time.
			ID3D12RootSignature *pRootSignature;
			D3D12_SHADER_BYTECODE VS;
			D3D12_SHADER_BYTECODE PS;
			D3D12_SHADER_BYTECODE DS; (NOT USED)
			D3D12_SHADER_BYTECODE HS; (NOT USED)
			D3D12_SHADER_BYTECODE GS; (NOT USED)
			D3D12_STREAM_OUTPUT_DESC StreamOutput; (NOT USED)
			D3D12_INPUT_LAYOUT_DESC InputLayout;
			D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;

		Draw call specific, set for each draw.
			D3D12_BLEND_DESC BlendState;
			UINT SampleMask; (NOT USED)
			D3D12_RASTERIZER_DESC RasterizerState;
			D3D12_DEPTH_STENCIL_DESC DepthStencilState;
			D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
	*/

	struct PIPELINE_STATE_DESC
	{
		ID3D12RootSignature *pRootSignature;
		D3D12_SHADER_BYTECODE VS;
		D3D12_SHADER_BYTECODE PS;
		D3D12_INPUT_LAYOUT_DESC InputLayout;
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
	};

	struct PIPELINE_TARGET_STATE_DESC
	{
		UINT NumRenderTargets;
		DXGI_FORMAT RTVFormats[8];
		DXGI_FORMAT DSVFormat;
		DXGI_SAMPLE_DESC SampleDesc;
	};

	struct PIPELINE_DRAW_STATE_DESC
	{
		UINT32 StateBits; // GLS_* flags
		UINT32 CullMode : 2;
		UINT32 PolyOffset : 1;
		UINT32 Outline : 1;
	};

#if _MSC_VER > 1800
	static_assert(
		sizeof(PIPELINE_TARGET_STATE_DESC::RTVFormats) ==
		sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC::RTVFormats),
		"Ensure that the RTV formats in PIPELINE_TARGET_STATE_DESC match.");
#endif

    class PSODB;

	class PipelineState
	{
		// --- INSTANCE MEMBERS ---
	private:
		PIPELINE_STATE_DESC m_pipelineDesc;
		PIPELINE_TARGET_STATE_DESC m_targetDesc;
		std::map<UINT64, ID3D12PipelineState*> m_stateMatrix;
        std::unique_ptr<PSODB> m_db;

        ID3D12PipelineState* CreateAndCachePipelineState(
            _In_z_ const char* caller,
            _In_ const PIPELINE_DRAW_STATE_DESC * pDrawState,
            _In_ UINT64 hash,
            _In_ BOOL bLateCache);

        void LoadCache();

	public:
        PipelineState();
        ~PipelineState();

		void Init(
            _In_z_ const char* pDatabaseName,
            _In_ PIPELINE_STATE_DESC* pPipelineState,
			_In_ PIPELINE_TARGET_STATE_DESC* pTargetState);

		void Destroy();

		ID3D12PipelineState* PrecachePipelineState(
            _In_z_ const char* caller,
			_In_ const PIPELINE_DRAW_STATE_DESC* pDrawState);

		ID3D12PipelineState* GetPipelineState(
            _In_z_ const char* caller,
			_In_ const PIPELINE_DRAW_STATE_DESC* pDrawState);

		//ID3D12PipelineState* GetPipelineState(
		//	_In_ const PIPELINE_DRAW_STATE_DESC* pDrawState) const
		//{
		//	auto i = m_stateMatrix.find(
		//		HashPipelineDrawStateDesc(
		//			pDrawState));
		//	assert(i != std::end(m_stateMatrix));
		//	return i->second;
		//}

		// --- STATICS ---
    private:
        static cvar_t* dx12_psoWarn;

		static inline UINT64 HashPipelineDrawStateDesc(
			_In_ const PIPELINE_DRAW_STATE_DESC* pDesc)
		{
			static_assert(sizeof(PIPELINE_DRAW_STATE_DESC) == sizeof(UINT64), "Check pipeline draw state desc");
			return *(reinterpret_cast<const UINT64*>(pDesc));
		}

		static void ComputeGraphicsPipelineStateDesc(
			_In_ const PIPELINE_STATE_DESC* pPipelineState,
			_In_ const PIPELINE_TARGET_STATE_DESC* pTargetState,
			_In_ const PIPELINE_DRAW_STATE_DESC* pDrawState,
			_Out_ D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc);

        static cvar_t* GetPrecacheWarningCvar();

	public:
		// All possible combinations are represented by these bits
		static const PIPELINE_DRAW_STATE_DESC ALL_BITS;

        static BOOL GetPrecacheWarning();
        static void SetPrecacheWarning(BOOL bEnabled);
	};

	void InitializeDefaultTargetDesc(
		_Out_ PIPELINE_TARGET_STATE_DESC* pDesc);

    void CachePipelineStateBits(
        _In_z_ const char* Caller,
        _In_ PipelineState* pPSO,
		_In_ UINT32 StateBits, // GLS_* flags
		_In_ UINT32 CullMode,
        _In_ UINT32 PolyOffset,
        _In_ UINT32 Outline );

    void CacheGenericPipelineStateBits(
        _In_z_ const char* Caller,
        _In_ PipelineState* pPSO,
		_In_ UINT32 StateBits );

    void CacheShaderPipelineState(
        _In_z_ const char* Caller,
        _In_ const shader_t* pShader,
        _In_ PipelineState* pPSO );

    void CacheShaderStagePipelineState(
        _In_z_ const char* Caller,
        _In_ const shader_t* pShader,
        _In_ const shaderStage_t* pStage,
        _In_ PipelineState* pPSO );
}
