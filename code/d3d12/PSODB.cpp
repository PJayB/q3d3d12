#include "PipelineState.h"
#include "PSODB.h"

#ifdef _XBOX_ONE
#   define PSODB_PATH "psodb/xboxone"
#else
#   define PSODB_PATH "psodb/pc"
#endif
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

#ifdef _XBOX_ONE
    static void SerializeBlob(std::vector<uint8_t>& v, CachedPSO::BlobOffset& packet, ID3DBlob* blob)
    {
        if (blob == nullptr || blob->GetBufferSize() == 0)
        {
            packet.Offset = 0;
            packet.Size = 0;
            return;
        }

        size_t offset = SerializeData(
            v, 
            blob->GetBufferPointer(), 
            blob->GetBufferSize());

        packet.Offset = offset;
        packet.Size = blob->GetBufferSize();
    }
#endif

    qboolean PSODB::CacheBlob(uint64_t hash, const PIPELINE_DRAW_STATE_DESC* desc, ID3D12PipelineState* pso)
    {
#ifdef _XBOX_ONE
        D3D12XBOX_SERIALIZE_GRAPHICS_PIPELINE_STATE state;
        Device::Get()->SerializeGraphicsPipelineStateX(pso, D3D12XBOX_SERIALIZE_FLAGS_NONE, &state);

        static const size_t headerSize = (size_t) &((CachedPSO*)0)->BlobData;

        size_t totalSize = 
            headerSize +
            BlobSize(state.pPacket) +
            BlobSize(state.pMetaData) +
            BlobSize(state.PS.pGpuInstructions) +
            BlobSize(state.PS.pMetaData) +
            BlobSize(state.VS.pGpuInstructions) +
            BlobSize(state.VS.pMetaData) +
            BlobSize(state.GS.pGpuInstructions) +
            BlobSize(state.GS.pMetaData) +
            BlobSize(state.ES.pGpuInstructions) +
            BlobSize(state.ES.pMetaData) +
            BlobSize(state.HS.pGpuInstructions) +
            BlobSize(state.HS.pMetaData) +
            BlobSize(state.LS.pGpuInstructions) +
            BlobSize(state.LS.pMetaData);

        std::vector<uint8_t> blob;
        blob.reserve(totalSize);
        blob.resize(headerSize);

        // Get the CachedPSO header
        CachedPSO* header = (CachedPSO*) blob.data();
        header->Desc = *desc;
        SerializeBlob(blob, header->Packet, state.pPacket);
        SerializeBlob(blob, header->Meta,   state.pMetaData);
        SerializeBlob(blob, header->PS,     state.PS.pGpuInstructions);
        SerializeBlob(blob, header->PSMeta, state.PS.pMetaData);
        SerializeBlob(blob, header->VS,     state.VS.pGpuInstructions);
        SerializeBlob(blob, header->VSMeta, state.VS.pMetaData);
        SerializeBlob(blob, header->GS,     state.GS.pGpuInstructions);
        SerializeBlob(blob, header->GSMeta, state.GS.pMetaData);
        SerializeBlob(blob, header->ES,     state.ES.pGpuInstructions);
        SerializeBlob(blob, header->ESMeta, state.ES.pMetaData);
        SerializeBlob(blob, header->HS,     state.HS.pGpuInstructions);
        SerializeBlob(blob, header->HSMeta, state.HS.pMetaData);
        SerializeBlob(blob, header->LS,     state.LS.pGpuInstructions);
        SerializeBlob(blob, header->LSMeta, state.LS.pMetaData);

        // Serialize the file
        char filename[MAX_PATH];
        sprintf_s(filename, "%s/%s/%llx%s", PSODB_PATH, m_name.c_str(), hash, PSODB_EXTENSION);

        FS_WriteFile(filename, blob.data(), (int) blob.size());

        // Release the blobs
        SAFE_RELEASE(state.pPacket);
        SAFE_RELEASE(state.pMetaData);
        SAFE_RELEASE(state.PS.pGpuInstructions);
        SAFE_RELEASE(state.PS.pMetaData);
        SAFE_RELEASE(state.VS.pGpuInstructions);
        SAFE_RELEASE(state.VS.pMetaData);
        SAFE_RELEASE(state.GS.pGpuInstructions);
        SAFE_RELEASE(state.GS.pMetaData);
        SAFE_RELEASE(state.ES.pGpuInstructions);
        SAFE_RELEASE(state.ES.pMetaData);
        SAFE_RELEASE(state.HS.pGpuInstructions);
        SAFE_RELEASE(state.HS.pMetaData);
        SAFE_RELEASE(state.LS.pGpuInstructions);
        SAFE_RELEASE(state.LS.pMetaData);
#else
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
#endif

        return qtrue;
    }

#ifdef _XBOX_ONE
    void ConvertCachedPSOShaderBlobToState(
        const uint8_t* blob,
        const CachedPSO::BlobOffset* dataPacket,
        const CachedPSO::BlobOffset* metaPacket,
        D3D12XBOX_DESERIALIZE_SHADER* shader)
    {
        if (dataPacket->Size > 0)
        {
            shader->pGpuInstructions = (void*) &blob[dataPacket->Offset];
            shader->GpuInstructionsSize = dataPacket->Size;
        }
        if (metaPacket->Size > 0)
        {
            shader->pMetaData = (void*) &blob[metaPacket->Offset];
            shader->MetaDataSize = metaPacket->Size;
        }
    }

    void ConvertCachedPSOBlobToState(
        const CachedPSO* cachedPSO,
        ID3D12RootSignature* rootSig,
        D3D12XBOX_DESERIALIZE_GRAPHICS_PIPELINE_STATE* state)
    {
        *state = {}; // zero the memory

        state->pRootSignature = rootSig;
        state->pPacket = (void*) &cachedPSO->BlobData[cachedPSO->Packet.Offset];
        state->PacketSize = cachedPSO->Packet.Size;
        state->pMetaData = (void*) &cachedPSO->BlobData[cachedPSO->Meta.Offset];
        state->MetaDataSize = cachedPSO->Meta.Size;
        ConvertCachedPSOShaderBlobToState(cachedPSO->BlobData, &cachedPSO->PS, &cachedPSO->PSMeta, &state->PS);
        ConvertCachedPSOShaderBlobToState(cachedPSO->BlobData, &cachedPSO->VS, &cachedPSO->VSMeta, &state->VS);
        ConvertCachedPSOShaderBlobToState(cachedPSO->BlobData, &cachedPSO->GS, &cachedPSO->GSMeta, &state->GS);
        ConvertCachedPSOShaderBlobToState(cachedPSO->BlobData, &cachedPSO->ES, &cachedPSO->ESMeta, &state->ES);
        ConvertCachedPSOShaderBlobToState(cachedPSO->BlobData, &cachedPSO->HS, &cachedPSO->HSMeta, &state->HS);
        ConvertCachedPSOShaderBlobToState(cachedPSO->BlobData, &cachedPSO->LS, &cachedPSO->LSMeta, &state->LS);
    }
#endif
}
