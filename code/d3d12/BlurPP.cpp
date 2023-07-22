#include "BlurPP.h"
#include "RootSignature.h"
#include "Frame.h"
#include "Device.h"

namespace QD3D12
{
    Shader BlurPP::s_shader;
    ID3D12PipelineState* BlurPP::s_pso = nullptr;
    ID3D12RootSignature* BlurPP::s_rsig = nullptr;

    void BlurPP::Init()
    {
        s_shader.Init("blur_cs");

		CD3DX12_DESCRIPTOR_RANGE dt[2];
		dt[0].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 );
		dt[1].Init( D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0 );
        
		CD3DX12_ROOT_PARAMETER params[2];
		params[0].InitAsDescriptorTable( _countof(dt), dt, D3D12_SHADER_VISIBILITY_ALL);
        params[1].InitAsConstants(2, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_ROOT_SIGNATURE_DESC rsig;
		rsig.Init(_countof(params), params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        s_rsig = RootSignature::Create( &rsig );

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
        ZeroMemory( &psoDesc, sizeof(psoDesc) );
        psoDesc.CS = s_shader.ByteCode();
        psoDesc.NodeMask = QD3D12_NODE_MASK;
        psoDesc.pRootSignature = s_rsig;

        COM_ERROR_TRAP( Device::Get()->CreateComputePipelineState(
            &psoDesc,
            __uuidof(ID3D12PipelineState),
            (void**) &s_pso ) );
    }

    void BlurPP::Destroy()
    {
        s_shader.Destroy();

        SAFE_RELEASE( s_rsig );
        SAFE_RELEASE( s_pso );
    }

    void BlurPP::Apply(
        ID3D12GraphicsCommandList* pCmdList,
        ID3D12Resource* pSource,
        ID3D12Resource* pTarget )
    {
		// TODO: remove all of this
        D3D12_RESOURCE_DESC srcDesc = pSource->GetDesc();
        D3D12_RESOURCE_DESC dstDesc = pTarget->GetDesc();

        DWORD cbuf[] = { (DWORD) srcDesc.Width, (DWORD) srcDesc.Height };

		DescriptorBatch* pDescriptors = Frame::CurrentDescriptorBatch(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DESCRIPTOR_HANDLE_TUPLE hDescriptors = pDescriptors->AllocateDescriptors(2);

		ID3D12Device* pDevice = Device::Get();

		UINT increment = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof(srvDesc) );
		srvDesc.Format = srcDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		pDevice->CreateShaderResourceView( pSource, &srvDesc, hDescriptors.Cpu );

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory( &uavDesc, sizeof(uavDesc) );
		uavDesc.Format = dstDesc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		pDevice->CreateUnorderedAccessView( pTarget, nullptr, &uavDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(hDescriptors.Cpu, 1, increment) );
        
        pCmdList->SetPipelineState( s_pso );
        pCmdList->SetComputeRootSignature( s_rsig );
        pCmdList->SetComputeRootDescriptorTable( 0, hDescriptors.Gpu );
        pCmdList->SetComputeRoot32BitConstants( 1, _countof(cbuf), cbuf, 0 );
        pCmdList->Dispatch( dstDesc.Width, dstDesc.Height, 1 );
    }
}
