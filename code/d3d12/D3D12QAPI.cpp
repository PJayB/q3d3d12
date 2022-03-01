// D3D headers
#include "pch.h"
#include "D3D12QAPI.h"
#include "Device.h"
#include "SwapChain.h"
#include "Frame.h"
#include "Image.h"
#include "FSQ.h"
#include "RenderState.h"
#include "Upload.h"
#include "SkyBox.h"
#include "DrawStages.h"
#include "GenericDrawStage.h"
#include "DirectXTK12/DDSTextureLoader.h"

using namespace QD3D12;

D3D_PUBLIC void D3D12_Window_Init();
D3D_PUBLIC void D3D12_Window_Shutdown();

#ifdef _XBOX_ONE
static cvar_t* xbo_presentThreshold = NULL;
#endif

//----------------------------------------------------------------------------
// Local data
//----------------------------------------------------------------------------
static HRESULT g_hrLastError = S_OK;

static HANDLE g_currentThread = NULL;

static cvar_t* d3d_showdepth = NULL;
static cvar_t* d3d_threadValidate = NULL;

static void D3D12API_ValidateThread()
{
    if (d3d_threadValidate->integer && 
        g_currentThread != NULL && 
        GetCurrentThread() != g_currentThread) {
            __debugbreak();
    }
}

//----------------------------------------------------------------------------
//
// DRIVER INTERFACE
//
//----------------------------------------------------------------------------

D3D_PUBLIC void D3D12API_CreateImage(const image_t* image, const byte *pic, qboolean /*isLightmap*/)
{
    // If the stream is a DDS, load that instead
    if ( Image::IsBlobDDS( pic ) )
    {
        Image::LoadDDS( image, pic );
    }
    else if (Q_strncmp(image->imgName, "*scratch", sizeof(image->imgName)) == 0)
    {
        ASSERT( !image->mipmap );
        if ( image->mipmap )
        {
            RI_Printf( PRINT_WARNING, "Request to mipmap a dynamic texture! %dx%d '%s'\n",
                image->width, image->height, image->imgName );
        }

        Image::LoadDynamic( image, image->width, image->height, pic );
    }
    else
    {
	    // Count the number of miplevels for this image
	    int mipLevels = 1;
	    if (image->mipmap)
	    {
		    int mipWidth = image->width;
		    int mipHeight = image->height;
		    mipLevels = 0;
		    while (mipWidth > 0 || mipHeight > 0)
		    {
			    mipWidth >>= 1;
			    mipHeight >>= 1;
			    mipLevels++;
		    }

		    // Don't allow zero mips
		    mipLevels = max(1, mipLevels);
	    }

	    Image::Load(image, mipLevels, pic );
    }
}

D3D_PUBLIC void D3D12API_DeleteImage(const image_t* image)
{
	Image::Delete(image);
}

D3D_PUBLIC void D3D12API_UpdateCinematic(const image_t* image, const byte* pic, int cols, int rows, qboolean dirty)
{
	PROFILE_FUNC();
	if (cols != image->width || rows != image->height) 
    {
		Image::Delete(image);

		// Regenerate the texture
		Image::LoadDynamic(
			image,
			cols,
			rows,
			pic );
	}
	else 
    {
		if (dirty) 
        {

			Image* d3dImage = Image::Get(image->index);
			d3dImage->Update(
				Frame::CurrentCommandList(),
				pic);
		}
	}
}

D3D_PUBLIC imageFormat_t D3D12API_GetImageFormat(const image_t* image)
{
	// @pjb: hack: all images are RGBA8 for now
	return IMAGEFORMAT_RGBA;
}

D3D_PUBLIC void D3D12API_CreateShader( const shader_t* shader )
{
    // Create PSOs
    BOOL bOldWarning = PipelineState::GetPrecacheWarning();
    PipelineState::SetPrecacheWarning( FALSE );
    GenericDrawStage::CacheShaderPipelineState( shader );
    PipelineState::SetPrecacheWarning( bOldWarning );
}

D3D_PUBLIC int D3D12API_GetFrameImageMemoryUsage(void)
{
	// TODO
	return 0;
}

D3D_PUBLIC void D3D12API_DrawSkyBox(const skyboxDrawInfo_t* skybox, const float* eye_origin, const float* colorTint)
{
	PROFILE_FUNC();
	SkyBox::Draw(skybox, eye_origin, colorTint);
}

D3D_PUBLIC void D3D12API_DrawStageDepth(const shaderCommands_t *input)
{
	PROFILE_FUNC();
	assert(RB_IsDepthPass());

    GenericDrawStage stage;
    stage.DrawDepth(
        Frame::CurrentCommandList(),
        input);
}

D3D_PUBLIC void D3D12API_DrawStageSky(const shaderCommands_t *input)
{
	PROFILE_FUNC();
	assert(RB_IsColorPass());

    GenericDrawStage stage;
    stage.DrawSky(
        Frame::CurrentCommandList(),
        input);
}

D3D_PUBLIC void D3D12API_DrawStageGeneric(const shaderCommands_t *input)
{
	PROFILE_FUNC();
	assert(RB_IsColorPass());

	GenericDrawStage stage;
	stage.Draw(
		Frame::CurrentCommandList(),
		input);
}

D3D_PUBLIC void D3D12API_DrawStageVertexLitTexture(const shaderCommands_t *input)
{
	// Optimizing this is a low priority
	PROFILE_FUNC();
	D3D12API_DrawStageGeneric(input);
}

D3D_PUBLIC void D3D12API_DrawStageLightmappedMultitexture(const shaderCommands_t *input)
{
	// Optimizing this is a low priority
	PROFILE_FUNC();
	D3D12API_DrawStageGeneric(input);
}

D3D_PUBLIC void D3D12API_DebugDrawAxis(void)
{
	PROFILE_FUNC();

}

D3D_PUBLIC void D3D12API_DebugDrawTris(const shaderCommands_t *input)
{
	PROFILE_FUNC();

}

D3D_PUBLIC void D3D12API_DebugDrawNormals(const shaderCommands_t *input)
{
	PROFILE_FUNC();

}

D3D_PUBLIC void D3D12API_DebugDrawPolygon(int color, int numPoints, const float* points)
{
	PROFILE_FUNC();

}

D3D_PUBLIC void D3D12API_Shutdown( void )
{
    D3D12API_ValidateThread();
    QD3D12::RenderState::Destroy();
    D3D12_Window_Shutdown();
}

D3D_PUBLIC void D3D12API_UnbindResources( void )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();
	Device::WaitForGpu();
}

D3D_PUBLIC size_t D3D12API_LastError( void )
{
    return (size_t) g_hrLastError;
}

D3D_PUBLIC void D3D12API_DrawImage( const image_t* image, const float* coords, const float* texcoords, const float* color )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

    FSQ::Draw(
        Image::Get(image->index),
        coords,
        texcoords,
        color );
}

D3D_PUBLIC void D3D12API_SetGamma( float gamma, float intensity )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

    // @pjb: todo
    RI_Printf( PRINT_WARNING, "D3D12 gamma not implemented yet.\n" );
}

D3D_PUBLIC void D3D12API_GraphicsInfo( void )
{
	RI_Printf( PRINT_ALL, "----- Direct3D 12 -----\n" );
	// TODO
}

D3D_PUBLIC void D3D12API_Clear( unsigned long bits, const float* clearCol, unsigned long stencil, float depth )
{
	PROFILE_FUNC();
    D3D12API_ValidateThread();

    ID3D12GraphicsCommandList* pCmdList = Frame::CurrentCommandList();
	BackBufferState* pBBState = Frame::CurrentBackBufferState();

	if ( bits & CLEAR_COLOR )
    {
        static float defaultCol[] = { 0, 0, 0, 0 };
        if ( !clearCol ) { clearCol = defaultCol; }

		pCmdList->ClearRenderTargetView(pBBState->ColorBufferRTV(), clearCol, 0, nullptr);
    }

    if ( bits & ( CLEAR_DEPTH | CLEAR_STENCIL ) )
    {
		D3D12_CLEAR_FLAGS clearBits = (D3D12_CLEAR_FLAGS)0;
        if ( bits & CLEAR_DEPTH ) { clearBits |= D3D12_CLEAR_FLAG_DEPTH; }
        if ( bits & CLEAR_STENCIL ) { clearBits |= D3D12_CLEAR_FLAG_STENCIL; }
		pCmdList->ClearDepthStencilView(pBBState->DepthBufferDSV(), clearBits, depth, (UINT8) stencil, 0, nullptr );
    }
}

D3D_PUBLIC void D3D12API_SetProjectionMatrix( const float* projMatrix )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();
	RenderState::SetProjection(projMatrix);
}

D3D_PUBLIC void D3D12API_GetProjectionMatrix( float* projMatrix )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

	memcpy( projMatrix, RenderState::VSConstants()->Projection, sizeof(float) * 16 );
}

D3D_PUBLIC void D3D12API_SetModelViewMatrix( const float* modelViewMatrix )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();
	RenderState::SetModelView(modelViewMatrix);
}

D3D_PUBLIC void D3D12API_GetModelViewMatrix( float* modelViewMatrix )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

    memcpy( modelViewMatrix, RenderState::VSConstants()->View, sizeof(float) * 16 );
}

D3D_PUBLIC void D3D12API_SetViewport( int left, int top, int width, int height )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

	const BackBufferState* bbState = Frame::CurrentBackBufferState();

	auto bb_width = bbState->SwapChainDesc()->BufferDesc.Width;
	auto bb_height = bbState->SwapChainDesc()->BufferDesc.Height;

	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = max(0, left);
	viewport.TopLeftY = max(0, (int)bb_height - top - height);
	viewport.Width = min((int)bb_width - viewport.TopLeftX, width);
	viewport.Height = min((int)bb_height - viewport.TopLeftY, height);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	D3D12_RECT scissor = {
		(LONG) (viewport.TopLeftX),
		(LONG) (viewport.TopLeftY),
		(LONG) (viewport.TopLeftX + viewport.Width),
		(LONG) (viewport.TopLeftY + viewport.Height)
	};

	ID3D12GraphicsCommandList* pCmdList = Frame::CurrentCommandList();
	pCmdList->RSSetViewports(1, &viewport);
	pCmdList->RSSetScissorRects(1, &scissor);

	D3D12_CPU_DESCRIPTOR_HANDLE cbrtv = bbState->ColorBufferRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dbdsv = bbState->DepthBufferDSV();
	pCmdList->OMSetRenderTargets(1, &cbrtv, true, &dbdsv);
}

D3D_PUBLIC void D3D12API_Flush( void )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();
}

D3D_PUBLIC void D3D12API_SetState(unsigned long stateBits)
{
	RenderState::SetStateBits(stateBits);
}

D3D_PUBLIC void D3D12API_SetPortalRendering( qboolean enabled, const float* flipMatrix, const float* plane )
{
    PROFILE_FUNC();
    D3D12API_ValidateThread();

    if ( enabled ) 
    {
        // Transform the supplied plane by the INVERSE of the flipMatrix
        // We can just transpose the flipMatrix because it's orthogonal
        // To clip, dot(vertex, plane) < 0
		float clipPlane[] = {
			flipMatrix[0] * plane[0] + flipMatrix[4] * plane[1] + flipMatrix[8] * plane[2] + flipMatrix[12] * plane[3],
			flipMatrix[1] * plane[0] + flipMatrix[5] * plane[1] + flipMatrix[9] * plane[2] + flipMatrix[13] * plane[3],
			flipMatrix[2] * plane[0] + flipMatrix[6] * plane[1] + flipMatrix[10] * plane[2] + flipMatrix[14] * plane[3],
			flipMatrix[ 3] * plane[0] + flipMatrix[ 7] * plane[1] + flipMatrix[11] * plane[2] + flipMatrix[15] * plane[3]
		};
		RenderState::SetClipPlane(clipPlane);
    }
    else
    {
        // Reset the clip plane
		float clipPlane[] = { 0, 0, 0, 0 };
        RenderState::SetClipPlane(clipPlane);
    }
}

D3D_PUBLIC void D3D12API_SetDepthRange( float minRange, float maxRange )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

	RenderState::SetDepthRange(minRange, maxRange);
}

D3D_PUBLIC void D3D12API_ResetState2D( void )
{
	PROFILE_FUNC();
    D3D12API_ValidateThread();

    D3D12API_SetModelViewMatrix( s_identityMatrix );
    D3D12API_SetState( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

    D3D12API_SetPortalRendering( qfalse, NULL, NULL );
    D3D12API_SetDepthRange( 0, 0 );
}

D3D_PUBLIC void D3D12API_ResetState3D( void )
{
    PROFILE_FUNC();
    D3D12API_ValidateThread();

    D3D12API_SetModelViewMatrix( s_identityMatrix );
    D3D12API_SetState( GLS_DEFAULT );
    D3D12API_SetDepthRange( 0, 1 );
}

D3D_PUBLIC void D3D12API_BeginFrame( void )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

	// Sometimes Quake will call this even though renderer is not running
	if (Device::IsStarted()) 
    {
        Frame::Begin();
    }
}

D3D_PUBLIC void D3D12API_EndFrame( void )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

	// Sometimes Quake will call this even though renderer is not running
	if (!Device::IsStarted())
		return;

    int frequency = 0;
	if ( r_swapInterval->integer > 0 ) 
    {
        assert( vdConfig.displayFrequency == 60 || vdConfig.displayFrequency == 30 );
		frequency = max( 60 / vdConfig.displayFrequency, r_swapInterval->integer );
    }

	// Fence the end of this frame
	Frame::End(
		SwapChain::CurrentBackBuffer(),
		SwapChain::Desc()->Format );

#ifdef _XBOX_ONE
	const BackBufferState* ppState = Frame::GetBackBufferState(0);

	SwapChain::XBOX_ONE_PRESENT_PARAMETERS params;
	ZeroMemory(&params, sizeof(params));
	params.BackBufferWidth = ppState->SwapChainDesc()->BufferDesc.Width;
	params.BackBufferHeight = ppState->SwapChainDesc()->BufferDesc.Height;
	params.Interval = (frequency == 1 || frequency == 2) ? frequency : 0;
	params.PresentThreshold = xbo_presentThreshold ? xbo_presentThreshold->integer : 0;

    HRESULT hr = SwapChain::Present(&params);
#else
    HRESULT hr = S_OK;
    switch (frequency)
    {
    case 1:
    case 2:
        hr = SwapChain::Present( frequency ); 
        break;
    default: 
        hr = SwapChain::Present( 0 ); 
        break; 
    }
#endif

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
        //Cbuf_AddText( "vid_restart\n" );
	}
}

// @pjb: todo: get rid of this?
D3D_PUBLIC void D3D12API_MakeCurrent( qboolean current )
{
    PROFILE_FUNC();
    g_currentThread = current ? GetCurrentThread() : NULL;
}

D3D_PUBLIC void D3D12API_DebugDraw( void )
{
	PROFILE_FUNC();
	D3D12API_ValidateThread();

    if ( d3d_showdepth->integer )
    {
        // TODO: DebugDrawDepth();
    }
}

//----------------------------------------------------------------------------
// Returns the number of bits in the particular pixel format
//----------------------------------------------------------------------------
HRESULT GetBitDepthForDepthStencilFormat(
	_In_ DXGI_FORMAT fmt,
	_Out_ DWORD* depthBits,
	_Out_ DWORD* stencilBits)
{
	switch (fmt)
	{
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		*depthBits = 32; *stencilBits = 8; break;
	case DXGI_FORMAT_D32_FLOAT:
		*depthBits = 32; *stencilBits = 0; break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		*depthBits = 24; *stencilBits = 8; break;
	case DXGI_FORMAT_D16_UNORM:
		*depthBits = 16; *stencilBits = 0; break;
	default:
		return E_INVALIDARG;
	}
	return S_OK;
}

static void SetupVideoConfig()
{
    // Set up a bunch of default state
    const char* d3dVersionStr = "Direct3D 12";
    switch ( Device::FeatureLevel() )
    {
    case D3D_FEATURE_LEVEL_9_1 : d3dVersionStr = "Feature Level 9.1 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_9_2 : d3dVersionStr = "Feature Level 9.2 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_9_3 : d3dVersionStr = "Feature Level 9.3 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_10_0: d3dVersionStr = "Feature Level 10.0 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_10_1: d3dVersionStr = "Feature Level 10.1 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_11_0: d3dVersionStr = "Feature Level 11.0"; break;
    case D3D_FEATURE_LEVEL_11_1: d3dVersionStr = "Feature Level 11.1"; break;
    case D3D_FEATURE_LEVEL_12_0: d3dVersionStr = "Feature Level 12.0"; break;
    case D3D_FEATURE_LEVEL_12_1: d3dVersionStr = "Feature Level 12.1"; break;
    }
    Q_strncpyz( vdConfig.renderer_string, "Direct3D 12", sizeof( vdConfig.renderer_string ) );
    Q_strncpyz( vdConfig.version_string, d3dVersionStr, sizeof( vdConfig.version_string ) );
    Q_strncpyz( vdConfig.vendor_string, "Microsoft Corporation", sizeof( vdConfig.vendor_string ) );

    BackBufferState* pBBState = Frame::GetBackBufferState(0);
	D3D12_RESOURCE_DESC depthBufferDesc = pBBState->DepthBufferResource()->GetDesc();
	const DXGI_MODE_DESC& bbDesc = pBBState->SwapChainDesc()->BufferDesc;

    size_t colorDepth = DirectX::BitsPerPixel( bbDesc.Format );
    if ( colorDepth == 0 )
        RI_Error( ERR_FATAL, "Bad bit depth supplied for color channel (%x)\n", bbDesc.Format);

    DWORD depthDepth = 0, stencilDepth = 0;
    if ( FAILED( GetBitDepthForDepthStencilFormat(depthBufferDesc.Format, &depthDepth, &stencilDepth ) ) )
        RI_Error( ERR_FATAL, "Bad bit depth supplied for depth-stencil (%x)\n", depthBufferDesc.Format );

    vdConfig.maxTextureSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    vdConfig.maxActiveTextures = D3D12_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
    vdConfig.colorBits = (int) colorDepth;
    vdConfig.depthBits = (int) depthDepth;
    vdConfig.stencilBits = (int) stencilDepth;
	
    vdConfig.deviceSupportsGamma = qfalse; // @pjb: todo?
    vdConfig.textureCompression = TC_NONE; // @pjb: todo: d3d texture compression
    vdConfig.textureEnvAddAvailable = qtrue;
    vdConfig.stereoEnabled = qfalse; // @pjb: todo: d3d stereo support

#ifndef Q_WINRT_PLATFORM
	DEVMODE dm;
    memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );
	if ( EnumDisplaySettingsEx( NULL, ENUM_CURRENT_SETTINGS, &dm, 0 ) )
	{
		vdConfig.displayFrequency = dm.dmDisplayFrequency;
	}
#else
    // @pjb: todo: EnumDisplaySettingsEx doesn't exist.
    vdConfig.displayFrequency = 60;
#endif
    
#ifdef Q_WINRT_PLATFORM
    // We expect vidWidth, vidHeight and windowAspect to all be set by now in most cases,
    // but on Win8 they can be enforced by the system instead of set by us.
	BackBufferState* bbState = Frame::GetBackBufferState(0);
    vdConfig.vidWidth = bbState->SwapChainDesc()->BufferDesc.Width;
    vdConfig.vidHeight = bbState->SwapChainDesc()->BufferDesc.Height;
    vdConfig.windowAspect = vdConfig.vidWidth / (float)vdConfig.vidHeight;
#endif
}

D3D_PUBLIC qboolean D3D12_DriverIsSupported(void)
{
    return Device::IsSupported() ? qtrue : qfalse;
}

D3D_PUBLIC void D3D12_DriverInit( graphicsApi_t* api )
{
    // Set up the API
    api->Shutdown = D3D12API_Shutdown;
    api->UnbindResources = D3D12API_UnbindResources;
    api->LastError = D3D12API_LastError;

    api->CreateImage = D3D12API_CreateImage;
    api->DeleteImage = D3D12API_DeleteImage;
    api->CreateShader = D3D12API_CreateShader;
    api->UpdateCinematic = D3D12API_UpdateCinematic;
    api->DrawImage = D3D12API_DrawImage;
    api->GetImageFormat = D3D12API_GetImageFormat;
    api->SetGamma = D3D12API_SetGamma;
    api->GetFrameImageMemoryUsage = D3D12API_GetFrameImageMemoryUsage;
    api->GraphicsInfo = D3D12API_GraphicsInfo;
    api->Clear = D3D12API_Clear;
    api->SetProjectionMatrix = D3D12API_SetProjectionMatrix;
    api->GetProjectionMatrix = D3D12API_GetProjectionMatrix;
    api->SetModelViewMatrix = D3D12API_SetModelViewMatrix;
    api->GetModelViewMatrix = D3D12API_GetModelViewMatrix;
    api->SetViewport = D3D12API_SetViewport;
    api->Flush = D3D12API_Flush;
    api->SetState = D3D12API_SetState;
    api->ResetState2D = D3D12API_ResetState2D;
    api->ResetState3D = D3D12API_ResetState3D;
    api->SetPortalRendering = D3D12API_SetPortalRendering;
    api->SetDepthRange = D3D12API_SetDepthRange;
    api->BeginFrame = D3D12API_BeginFrame;
    api->EndFrame = D3D12API_EndFrame;
    api->MakeCurrent = D3D12API_MakeCurrent;
    api->DrawSkyBox = D3D12API_DrawSkyBox;
    api->DrawStageDepth = D3D12API_DrawStageDepth;
    api->DrawStageSky = D3D12API_DrawStageSky;
    api->DrawStageGeneric = D3D12API_DrawStageGeneric;
    api->DrawStageVertexLitTexture = D3D12API_DrawStageVertexLitTexture;
    api->DrawStageLightmappedMultitexture = D3D12API_DrawStageLightmappedMultitexture;

    api->DebugDrawAxis = D3D12API_DebugDrawAxis;
    api->DebugDrawTris = D3D12API_DebugDrawTris;
    api->DebugDrawNormals = D3D12API_DebugDrawNormals;
    api->DebugDrawPolygon = D3D12API_DebugDrawPolygon;
    api->DebugDraw = D3D12API_DebugDraw;

    // Set up cvars
    d3d_showdepth = Cvar_Get( "d3d_showdepth", "0", CVAR_TEMP );
    d3d_threadValidate = Cvar_Get( "d3d_threadValidate", "0", CVAR_ARCHIVE );

#ifdef _XBOX_ONE
	xbo_presentThreshold = Cvar_Get( "xbo_presentThreshold", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SYSTEM_SET );
#endif

    D3D12API_MakeCurrent(qtrue);

    QD3D12::PipelineState::SetPrecacheWarning(FALSE);

    // This, weirdly, can be called multiple times. Catch that if that's the case.
	if (!Device::IsStarted())
    {
        D3D12_Window_Init();
    }
    else
    {
        // Already got a window. Clean up before re-init.
        QD3D12::RenderState::Destroy();
    }

    QD3D12::RenderState::Init();

    // Set up vdConfig global
    SetupVideoConfig();

    QD3D12::PipelineState::SetPrecacheWarning(TRUE);
}

