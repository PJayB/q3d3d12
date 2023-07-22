#include "FSQ.h"
#include "Resources.h"
#include "RootSignature.h"
#include "Frame.h"
#include "Image.h"
#include "RenderState.h"

namespace QD3D12
{
	FSQ::State FSQ::s_State;
	UploadRingBuffer FSQ::s_VBuf;
	PipelineState FSQ::s_PSO;
	Shader FSQ::s_VS;
	Shader FSQ::s_PS;

	void FSQ::Init()
	{
		ZeroMemory(&s_State, sizeof(s_State));

		// Shaders
		s_VS.Init("fsq_vs");
		s_PS.Init("fsq_ps");

		// Root signature
		CD3DX12_DESCRIPTOR_RANGE ranges[2];
		// TODO: make these constants
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);   // Pixel shader reads one SRV.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);   // Pixel shader reads one Sampler.

		CD3DX12_ROOT_PARAMETER params[4];
		params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		params[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		params[2].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		params[3].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		s_State.pRootSignature = RootSignature::Create(&rsig);

		// Vertex layout
		static D3D12_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// Pipeline state
		PIPELINE_STATE_DESC psoDesc;
		ZeroMemory(&psoDesc, sizeof(psoDesc));
		psoDesc.pRootSignature = s_State.pRootSignature;
		psoDesc.VS = s_VS.ByteCode();
		psoDesc.PS = s_PS.ByteCode();
		psoDesc.InputLayout.pInputElementDescs = elements;
		psoDesc.InputLayout.NumElements = _countof(elements);
		PIPELINE_TARGET_STATE_DESC targetDesc;
		InitializeDefaultTargetDesc(&targetDesc);
		s_PSO.Init(
            "FSQ",
			&psoDesc,
			&targetDesc);
		
        CachePipelineStateBits( __FUNCTION__, &s_PSO, 0, CT_TWO_SIDED, FALSE, FALSE );
        CachePipelineStateBits( __FUNCTION__, &s_PSO, GLS_DEFAULT, CT_TWO_SIDED, FALSE, FALSE );
        CachePipelineStateBits( __FUNCTION__, &s_PSO, GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA|GLS_DEPTHTEST_DISABLE, CT_TWO_SIDED, FALSE, FALSE );

		//
		// Index buffer
		//
		static const USHORT indices[] =
		{
			1, 2, 0,
			2, 3, 0
		};

		Resources::CreateImmutableBuffer(
			D3D12_RESOURCE_STATE_GENERIC_READ,
			indices,
			sizeof(indices),
			&s_State.pIndexBufferResource);

		SetResourceName(s_State.pIndexBufferResource, "FSQ Index Buffer");

		s_State.IndexBufferView.BufferLocation = s_State.pIndexBufferResource->GetGPUVirtualAddress();
		s_State.IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
		s_State.IndexBufferView.SizeInBytes = sizeof(indices);

		//
		// Circular vertex buffer
		//
		s_State.VertexBufferSize = sizeof(Vertex) * 4 * QD3D12_ESTIMATED_DRAW_CALLS;
		s_VBuf.Init(s_State.VertexBufferSize);

		SetResourceName(s_VBuf.Resource(), "FSQ Vertex Buffer");

		// TODO: draw bundle
	}

	void FSQ::Destroy()
	{
		SAFE_RELEASE(s_State.pIndexBufferResource);
		SAFE_RELEASE(s_State.pRootSignature);
		s_VBuf.Destroy();
		s_PSO.Destroy();
		s_VS.Destroy();
		s_PS.Destroy();
	}

	void FSQ::Draw(
		_In_ const Image* image,
		_In_reads_(4) const float* coords,
		_In_reads_(4) const float* texcoords,
		_In_reads_(3) const float* in_color)
	{
		float color[3] = { 1, 1, 1 };
		if (in_color) {
			memcpy(color, in_color, sizeof(float) * 3);
		}

		Vertex vb[4];
		vb[0].Texcoords[0] = texcoords[0]; vb[0].Texcoords[1] = texcoords[1];
		vb[1].Texcoords[0] = texcoords[2]; vb[1].Texcoords[1] = texcoords[1];
		vb[2].Texcoords[0] = texcoords[2]; vb[2].Texcoords[1] = texcoords[3];
		vb[3].Texcoords[0] = texcoords[0]; vb[3].Texcoords[1] = texcoords[3];

		vb[0].Coords[0] = coords[0]; vb[0].Coords[1] = coords[1];
		vb[1].Coords[0] = coords[2]; vb[1].Coords[1] = coords[1];
		vb[2].Coords[0] = coords[2]; vb[2].Coords[1] = coords[3];
		vb[3].Coords[0] = coords[0]; vb[3].Coords[1] = coords[3];

		vb[0].Color[0] = color[0]; vb[0].Color[1] = color[1]; vb[0].Color[2] = color[2];
		vb[1].Color[0] = color[0]; vb[1].Color[1] = color[1]; vb[1].Color[2] = color[2];
		vb[2].Color[0] = color[0]; vb[2].Color[1] = color[1]; vb[2].Color[2] = color[2];
		vb[3].Color[0] = color[0]; vb[3].Color[1] = color[1]; vb[3].Color[2] = color[2];

		D3D12_VERTEX_BUFFER_VIEW vbView;
		vbView.BufferLocation = s_VBuf.Write(vb, sizeof(vb));
		vbView.SizeInBytes = sizeof(vb);
		vbView.StrideInBytes = sizeof(Vertex);

		// TODO: cache the state of the view constants
		UINT64 vsGpuLoc = RenderState::UpdatedVSConstants();
		UINT64 psGpuLoc = RenderState::UpdatedPSConstants();

		ID3D12DescriptorHeap* pDescriptorHeaps[] = {
            Image::ShaderVisibleResourceHeap(),
            Image::ShaderVisibleSamplerHeap(),
        };

		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandles[] = {
            image->ShaderVisibleSRV(),
            image->ShaderVisibleSampler()
        };

		PIPELINE_DRAW_STATE_DESC drawState;
		RenderState::ConfigureDrawState(&drawState, CT_TWO_SIDED, FALSE, FALSE);

		ID3D12GraphicsCommandList* pList = Frame::CurrentCommandList();
		pList->SetGraphicsRootSignature(s_State.pRootSignature);
		pList->SetPipelineState(s_PSO.GetPipelineState(__FUNCTION__, &drawState));
		pList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pList->IASetVertexBuffers(0, 1, &vbView);
		pList->IASetIndexBuffer(&s_State.IndexBufferView);
		pList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);
		pList->SetGraphicsRootConstantBufferView(0, vsGpuLoc);
		pList->SetGraphicsRootConstantBufferView(1, psGpuLoc);
		SetGraphicsRootDescriptorTables<2, _countof(gpuDescriptorHandles)>(pList, gpuDescriptorHandles);
		pList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}
}
