#include "pch.h"
#include "PipelineState.h"
#include "PSODB.h"

#define PSODB_PATH "psodb/pc"
#define PSODB_EXTENSION ".pso"

using namespace Microsoft::WRL;

namespace QD3D12
{
    PSODB::PSODB(const char* name)
        : m_name(name)
    {
    }
    
    std::vector<uint64_t> PSODB::EnumerateHashes() const
    {
        char path[MAX_PATH];
        sprintf_s(path, "%s/%s", PSODB_PATH, m_name.c_str());

        // Scan the filesystem
        int numFiles = 0;
        char** files = FS_ListFiles(path, PSODB_EXTENSION, &numFiles);

        // Reserve enough memory for the hashes
        std::vector<uint64_t> hashes;
        hashes.reserve(numFiles);

        // Add the hashes from the filenames
        for (int i = 0; i < numFiles; ++i)
        {
            uint64_t hash = 0;
            if (sscanf_s(files[i], "%llx" PSODB_EXTENSION, &hash) == 1)
            {
                // Load the PSOs
                hashes.push_back(hash);
            }
        }

        FS_FreeFileList(files);

        return std::move(hashes);
    }

    qboolean PSODB::LoadCachedBlob(uint64_t hash, CachedPSO** cachedPSO)
    {
        char filename[MAX_PATH];
        sprintf_s(filename, "%s/%s/%llx%s", PSODB_PATH, m_name.c_str(), hash, PSODB_EXTENSION);

        void* blob = nullptr;
        int size = FS_ReadFile(filename, &blob);
        if (size <= 0 || !blob) {
            return qfalse;
        }

        // TODO: go through Q3's memory allocator?
        *cachedPSO = (CachedPSO*) new uint8_t[size];
        Com_Memcpy((void*) *cachedPSO, blob, size);

        FS_FreeFile(blob);
        return qtrue;
    }

    void PSODB::FreeCachedBlob(CachedPSO* cachedPSO)
    {
        delete [] cachedPSO;
    }

    static inline size_t BlobSize(ID3DBlob* blob)
    {
        return blob == nullptr ? 0 : blob->GetBufferSize();
    }

    static size_t SerializeData(std::vector<uint8_t>& v, const void* data, size_t size)
    {
        if (size == 0)
            return v.size();

        size_t offset = v.size();
        v.resize(offset + size);
        memcpy(v.data() + offset, data, size);
        return offset;
    }

    qboolean PSODB::CacheBlob(uint64_t hash, const PIPELINE_DRAW_STATE_DESC* desc, ID3D12PipelineState* pso)
    {
        ComPtr<ID3DBlob> blob;
        if (FAILED(pso->GetCachedBlob(blob.ReleaseAndGetAddressOf())))
        {
            return qfalse;
        }

        char filename[MAX_PATH];
        sprintf_s(filename, "%s/%s/%llx%s", PSODB_PATH, m_name.c_str(), hash, PSODB_EXTENSION);

        size_t totalSize = sizeof(PIPELINE_DRAW_STATE_DESC) + sizeof(size_t) + blob->GetBufferSize();

        CachedPSO* cpso = (CachedPSO*) new uint8_t[totalSize];
        memcpy(&cpso->Desc, desc, sizeof(*desc));
        memcpy(&cpso->BlobData, blob->GetBufferPointer(), blob->GetBufferSize());
        cpso->BlobSize = blob->GetBufferSize();

        FS_WriteFile(filename, cpso, (int)totalSize);

        delete [] cpso;

        return qtrue;
    }
}
