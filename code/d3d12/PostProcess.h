#pragma once

namespace QD3D12
{
    class PostProcess
    {
    public:

        static void ResolveTarget(
            _In_ ID3D12GraphicsCommandList* pCmdList,
            _In_ ID3D12Resource* pDest,
            _In_ ID3D12Resource* pSrc,
            _In_ DXGI_FORMAT format,
            _In_ BOOL multisampled );
    };
}
