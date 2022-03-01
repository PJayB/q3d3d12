#pragma once

#include <vector>
#include <string>

namespace QD3D12
{
    struct CachedPSO
    {
        PIPELINE_DRAW_STATE_DESC Desc;
#ifdef _XBOX_ONE
        struct BlobOffset
        {
            size_t Offset;
            size_t Size;
        };

        BlobOffset Packet;
        BlobOffset Meta;
        BlobOffset PS;
        BlobOffset PSMeta;
        BlobOffset VS;
        BlobOffset VSMeta;
        BlobOffset GS;
        BlobOffset GSMeta;
        BlobOffset ES;
        BlobOffset ESMeta;
        BlobOffset HS;
        BlobOffset HSMeta;
        BlobOffset LS;
        BlobOffset LSMeta;
        uint8_t BlobData[1];
#else
        size_t BlobSize;
        uint8_t BlobData[1];
#endif
    };

    class PSODB
    {
    public:
        PSODB(const char* dbname);

        std::vector<uint64_t> EnumerateHashes() const;

        qboolean LoadCachedBlob(uint64_t hash, CachedPSO** cachedPSO);
        void FreeCachedBlob(CachedPSO* cachedPSO);

        qboolean CacheBlob(uint64_t hash, const PIPELINE_DRAW_STATE_DESC* desc, ID3D12PipelineState* state);

    private:
        std::string m_name;
    };

#ifdef _XBOX_ONE
    void ConvertCachedPSOBlobToState(
        const CachedPSO* cachedPSO,
        ID3D12RootSignature* rootSig,
        D3D12XBOX_DESERIALIZE_GRAPHICS_PIPELINE_STATE* state);
#endif
}
