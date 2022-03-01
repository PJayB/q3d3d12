#pragma once
#include "GenericShaders.h"
#include "PipelineState.h"

namespace QD3D12 
{
    class Image;

    struct GENERIC_DEPTH_DRAW_DESC
	{
		D3D12_VERTEX_BUFFER_VIEW TexCoordVertexBufferView;
		GENERIC_ROOT_SIGNATURE_UNLIT_CONSTANT_BUFFERS CommonCBs;

		PIPELINE_DRAW_STATE_DESC DrawStateDesc;
		UINT NumIndices;
	};

	class GenericDepthDrawStage
	{
	private:
		static const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[];

		ID3D12RootSignature* m_pRSig;
		PipelineState m_pso;

		enum RS_CUSTOM_SLOT
		{
			GD_START_SLOT = GENERIC_ROOT_SIGNATURE_SLOT_UNLIT_CUSTOM_START,
			GD_SRV_SLOT = GD_START_SLOT,
			GD_SAMPLER_SLOT,
			GD_SLOT_COUNT
		};

	public:
		void Init();
		void Destroy();

		void Draw(
            _In_ ID3D12GraphicsCommandList* cmdList,
            _In_ const Image* tex,
            _In_ const GENERIC_DEPTH_DRAW_DESC* desc);

        void CacheShaderPipelineState(
            _In_ const shader_t* shader );
	};
}
