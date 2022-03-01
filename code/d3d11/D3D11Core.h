#pragma once

#include "..\d3d\Direct3DCommon.h"

#ifdef _WIN32_WINNT_WINBLUE
#   include <d3d11_2.h>
#   define QD3D11Device     ID3D11Device2
#else
#   include <d3d11_1.h>
#   define QD3D11Device     ID3D11Device1
#endif

#ifndef V_SOFT
#   define V_SOFT(x)       { HRESULT _hr=(x); if (FAILED(_hr)) g_hrLastError = _hr; }
#endif

#ifndef V
#   define V(x)            { HRESULT _hr=(x); if (FAILED(_hr)) Com_Error( ERR_FATAL, "Direct3D Error 0x%08X: %s", _hr, #x); }
#endif

#ifndef V_RETURN
#	define V_RETURN(x) { HRESULT hr = (x); assert(SUCCEEDED(hr)); }
#endif

namespace QD3D11
{
	//----------------------------------------------------------------------------
	// Returns true if the debug layers are available
	//----------------------------------------------------------------------------
    BOOL
    IsSdkDebugLayerAvailable();

	//----------------------------------------------------------------------------
	// Creates a device with the default settings and returns the maximum feature 
	// level
	//----------------------------------------------------------------------------
	HRESULT 
	CreateDefaultDevice(
		_In_ D3D_DRIVER_TYPE driver, 
		_Out_ QD3D11Device** device,
		_Out_ ID3D11DeviceContext1** context,
		_Out_ D3D_FEATURE_LEVEL* featureLevel);

	//----------------------------------------------------------------------------
	// Creates a default 8-bit swap chain description with no MSAA
	//----------------------------------------------------------------------------
	void
	GetDefaultSwapChainDesc(
		_Out_ DXGI_SWAP_CHAIN_DESC1* swapChainDesc);

	//----------------------------------------------------------------------------
	// Creates a multisampled swap chain description.
	// Automatically selects the highest possible quality level for the MSAA.
	//----------------------------------------------------------------------------
	HRESULT 
	GetMultiSampledSwapChainDesc(
		_In_ QD3D11Device* device,
		_In_ UINT multiSampleCount,
		_Out_ DXGI_SWAP_CHAIN_DESC1* swapChainDesc);

	//----------------------------------------------------------------------------
	// Returns the highest possible quality swap chain description, starting with
	// full MSAA and working downwards until the driver accepts the input values.
	// 
	// This will try 16x MSAA, 8x, 4x, 2x and no MSAA. It is guaranteed to return
	// a valid swap chain description.
	//----------------------------------------------------------------------------
	void 
	GetBestQualitySwapChainDesc(
		_In_ QD3D11Device* device,
		_Out_ DXGI_SWAP_CHAIN_DESC1* swapChainDesc);
	
    //----------------------------------------------------------------------------
	// Gets the DXGI device
	//----------------------------------------------------------------------------
    HRESULT GetDxgiDevice( 
        _In_ QD3D11Device* device, 
        _Out_ QDXGIDevice** dxgiDevice );

    //----------------------------------------------------------------------------
	// Gets the DXGI adapter
	//----------------------------------------------------------------------------
    HRESULT GetDxgiAdapter( 
        _In_ QD3D11Device* device, 
        _Out_ IDXGIAdapter** dxgiAdapter );

    //----------------------------------------------------------------------------
	// Gets the DXGI factory
	//----------------------------------------------------------------------------
    HRESULT GetDxgiFactory( 
        _In_ QD3D11Device* device, 
        _Out_ IDXGIFactory2** dxgiFactory );

	//----------------------------------------------------------------------------
	// Creates a swap chain
	//----------------------------------------------------------------------------
	HRESULT
	CreateSwapChain(
		_In_ QD3D11Device* device,
        _In_ HWND hWnd,
		_In_ const DXGI_SWAP_CHAIN_DESC1* scd,
        _In_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fsd,
		_Out_ IDXGISwapChain1** swapChain);

	//----------------------------------------------------------------------------
	// Extracts the back buffer from the swap chain
	//----------------------------------------------------------------------------
	HRESULT 
	GetBackBuffer(
		_In_ IDXGISwapChain* swapChain,
		_Out_ ID3D11Texture2D** backBufferTexture);

	//----------------------------------------------------------------------------
	// Fills a viewport struct for a render target.
	// This is for convenience only; you shouldn't call this every frame.
	//----------------------------------------------------------------------------
	HRESULT
	GetRenderTargetViewport(
		_In_ ID3D11RenderTargetView* pRTV,
		_Out_ D3D11_VIEWPORT* pViewport);

    //----------------------------------------------------------------------------
    // Returns the number of bits in the particular pixel format
   	//----------------------------------------------------------------------------
    HRESULT
    GetBitDepthForFormat(
        _In_ DXGI_FORMAT fmt,
        _Out_ DWORD* bits );

    //----------------------------------------------------------------------------
    // Returns the number of bits in the particular pixel format
   	//----------------------------------------------------------------------------
    HRESULT
    GetBitDepthForDepthStencilFormat(
        _In_ DXGI_FORMAT fmt,
        _Out_ DWORD* depthBits,
        _Out_ DWORD* stencilBits );

    //----------------------------------------------------------------------------
    // Helper function for creating 2D textures
   	//----------------------------------------------------------------------------
	ID3D11Texture2D* CreateTexture2D(
        _In_ QD3D11Device* device, 
        _In_ UINT width,
        _In_ UINT height, 
        _In_ DXGI_FORMAT t2d_format, 
        _In_ LPCVOID pData,
        _In_opt_ UINT mipLevels = 1,
        _In_opt_ UINT msaaSamples = 1, 
        _In_opt_ UINT msaaQuality = 0, 
        _In_opt_ UINT bindFlags = D3D11_BIND_SHADER_RESOURCE );

    //----------------------------------------------------------------------------
    // Helper function for creating 3D textures
   	//----------------------------------------------------------------------------
	ID3D11Texture3D* CreateTexture3D(
        _In_ QD3D11Device* device, 
        _In_ UINT width, 
        _In_ UINT height, 
        _In_ UINT depth, 
        _In_ DXGI_FORMAT t3d_format, 
        _In_ LPCVOID pData,
        _In_opt_ UINT bindFlags = D3D11_BIND_SHADER_RESOURCE );

    //----------------------------------------------------------------------------
    // Helper function for creating a view of the back buffer
   	//----------------------------------------------------------------------------
	ID3D11RenderTargetView* CreateBackBufferView(
        _In_ IDXGISwapChain1* swapChain,
        _In_ QD3D11Device* device, 
        _Out_opt_ D3D11_TEXTURE2D_DESC* optionalOut_BBDesc = NULL);

    //----------------------------------------------------------------------------
    // Helper function for creating a view of a depth buffer
   	//----------------------------------------------------------------------------
	ID3D11DepthStencilView* CreateDepthBufferView(
        _In_ QD3D11Device* device, 
        _In_ UINT width, 
        _In_ UINT height, 
        _In_ DXGI_FORMAT t2d_format, 
        _In_ DXGI_FORMAT dsv_format, 
        _In_opt_ UINT msaaSamples = 1, 
        _In_opt_ UINT msaaQuality = 0, 
        _In_opt_ UINT bindFlags = 0);

    //----------------------------------------------------------------------------
    // Helper function for creating a view of a render target
   	//----------------------------------------------------------------------------
	ID3D11RenderTargetView* CreateRenderTargetView(
        _In_ QD3D11Device* device, 
        _In_ UINT width, 
        _In_ UINT height, 
        _In_ DXGI_FORMAT t2d_format, 
        _In_ DXGI_FORMAT rtv_format, 
        _In_opt_ UINT msaaSamples = 1, 
        _In_opt_ UINT msaaQuality = 0, 
        _In_opt_ UINT bindFlags = 0);

    //----------------------------------------------------------------------------
    // Helper function for creating a view of a 2D render target
   	//----------------------------------------------------------------------------
	ID3D11RenderTargetView* CreateTexture2DRenderTargetView(
        _In_ QD3D11Device* device, 
        _In_ ID3D11Texture2D* texture, 
        _In_ DXGI_FORMAT rtv_format);

    //----------------------------------------------------------------------------
    // Helper function for creating a view of a 2D depth buffer
   	//----------------------------------------------------------------------------
	ID3D11DepthStencilView* CreateTexture2DDepthBufferView(
        _In_ QD3D11Device* device, 
        _In_ ID3D11Texture2D* texture, 
        _In_ DXGI_FORMAT dsv_format);

    //----------------------------------------------------------------------------
    // Helper function for creating a view of a 2D texture
   	//----------------------------------------------------------------------------
	ID3D11ShaderResourceView* CreateTexture2DShaderResourceView(
        _In_ QD3D11Device* device, 
        _In_ ID3D11Texture2D* texture, 
        _In_ DXGI_FORMAT srv_format);

	//----------------------------------------------------------------------------
	// Creates a buffer full of GPU-read CPU-no-access fixed data. 
	//----------------------------------------------------------------------------
	ID3D11Buffer* 
	CreateImmutableBuffer(
		_In_ QD3D11Device* device,
		_In_ UINT bindFlags, 
		_In_count_(size) const void* data, 
		_In_ size_t size);

	//----------------------------------------------------------------------------
	// Quick helper function for creating a CPU writable buffer of a given size 
	// and type. This will be created with the DYNAMIC usage flags and writable
	// by CPU.
    //
	// This function will also ASSERT if your structure is not a multiple of 16
	// bytes in size.
	//
    // Only use this for small buffers as this allocates on the stack.
	//
	// Usage: 
	//	
	//	struct Foo { ... };
	//
	//	ID3D11Buffer* buffer = DirectX::CreateDynamicBuffer<Foo>(pDevice, D3D11_BIND_VERTEX_BUFFER, nElements);
	//
	//----------------------------------------------------------------------------
	template<class Struct>
	ID3D11Buffer* 
	CreateDynamicBuffer(
		_In_ QD3D11Device* device, 
		_In_ UINT bindFlags,
		_In_ UINT count)
	{
        ASSERT( ( ( count * sizeof(Struct) ) & 15 ) == 0 );

        const size_t c_MaxStackThreshold = 256;

        Struct* default_constants = nullptr;

        // Allocate on the stack if it's small enough
        if (sizeof(Struct) * count <= c_MaxStackThreshold)
        {
		    default_constants = (Struct*)_alloca(sizeof(Struct) * count);
		    for (UINT i = 0; i < count; ++i)
			    default_constants[i] = Struct();
        }
        else
            default_constants = new Struct[count];

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(Struct) * count;
		bd.BindFlags = bindFlags;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	
		D3D11_SUBRESOURCE_DATA srd;
		ZeroMemory(&srd, sizeof(srd));

		srd.pSysMem = default_constants;

		ID3D11Buffer* buffer;
		device->CreateBuffer(&bd, &srd, &buffer);

		ASSERT(buffer);

        if (sizeof(Struct) * count > c_MaxStackThreshold)
            delete [] default_constants;

		return buffer;
	}

	//----------------------------------------------------------------------------
	// Quick helper function for creating a dynamic buffer of a given size 
	// and type. This function only allocates space for ONE entry.
	//
	// This function will also ASSERT if your structure is not a multiple of 16
	// bytes in size.
	//
    // Only use this for small buffers as this allocates on the stack.
	//
	// Usage: 
	//	
	//	struct Foo { ... };
	//
	//	ID3D11Buffer* constantBuffer = DirectX::CreateDynamicBuffer<Foo>(pDevice);
	//
	//----------------------------------------------------------------------------
	template<class Struct>
	ID3D11Buffer* 
	CreateDynamicBuffer(
		_In_ QD3D11Device* device, 
		_In_ UINT bindFlags)
	{
		return CreateDynamicBuffer<Struct>(device, bindFlags, 1);
	}

	//----------------------------------------------------------------------------
	// Shorthand for mapping a dynamic buffer and discard the current contents
	//----------------------------------------------------------------------------
	template<class Struct>
	Struct* 
	MapDynamicBuffer(
		_In_ ID3D11DeviceContext* context, 
		_In_ ID3D11Buffer* buffer)
	{
		D3D11_MAPPED_SUBRESOURCE map;
		context->Map( buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		return (Struct*)map.pData;
	}

	//----------------------------------------------------------------------------
	// Shorthand for mapping, copying and unmapping a dynamic buffer.
	//----------------------------------------------------------------------------
	template<class Struct>
	BOOL 
	UploadDynamicBuffer(
		_In_ ID3D11DeviceContext* context, 
		_In_ ID3D11Buffer* buffer, 
		_In_ UINT count, 
		_In_ const Struct* structs)
	{
		// optionally don't block using D3D11_MAP_FLAG_DO_NOT_WAIT for second to last parameter

		D3D11_MAPPED_SUBRESOURCE map;
		if (FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
		{
			return FALSE;
		}

		memcpy(map.pData, structs, sizeof(Struct) * count);
		context->Unmap(buffer, 0);

		return TRUE;
	}

	//----------------------------------------------------------------------------
    // A dynamic buffer
	//----------------------------------------------------------------------------
    template<class T>
    class DynamicBuffer
    {
    public:

        DynamicBuffer(
            _In_ QD3D11Device* pDevice,
            _In_ UINT bindFlags);
        DynamicBuffer(
            _In_ QD3D11Device* pDevice,
            _In_ UINT bindFlags, 
            _In_ UINT count);
        ~DynamicBuffer();

        UINT Size() const { return m_count; }

        T* MapDiscard(
            _In_ ID3D11DeviceContext* context);
        void Unmap(
            _In_ ID3D11DeviceContext* context);

        BOOL UploadDiscard(
            _In_ ID3D11DeviceContext* context,
            _In_ const T* data);

		ID3D11Buffer* Buffer() const { return m_buffer; }
        
    private:

        UINT m_count;
        ID3D11Buffer* m_buffer;
    };

	//----------------------------------------------------------------------------
    // Dynamic buffer methods
	//----------------------------------------------------------------------------
    template<class T> 
    DynamicBuffer<T>::DynamicBuffer(
        _In_ QD3D11Device* pDevice,
        _In_ UINT bindFlags)
    {
        m_count = 1;
        m_buffer = CreateDynamicBuffer<T>(pDevice, bindFlags);
    }

    template<class T> 
    DynamicBuffer<T>::DynamicBuffer(
        _In_ QD3D11Device* pDevice,
        _In_ UINT bindFlags,
        _In_ UINT count)
    {
        m_count = count;
        m_buffer = CreateDynamicBuffer<T>(pDevice, bindFlags, count);
    }

    template<class T> 
    DynamicBuffer<T>::~DynamicBuffer()
    {
        SAFE_RELEASE(m_buffer);
    }

    template<class T> 
    T* 
    DynamicBuffer<T>::MapDiscard(
        _In_ ID3D11DeviceContext* context)
    {
        return MapDynamicBuffer<T>(context, m_buffer);
    }

    template<class T>
    void 
    DynamicBuffer<T>::Unmap(
        _In_ ID3D11DeviceContext* context)
    {
        context->Unmap(m_buffer);
    }

    template<class T>
    BOOL
    DynamicBuffer<T>::UploadDiscard(
        _In_ ID3D11DeviceContext* context,
        _In_ const T* data)
    {
        return UploadDynamicBuffer<T>(
            context,
            m_buffer,
            m_count,
            data);
    }

    //----------------------------------------------------------------------------
	// 
	//----------------------------------------------------------------------------
	class SavedRasterizerState
	{
	private:

		ID3D11DeviceContext* m_Context;
		ID3D11RasterizerState* m_PrevState;
		
	public:

		inline SavedRasterizerState(
			_In_ ID3D11DeviceContext* context)
			: m_Context(SAFE_ADDREF(context))
		{
			m_Context->RSGetState(&m_PrevState);
		}

		inline ~SavedRasterizerState()
		{
			m_Context->RSSetState(m_PrevState);
			SAFE_RELEASE(m_PrevState);
			SAFE_RELEASE(m_Context);
		}
	};

	//----------------------------------------------------------------------------
	// 
	//----------------------------------------------------------------------------
	class ScopedRasterizerState
	{
	private:

		SavedRasterizerState m_PrevState;
		
	public:

		inline ScopedRasterizerState(
			_In_ ID3D11DeviceContext* context, 
			_In_opt_ ID3D11RasterizerState* state)
			: m_PrevState(context)
		{
			context->RSSetState(state);
		}
	};

	//----------------------------------------------------------------------------
	// 
	//----------------------------------------------------------------------------
	class SavedBlendState
	{
	private:

		ID3D11DeviceContext* m_Context;
		ID3D11BlendState* m_PrevState;
		float m_PrevBlendFactor[4];
		UINT m_PrevMask;
		
	public:

		inline SavedBlendState(
			_In_ ID3D11DeviceContext* context)
			: m_Context(SAFE_ADDREF(context))
		{
			m_Context->OMGetBlendState(&m_PrevState, m_PrevBlendFactor, &m_PrevMask);
		}

		inline ~SavedBlendState()
		{
			m_Context->OMSetBlendState(m_PrevState, m_PrevBlendFactor, m_PrevMask);
			SAFE_RELEASE(m_PrevState);
			SAFE_RELEASE(m_Context);
		}
	};

	//----------------------------------------------------------------------------
	// 
	//----------------------------------------------------------------------------
	class ScopedBlendState
	{
	private:

		SavedBlendState m_PrevState;
		
	public:

		inline ScopedBlendState(
			_In_ ID3D11DeviceContext* context, 
			_In_opt_ ID3D11BlendState* state, 
			_In_opt_ const FLOAT* blendFactors, 
			_In_ UINT sampleMask)
			: m_PrevState(SAFE_ADDREF(context))
		{
			context->OMSetBlendState(state, blendFactors, sampleMask);
		}

		inline ScopedBlendState(
			_In_ ID3D11DeviceContext* context, 
			_In_opt_ ID3D11BlendState* state)
			: m_PrevState(SAFE_ADDREF(context))
		{
			const FLOAT blendFactors[] = {1, 1, 1, 1};
			const UINT sampleMask = 0xFFFFFFFF;
			context->OMSetBlendState(state, blendFactors, sampleMask);
		}
	};

	//----------------------------------------------------------------------------
	// 
	//----------------------------------------------------------------------------
	class SavedDepthStencilState
	{
	private:

		ID3D11DeviceContext* m_Context;
		ID3D11DepthStencilState* m_PrevState;
		UINT m_PrevMask;
		
	public:

		inline SavedDepthStencilState(
			_In_ ID3D11DeviceContext* context)
			: m_Context(SAFE_ADDREF(context))
		{
			m_Context->OMGetDepthStencilState(&m_PrevState, &m_PrevMask);
		}

		inline ~SavedDepthStencilState()
		{
			m_Context->OMSetDepthStencilState(m_PrevState, m_PrevMask);
			SAFE_RELEASE(m_PrevState);
			SAFE_RELEASE(m_Context);
		}
	};

	//----------------------------------------------------------------------------
	// 
	//----------------------------------------------------------------------------
	class ScopedDepthStencilState
	{
	private:

		SavedDepthStencilState m_PrevState;
		
	public:

		inline ScopedDepthStencilState(
			_In_ ID3D11DeviceContext* context, 
			_In_opt_ ID3D11DepthStencilState* state, 
			_In_ UINT stencilRef)
			: m_PrevState(SAFE_ADDREF(context))
		{
			context->OMSetDepthStencilState(state, stencilRef);
		}
	};

}

extern "C" 
{
    void D3D11API_Shutdown( void );
    void D3D11API_UnbindResources( void );
    size_t D3D11API_LastError( void );

    void D3D11API_CreateImage( const image_t* image, const byte *pic, qboolean isLightmap );
    void D3D11API_DeleteImage( const image_t* image );
    void D3D11API_CreateShader( const shader_t* shader );
    void D3D11API_UpdateCinematic( const image_t* image, const byte* pic, int cols, int rows, qboolean dirty );
    void D3D11API_DrawImage( const image_t* image, const float* coords, const float* texcoords, const float* color );
    imageFormat_t D3D11API_GetImageFormat( const image_t* image );
    void D3D11API_SetGamma( float gamma, float intensity );
    int D3D11API_GetFrameImageMemoryUsage( void );
    void D3D11API_GraphicsInfo( void );
    void D3D11API_Clear( unsigned long bits, const float* clearCol, unsigned long stencil, float depth );
    void D3D11API_SetProjectionMatrix( const float* projMatrix );
    void D3D11API_GetProjectionMatrix( float* projMatrix );
    void D3D11API_SetModelViewMatrix( const float* modelViewMatrix );
    void D3D11API_GetModelViewMatrix( float* modelViewMatrix );
    void D3D11API_SetViewport( int left, int top, int width, int height );
    void D3D11API_Flush( void );
    void D3D11API_SetState( unsigned long stateMask );
    void D3D11API_ResetState2D( void );
    void D3D11API_ResetState3D( void );
    void D3D11API_SetPortalRendering( qboolean enabled, const float* flipMatrix, const float* plane );
    void D3D11API_SetDepthRange( float minRange, float maxRange );
    void D3D11API_BeginFrame(void);
    void D3D11API_EndFrame( void );
    void D3D11API_MakeCurrent( qboolean current );
    void D3D11API_DrawSkyBox( const skyboxDrawInfo_t* skybox, const float* eye_origin, const float* colorTint );
    void D3D11API_DrawStageDepth( const shaderCommands_t *input );
    void D3D11API_DrawStageSky( const shaderCommands_t *input );
    void D3D11API_DrawStageGeneric( const shaderCommands_t *input );
    void D3D11API_DrawStageVertexLitTexture( const shaderCommands_t *input );
    void D3D11API_DrawStageLightmappedMultitexture( const shaderCommands_t *input );

    void D3D11API_DebugDrawAxis( void );
    void D3D11API_DebugDrawTris( const shaderCommands_t *input );
    void D3D11API_DebugDrawNormals( const shaderCommands_t *input );
    void D3D11API_DebugDrawPolygon( int color, int numPoints, const float* points );
    void D3D11API_DebugDraw( void );
}
