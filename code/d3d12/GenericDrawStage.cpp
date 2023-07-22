#include "GenericDrawStage.h"
#include "RenderState.h"
#include "ConstantBuffer.h"
#include "Frame.h"
#include "Image.h"

namespace QD3D12
{
    GenericUnlitSingleTextureDrawStage GenericDrawStage::s_unlitst;
    GenericSingleTextureDrawStage GenericDrawStage::s_st;
    GenericMultiTextureDrawStage GenericDrawStage::s_mt;
    GenericDepthDrawStage GenericDrawStage::s_depth;
    GenericDlightDrawStage GenericDrawStage::s_dlight;

    static const Image* GetAnimatedImage(
        const textureBundle_t *bundle,
        float shaderTime) 
    {
        if (!r_uiFullScreen->integer && r_showlighting->integer) {
            return Image::Get(tr.whiteImage->index);
        }
        else {
            return Image::GetAnimatedImage(bundle, shaderTime);
        }
    }

	GenericDrawStage::GenericDrawStage()
	{
	}
    
	void GenericDrawStage::IterateStages(
		ID3D12GraphicsCommandList* cmdList,
		const shaderCommands_t* input)
	{
		// Copy the global pixel shader state
		VIEW_CONSTANTS_PS psConstants = *RenderState::PSConstants();
		
		// Create a placeholder draw state
		PIPELINE_DRAW_STATE_DESC state;
		state.StateBits = RenderState::GetStateBits();
		state.Outline = FALSE;

		// Prime the ps constants
		ConstantBuffer psCB;
		psCB.Write(&psConstants, sizeof(psConstants));

		// Update vertex shader view constants
		// TODO: only do this if they're dirty
		GENERIC_ROOT_SIGNATURE_LIT_CONSTANT_BUFFERS cbLocs;
		ZeroMemory(&cbLocs, sizeof(cbLocs));

		cbLocs.VSCommonCB = RenderState::UpdatedVSConstants();
		cbLocs.PSCommonCB = psCB.Address();
		cbLocs.PSDlightArrayCB = m_dlights.ModLights;
        cbLocs.DlightCounts = m_dlights.Counts;

		// Iterate over each stage
		for (int stage = 0; stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t *pStage = input->xstages[stage];

			if (!pStage)
			{
				break;
			}

			UINT stateBits = pStage->stateBits;

			// Disable depth writes and set depth compare equal if we're in color-only mode
			// (depth test bit is not affected)
			if ( input->allowPreZ && !RB_IsDepthPass() && !StageSkipsPreZ( pStage ) )
			{
				stateBits = (stateBits & ~GLS_DEPTHMASK_TRUE) | GLS_DEPTHFUNC_EQUAL;
			}

			// Upload the PS constants if they're different
			// TODO: only do this if they're dirty
			if (RenderState::GetPixelShaderViewDataFromState(
				stateBits,
				state.StateBits,
				&psConstants))
			{
				cbLocs.PSCommonCB = psCB.Write(
					&psConstants,
					sizeof(psConstants));
			}

			// Configure the draw state
			state.StateBits = stateBits;
			state.CullMode = RenderState::GetCullMode(input->shader->cullType);
			state.PolyOffset = input->shader->polygonOffset;

			//
			// draw single or multi-textured
			//
			if (pStage->bundle[1].image[0] != nullptr && r_lightmap->integer == 0)
			{
                GENERIC_MULTI_TEXTURE_DRAW_DESC drawDesc;
				drawDesc.NumIndices = input->numIndexes;
				drawDesc.DrawStateDesc = state;
                drawDesc.VertexBufferViews.TexCoord0 = m_bindings.Stages[stage].TexCoords[0];
                drawDesc.VertexBufferViews.TexCoord1 = m_bindings.Stages[stage].TexCoords[1];
				drawDesc.VertexBufferViews.Color = m_bindings.Stages[stage].Colors;
				drawDesc.CommonCBs = cbLocs;

                const Image* tex0 = Image::GetAnimatedImage(&pStage->bundle[0], input->shaderTime);
                const Image* tex1 = Image::GetAnimatedImage(&pStage->bundle[1], input->shaderTime);

                s_mt.Draw(
					cmdList,
                    tex0,
                    tex1,
					&drawDesc);
			}
			else
			{
				GENERIC_SINGLE_TEXTURE_DRAW_DESC drawDesc;
				drawDesc.NumIndices = input->numIndexes;
				drawDesc.DrawStateDesc = state;
				drawDesc.VertexBufferViews.TexCoord = m_bindings.Stages[stage].TexCoords[0];
				drawDesc.VertexBufferViews.Color = m_bindings.Stages[stage].Colors;
				drawDesc.CommonCBs = cbLocs;

				const Image* tex = GetAnimatedImage(&pStage->bundle[0], input->shaderTime);

				if (r_lightmap->integer) {
					if (pStage->bundle[1].isLightmap) {
						tex = GetAnimatedImage(&pStage->bundle[1], input->shaderTime);
						drawDesc.VertexBufferViews.TexCoord = m_bindings.Stages[stage].TexCoords[1];
					} else {
						tex = GetAnimatedImage(&pStage->bundle[0], input->shaderTime);
					}
				}

                s_st.Draw(
					cmdList,
					tex,
					&drawDesc);
			}

			// allow skipping out to show just lighting during development
			if (r_showlighting->integer)
			{
				break;
			}

			// allow skipping out to show just lightmaps during development
			if (r_lightmap->integer && (pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap))
			{
				break;
			}
		}
	}

    void GenericDrawStage::AdditiveDlights(
        ID3D12GraphicsCommandList* cmdList,
        const shaderCommands_t* input)
    {
        GENERIC_DLIGHT_DRAW_DESC drawDesc;
        drawDesc.NumIndices = input->numIndexes;

        // Create a placeholder draw state
        drawDesc.DrawStateDesc.StateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
        drawDesc.DrawStateDesc.Outline = FALSE;
        drawDesc.DrawStateDesc.PolyOffset = input->shader->polygonOffset;
        drawDesc.DrawStateDesc.CullMode = RenderState::GetCullMode(input->shader->cullType);

        // Update vertex shader view constants
        ZeroMemory(&drawDesc.CommonCBs, sizeof(drawDesc.CommonCBs));
        drawDesc.CommonCBs.VSCommonCB = RenderState::UpdatedVSConstants();
        drawDesc.CommonCBs.PSCommonCB = RenderState::UpdatedPSConstants();
        drawDesc.CommonCBs.DlightCounts = m_dlights.Counts;
        drawDesc.CommonCBs.PSDlightArrayCB = m_dlights.AddLights;

        s_dlight.Draw(
            cmdList,
            &drawDesc);
    }

    void GenericDrawStage::Fog(
        ID3D12GraphicsCommandList* cmdList,
        const shaderCommands_t* input)
    {
        GENERIC_UNLIT_SINGLE_TEXTURE_DRAW_DESC drawDesc;
        drawDesc.NumIndices = input->numIndexes;
        drawDesc.VertexBufferViews.TexCoord = m_bindings.Fog.TexCoords;
        drawDesc.VertexBufferViews.Color = m_bindings.Fog.Colors;

        // Create a placeholder draw state
        if (input->shader->fogPass == FP_EQUAL) {
            drawDesc.DrawStateDesc.StateBits = (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
        }
        else {
            drawDesc.DrawStateDesc.StateBits = (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
        }
        drawDesc.DrawStateDesc.Outline = FALSE;
        drawDesc.DrawStateDesc.PolyOffset = input->shader->polygonOffset;
        drawDesc.DrawStateDesc.CullMode = RenderState::GetCullMode(input->shader->cullType);

        // Update vertex shader view constants
        ZeroMemory(&drawDesc.CommonCBs, sizeof(drawDesc.CommonCBs));
        drawDesc.CommonCBs.VSCommonCB = RenderState::UpdatedVSConstants();
        drawDesc.CommonCBs.PSCommonCB = RenderState::UpdatedPSConstants();

        s_unlitst.Draw(
            cmdList,
            Image::Get(tr.fogImage->index),
            &drawDesc);
    }

	void GenericDrawStage::Draw(
		ID3D12GraphicsCommandList* cmdList,
		const shaderCommands_t* input)
	{
		TessBuffers* tessBufs = Frame::CurrentTessBuffers();

		// Determine what buffers we need to update for this draw call
		bool needDLights = input->dlightBits && input->shader->sort <= SS_OPAQUE
			&& !(input->shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY));
		bool needFog = input->fogNum && input->shader->fogPass;

		// Upload the tess buffer information and create index&vertex buffer views
		m_bindings.Upload(
			input,
			tessBufs,
			needFog);

		DlightBuffers* dlightBuffers = Frame::CurrentDlightBuffers();
		if (needDLights)
		{
			m_dlights.Upload(input, dlightBuffers);
		}
		else
		{
			m_dlights.Zero(dlightBuffers);
		}
		
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetIndexBuffer(&m_bindings.Indices);
		SetTessBaseVertexBuffers(cmdList, &m_bindings);

		IterateStages(
			cmdList,
			input);

		// Do additive light pass?
		if (m_dlights.Counts.AddLightCount > 0)
		{
			AdditiveDlights(
                cmdList,
                input);
		}

		if (needFog)
		{
			Fog(
                cmdList,
                input);
		}
	}

    void GenericDrawStage::DrawSky(
        ID3D12GraphicsCommandList* cmdList,
        const shaderCommands_t* input)
    {
        TessBuffers* tessBufs = Frame::CurrentTessBuffers();

        m_bindings.Upload(
            input,
			tessBufs,
            FALSE);

        DlightBuffers* dlightBuffers = Frame::CurrentDlightBuffers();
        m_dlights.Zero(dlightBuffers);

        // Create a placeholder draw state
        PIPELINE_DRAW_STATE_DESC state;
        state.StateBits = RenderState::GetStateBits();
        state.Outline = FALSE;
        state.PolyOffset = input->shader->polygonOffset;
        state.CullMode = RenderState::GetCullMode(input->shader->cullType);

        // Update vertex shader view constants
        GENERIC_ROOT_SIGNATURE_UNLIT_CONSTANT_BUFFERS cbLocs;
        ZeroMemory(&cbLocs, sizeof(cbLocs));
        cbLocs.VSCommonCB = RenderState::UpdatedVSConstants();
        cbLocs.PSCommonCB = RenderState::UpdatedPSConstants();

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetIndexBuffer(&m_bindings.Indices);
        SetTessBaseVertexBuffers(cmdList, &m_bindings);

        // Iterate over each stage
        for (int stage = 0; stage < MAX_SHADER_STAGES; stage++)
        {
            shaderStage_t *pStage = input->xstages[stage];
            if (!pStage)
            {
                break;
            }

            // Configure the draw state
            state.StateBits = pStage->stateBits & (~GLS_DEPTHMASK_TRUE);

            //
            // draw single-textured
            //
            GENERIC_UNLIT_SINGLE_TEXTURE_DRAW_DESC drawDesc;
            drawDesc.NumIndices = input->numIndexes;
            drawDesc.DrawStateDesc = state;
            drawDesc.VertexBufferViews.TexCoord = m_bindings.Stages[stage].TexCoords[0];
            drawDesc.VertexBufferViews.Color = m_bindings.Stages[stage].Colors;
            drawDesc.CommonCBs = cbLocs;

            const Image* tex = GetAnimatedImage(&pStage->bundle[0], input->shaderTime);
            s_unlitst.Draw(
                cmdList,
                tex,
                &drawDesc);
        }
    }

    void GenericDrawStage::DrawDepth(
        ID3D12GraphicsCommandList* cmdList,
        const shaderCommands_t* input)
    {
        TessBuffers* tessBufs = Frame::CurrentTessBuffers();

        m_bindings.Upload(
            input,
			tessBufs,
            FALSE);

        // Create a placeholder draw state
        PIPELINE_DRAW_STATE_DESC state;
        state.StateBits = RenderState::GetStateBits();
        state.Outline = FALSE;
        state.PolyOffset = input->shader->polygonOffset;
        state.CullMode = input->shader->cullType;

        // Update vertex shader view constants
        GENERIC_ROOT_SIGNATURE_UNLIT_CONSTANT_BUFFERS cbLocs;
        ZeroMemory(&cbLocs, sizeof(cbLocs));
        cbLocs.VSCommonCB = RenderState::UpdatedVSConstants();
        cbLocs.PSCommonCB = RenderState::UpdatedPSConstants();

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetIndexBuffer(&m_bindings.Indices);
        SetTessBaseVertexBuffers(cmdList, &m_bindings);

        // Iterate over each stage
        for (int stage = 0; stage < MAX_SHADER_STAGES; stage++)
        {
            shaderStage_t *pStage = input->xstages[stage];
            if (!pStage)
            {
                break;
            }

            // Configure the draw state
            state.StateBits =
                (pStage->stateBits & ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
                | GLS_SRCBLEND_ZERO
                | GLS_DSTBLEND_ZERO;

            //
            // draw single or multi-textured
            //
            GENERIC_DEPTH_DRAW_DESC drawDesc;
            drawDesc.NumIndices = input->numIndexes;
            drawDesc.DrawStateDesc = state;
            drawDesc.TexCoordVertexBufferView = m_bindings.Stages[stage].TexCoords[0];
            drawDesc.CommonCBs = cbLocs;

            s_depth.Draw(
                cmdList,
                Image::GetAnimatedImage(&pStage->bundle[0], input->shaderTime),
                &drawDesc);
        }
    }

	void GenericDrawStage::Init()
	{
        s_unlitst.Init();
        s_st.Init();
        s_mt.Init();
        s_depth.Init();
        s_dlight.Init();
	}

	void GenericDrawStage::Destroy()
	{
        s_unlitst.Destroy();
        s_st.Destroy();
        s_mt.Destroy();
        s_depth.Destroy();
        s_dlight.Destroy();
	}

    void GenericDrawStage::CacheShaderPipelineState(
        const shader_t* shader )
    {
        s_depth.CacheShaderPipelineState( shader );
        s_st.CacheShaderPipelineState( shader );
        s_mt.CacheShaderPipelineState( shader );
        s_depth.CacheShaderPipelineState( shader );
        s_dlight.CacheShaderPipelineState( shader );
    }
}
