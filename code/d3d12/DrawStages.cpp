#include "pch.h"
#include "DrawStages.h"
#include "TessBuffers.h"
#include "Frame.h"
#include "RenderState.h"
#include "PipelineState.h"
#include "ConstantBuffer.h"
#include "Image.h"
#include "RootSignature.h"

namespace QD3D12
{
	void SetTessBaseVertexBuffers(
		ID3D12GraphicsCommandList* cmdList,
		const TessBufferBindings* bindings)
	{
		cmdList->IASetVertexBuffers(
			0, 
			TessBufferBindings::VERTEX_BUFFER_COUNT,
            bindings->BaseVertexBuffers);
	}

	bool StageSkipsPreZ(const shaderStage_t* pStage)
	{
		return (pStage->stateBits & ~GLS_ATEST_BITS) != GLS_DEPTHMASK_TRUE;
	}
}
