#pragma once

#include "Shader.h"
#include "UploadRingBuffer.h"
#include "ConstantBuffer.h" // TODO: remove me
#include "PipelineState.h"

namespace QD3D12 
{
	class Image;

	class FSQ
	{
	private:
		// This is zero'd out, so don't put anything in here
		// that has a constructor!
		struct State
		{
			ID3D12Resource* pIndexBufferResource;
			ID3D12RootSignature* pRootSignature;
			D3D12_INDEX_BUFFER_VIEW IndexBufferView;
			UINT64 VertexBufferSize;
		};

		struct Vertex
		{
			float Coords[2];
			float Texcoords[2];
			float Color[3];
		};

		static State s_State;
		static PipelineState s_PSO;
		static UploadRingBuffer s_VBuf;
		static Shader s_VS;
		static Shader s_PS;

	public:

		static void Init();
		static void Destroy();

		static void Draw(
			_In_ const Image* image,
			_In_reads_(4) const float* coords,
			_In_reads_(4) const float* texcoords,
			_In_reads_(3) const float* color);
	};
}
