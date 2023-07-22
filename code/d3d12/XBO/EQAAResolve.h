#pragma once

#include "../Shader.h"
#include "../PipelineState.h"

namespace QD3D12
{
	class EQAAResolve
	{
	private:
        static Shader s_shader;
        static ID3D12PipelineState* s_pso;
        static ID3D12RootSignature* s_rsig;

	public:
        static void Init();
        static void Destroy();

        static void Apply(
            ID3D12GraphicsCommandList* pCmdList,
            ID3D12Resource* pSource,
            ID3D12Resource* pFMask,
            ID3D12Resource* pTarget );
	};
}
