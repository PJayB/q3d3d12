#pragma once

#include "Shader.h"
#include "UploadRingBuffer.h"
#include "ConstantBuffer.h" // TODO: remove me
#include "PipelineState.h"

namespace QD3D12 
{
	class Image;

	class SkyBox
	{
	private:
		// This is zero'd out, so don't put anything in here
		// that has a constructor!
		struct State
		{
			ID3D12Resource* pVertexBufferResource;
			ID3D12RootSignature* pRootSignature;
			D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		};

        struct SkyBoxVSCB
        {
            float EyePos[3];
        };

        struct SkyBoxPSCB
        {
            float ColorTint[4];
        };

		struct Vertex
		{
			float Coords[3];
			float Texcoords[2];
		};

		static State s_State;
		static PipelineState s_PSO;
		static Shader s_VS;
		static Shader s_PS;

	public:

		static void Init();
		static void Destroy();

		static void Draw(
            _In_ const skyboxDrawInfo_t* skybox,
            _In_ const float* eye_origin,
            _In_ const float* colorTint);
	};
}
