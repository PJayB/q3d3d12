#include "pch.h"
#include "Device.h"
#include "SwapChain.h"
#include "RenderState.h"
#include "Frame.h"
#include "FSQ.h"
#include "RootSignature.h"
#include "Image.h"
#include "Upload.h"
#include "RenderState.h"
#include "SkyBox.h"
#include "GenericDrawStage.h"
#include "ConstantBuffer.h"
#include "BlurPP.h"
#include "GlobalCommandList.h"

namespace QD3D12
{
	VIEW_CONSTANTS_VS RenderState::s_vsConstants;
	VIEW_CONSTANTS_PS RenderState::s_psConstants;
	ConstantBuffer RenderState::s_vsCB;
	ConstantBuffer RenderState::s_psCB;
	BOOL RenderState::s_vsConstantsDirty;
	BOOL RenderState::s_psConstantsDirty;
	UINT RenderState::s_glsState = GLS_DEFAULT;

	BOOL RenderState::SetAndConfigureDrawState(
		PIPELINE_DRAW_STATE_DESC* pState,
		UINT stateBits,
		UINT cullMode,
		BOOL polyOffset,
		BOOL outline)
	{
		BOOL r = SetStateBits(stateBits);
		ConfigureDrawState(pState, cullMode, polyOffset, outline);
		return r;
	}

	void RenderState::ConfigureDrawState(
		PIPELINE_DRAW_STATE_DESC* pState,
		UINT cullMode,
		BOOL polyOffset,
		BOOL outline)
	{
		pState->CullMode = GetCullMode(cullMode);
		pState->Outline = outline;
		pState->PolyOffset = polyOffset;
		pState->StateBits = s_glsState;
	}

	void RenderState::Init()
	{
		InitData();

		// Initialize the global state
		RootSignature::Init();
		Frame::Init();
		Upload::Init();
		Image::Init();

		// Initialize draw data
		FSQ::Init();
        SkyBox::Init();
        BlurPP::Init();
		GenericShaders::Init();
		GenericDrawStage::Init();

        GlobalCommandList::KickOff();
	}

	void RenderState::Destroy()
	{
		// This guarantees that nothing is being used by the GPU any more
		Device::WaitForGpu();

		GenericDrawStage::Destroy();
		GenericShaders::Destroy();
        SkyBox::Destroy();
		FSQ::Destroy();
        BlurPP::Destroy();

		Image::Destroy();
		Upload::Destroy();
		Frame::Destroy();
		RootSignature::Destroy();

		DestroyData();
	}

	void RenderState::InitData()
	{
		ZeroMemory(&s_vsConstants, sizeof(s_vsConstants));
		ZeroMemory(&s_psConstants, sizeof(s_psConstants));

		Com_Memcpy(s_vsConstants.View, s_identityMatrix, sizeof(float) * 16);
		Com_Memcpy(s_vsConstants.Projection, s_identityMatrix, sizeof(float) * 16);
		s_vsConstants.DepthRange[0] = 0;
		s_vsConstants.DepthRange[1] = 1;

		s_psConstants.AlphaClip[0] = 1;
		s_psConstants.AlphaClip[1] = 0;

		SetStateBits(GLS_DEFAULT);

		s_vsConstantsDirty = TRUE;
		s_psConstantsDirty = TRUE;
	}

	void RenderState::DestroyData()
	{
	}

	void RenderState::SetClipPlane(const float plane[])
	{
		memcpy(s_psConstants.ClipPlane, plane, sizeof(float) * 4);
		s_psConstantsDirty = TRUE;
	}

	void RenderState::SetDepthRange(float min, float max)
	{
		s_vsConstants.DepthRange[0] = min;
		s_vsConstants.DepthRange[1] = max - min;
		s_vsConstantsDirty = TRUE;
	}

	void RenderState::SetProjection(const float* projMatrix)
	{
		memcpy(s_vsConstants.Projection, projMatrix, sizeof(float) * 16);
		s_vsConstantsDirty = TRUE;
	}

	void RenderState::SetModelView(const float* modelViewMatrix)
	{
		memcpy(s_vsConstants.View, modelViewMatrix, sizeof(float) * 16);
		s_vsConstantsDirty = TRUE;
	}

	D3D12_GPU_VIRTUAL_ADDRESS RenderState::UpdatedVSConstants()
	{
		if (s_vsConstantsDirty)
		{
			s_vsConstantsDirty = FALSE;
			return s_vsCB.Write(&s_vsConstants, sizeof(s_vsConstants));
		}
		return s_vsCB.Address();
	}

	D3D12_GPU_VIRTUAL_ADDRESS RenderState::UpdatedPSConstants()
	{
		if (s_psConstantsDirty)
		{
			s_psConstantsDirty = FALSE;
			return s_psCB.Write(&s_psConstants, sizeof(s_psConstants));
		}
		return s_psCB.Address();
	}

	BOOL RenderState::IsMirror()
	{
		return backEnd.viewParms.isMirror != 0;
	}

	UINT RenderState::GetCullMode(UINT cullMode)
	{
		switch (cullMode)
		{
		case CT_TWO_SIDED:
			return CT_TWO_SIDED;
		case CT_BACK_SIDED:
			return IsMirror() ? CT_FRONT_SIDED : CT_BACK_SIDED;
		case CT_FRONT_SIDED:
			return IsMirror() ? CT_BACK_SIDED : CT_FRONT_SIDED;
		default:
			assert(0);
			return CT_TWO_SIDED;
		}
	}

	BOOL RenderState::GetPixelShaderViewDataFromState(
		_In_ UINT stateBits,
		_In_ UINT oldStateBits,
		_Out_ VIEW_CONSTANTS_PS* psConstants)
	{
		const float alphaEps = 0.00001f; // @pjb: HACK HACK HACK

		stateBits &= GLS_ATEST_BITS;
		oldStateBits &= GLS_ATEST_BITS;
		
		switch (stateBits)
		{
		case 0:
			psConstants->AlphaClip[0] = 1;
			psConstants->AlphaClip[1] = 0;
			break;
		case GLS_ATEST_GT_0:
			psConstants->AlphaClip[0] = 1;
			psConstants->AlphaClip[1] = alphaEps;
			break;
		case GLS_ATEST_LT_80:
			psConstants->AlphaClip[0] = -1;
			psConstants->AlphaClip[1] = 0.5f;
			break;
		case GLS_ATEST_GE_80:
			psConstants->AlphaClip[0] = 1;
			psConstants->AlphaClip[1] = 0.5f;
			break;
		default:
			ASSERT(0);
			break;
		}

		// Return true if the a-test bits are different from the previous state
		return stateBits != oldStateBits;
	}

	BOOL RenderState::SetStateBits(UINT stateBits)
	{
		PROFILE_FUNC();
		UINT diff = stateBits ^ s_glsState;

		if (!diff)
		{
			return FALSE;
		}

		BOOL psConstantsDifferent = FALSE;
		
		//
		// In our shader we need to convert these operations:
		//  pass if > 0
		//  pass if < 0.5
		//  pass if >= 0.5
		// to: 
		//  fail if <= 0
		//  fail if >= 0.5
		//  fail if < 0.5
		// clip() will kill any alpha < 0.
		if (diff & GLS_ATEST_BITS)
		{
			psConstantsDifferent |= GetPixelShaderViewDataFromState(
				stateBits,
				s_glsState,
				&s_psConstants);
		}

		// Cache the bits
		s_glsState = stateBits;
		s_psConstantsDirty = psConstantsDifferent;
		return psConstantsDifferent;
	}
}
