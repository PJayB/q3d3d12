#include "tr_local.h"
#include "tr_layer.h"
#include "proxy_main.h"

#define PROXY_OPENGL 1

#ifdef Q_HAS_D3D12
#	define PROXY_D3D12 1
#else
#	define PROXY_D3D12 0
#endif

// D3D headers
#if PROXY_D3D12
#	include "../d3d12/QAPI.h"
#	include "../win32/win_dx12.h"
#endif

// GL headers
#if PROXY_OPENGL
#	include "gl_common.h"
#endif

typedef struct graphicsApiLayer_s {
    void            (* Shutdown)( void );
    void            (* UnbindResources)( void );
    size_t          (* LastError)( void );
    void            (* ReadPixels)( int x, int y, int width, int height, imageFormat_t requestedFmt, void* dest );
    void            (* ReadDepth)( int x, int y, int width, int height, float* dest );
    void            (* ReadStencil)( int x, int y, int width, int height, byte* dest );
    void            (* CreateImage)( const image_t* image, const byte *pic, qboolean isLightmap );
    void            (* DeleteImage)( const image_t* image );
    void            (* UpdateCinematic)( const image_t* image, const byte* pic, int cols, int rows, qboolean dirty );
    void            (* DrawImage)( const image_t* image, const float* coords, const float* texcoords, const float* color );
    imageFormat_t   (* GetImageFormat)( const image_t* image );
    void            (* SetGamma)( unsigned char red[256], unsigned char green[256], unsigned char blue[256] );
    int             (* GetFrameImageMemoryUsage)( void );
    void            (* GraphicsInfo)( void );
    void            (* Clear)( unsigned long bits, const float* clearCol, unsigned long stencil, float depth );
    void            (* SetProjectionMatrix)( const float* projMatrix );
    void            (* GetProjectionMatrix)( float* projMatrix );
    void            (* SetModelViewMatrix)( const float* modelViewMatrix );
    void            (* GetModelViewMatrix)( float* modelViewMatrix );
    void            (* SetViewport)( int left, int top, int width, int height );
    void            (* Flush)( void );
    void            (* ResetState2D)( void );
    void            (* ResetState3D)( void );
    void            (* SetPortalRendering)( qboolean enabled, const float* flipMatrix, const float* plane );
    void            (* SetDepthRange)( float minRange, float maxRange );
    void            (* SetDrawBuffer)( int buffer );
    void            (* BeginFrame)( void );
    void            (* EndFrame)( void );
    void            (* MakeCurrent)( qboolean current );
    void            (* ShadowSilhouette)( const float* edges, int edgeCount );
    void            (* ShadowFinish)( void );
    void            (* DrawSkyBox)( const skyboxDrawInfo_t* skybox, const float* eye_origin, const float* colorTint );
    void            (* DrawBeam)( const image_t* image, const float* color, const vec3_t startPoints[], const vec3_t endPoints[], int segs );
    void            (* DrawStageDepth)( const shaderCommands_t *input );
    void            (* DrawStageSky)( const shaderCommands_t *input );
    void            (* DrawStageGeneric)( const shaderCommands_t *input );
    void            (* DrawStageVertexLitTexture)( const shaderCommands_t *input );
    void            (* DrawStageLightmappedMultitexture)( const shaderCommands_t *input );
    void            (* DebugDrawAxis)( void );
    void            (* DebugDrawNormals)( const shaderCommands_t *input );
    void            (* DebugDrawTris)( const shaderCommands_t *input );
    void            (* DebugSetOverdrawMeasureEnabled)( qboolean enabled );
    void            (* DebugSetTextureMode)( const char* mode );
    void            (* DebugDrawPolygon)( int color, int numPoints, const float* points );
    void            (* DebugDraw)( void );
} graphicsApiLayer_t;

static qboolean gfx_initialized_configs = qfalse;
static cvar_t* proxy_driverPriority = NULL;

enum {
#if PROXY_D3D12
	gfx_driver_d3d12,
#endif
#if PROXY_OPENGL
	gfx_driver_opengl,
#endif
	gfx_driver_count
};

struct gfx_driver_info_s {
	const char* name;
	void( *init_func )( void );
	HWND( *get_hwnd_func )( void );
};

static const struct gfx_driver_info_s gfx_driver_infos[] = 
{
#if PROXY_D3D12
	{ "DirectX 12", DX12_DriverInit, D3D12_Window_GetWindowHandle },
#endif
#if PROXY_OPENGL
	{ "OpenGL", GLRB_DriverInit, GLimp_GetWindowHandle },
#endif
};

static vdconfig_t gfx_configs[gfx_driver_count];
static graphicsApiLayer_t gfx_drivers[gfx_driver_count];
static graphicsApiLayer_t* const gfx_drivers_end = gfx_drivers + gfx_driver_count;

#define FOR_EACH_DRIVER(x) for (graphicsApiLayer_t* x = gfx_drivers; x != gfx_drivers_end; ++x) if (x)
#define DRIVER_INDEX(x) ((int)(x - gfx_drivers))

static graphicsApiLayer_t* GetPriorityDriver()
{
	int index = proxy_driverPriority->integer;
	if (index < 0 || index >= gfx_driver_count)
	{
		assert(index >= 0 && index < gfx_driver_count);
		return NULL;
	}

	return &gfx_drivers[index];
}

void PROXY_Shutdown( void )
{
	FOR_EACH_DRIVER(d) d->Shutdown();
}

// Why? If the D3D driver fails, we can't call regular Shutdown because glDriver will still be null
void PROXY_ShutdownOne( void )
{
	FOR_EACH_DRIVER(d) d->Shutdown();
}

void PROXY_UnbindResources( void )
{
	FOR_EACH_DRIVER(d) d->UnbindResources();
}

size_t PROXY_LastError( void )
{
	graphicsApiLayer_t* driver = GetPriorityDriver();
	if (driver)
		return driver->LastError();
	else
		return 0;
}

void PROXY_ReadPixels( int x, int y, int width, int height, imageFormat_t requestedFmt, void* dest )
{
	graphicsApiLayer_t* driver = GetPriorityDriver();
	if (driver)
		driver->ReadPixels( x, y, width, height, requestedFmt, dest );
}

void PROXY_ReadDepth( int x, int y, int width, int height, float* dest )
{
	graphicsApiLayer_t* driver = GetPriorityDriver();
	if (driver)
		driver->ReadDepth( x, y, width, height, dest );
}

void PROXY_ReadStencil( int x, int y, int width, int height, byte* dest )
{
	graphicsApiLayer_t* driver = GetPriorityDriver();
	if (driver)
		driver->ReadStencil(x, y, width, height, dest);
}

void PROXY_CreateImage( const image_t* image, const byte *pic, qboolean isLightmap )
{
	FOR_EACH_DRIVER(d) d->CreateImage(image, pic, isLightmap);
}

void PROXY_DeleteImage( const image_t* image )
{
	FOR_EACH_DRIVER(d) d->DeleteImage(image);
}

void PROXY_UpdateCinematic( const image_t* image, const byte* pic, int cols, int rows, qboolean dirty )
{
	FOR_EACH_DRIVER(d) d->UpdateCinematic(image, pic, cols, rows, dirty);
}

void PROXY_DrawImage( const image_t* image, const float* coords, const float* texcoords, const float* color )
{
	FOR_EACH_DRIVER(d) d->DrawImage(image, coords, texcoords, color);
}

imageFormat_t PROXY_GetImageFormat( const image_t* image )
{
    graphicsApiLayer_t* driver = GetPriorityDriver();
	if (driver)
		return driver->GetImageFormat( image );
	else
		return 0;
}

void PROXY_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	FOR_EACH_DRIVER(d) d->SetGamma(red, green, blue);
}

int PROXY_SumOfUsedImages( void )
{
	int memUsage = 0;
	FOR_EACH_DRIVER(d) memUsage += d->GetFrameImageMemoryUsage();
	return memUsage;
}

void PROXY_GfxInfo( void )
{
	FOR_EACH_DRIVER( d )
	{
		RI_Printf( PRINT_ALL, "----- %s -----\n", gfx_driver_infos[DRIVER_INDEX(d)].name );
		d->GraphicsInfo();
	}

    // @pjb: todo
	RI_Printf( PRINT_ALL, "----- PROXY -----\n" );
    RI_Printf( PRINT_ALL, "Using proxied driver: all commands issued to multiple APIs.\n" );
}

void PROXY_Clear( unsigned long bits, const float* clearCol, unsigned long stencil, float depth )
{
	FOR_EACH_DRIVER(d) d->Clear(bits, clearCol, stencil, depth);
}

void PROXY_SetProjection( const float* projMatrix )
{
	FOR_EACH_DRIVER(d) d->SetProjectionMatrix(projMatrix);
}

void PROXY_GetProjection( float* projMatrix )
{
    graphicsApiLayer_t* driver = GetPriorityDriver();
	if (driver)
		driver->GetProjectionMatrix( projMatrix );
}

void PROXY_SetModelView( const float* modelViewMatrix )
{
	FOR_EACH_DRIVER(d) d->SetModelViewMatrix(modelViewMatrix);
}

void PROXY_GetModelView( float* modelViewMatrix )
{
    graphicsApiLayer_t* driver = GetPriorityDriver();
	if (driver)
		driver->GetModelViewMatrix( modelViewMatrix );
}

void PROXY_SetViewport( int left, int top, int width, int height )
{
	FOR_EACH_DRIVER(d) d->SetViewport(left, top, width, height);
}

void PROXY_Flush( void )
{
	FOR_EACH_DRIVER(d) d->Flush();
}

void PROXY_ResetState2D( void )
{
	FOR_EACH_DRIVER(d) d->ResetState2D();
}

void PROXY_ResetState3D( void )
{
	FOR_EACH_DRIVER(d) d->ResetState3D();
}

void PROXY_SetPortalRendering( qboolean enabled, const float* flipMatrix, const float* plane )
{
	FOR_EACH_DRIVER(d) d->SetPortalRendering(enabled, flipMatrix, plane);
}

void PROXY_SetDepthRange( float minRange, float maxRange )
{
	FOR_EACH_DRIVER(d) d->SetDepthRange(minRange, maxRange);
}

void PROXY_SetDrawBuffer( int buffer )
{
	FOR_EACH_DRIVER(d) d->SetDrawBuffer(buffer);
}

void PROXY_EndFrame( void )
{
	FOR_EACH_DRIVER(d) d->EndFrame();
}

void PROXY_MakeCurrent( qboolean current )
{
	FOR_EACH_DRIVER(d) d->MakeCurrent(current);
}

void PROXY_ShadowSilhouette( const float* edges, int edgeCount )
{
	FOR_EACH_DRIVER(d) d->ShadowSilhouette(edges, edgeCount);
}

void PROXY_ShadowFinish( void )
{
	FOR_EACH_DRIVER(d) d->ShadowFinish();
}

void PROXY_DrawSkyBox( const skyboxDrawInfo_t* skybox, const float* eye_origin, const float* colorTint )
{
	FOR_EACH_DRIVER(d) d->DrawSkyBox(skybox, eye_origin, colorTint);
}

void PROXY_DrawBeam( const image_t* image, const float* color, const vec3_t startPoints[], const vec3_t endPoints[], int segs )
{
	FOR_EACH_DRIVER(d) d->DrawBeam(image, color, startPoints, endPoints, segs);
}

void PROXY_DrawStageDepth( const shaderCommands_t *input )
{
	FOR_EACH_DRIVER(d) d->DrawStageDepth(input);
}

void PROXY_DrawStageSky( const shaderCommands_t *input )
{
	FOR_EACH_DRIVER(d) d->DrawStageSky(input);
}

void PROXY_DrawStageGeneric( const shaderCommands_t *input )
{
	FOR_EACH_DRIVER(d) d->DrawStageGeneric(input);
}

void PROXY_DrawStageVertexLitTexture( const shaderCommands_t *input )
{
	FOR_EACH_DRIVER(d) d->DrawStageVertexLitTexture(input);
}

void PROXY_DrawStageLightmappedMultitexture( const shaderCommands_t *input )
{
	FOR_EACH_DRIVER(d) d->DrawStageLightmappedMultitexture(input);
}

void PROXY_DebugDrawAxis( void )
{
	FOR_EACH_DRIVER(d) d->DebugDrawAxis();
}

void PROXY_DebugDrawTris( const shaderCommands_t *input )
{
	FOR_EACH_DRIVER(d) d->DebugDrawTris(input);
}

void PROXY_DebugDrawNormals( const shaderCommands_t *input )
{
	FOR_EACH_DRIVER(d) d->DebugDrawNormals(input);
}

void PROXY_DebugSetOverdrawMeasureEnabled( qboolean enabled )
{
	FOR_EACH_DRIVER(d) d->DebugSetOverdrawMeasureEnabled(enabled);
}

void PROXY_DebugSetTextureMode( const char* mode )
{
	FOR_EACH_DRIVER(d) d->DebugSetTextureMode(mode);
}

void PROXY_DebugDrawPolygon( int color, int numPoints, const float* points )
{
	FOR_EACH_DRIVER(d) d->DebugDrawPolygon(color, numPoints, points);
}

void PROXY_DebugDraw( void )
{
	FOR_EACH_DRIVER(d) d->DebugDraw();
}

static void PositionWindowRelativeTo( HWND basis, HWND wnd, int pos )
{
    RECT d3dR, glR;
    int left, top;

	GetWindowRect(basis, &d3dR);
	GetWindowRect(wnd, &glR);

    left = 0;
    if (pos & 1) {
        left = d3dR.right;
    }
    top = 0;
    if (pos & 2) {
        top = d3dR.bottom;
    }

	if (!MoveWindow(wnd, left, top, (glR.right - glR.left), (glR.bottom - glR.top), TRUE))
    {
        RI_Printf( PRINT_WARNING, "Failed to move window: %d, %d\n",
            left, top );
    }
}

static void PositionWindows()
{
    HWND windows[gfx_driver_count];
    int windowCount = 0;

    int fail_to_compile_if_too_many_drivers[(gfx_driver_count <= 4)];

	for ( int i = 0; i < gfx_driver_count; ++i )
	{
        HWND wnd = gfx_driver_infos[i].get_hwnd_func();
        if (wnd != NULL)
            windows[windowCount++] = wnd;
	}

    for ( int i = 0; i < windowCount; ++i )
    {
        PositionWindowRelativeTo(windows[0], windows[i], i);
    }
}

static void SetToQuarterResolution(int* width, int* height)
{
	DEVMODE dm;
	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );
	if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ) )
	{
        *width = (int)(dm.dmPelsWidth / 2) - 20;
        *height = (int)(dm.dmPelsHeight / 2) - 50;
    }
}

static void ReconcileVideoConfigs( const vdconfig_t* d3dConfig, const vdconfig_t* glConfig, vdconfig_t* outConfig )
{
    if ( glConfig->deviceSupportsGamma != d3dConfig->deviceSupportsGamma ) {
        Com_Error( ERR_FATAL, "Conflicting gamma support: D3D %s, GL %s\n", 
            d3dConfig->deviceSupportsGamma ? "yes" : "no",
            glConfig->deviceSupportsGamma ? "yes" : "no" );
    }
    
    if ( glConfig->stereoEnabled != d3dConfig->stereoEnabled ) {
        Com_Error( ERR_FATAL, "Conflicting stereo support: D3D %s, GL %s\n", 
            d3dConfig->stereoEnabled ? "yes" : "no",
            glConfig->stereoEnabled ? "yes" : "no" );
    }
    
    if ( glConfig->isFullscreen != d3dConfig->isFullscreen ) {
        Com_Error( ERR_FATAL, "Conflicting fullscreen support: D3D %s, GL %s\n", 
            d3dConfig->isFullscreen ? "yes" : "no",
            glConfig->isFullscreen ? "yes" : "no" );
    }
    
    if ( glConfig->textureEnvAddAvailable != d3dConfig->textureEnvAddAvailable ) {
        Com_Error( ERR_FATAL, "Conflicting texenv add support: D3D %s, GL %s\n", 
            d3dConfig->textureEnvAddAvailable ? "yes" : "no",
            glConfig->textureEnvAddAvailable ? "yes" : "no" );
    }
    
    if ( glConfig->displayFrequency != d3dConfig->displayFrequency ) {
        Com_Error( ERR_FATAL, "Conflicting displayFrequency: D3D %d, GL %d\n", 
            d3dConfig->displayFrequency,
            glConfig->displayFrequency );
    }

    if ( glConfig->vidWidth != d3dConfig->vidWidth ||
         glConfig->vidHeight != d3dConfig->vidHeight ) {
        Com_Error( ERR_FATAL, "Conflicting video resolution: D3D %dx%d, GL %dx%d\n", 
            d3dConfig->vidWidth, d3dConfig->vidHeight,
            glConfig->vidWidth, glConfig->vidHeight );
    }
    
    outConfig->maxTextureSize = min( d3dConfig->maxTextureSize, glConfig->maxTextureSize );
    outConfig->colorBits = min( d3dConfig->colorBits, glConfig->colorBits );
    outConfig->depthBits = min( d3dConfig->depthBits, glConfig->depthBits );
    outConfig->stencilBits = min( d3dConfig->stencilBits, glConfig->stencilBits );
}

void ShadowGfxApiBindings( graphicsApiLayer_t* layer )
{
    layer->Shutdown = GFX_Shutdown;
    layer->UnbindResources = GFX_UnbindResources;
    layer->LastError = GFX_LastError;
    layer->ReadPixels = GFX_ReadPixels;
    layer->ReadDepth = GFX_ReadDepth;
    layer->ReadStencil = GFX_ReadStencil;
    layer->CreateImage = GFX_CreateImage;
    layer->DeleteImage = GFX_DeleteImage;
    layer->UpdateCinematic = GFX_UpdateCinematic;
    layer->DrawImage = GFX_DrawImage;
    layer->GetImageFormat = GFX_GetImageFormat;
    layer->SetGamma = GFX_SetGamma;
    layer->GetFrameImageMemoryUsage = GFX_GetFrameImageMemoryUsage;
    layer->GraphicsInfo = GFX_GraphicsInfo;
    layer->Clear = GFX_Clear;
    layer->SetProjectionMatrix = GFX_SetProjectionMatrix;
    layer->GetProjectionMatrix = GFX_GetProjectionMatrix;
    layer->SetModelViewMatrix = GFX_SetModelViewMatrix;
    layer->GetModelViewMatrix = GFX_GetModelViewMatrix;
    layer->SetViewport = GFX_SetViewport;
    layer->Flush = GFX_Flush;
    layer->ResetState2D = GFX_ResetState2D;
    layer->ResetState3D = GFX_ResetState3D;
    layer->SetPortalRendering = GFX_SetPortalRendering;
    layer->SetDepthRange = GFX_SetDepthRange;
    layer->SetDrawBuffer = GFX_SetDrawBuffer;
    layer->BeginFrame = GFX_BeginFrame;
    layer->EndFrame = GFX_EndFrame;
    layer->MakeCurrent = GFX_MakeCurrent;
    layer->ShadowSilhouette = GFX_ShadowSilhouette;
    layer->ShadowFinish = GFX_ShadowFinish;
    layer->DrawSkyBox = GFX_DrawSkyBox;
    layer->DrawBeam = GFX_DrawBeam;
    layer->DrawStageDepth = GFX_DrawStageDepth;
    layer->DrawStageSky = GFX_DrawStageSky;
    layer->DrawStageGeneric = GFX_DrawStageGeneric;
    layer->DrawStageVertexLitTexture = GFX_DrawStageVertexLitTexture;
    layer->DrawStageLightmappedMultitexture = GFX_DrawStageLightmappedMultitexture;
    layer->DebugDrawAxis = GFX_DebugDrawAxis;
    layer->DebugDrawTris = GFX_DebugDrawTris;
    layer->DebugDrawNormals = GFX_DebugDrawNormals;
    layer->DebugSetOverdrawMeasureEnabled = GFX_DebugSetOverdrawMeasureEnabled;
    layer->DebugSetTextureMode = GFX_DebugSetTextureMode;
    layer->DebugDrawPolygon = GFX_DebugDrawPolygon;
    layer->DebugDraw = GFX_DebugDraw;
}

void SetProxyBindings( void )
{
    GFX_Shutdown = PROXY_ShutdownOne;
    GFX_UnbindResources = PROXY_UnbindResources;
    GFX_LastError = PROXY_LastError;
    GFX_ReadPixels = PROXY_ReadPixels;
    GFX_ReadDepth = PROXY_ReadDepth;
    GFX_ReadStencil = PROXY_ReadStencil;
    GFX_CreateImage = PROXY_CreateImage;
    GFX_DeleteImage = PROXY_DeleteImage;
    GFX_UpdateCinematic = PROXY_UpdateCinematic;
    GFX_DrawImage = PROXY_DrawImage;
    GFX_GetImageFormat = PROXY_GetImageFormat;
    GFX_SetGamma = PROXY_SetGamma;
    GFX_GetFrameImageMemoryUsage = PROXY_SumOfUsedImages;
    GFX_GraphicsInfo = PROXY_GfxInfo;
    GFX_Clear = PROXY_Clear;
    GFX_SetProjectionMatrix = PROXY_SetProjection;
    GFX_GetProjectionMatrix = PROXY_GetProjection;
    GFX_SetModelViewMatrix = PROXY_SetModelView;
    GFX_GetModelViewMatrix = PROXY_GetModelView;
    GFX_SetViewport = PROXY_SetViewport;
    GFX_Flush = PROXY_Flush;
    GFX_ResetState2D = PROXY_ResetState2D;
    GFX_ResetState3D = PROXY_ResetState3D;
    GFX_SetPortalRendering = PROXY_SetPortalRendering;
    GFX_SetDepthRange = PROXY_SetDepthRange;
    GFX_SetDrawBuffer = PROXY_SetDrawBuffer;
    GFX_EndFrame = PROXY_EndFrame;
    GFX_MakeCurrent = PROXY_MakeCurrent;
    GFX_ShadowSilhouette = PROXY_ShadowSilhouette;
    GFX_ShadowFinish = PROXY_ShadowFinish;
    GFX_DrawSkyBox = PROXY_DrawSkyBox;
    GFX_DrawBeam = PROXY_DrawBeam;
    GFX_DrawStageDepth = PROXY_DrawStageDepth;
    GFX_DrawStageSky = PROXY_DrawStageSky;
    GFX_DrawStageGeneric = PROXY_DrawStageGeneric;
    GFX_DrawStageVertexLitTexture = PROXY_DrawStageVertexLitTexture;
    GFX_DrawStageLightmappedMultitexture = PROXY_DrawStageLightmappedMultitexture;
    GFX_DebugDrawAxis = PROXY_DebugDrawAxis;
    GFX_DebugDrawTris = PROXY_DebugDrawTris;
    GFX_DebugDrawNormals = PROXY_DebugDrawNormals;
    GFX_DebugSetOverdrawMeasureEnabled = PROXY_DebugSetOverdrawMeasureEnabled;
    GFX_DebugSetTextureMode = PROXY_DebugSetTextureMode;
    GFX_DebugDrawPolygon = PROXY_DebugDrawPolygon;
    GFX_DebugDraw = PROXY_DebugDraw;
}

/*
    @pjb: Validates the driver is completely full of pointers
*/
void ValidateDriverLayerMemory( size_t* memory, size_t size )
{
    size_t i;
    for ( i = 0; i < size / sizeof(size_t); ++i ) {
        if ( memory[i] == 0 )
            Com_Error( ERR_FATAL, "Video API layer not set up correctly: missing binding in slot %d.\n", i );
    }
}

/*
    @pjb: Expose the validation API to other areas of the game
*/
void ValidateGraphicsLayer( graphicsApiLayer_t* layer )
{
    ValidateDriverLayerMemory( (size_t*) layer, sizeof( graphicsApiLayer_t ) );
}

void PROXY_DriverInit( void )
{
    vdconfig_t old_vdConfig, gl_vdConfig, d3d_vdConfig;

    // Get the configurable driver priority
    proxy_driverPriority = Cvar_Get( "proxy_driverPriority", "0", CVAR_ARCHIVE );

    // If using the proxy driver we cannot use fullscreen
    Cvar_Set( "r_fullscreen", "0" );

    // Ignore hardware gamma (D3D does it another way)
    Cvar_Set( "r_ignorehwgamma", "1" );

    // Set the vdConfig up so that the resolution is exactly a quarter of the screen resolution
    SetToQuarterResolution(&vdConfig.vidWidth, &vdConfig.vidHeight);

    // Backup the vdConfig because the drivers will make changes to this (unfortunately)
    old_vdConfig = vdConfig;
    if (!gfx_initialized_configs) {
        for (int i = 0; i < gfx_driver_count; ++i) {
            gfx_configs[i] = vdConfig;
        }
        gfx_initialized_configs = qtrue;
    }

    // Init the D3D driver and back up the vdConfigs each time
	FOR_EACH_DRIVER( d )
	{
		const struct gfx_driver_info_s* driver_info = &gfx_driver_infos[DRIVER_INDEX( d )];

		RI_Printf( PRINT_ALL, "----- %s -----\n", driver_info->name );

        vdConfig = gfx_configs[DRIVER_INDEX( d )];

		driver_info->init_func();
		ShadowGfxApiBindings( d );
		ValidateGraphicsLayer( d );

		gfx_configs[DRIVER_INDEX( d )] = vdConfig;
	}

    // Now bind our abstracted functions
    SetProxyBindings();

    // Let's arrange these windows shall we?
    PositionWindows();

    // Restore the old config and reconcile the new configs
    vdConfig = old_vdConfig;
	for ( int i = 1; i < gfx_driver_count; ++i )
	{
		ReconcileVideoConfigs( &gfx_configs[i - 1], &gfx_configs[i], &vdConfig );
	}

	// Copy the resource strings to the vgConfig
	Q_strncpyz( vdConfig.renderer_string, "PROXY DRIVER", sizeof( vdConfig.renderer_string ) );
	Q_strncpyz( vdConfig.vendor_string, "@pjb", sizeof( vdConfig.vendor_string ) );
	Q_strncpyz( vdConfig.version_string, "1.0", sizeof( vdConfig.version_string ) );
}


