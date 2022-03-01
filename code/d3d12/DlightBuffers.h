#pragma once
#include "UploadScratch.h"

namespace QD3D12 
{
	struct DLIGHT_INFO
	{
		UINT ModLightCount;
		UINT AddLightCount;
	};

	struct DLIGHT
	{
		vec3_t Origin;
		float Radius;
		vec3_t Color;
		float __padding;
	};

	class DlightBuffers
	{
	private:
		UploadScratch m_arrayScratch;

	public:
		void Init();
		void Destroy();
		void Reset();
		
		inline UploadScratch* DlightScratch() { return &m_arrayScratch; }

	public:
		static inline UINT64 BufferCapacity() { return QD3D12_ESTIMATED_DRAW_CALLS; }
	};

	class DlightBufferBindings
	{
	private:
		void SetDlightCounts(
			_In_ UINT add,
			_In_ UINT mod);
	public:
		DlightBufferBindings();

		void Zero(
			_In_ DlightBuffers* dlights);

		void Upload(
			_In_ const shaderCommands_t* input,
			_In_ DlightBuffers* dlights);

		DLIGHT_INFO Counts;
		D3D12_GPU_VIRTUAL_ADDRESS ModLights;
		D3D12_GPU_VIRTUAL_ADDRESS AddLights;
	};
}
