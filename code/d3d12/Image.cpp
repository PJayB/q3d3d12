#include "Image.h"
#include "Upload.h"
#include "../d3d/DirectXTK/DDS.h"
#include "DirectXTK12/DDSTextureLoader.h"

namespace QD3D12
{
    Image Image::s_images[MAX_DRAWIMAGES];
    DescriptorArray Image::s_shaderVisibleResourceDH;
    DescriptorArray Image::s_shaderVisibleSamplerDH;
    DescriptorArray Image::s_copySourceResourceDH;
    DescriptorArray Image::s_copySourceSamplerDH;

	Image::Image()
		: m_width(0)
		, m_height(0)
		, m_lastFrameUsed(0)
		, m_format(DXGI_FORMAT_UNKNOWN)
		, m_resource(nullptr)
		, m_uploadResource(nullptr)
	{

	}

	Image::~Image()
	{
		assert(m_resource == nullptr);
	}

	void Image::InitOne(
        ID3D12Resource* resource,
        ID3D12Resource* uploadResource,
        UINT index,
        UINT width,
        UINT height,
        UINT mipLevels,
        DXGI_FORMAT format )
	{
        // Grab handles into the descriptor arrays
        m_shaderVisibleSRV = s_shaderVisibleResourceDH.GetGpuHandle(index);
        m_shaderVisibleSampler = s_shaderVisibleSamplerDH.GetGpuHandle(index);
        m_copySourceSRV = s_copySourceResourceDH.GetCpuHandle(index);
        m_copySourceSampler = s_copySourceSamplerDH.GetCpuHandle(index);

        m_width = width;
        m_height = height;
        m_mipLevels = mipLevels;
        m_format = format;
        m_lastFrameUsed = 0;
        m_resource = resource;
        m_uploadResource = uploadResource;
	}

	void Image::DestroyOne()
	{
		SAFE_RELEASE(m_resource);
		SAFE_RELEASE(m_uploadResource);

		m_width = 0;
		m_height = 0; 
        m_mipLevels = 0;
		m_lastFrameUsed = 0;
		m_format = DXGI_FORMAT_UNKNOWN;
	}

	void Image::Update(
		_In_ ID3D12GraphicsCommandList* pCmdList,
		_In_ const void* pImageData)
	{
		assert(IsDynamic());
		assert(m_mipLevels == 1);

		D3D12_SUBRESOURCE_DATA subres;
		subres.pData = pImageData;
		subres.RowPitch = m_width * sizeof(UINT);
		subres.SlicePitch = subres.RowPitch * m_height;
		Upload::SingleSubresourceDynamic(
			pCmdList,
			Resource(),
			m_uploadResource,
			0,
			&subres,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void Image::Init()
	{
        s_shaderVisibleResourceDH.Init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, MAX_DRAWIMAGES);
        s_shaderVisibleSamplerDH.Init(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, MAX_DRAWIMAGES);
        s_copySourceResourceDH.Init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, MAX_DRAWIMAGES);
        s_copySourceSamplerDH.Init(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, MAX_DRAWIMAGES);
    }

	void Image::Destroy()
	{
        for (UINT i = 0; i < _countof(s_images); ++i)
		{
			s_images[i].DestroyOne();
		}

        s_shaderVisibleResourceDH.Destroy();
        s_shaderVisibleSamplerDH.Destroy();
        s_copySourceResourceDH.Destroy();
        s_copySourceSamplerDH.Destroy();
	}

    void Image::CreateImageSRV(
        ID3D12Device* pDevice,
        ID3D12Resource* pResource,
        DXGI_FORMAT format,
        UINT mipLevels,
        UINT index )
    {
		// Create a shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = mipLevels;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // Create shader-visible SRV
		pDevice->CreateShaderResourceView(
			pResource,
			&srvDesc,
			s_shaderVisibleResourceDH.GetCpuHandle(index));

        // Create copy-source SRV
		pDevice->CreateShaderResourceView(
			pResource,
			&srvDesc,
			s_copySourceResourceDH.GetCpuHandle(index));
    }

    void Image::CreateImageSampler(
        ID3D12Device* pDevice,
        BOOL mipMap,
        wrapClampMode_t wcMode,
        UINT index )
    {
		// Create the sampler
		D3D12_SAMPLER_DESC samplerDesc;
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));

		if (mipMap)
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		else
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;

		switch (wcMode)
		{
		case WRAPMODE_CLAMP:
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		case WRAPMODE_REPEAT:
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		}

		samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        // Create shader visible and copy source samplers
        pDevice->CreateSampler(&samplerDesc, s_shaderVisibleSamplerDH.GetCpuHandle(index));
        pDevice->CreateSampler(&samplerDesc, s_copySourceSamplerDH.GetCpuHandle(index));
    }

    ID3D12Resource* Image::CreateImageResource(
        ID3D12Device* pDevice,
        const char* imgName,
        UINT width,
        UINT height,
        UINT mipLevels,
        DXGI_FORMAT format )
    {
		// Create the texture
		D3D12_RESOURCE_DESC dsd;
		ZeroMemory(&dsd, sizeof(dsd));
		dsd.Width = width;
		dsd.Height = height;
		dsd.MipLevels = mipLevels;
		dsd.DepthOrArraySize = 1;
		dsd.Format = format;
		dsd.SampleDesc.Count = 1;
		dsd.SampleDesc.Quality = 0;
		dsd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        ID3D12Resource* pResource = nullptr;

		COM_ERROR_TRAP(pDevice->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&dsd,
			QD3D12_RESOURCE_STATE_INITIAL,
            nullptr, // Optimized clear value
			__uuidof(ID3D12Resource*),
			(void**)&pResource));

		// Set the name of the resource
		SetResourceName(pResource, imgName);

        return pResource;
    }

    void Image::Load(
		_In_ const image_t* image,
		_In_ int mipLevels,
		_In_ const byte* pic )
	{
		// Choose the internal format
		// @pjb: because of the way q3 loads textures, they're all RGBA.
		const DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// Re-allocate space for the image and light scale it
		size_t imageSizeBytes = image->width * image->height * sizeof(UINT);
		void* lightscaledCopy = RI_AllocateTempMemory((int)imageSizeBytes);
		memcpy(lightscaledCopy, pic, imageSizeBytes);

		// Now generate the mip levels
		int mipWidth = image->width;
		int mipHeight = image->height;
		D3D12_SUBRESOURCE_DATA* subres = (D3D12_SUBRESOURCE_DATA*)RI_AllocateTempMemory((int) sizeof(D3D12_SUBRESOURCE_DATA) * mipLevels);
		for (int mip = 0; mip < mipLevels; ++mip)
		{
			if (mip != 0) {
				// We downsample the lightscaledCopy each time
				R_MipMap((byte*)lightscaledCopy, mipWidth, mipHeight);
				mipWidth = max(1, mipWidth >> 1);
				mipHeight = max(1, mipHeight >> 1);
			}

			int mipSize = mipWidth * mipHeight * sizeof(UINT);

			// Set up the resource sizes and memory
			subres[mip].pData = RI_AllocateTempMemory(mipSize);
			subres[mip].RowPitch = mipWidth * sizeof(UINT);
			subres[mip].SlicePitch = subres[mip].RowPitch;

			// Copy the mip data over
			memcpy((void*)subres[mip].pData, lightscaledCopy, mipSize);
		}

		ID3D12Device* pDevice = Device::Get();

        ID3D12Resource* pResource = CreateImageResource(
            pDevice,
            image->imgName,
            image->width,
            image->height, 
            mipLevels,
            format );

		Upload::Subresources(
			pResource, 
			0, 
			subres, 
			mipLevels,
			QD3D12_RESOURCE_STATE_INITIAL,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
		for (int mip = mipLevels - 1; mip >= 0; --mip)
			RI_FreeTempMemory((void*)subres[mip].pData);
		RI_FreeTempMemory(subres);

		RI_FreeTempMemory(lightscaledCopy);

        CreateImageSRV(
            pDevice,
            pResource, 
            format,
            mipLevels,
            image->index );

        CreateImageSampler(
            pDevice,
            mipLevels > 1,
            image->wrapClampMode,
            image->index );
		
		// All done.
		Image* d3dImage = Get(image->index);
		d3dImage->InitOne(
            pResource,
            nullptr,
            image->index,
            image->width,
            image->height,
            mipLevels,
            format );
	}

    void Image::LoadDynamic(
		_In_ const image_t* image,
		_In_ int width,
		_In_ int height,
		_In_ const byte* pic )
	{
		// Choose the internal format
		// @pjb: because of the way q3 loads textures, they're all RGBA.
		const DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// Set up the resource sizes and memory
		D3D12_SUBRESOURCE_DATA subres;
		subres.pData = pic;
		subres.RowPitch = width * sizeof(UINT);
		subres.SlicePitch = subres.RowPitch;

		ID3D12Device* pDevice = Device::Get();
        ID3D12Resource* pResource = nullptr;
        ID3D12Resource* pUploadResource = nullptr;

        pResource = CreateImageResource(
            pDevice,
            image->imgName,
            width,
            height, 
            1,
            format );

		Upload::Subresources(
			pResource, 
			0, 
			&subres, 
			1,
			QD3D12_RESOURCE_STATE_INITIAL,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
		UINT64 uploadSize = GetRequiredIntermediateSize(
			pResource,
			0,
			1);

		CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

		COM_ERROR_TRAP(pDevice->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, // Optimized clear value
			__uuidof(ID3D12Resource*),
			(void**)&pUploadResource));

		// Set the name of the resource
		SetResourceName(pUploadResource, "%s (upload)", image->imgName);

        CreateImageSRV(
            pDevice,
            pResource, 
            format,
            1,
            image->index );

        CreateImageSampler(
            pDevice,
            qfalse,
            image->wrapClampMode,
            image->index );
		
		// All done.
		Image* d3dImage = Get(image->index);
		d3dImage->InitOne(
            pResource,
            pUploadResource,
            image->index,
            width,
            height,
            1,
            format );
	}

	void Image::LoadDDS(
		_In_ const image_t* image,
		_In_ const byte* ddsData )
	{
        using namespace DirectX;
        ID3D12Device* pDevice = Device::Get();

        // Get the texture data
        const DDS_HEADER* pDDSHeader = nullptr;
        const uint8_t* pDDSData = nullptr;
        HRESULT hr = GetTextureHeadersFromDDS(
            ddsData,
            &pDDSHeader,
            &pDDSData );
        if ( FAILED(hr) )
        {
            Com_Error( ERR_DROP, "LoadDDS(%s): failed to get DDS headers: %s.\n", 
                image->imgName, QD3D::HResultToString(hr) );
        }

        // Get the image information from the DDS file
        DDS_DESC ddsDesc;
        ZeroMemory( &ddsDesc, sizeof(ddsDesc) );
        hr = GetTextureInfoFromDDS( 
            pDDSHeader, 
            0, 
            &ddsDesc );
        if ( FAILED(hr) )
        {
            Com_Error( ERR_DROP, "LoadDDS(%s): failed to get DDS info: %s.\n", 
                image->imgName, QD3D::HResultToString(hr) );
        }

        // Now get the DDS image data
		D3D12_SUBRESOURCE_DATA* subres = (D3D12_SUBRESOURCE_DATA*)RI_AllocateTempMemory( (int) sizeof(D3D12_SUBRESOURCE_DATA) * ddsDesc.MipCount );

        hr = GetTextureDataFromDDS(
            &ddsDesc,
            pDDSData,
            subres );
        if ( FAILED(hr) )
        {
            Com_Error( ERR_DROP, "LoadDDS(%s): failed to get DDS image data: %s.\n", 
                image->imgName, QD3D::HResultToString(hr) );
        }

        ID3D12Resource* pResource = CreateImageResource(
            pDevice,
            image->imgName,
            ddsDesc.Width,
            ddsDesc.Height, 
            ddsDesc.MipCount,
            ddsDesc.Format );

		Upload::Subresources(
			pResource, 
			0, 
			subres, 
			ddsDesc.MipCount,
			QD3D12_RESOURCE_STATE_INITIAL,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
		RI_FreeTempMemory(subres);

        CreateImageSRV(
            pDevice,
            pResource, 
            ddsDesc.Format,
            ddsDesc.MipCount,
            image->index );

        CreateImageSampler(
            pDevice,
            ddsDesc.MipCount > 1,
            image->wrapClampMode,
            image->index );
		
		// All done.
		Image* d3dImage = Get(image->index);
		d3dImage->InitOne(
            pResource,
            nullptr,
            image->index,
            ddsDesc.Width,
            ddsDesc.Height, 
            ddsDesc.MipCount,
            ddsDesc.Format );
    }

	void Image::Delete(
		_In_ const image_t* image)
	{
		Image* d3dImage = Get(image->index);
		d3dImage->DestroyOne();
	}

	const Image* Image::GetAnimatedImage(
		const textureBundle_t *bundle, 
		float shaderTime) {
		int		index;

		PROFILE_FUNC();
		if (bundle->isVideoMap) {
			CIN_RunCinematic(bundle->videoMapHandle);
			CIN_UploadCinematic(bundle->videoMapHandle);
			return Get(tr.scratchImage[bundle->videoMapHandle]->index);
		}

		if (bundle->numImageAnimations <= 1) {
			return Get(bundle->image[0]->index);
		}

		// it is necessary to do this messy calc to make sure animations line up
		// exactly with waveforms of the same frequency
		index = myftol(shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
		index >>= FUNCTABLE_SIZE2;

		if (index < 0) {
			index = 0;	// may happen with shader time offsets
		}
		index %= bundle->numImageAnimations;

		return Get(bundle->image[index]->index);
	}

    BOOL Image::IsBlobDDS(
        _In_ const void* pData )
    {
        using namespace DirectX;

        const DWORD* pDwords = reinterpret_cast<const DWORD*>( pData );

        // Check the magic header
        if ( *pDwords != DDS_MAGIC )
            return FALSE;

        // Validate the header
        const DDS_HEADER* pHeader = reinterpret_cast<const DDS_HEADER*>( pDwords + 1 );
        if ( pHeader->size != sizeof(DDS_HEADER) ||
             pHeader->ddspf.size != sizeof(DDS_PIXELFORMAT) )
             return FALSE;

        return TRUE;
    }
}
