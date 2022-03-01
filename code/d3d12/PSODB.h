#pragma once

#include <vector>
#include <string>

namespace QD3D12
{
    struct CachedPSO
    {
        PIPELINE_DRAW_STATE_DESC Desc;
        size_t BlobSize;
        uint8_t BlobData[1];
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
}
