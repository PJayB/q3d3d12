#pragma once
#include "UploadScratch.h"
#include "TessBuffers.h"
#include "DlightBuffers.h"
#include "DrawStages.h"
#include "GenericDrawStageST.h"
#include "GenericDrawStageMT.h"
#include "GenericDrawStageUnlitST.h"
#include "GenericDrawStageDepth.h"
#include "GenericDrawStageDlight.h"

namespace QD3D12 
{
	class GenericDrawStage
	{
	private:
		TessBufferBindings m_bindings;
		DlightBufferBindings m_dlights;
		DlightTessBufferBindings m_dlightBindings;

		void IterateStages(
			ID3D12GraphicsCommandList* cmdList,
			const shaderCommands_t* input);

        void AdditiveDlights(
            ID3D12GraphicsCommandList* cmdList,
            const shaderCommands_t* input);

        void Fog(
            ID3D12GraphicsCommandList* cmdList,
            const shaderCommands_t* input);

	public:
		GenericDrawStage();

		void Draw(
			ID3D12GraphicsCommandList* cmdList,
			const shaderCommands_t* input);

        void DrawSky(
            ID3D12GraphicsCommandList* cmdList,
            const shaderCommands_t* input);

        void DrawDepth(
            ID3D12GraphicsCommandList* cmdList,
            const shaderCommands_t* input);

	private:
        static GenericUnlitSingleTextureDrawStage s_unlitst;
        static GenericSingleTextureDrawStage s_st;
        static GenericMultiTextureDrawStage s_mt;
        static GenericDepthDrawStage s_depth;
        static GenericDlightDrawStage s_dlight;

	public:
		static void Init();
		static void Destroy();

        static void CacheShaderPipelineState(
            const shader_t* shader );
	};
}
