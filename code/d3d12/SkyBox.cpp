#include "pch.h"
#include "SkyBox.h"
#include "Resources.h"
#include "RootSignature.h"
#include "Frame.h"
#include "Image.h"
#include "RenderState.h"

namespace QD3D12
{
	SkyBox::State SkyBox::s_State;
    PipelineState SkyBox::s_PSO;
	Shader SkyBox::s_VS;
	Shader SkyBox::s_PS;

	void SkyBox::Init()
	{
		ZeroMemory(&s_State, sizeof(s_State));

		// Shaders
		s_VS.Init("skybox_vs");
		s_PS.Init("skybox_ps");

		// Root signature
		CD3DX12_DESCRIPTOR_RANGE ranges[2];
		// TODO: make these constants
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);   // Pixel shader reads one SRV.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);   // Pixel shader reads one Sampler.

		CD3DX12_ROOT_PARAMETER params[5]; int paramSlot = 0;
		params[paramSlot++].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        params[paramSlot++].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        params[paramSlot++].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
		params[paramSlot++].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		params[paramSlot++].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		s_State.pRootSignature = RootSignature::Create(&rsig);

		// Vertex layout
		static D3D12_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
            "SkyBox",
			&psoDesc,
			&targetDesc);
		
		// Cache common states
		PIPELINE_DRAW_STATE_DESC drawState;
		drawState.CullMode = CT_TWO_SIDED;
		drawState.Outline = FALSE;
		drawState.PolyOffset = FALSE;
		drawState.StateBits = 0;
		s_PSO.PrecachePipelineState(__FUNCTION__, &drawState);

		//
		// Vertex buffer
		//
		static const Vertex skyboxVertexData[] =
		{
			// Right
			{ 1, -1, -1, 1, 1 }, // 1
			{ 1, -1, 1, 1, 0 }, // 5
			{ 1, 1, 1, 0, 0 }, // 7
			{ 1, -1, -1, 1, 1 }, // 1
			{ 1, 1, 1, 0, 0 }, // 7
			{ 1, 1, -1, 0, 1 }, // 3

			// Left      
            { -1, -1, 1, 0, 0 }, // 4
            { -1, -1, -1, 0, 1 }, // 0
            { -1, 1, -1, 1, 1 }, // 2
            { -1, -1, 1, 0, 0 }, // 4
            { -1, 1, -1, 1, 1 }, // 2
            { -1, 1, 1, 1, 0 }, // 6

			// Back     
            { -1, 1, 1, 0, 0 }, // 6
            { -1, 1, -1, 0, 1 }, // 2
            { 1, 1, -1, 1, 1 }, // 3
            { -1, 1, 1, 0, 0 }, // 6
            { 1, 1, -1, 1, 1 }, // 3
            { 1, 1, 1, 1, 0 }, // 7

			// Front     
            { 1, -1, 1, 0, 0 }, // 5
            { 1, -1, -1, 0, 1 }, // 1
            { -1, -1, -1, 1, 1 }, // 0
            { 1, -1, 1, 0, 0 }, // 5
            { -1, -1, -1, 1, 1 }, // 0
            { -1, -1, 1, 1, 0 }, // 4

			// Up        
            { 1, -1, 1, 1, 1 }, // 5
            { -1, -1, 1, 1, 0 }, // 4
            { -1, 1, 1, 0, 0 }, // 6
            { 1, -1, 1, 1, 1 }, // 5
            { -1, 1, 1, 0, 0 }, // 6
            { 1, 1, 1, 0, 1 }, // 7

			// Down      
            { -1, -1, -1, 1, 0 }, // 0
            { 1, -1, -1, 1, 1 }, // 1
            { 1, 1, -1, 0, 1 }, // 3
            { -1, -1, -1, 1, 0 }, // 0
            { 1, 1, -1, 0, 1 }, // 3
            { -1, 1, -1, 0, 0 }, // 2
		};

		Resources::CreateImmutableBuffer(
			D3D12_RESOURCE_STATE_GENERIC_READ,
			skyboxVertexData,
			sizeof(skyboxVertexData),
			&s_State.pVertexBufferResource);

		s_State.VertexBufferView.BufferLocation = s_State.pVertexBufferResource->GetGPUVirtualAddress();
		s_State.VertexBufferView.StrideInBytes = sizeof(Vertex);
		s_State.VertexBufferView.SizeInBytes = sizeof(skyboxVertexData);

		// TODO: draw bundle
	}

	void SkyBox::Destroy()
	{
		SAFE_RELEASE(s_State.pVertexBufferResource);
		SAFE_RELEASE(s_State.pRootSignature);
		s_PSO.Destroy();
		s_VS.Destroy();
		s_PS.Destroy();
	}

	void SkyBox::Draw(
        _In_ const skyboxDrawInfo_t* skybox,
        _In_ const float* eyeOrigin,
        _In_ const float* in_colorTint)
	{
        float colorTint[4] = { 1, 1, 1, 1 };
        if (in_colorTint) {
            memcpy(colorTint, in_colorTint, sizeof(float) * 3);
        }

		// TODO: cache the state of the view constants
		D3D12_GPU_VIRTUAL_ADDRESS vsCb1GpuLoc = RenderState::UpdatedVSConstants();
		D3D12_GPU_VIRTUAL_ADDRESS vsCb2GpuLoc = ConstantBuffer::GlobalWrite(
			eyeOrigin, 
			sizeof(float) * 3);
		D3D12_GPU_VIRTUAL_ADDRESS psCbGpuLoc = ConstantBuffer::GlobalWrite(
			colorTint, 
			sizeof(float) * 4);

		PIPELINE_DRAW_STATE_DESC drawState;
        drawState.CullMode = CT_TWO_SIDED;
        drawState.Outline = FALSE;
        drawState.PolyOffset = FALSE;
        drawState.StateBits = 0;

		ID3D12GraphicsCommandList* pList = Frame::CurrentCommandList();
		pList->SetPipelineState(s_PSO.GetPipelineState(__FUNCTION__, &drawState));
		pList->SetGraphicsRootSignature(s_State.pRootSignature);
		pList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pList->IASetVertexBuffers(0, 1, &s_State.VertexBufferView);
		pList->SetGraphicsRootConstantBufferView(0, vsCb1GpuLoc);
		pList->SetGraphicsRootConstantBufferView(1, vsCb2GpuLoc);
		pList->SetGraphicsRootConstantBufferView(2, psCbGpuLoc);

        for (int i = 0; i < 6; ++i)
        {
            const skyboxSideDrawInfo_t* side = &skybox->sides[i];

            if (!side->image)
                continue;

            const Image* image = Image::Get(side->image->index);

		    ID3D12DescriptorHeap* pDescriptorHeaps[] = {
			    Image::ShaderVisibleResourceHeap(),
			    Image::ShaderVisibleSamplerHeap(),
		    };

		    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandles[] = {
			    image->ShaderVisibleSRV(),
			    image->ShaderVisibleSampler()
		    };

            pList->SetDescriptorHeaps( _countof(pDescriptorHeaps), pDescriptorHeaps );
            SetGraphicsRootDescriptorTables<3, _countof(gpuDescriptorHandles)>(pList, gpuDescriptorHandles);
            pList->DrawInstanced(6, 1, i * 6, 0);
        }
	}
}
