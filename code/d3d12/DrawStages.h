#pragma once
#include "UploadScratch.h"
#include "TessBuffers.h"
#include "Shader.h"
#include "PipelineState.h"
#include "DlightBuffers.h"
#include "GenericShaders.h"

namespace QD3D12 
{
	class TessBuffers;
	class DlightTessBuffers;

    void SetTessBaseVertexBuffers(
        _In_ ID3D12GraphicsCommandList* cmdList,
        _In_ const TessBufferBindings* bindings);

    bool StageSkipsPreZ(
        _In_ const shaderStage_t* pStage);

    template<UINT Count>
    void SetTessArbVertexBuffers(
        _In_ ID3D12GraphicsCommandList* cmdList,
        _In_ const D3D12_VERTEX_BUFFER_VIEW* vbvs)
    {
        cmdList->IASetVertexBuffers(
            TessBufferBindings::VERTEX_BUFFER_COUNT,
            Count,
            vbvs);
	}
}
