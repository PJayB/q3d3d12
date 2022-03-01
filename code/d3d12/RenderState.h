#pragma once
#include "ConstantBuffer.h"

namespace QD3D12
{
	struct PIPELINE_DRAW_STATE_DESC;

	struct VIEW_CONSTANTS_VS
	{
		float Projection[16];
		float View[16];
		float DepthRange[2];
	};

	struct VIEW_CONSTANTS_PS
	{
		float ClipPlane[4];
		float AlphaClip[2];
	};

	class RenderState
	{
	private:
		static void InitData();
		static void DestroyData();

		static VIEW_CONSTANTS_VS s_vsConstants;
		static VIEW_CONSTANTS_PS s_psConstants;
		static ConstantBuffer s_vsCB;
		static ConstantBuffer s_psCB;
		static BOOL s_vsConstantsDirty;
		static BOOL s_psConstantsDirty;
		static UINT s_glsState;

	public:
		static inline const VIEW_CONSTANTS_VS* VSConstants() { return &s_vsConstants; }
		static inline const VIEW_CONSTANTS_PS* PSConstants() { return &s_psConstants; }

		static D3D12_GPU_VIRTUAL_ADDRESS UpdatedVSConstants();
		static D3D12_GPU_VIRTUAL_ADDRESS UpdatedPSConstants();

		static BOOL IsMirror();
		static UINT GetStateBits() { return s_glsState; }
		static BOOL SetStateBits(UINT bits);

		static void SetClipPlane(_In_ const float plane[4]);
		static void SetDepthRange(_In_ float min, _In_ float max);
		static void SetProjection(_In_ const float* projMatrix);
		static void SetModelView(_In_ const float* mvMatrix);

		static void ConfigureDrawState(
			_Out_ PIPELINE_DRAW_STATE_DESC* pState,
			_In_ UINT cullMode,
			_In_ BOOL polyOffset,
			_In_ BOOL outline);

		static BOOL SetAndConfigureDrawState(
			_Out_ PIPELINE_DRAW_STATE_DESC* pState,
			_In_ UINT overrideStateBits,
			_In_ UINT cullMode,
			_In_ BOOL polyOffset,
			_In_ BOOL outline);

		static UINT GetCullMode(UINT cullMode);

		static BOOL GetPixelShaderViewDataFromState(
			_In_ UINT stateBits,
			_In_ UINT previousStateBits,
			_Out_ VIEW_CONSTANTS_PS* psConstants);

		static void Init();
		static void Destroy();
	};
}
