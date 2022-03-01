#pragma once
#include "GenericShaders.h"

namespace QD3D12 
{
    struct GENERIC_DLIGHT_DRAW_DESC
    {
        GENERIC_ROOT_SIGNATURE_LIT_CONSTANT_BUFFERS CommonCBs;
        PIPELINE_DRAW_STATE_DESC DrawStateDesc;
        UINT NumIndices;
    };

	class GenericDlightDrawStage
	{
	private:
		static const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[];

		ID3D12RootSignature* m_pRSig;
		PipelineState m_pso;

	public:
		void Init();
		void Destroy();

		void Draw(
            _In_ ID3D12GraphicsCommandList* cmdList,
            _In_ const GENERIC_DLIGHT_DRAW_DESC* desc);

        void CacheShaderPipelineState(
            _In_ const shader_t* shader );
	};
}
