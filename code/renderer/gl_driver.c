#include "tr_local.h"
#include "tr_layer.h"
#include "qgl.h"
#include "gl_common.h"

glstate_t	glState;

void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

void ( APIENTRY * qglLockArraysEXT)( GLint, GLint);
void ( APIENTRY * qglUnlockArraysEXT) ( void );

void GLRB_RestoreTextureState( void )
{
    // Flush textures; reset texture state
	Com_Memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}

    GL_SetDefaultState();
}

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
	char renderer_buffer[1024];

	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_glDriver
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//
	
	if ( !glState.initialized ) // @pjb: vdConfig.vidWidth == 0 is no longer a good indicator
	{
		GLint		temp;
		
		GLimp_Init();

		strcpy( renderer_buffer, vdConfig.renderer_string );
		Q_strlwr( renderer_buffer );

		// OpenGL driver constants
		qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
		vdConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if ( vdConfig.maxTextureSize <= 0 ) 
		{
			vdConfig.maxTextureSize = 0;
		}

        glState.initialized = qtrue;
	}

	// print info
	GLRB_GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

/*-----------------------------------------------------------

Driver overloads

-----------------------------------------------------------*/

void GLRB_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
    if ( vdConfig.deviceSupportsGamma )
	{
        GLimp_SetGamma( red, green, blue );
    }
}

/*
================
GfxInfo_f
================
*/
void GLRB_GfxInfo_f( void ) 
{
	cvar_t *sys_cpustring = Cvar_Get( "sys_cpustring", "", 0 );
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	RI_Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glState.vendor_string );
	RI_Printf( PRINT_ALL, "GL_RENDERER: %s\n", glState.renderer_string );
	RI_Printf( PRINT_ALL, "GL_VERSION: %s\n", glState.version_string );
	RI_Printf( PRINT_ALL, "GL_EXTENSIONS: %s\n", glState.extensions_string );
	RI_Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", vdConfig.maxTextureSize );
	RI_Printf( PRINT_ALL, "GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", vdConfig.maxActiveTextures );
	RI_Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", vdConfig.colorBits, vdConfig.depthBits, vdConfig.stencilBits );
	RI_Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, vdConfig.vidWidth, vdConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
	if ( vdConfig.displayFrequency )
	{
		RI_Printf( PRINT_ALL, "%d\n", vdConfig.displayFrequency );
	}
	else
	{
		RI_Printf( PRINT_ALL, "N/A\n" );
	}
	if ( vdConfig.deviceSupportsGamma )
	{
		RI_Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		RI_Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}
	RI_Printf( PRINT_ALL, "CPU: %s\n", sys_cpustring->string );

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		RI_Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			} else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			RI_Printf( PRINT_ALL, "none\n" );
		} else if ( primitives == 2 ) {
			RI_Printf( PRINT_ALL, "single glDrawElements\n" );
		} else if ( primitives == 1 ) {
			RI_Printf( PRINT_ALL, "multiple glArrayElement\n" );
		} else if ( primitives == 3 ) {
			RI_Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	RI_Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	RI_Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	RI_Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	RI_Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	RI_Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	RI_Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[vdConfig.textureEnvAddAvailable != 0] );
	RI_Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[vdConfig.textureCompression!=TC_NONE] );
	if ( r_vertexLight->integer )
	{
		RI_Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}
	if ( vdConfig.smpActive ) {
		RI_Printf( PRINT_ALL, "Using dual processor acceleration\n" );
	}
	if ( r_finish->integer ) {
		RI_Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
}

size_t GLRB_LastError( void )
{
    return (size_t) qglGetError();
}

void GLRB_ReadPixels( int x, int y, int width, int height, imageFormat_t requestedFmt, void* dest )
{
    int glFmt = GL_ConvertImageFormat( requestedFmt );

    qglReadPixels( x, y, width, height, glFmt, GL_UNSIGNED_BYTE, dest );
}

void GLRB_ReadDepth( int x, int y, int width, int height, float* dest )
{
	qglReadPixels( x, y, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, dest );
}

void GLRB_ReadStencil( int x, int y, int width, int height, byte* dest )
{
	qglReadPixels( x, y, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, dest );
}

void GLRB_Shutdown( void )
{
    GLimp_Shutdown();
    glState.initialized = qfalse;
}

void GLRB_ResetState2D( void )
{
    qglLoadIdentity();

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
    GL_Cull( CT_TWO_SIDED );

	qglDisable( GL_CLIP_PLANE0 );
}

void GLRB_ResetState3D( void )
{
    // @pjb: todo

    qglLoadIdentity();

    GL_State( GLS_DEFAULT );
}

// This could be two functions, but I want to keep the number of different API calls to a minimum
void GLRB_SetPortalRendering( qboolean enabled, const float* flipMatrix, const float* plane )
{
    if ( enabled )
    {
        double dplane[4] = {
            plane[0],
            plane[1],
            plane[2],
            plane[3] };

        qglLoadMatrixf( flipMatrix );
        qglClipPlane( GL_CLIP_PLANE0, dplane );
        qglEnable( GL_CLIP_PLANE0 );
    }
    else
    {
        qglDisable( GL_CLIP_PLANE0 );
    }
}

void GLRB_DrawImage( const image_t* image, const float* coords, const float* texcoords, const float* color )
{
    GL_Bind( image );

    if ( color )
	    qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );
    else
        qglColor3f( 1, 1, 1 );

	qglBegin (GL_QUADS);
	qglTexCoord2f ( texcoords[0],  texcoords[1] );
	qglVertex2f (coords[0], coords[1]);
	qglTexCoord2f ( texcoords[2],  texcoords[1] );
	qglVertex2f (coords[2], coords[1]);
	qglTexCoord2f ( texcoords[2], texcoords[3] );
	qglVertex2f (coords[2], coords[3]);
	qglTexCoord2f ( texcoords[0], texcoords[3] );
	qglVertex2f (coords[0], coords[3]);
	qglEnd ();
}

void GLRB_SetDrawBuffer( int buffer )
{
    switch (buffer)
    {
    case DRAW_BUFFER_BACK:
        qglDrawBuffer( GL_BACK );
        break;
    case DRAW_BUFFER_FRONT:
        qglDrawBuffer( GL_FRONT );
        break;
    case DRAW_BUFFER_BACK_LEFT:
        qglDrawBuffer( GL_BACK_LEFT );
        break;
    case DRAW_BUFFER_BACK_RIGHT:
        qglDrawBuffer( GL_BACK_RIGHT );
        break;
    }
}

void GLRB_SetOverdrawMeasureEnabled( qboolean enabled )
{
    if ( !enabled )
    {
        qglDisable( GL_STENCIL_TEST );
    }
    else
    {
		qglEnable( GL_STENCIL_TEST );
		qglStencilMask( ~0U );
		qglClearStencil( 0U );
		qglStencilFunc( GL_ALWAYS, 0U, ~0U );
		qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
    }
}

/*
================
R_DebugPolygon
================
*/
void GLRB_DebugPolygon( int color, int numPoints, const float *points ) {
	int		i;

	GL_Bind( tr.whiteImage);
	GL_Cull( CT_FRONT_SIDED );
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	qglColor3f( color&1, (color>>1)&1, (color>>2)&1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglDepthRange( 0, 0 );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();
	qglDepthRange( 0, 1 );
}

void GLRB_DrawSkyBox( const skyboxDrawInfo_t* skybox, const float* eye_origin, const float* colorTint )
{
    int i, j, k;

    qglColor3fv( colorTint );		
	qglPushMatrix ();
	GL_State( 0 );
	qglTranslatef( eye_origin[0], eye_origin[1], eye_origin[2] );

    for ( i = 0; i < 6; ++i )
    {
        const skyboxSideDrawInfo_t* side = &skybox->sides[i];

        if ( !side->image )
            continue;

        GL_Bind( side->image );

        for ( j = 0; j < side->stripCount; ++j )
        {
            int start = side->stripInfo[j].offset;
            int end = side->stripInfo[j].length + start;

            qglBegin( GL_TRIANGLE_STRIP );

            for ( k = start; k < end; ++k )
            {
                qglTexCoord2fv( skybox->tbuffer + 2 * k );
                qglVertex3fv( skybox->vbuffer + 3 * k );
            }

            qglEnd();
        }
    }

	qglPopMatrix();
}

void GLRB_DrawBeam( const image_t* image, const float* color, const vec3_t startPoints[], const vec3_t endPoints[], int segs )
{
    int i;

	GL_Bind( image );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	qglColor3fv( color );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i <= segs; i++ ) {
		qglVertex3fv( startPoints[ i % segs] );
		qglVertex3fv( endPoints[ i % segs] );
	}
	qglEnd();
}

// Draws from modelview space
void GLRB_SurfaceAxis( void ) {
	GL_Bind( tr.whiteImage );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1,0,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 16,0,0 );
	qglColor3f( 0,1,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,16,0 );
	qglColor3f( 0,0,1 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,0,16 );
	qglEnd();
	qglLineWidth( 1 );
}

void GLRB_DebugDraw( void ) {

}

/*
@@@@@@@@@@@@@@@@@@@@@

Returns the opengl graphics driver and sets up global state

@@@@@@@@@@@@@@@@@@@@@
*/
void GLRB_DriverInit( void )
{
    GFX_Shutdown = GLRB_Shutdown;
    GFX_UnbindResources = GLRB_RestoreTextureState;
    GFX_LastError = GLRB_LastError;
    GFX_ReadPixels = GLRB_ReadPixels;
    GFX_ReadDepth = GLRB_ReadDepth;
    GFX_ReadStencil = GLRB_ReadStencil;
    GFX_CreateImage = GL_CreateImage;
    GFX_DeleteImage = GL_DeleteImage;
    GFX_UpdateCinematic = GL_UpdateCinematic;
    GFX_DrawImage = GLRB_DrawImage;
    GFX_GetImageFormat = GL_GetImageFormat;
    GFX_SetGamma = GLRB_SetGamma;
    GFX_GetFrameImageMemoryUsage = GL_SumOfUsedImages;
    GFX_GraphicsInfo = GLRB_GfxInfo_f;
    GFX_Clear = GL_Clear;
    GFX_SetProjectionMatrix = GL_SetProjection;
    GFX_GetProjectionMatrix = GL_GetProjection;
    GFX_SetModelViewMatrix = GL_SetModelView;
    GFX_GetModelViewMatrix = GL_GetModelView;
    GFX_SetViewport = GL_SetViewport;
    GFX_Flush = GL_Finish;
    GFX_ResetState2D = GLRB_ResetState2D;
    GFX_ResetState3D = GLRB_ResetState3D;
    GFX_SetPortalRendering = GLRB_SetPortalRendering;
    GFX_SetDepthRange = GL_SetDepthRange;
    GFX_SetDrawBuffer = GLRB_SetDrawBuffer;
    GFX_BeginFrame = GLimp_BeginFrame;
    GFX_EndFrame = GLimp_EndFrame;
    GFX_MakeCurrent = GLimp_MakeCurrent;
    GFX_ShadowSilhouette = GLRB_ShadowSilhouette;
    GFX_ShadowFinish = GLRB_ShadowFinish;
    GFX_DrawSkyBox = GLRB_DrawSkyBox;
    GFX_DrawBeam = GLRB_DrawBeam;
    GFX_DrawStageDepth = GLRB_StageIteratorDepth;
    GFX_DrawStageSky = GLRB_StageIteratorGeneric;
    GFX_DrawStageGeneric = GLRB_StageIteratorGeneric;
    GFX_DrawStageVertexLitTexture = GLRB_StageIteratorVertexLitTexture;
    GFX_DrawStageLightmappedMultitexture = GLRB_StageIteratorLightmappedMultitexture;
    GFX_DebugDrawAxis = GLRB_SurfaceAxis;
    GFX_DebugDrawTris = GLRB_DebugDrawTris;
    GFX_DebugDrawNormals = GLRB_DebugDrawNormals;
    GFX_DebugSetOverdrawMeasureEnabled = GLRB_SetOverdrawMeasureEnabled;
    GFX_DebugSetTextureMode = GL_TextureMode;
    GFX_DebugDrawPolygon = GLRB_DebugPolygon;
    GFX_DebugDraw = GLRB_DebugDraw;

    InitOpenGL();

    // Copy the resource strings to the vgConfig
    Q_strncpyz( vdConfig.renderer_string, glState.renderer_string, sizeof( vdConfig.renderer_string ) );
    Q_strncpyz( vdConfig.vendor_string, glState.vendor_string, sizeof( vdConfig.vendor_string ) );
    Q_strncpyz( vdConfig.version_string, glState.version_string, sizeof( vdConfig.version_string ) );
}
