#pragma once

#include "DescriptorHeaps.h"

namespace QD3D12
{


	class Image
	{
		// --- Instance members ---
	private:
		UINT m_width;
		UINT m_height;
		UINT m_mipLevels;
		UINT m_lastFrameUsed;
		DXGI_FORMAT m_format;
        D3D12_GPU_DESCRIPTOR_HANDLE m_shaderVisibleSRV; // shader visible SRV
        D3D12_GPU_DESCRIPTOR_HANDLE m_shaderVisibleSampler; // shader visible sampler
        D3D12_CPU_DESCRIPTOR_HANDLE m_copySourceSRV; // copy source SRV
        D3D12_CPU_DESCRIPTOR_HANDLE m_copySourceSampler; // copy source sampler

		ID3D12Resource* m_resource;
		ID3D12Resource* m_uploadResource;

		Image();
		~Image();

		void InitOne(
            ID3D12Resource* resource,
            ID3D12Resource* uploadResource,
            UINT index,
            UINT width,
            UINT height,
            UINT mipLevels,
            DXGI_FORMAT format );
		void DestroyOne();

	public:
		inline ID3D12Resource* Resource() const { return m_resource; }
		inline D3D12_GPU_DESCRIPTOR_HANDLE ShaderVisibleSRV() const { return m_shaderVisibleSRV; }
		inline D3D12_GPU_DESCRIPTOR_HANDLE ShaderVisibleSampler() const { return m_shaderVisibleSampler; }
		inline D3D12_CPU_DESCRIPTOR_HANDLE CopySourceSRV() const { return m_copySourceSRV; }
		inline D3D12_CPU_DESCRIPTOR_HANDLE CopySourceSampler() const { return m_copySourceSampler; }

		inline UINT Width() const { return m_width; }
		inline UINT Height() const { return m_height; }
		inline BOOL IsDynamic() const { return m_uploadResource != nullptr; }
		inline DXGI_FORMAT Format() const { return m_format; }

		void Update(
			_In_ ID3D12GraphicsCommandList* pCmdList,
			_In_ const void* pImageData);

		// --- Static members ---
	private:
		static Image s_images[MAX_DRAWIMAGES];
        static DescriptorArray s_shaderVisibleResourceDH;
        static DescriptorArray s_shaderVisibleSamplerDH;
        static DescriptorArray s_copySourceResourceDH;
		static DescriptorArray s_copySourceSamplerDH;

        static ID3D12Resource* CreateImageResource(
            ID3D12Device* pDevice,
            const char* imgName,
            UINT width,
            UINT height,
            UINT mipLevels,
            DXGI_FORMAT format );

        static void CreateImageSRV(
            ID3D12Device* pDevice,
            ID3D12Resource* pResource,
            DXGI_FORMAT format,
            UINT mipLevels,
            UINT index );

        static void CreateImageSampler(
            ID3D12Device* pDevice,
            BOOL mipMap,
            wrapClampMode_t wcMode,
            UINT index );

	public:
		static void Init();
		static void Destroy();

		static inline Image* Get(UINT index)
		{
			assert(index < _countof(s_images));
			return &s_images[index];
		}

		static const Image* GetAnimatedImage(
			const textureBundle_t *bundle,
			float shaderTime);

		static void Load(
			_In_ const image_t* image,
			_In_ int mipLevels,
			_In_ const byte* pic);

		static void LoadDynamic(
			_In_ const image_t* image,
			_In_ int width,
			_In_ int height,
			_In_ const byte* pic);

		static void LoadDDS(
			_In_ const image_t* image,
			_In_ const byte* pic);

		static void Delete(
			_In_ const image_t* image);

        // Returns true if the data provided is a valid DDS
        static BOOL IsBlobDDS( 
            _In_ const void* pData );

        static inline ID3D12DescriptorHeap* ShaderVisibleResourceHeap() { return s_shaderVisibleResourceDH.Heap(); }
        static inline ID3D12DescriptorHeap* ShaderVisibleSamplerHeap()  { return s_shaderVisibleSamplerDH.Heap(); }
        static inline ID3D12DescriptorHeap* CopySourceResourceHeap()    { return s_copySourceResourceDH.Heap(); }
        static inline ID3D12DescriptorHeap* CopySourceSamplerHeap()     { return s_copySourceSamplerDH.Heap(); }
    };
}
