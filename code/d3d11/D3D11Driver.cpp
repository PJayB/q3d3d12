// D3D headers
#include "D3D11Driver.h"

extern "C" {
    void D3D11_Window_Init();
    void D3D11_Window_Shutdown();
}

//----------------------------------------------------------------------------
// Local data
//----------------------------------------------------------------------------
HRESULT g_hrLastError = S_OK;

//----------------------------------------------------------------------------
//
// DRIVER INTERFACE
//
//----------------------------------------------------------------------------
void D3D11API_Shutdown( void )
{
    DestroyDrawState();

    g_pImmediateContext->ClearState();
    g_pImmediateContext->Flush();

    D3D11_Window_Shutdown();
}

void D3D11API_UnbindResources( void )
{
    g_pImmediateContext->ClearState();
}

size_t D3D11API_LastError( void )
{
    return (size_t) g_hrLastError;
}

void D3D11API_ReadPixels( int x, int y, int width, int height, imageFormat_t requestedFmt, void* dest )
{
    
}

void D3D11API_ReadDepth( int x, int y, int width, int height, float* dest )
{

}

void D3D11API_ReadStencil( int x, int y, int width, int height, byte* dest )
{

}

void D3D11API_DrawImage( const image_t* image, const float* coords, const float* texcoords, const float* color )
{
    DrawQuad(
        &g_DrawState.quadRenderData,
        GetImageRenderInfo( image ),
        coords,
        texcoords,
        color );
}

D3D_PUBLIC void D3D11API_SetGamma( float gamma, float intensity )
{
    // @pjb: todo?
    if ( vdConfig.deviceSupportsGamma )
    {
        RI_Error( ERR_FATAL, "D3D11 hardware gamma ramp not implemented." );
    }
}

void D3D11API_GfxInfo( void )
{
	RI_Printf( PRINT_ALL, "----- Direct3D 11 -----\n" );
    
}

void D3D11API_Clear( unsigned long bits, const float* clearCol, unsigned long stencil, float depth )
{
    if ( bits & CLEAR_COLOR )
    {
        static float defaultCol[] = { 0, 0, 0, 0 };
        if ( !clearCol ) { clearCol = defaultCol; }

        g_pImmediateContext->ClearRenderTargetView( g_BufferState.backBufferView, clearCol );
    }

    if ( bits & ( CLEAR_DEPTH | CLEAR_STENCIL ) )
    {
        DWORD clearBits = 0;
        if ( bits & CLEAR_DEPTH ) { clearBits |= D3D11_CLEAR_DEPTH; }
        if ( bits & CLEAR_STENCIL ) { clearBits |= D3D11_CLEAR_STENCIL; }
        g_pImmediateContext->ClearDepthStencilView( g_BufferState.depthBufferView, clearBits, depth, (UINT8) stencil );
    }
}

void D3D11API_SetProjectionMatrix( const float* projMatrix )
{
    memcpy( g_RunState.vsConstants.projectionMatrix, projMatrix, sizeof(float) * 16 );
    g_RunState.vsDirtyConstants = qtrue;
}

void D3D11API_GetProjectionMatrix( float* projMatrix )
{
    memcpy( projMatrix, g_RunState.vsConstants.projectionMatrix, sizeof(float) * 16 );
}

void D3D11API_SetModelViewMatrix( const float* modelViewMatrix )
{
    memcpy( g_RunState.vsConstants.modelViewMatrix, modelViewMatrix, sizeof(float) * 16 );
    g_RunState.vsDirtyConstants = qtrue;
}

void D3D11API_GetModelViewMatrix( float* modelViewMatrix )
{
    memcpy( modelViewMatrix, g_RunState.vsConstants.modelViewMatrix, sizeof(float) * 16 );
}

void D3D11API_SetViewport( int left, int top, int width, int height )
{
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = max( 0, left );
    viewport.TopLeftY = max( 0, (int)g_BufferState.backBufferDesc.Height - top - height );
    viewport.Width = min( (int)g_BufferState.backBufferDesc.Width - viewport.TopLeftX, width );
    viewport.Height = min( (int)g_BufferState.backBufferDesc.Height - viewport.TopLeftY, height );
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    g_pImmediateContext->RSSetViewports( 1, &viewport );
    g_pImmediateContext->RSSetScissorRects( 0, NULL );
}

void D3D11API_Flush( void )
{
    g_pImmediateContext->End( g_DrawState.frameQuery );

    BOOL finished = FALSE;
    HRESULT hr;
    do
    {
        YieldProcessor();
        hr = g_pImmediateContext->GetData( g_DrawState.frameQuery, &finished, sizeof(finished), 0 );
    }
    while ( ( hr == S_OK || hr == S_FALSE ) && finished == FALSE );

    assert( SUCCEEDED( hr ) );
}

void D3D11API_ResetState2D( void )
{
    D3D11API_SetModelViewMatrix( s_identityMatrix );
    D3D11API_SetState( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

    CommitRasterizerState( CT_TWO_SIDED, qfalse, qfalse );

    D3D11API_SetPortalRendering( qfalse, NULL, NULL );
    D3D11API_SetDepthRange( 0, 0 );
}

void D3D11API_ResetState3D( void )
{
    D3D11API_SetModelViewMatrix( s_identityMatrix );
    D3D11API_SetState( GLS_DEFAULT );
    D3D11API_SetDepthRange( 0, 1 );
}

void D3D11API_SetPortalRendering( qboolean enabled, const float* flipMatrix, const float* plane )
{
    if ( enabled ) 
    {
        // Transform the supplied plane by the INVERSE of the flipMatrix
        // We can just transpose the flipMatrix because it's orthogonal
        // To clip, dot(vertex, plane) < 0
        g_RunState.psConstants.clipPlane[0] = flipMatrix[ 0] * plane[0] + flipMatrix[ 4] * plane[1] + flipMatrix[ 8] * plane[2] + flipMatrix[12] * plane[3];
        g_RunState.psConstants.clipPlane[1] = flipMatrix[ 1] * plane[0] + flipMatrix[ 5] * plane[1] + flipMatrix[ 9] * plane[2] + flipMatrix[13] * plane[3]; 
        g_RunState.psConstants.clipPlane[2] = flipMatrix[ 2] * plane[0] + flipMatrix[ 6] * plane[1] + flipMatrix[10] * plane[2] + flipMatrix[14] * plane[3]; 
        g_RunState.psConstants.clipPlane[3] = flipMatrix[ 3] * plane[0] + flipMatrix[ 7] * plane[1] + flipMatrix[11] * plane[2] + flipMatrix[15] * plane[3]; 
    }
    else
    {
        // Reset the clip plane
        g_RunState.psConstants.clipPlane[0] = 
        g_RunState.psConstants.clipPlane[1] = 
        g_RunState.psConstants.clipPlane[2] = 
        g_RunState.psConstants.clipPlane[3] = 0;
    }

    g_RunState.psDirtyConstants = qtrue;
}

void D3D11API_SetDepthRange( float minRange, float maxRange )
{
    g_RunState.vsConstants.depthRange[0] = minRange;
    g_RunState.vsConstants.depthRange[1] = maxRange - minRange;
    g_RunState.vsDirtyConstants = qtrue;
}

void D3D11API_SetDrawBuffer( int buffer )
{

}

void D3D11API_BeginFrame( void )
{
    // Not required.
}

void D3D11API_EndFrame( void )
{
    int frequency = 0;
	if ( r_swapInterval->integer > 0 ) 
    {
		frequency = min( vdConfig.displayFrequency, 60 / r_swapInterval->integer );
    }

    DXGI_PRESENT_PARAMETERS parameters = {0};
	parameters.DirtyRectsCount = 0;
	parameters.pDirtyRects = nullptr;
	parameters.pScrollRect = nullptr;
	parameters.pScrollOffset = nullptr;
    
    HRESULT hr = S_OK;
    switch (frequency)
    {
    case 60: hr = g_pSwapChain->Present1( 1, 0, &parameters ); break; 
    case 30: hr = g_pSwapChain->Present1( 2, 0, &parameters ); break;
    default: hr = g_pSwapChain->Present1( 0, 0, &parameters ); break; 
    }

#ifdef Q_WINRT_PLATFORM
	// Discard the contents of the render target.
	// This is a valid operation only when the existing contents will be entirely
	// overwritten. If dirty or scroll rects are used, this call should be removed.
	g_pImmediateContext->DiscardView( g_BufferState.backBufferView );

	// Discard the contents of the depth stencil.
	g_pImmediateContext->DiscardView( g_BufferState.depthBufferView );

    // Present unbinds the rendertarget, so let's put it back (FFS)
    g_pImmediateContext->OMSetRenderTargets( 1, &g_BufferState.backBufferView, g_BufferState.depthBufferView );
#endif

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		// Someone kicked the cord out or something. REBOOT TEH VIDYOS!
        Cbuf_AddText( "vid_restart\n" );
	}
}

D3D_PUBLIC int D3D11API_GetFrameImageMemoryUsage(void)
{
	// TODO
	return 0;
}

D3D_PUBLIC void D3D11API_GraphicsInfo( void )
{
	RI_Printf( PRINT_ALL, "----- Direct3D 11 -----\n" );
	// TODO
}

void D3D11API_MakeCurrent( qboolean current )
{
    
}

void D3D11API_DebugSetOverdrawMeasureEnabled( qboolean enabled )
{

}

void D3D11API_DebugSetTextureMode( const char* mode )
{

}

D3D_PUBLIC void D3D11API_DebugDraw( void )
{

}

void SetupVideoConfig()
{
    // Set up a bunch of default state
    const char* d3dVersionStr = "Direct3D 11";
    switch ( g_BufferState.featureLevel )
    {
    case D3D_FEATURE_LEVEL_9_1 : d3dVersionStr = "v9.1 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_9_2 : d3dVersionStr = "v9.2 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_9_3 : d3dVersionStr = "v9.3 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_10_0: d3dVersionStr = "v10.0 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_10_1: d3dVersionStr = "v10.1 (Compatibility)"; break;
    case D3D_FEATURE_LEVEL_11_0: d3dVersionStr = "v11.0"; break;
    case D3D_FEATURE_LEVEL_11_1: d3dVersionStr = "v11.1"; break;
    }
    Q_strncpyz( vdConfig.renderer_string, "Direct3D 11", sizeof( vdConfig.renderer_string ) );
    Q_strncpyz( vdConfig.version_string, d3dVersionStr, sizeof( vdConfig.version_string ) );
    Q_strncpyz( vdConfig.vendor_string, "Microsoft Corporation", sizeof( vdConfig.vendor_string ) );

    D3D11_DEPTH_STENCIL_VIEW_DESC depthBufferViewDesc;
    g_BufferState.depthBufferView->GetDesc( &depthBufferViewDesc );

    DWORD colorDepth = 0, depthDepth = 0, stencilDepth = 0;
    if ( FAILED( QD3D11::GetBitDepthForFormat( g_BufferState.swapChainDesc.Format, &colorDepth ) ) )
        RI_Error( ERR_FATAL, "Bad bit depth supplied for color channel (%x)\n", g_BufferState.swapChainDesc.Format );

    if ( FAILED( QD3D11::GetBitDepthForDepthStencilFormat( depthBufferViewDesc.Format, &depthDepth, &stencilDepth ) ) )
        RI_Error( ERR_FATAL, "Bad bit depth supplied for depth-stencil (%x)\n", depthBufferViewDesc.Format );

    vdConfig.maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    vdConfig.maxActiveTextures = D3D11_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
    vdConfig.colorBits = (int) colorDepth;
    vdConfig.depthBits = (int) depthDepth;
    vdConfig.stencilBits = (int) stencilDepth;

    vdConfig.deviceSupportsGamma = qfalse; // @pjb: todo?
    vdConfig.textureCompression = TC_NONE; // @pjb: todo: d3d texture compression
    vdConfig.textureEnvAddAvailable = qtrue;
    vdConfig.stereoEnabled = qfalse; // @pjb: todo: d3d stereo support

#if !defined(Q_WINRT_PLATFORM)
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
    vdConfig.vidWidth = g_BufferState.swapChainDesc.Width;
    vdConfig.vidHeight = g_BufferState.swapChainDesc.Height;
    vdConfig.windowAspect = vdConfig.vidWidth / (float)vdConfig.vidHeight;
#endif
}

D3D_PUBLIC void D3D11_DriverInit( graphicsApi_t* api )
{
    // Set up the API
    api->Shutdown = D3D11API_Shutdown;
    api->UnbindResources = D3D11API_UnbindResources;
    api->LastError = D3D11API_LastError;

    api->CreateImage = D3D11API_CreateImage;
    api->DeleteImage = D3D11API_DeleteImage;
    api->CreateShader = D3D11API_CreateShader;
    api->UpdateCinematic = D3D11API_UpdateCinematic;
    api->DrawImage = D3D11API_DrawImage;
    api->GetImageFormat = D3D11API_GetImageFormat;
    api->SetGamma = D3D11API_SetGamma;
    api->GetFrameImageMemoryUsage = D3D11API_GetFrameImageMemoryUsage;
    api->GraphicsInfo = D3D11API_GraphicsInfo;
    api->Clear = D3D11API_Clear;
    api->SetProjectionMatrix = D3D11API_SetProjectionMatrix;
    api->GetProjectionMatrix = D3D11API_GetProjectionMatrix;
    api->SetModelViewMatrix = D3D11API_SetModelViewMatrix;
    api->GetModelViewMatrix = D3D11API_GetModelViewMatrix;
    api->SetViewport = D3D11API_SetViewport;
    api->Flush = D3D11API_Flush;
    api->SetState = D3D11API_SetState;
    api->ResetState2D = D3D11API_ResetState2D;
    api->ResetState3D = D3D11API_ResetState3D;
    api->SetPortalRendering = D3D11API_SetPortalRendering;
    api->SetDepthRange = D3D11API_SetDepthRange;
    api->BeginFrame = D3D11API_BeginFrame;
    api->EndFrame = D3D11API_EndFrame;
    api->MakeCurrent = D3D11API_MakeCurrent;
    api->DrawSkyBox = D3D11API_DrawSkyBox;
    api->DrawStageDepth = D3D11API_DrawStageDepth;
    api->DrawStageSky = D3D11API_DrawStageSky;
    api->DrawStageGeneric = D3D11API_DrawStageGeneric;
    api->DrawStageVertexLitTexture = D3D11API_DrawStageVertexLitTexture;
    api->DrawStageLightmappedMultitexture = D3D11API_DrawStageLightmappedMultitexture;

    api->DebugDrawAxis = D3D11API_DebugDrawAxis;
    api->DebugDrawTris = D3D11API_DebugDrawTris;
    api->DebugDrawNormals = D3D11API_DebugDrawNormals;
    api->DebugDrawPolygon = D3D11API_DebugDrawPolygon;
    api->DebugDraw = D3D11API_DebugDraw;

    // This, weirdly, can be called multiple times. Catch that if that's the case.
    if ( g_pDevice == nullptr )
    {
        D3D11_Window_Init();
    }

    InitDrawState();
    
    // Set up vdConfig global
    SetupVideoConfig();
}

