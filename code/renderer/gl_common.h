#ifndef __OGL_COMMON_H__
#define __OGL_COMMON_H__

#include "../qshared/q_shared.h"
#include "qgl.h"

typedef struct image_s image_t;

extern	int			gl_filter_min, gl_filter_max;



void GL_Bind( const image_t *image );
void GL_SetDefaultState (void);
void GL_SelectTexture( int unit );
void GL_TextureMode( const char *string );
void GL_CheckErrors( void );
void GL_State( unsigned long stateVector );
void GL_TexEnv( int env );
void GL_Cull( int cullType );
void GL_Clear( unsigned long bits, const float* clearCol, unsigned long stencil, float depth );
void GL_SetProjection( const float* projMatrix );
void GL_GetProjection( float* projMatrix );
void GL_SetModelView( const float* modelViewMatrix );
void GL_GetModelView( float* modelViewMatrix );
void GL_SetViewport( int left, int top, int width, int height );
void GL_Finish( void );
void GL_SetDepthRange( float minRange, float maxRange );


// @pjb: switched to shorts for d3d9 compat
#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT

/*
====================================================================

GL-SPECIFIC CONFIG

====================================================================
*/

// the renderer front end should never modify glstate_t
typedef struct {
    char            renderer_string[MAX_STRING_CHARS];
    char            vendor_string[MAX_STRING_CHARS];
    char            version_string[MAX_STRING_CHARS];
    char            extensions_string[BIG_INFO_STRING];

    qboolean        initialized;
	int			    currenttextures[2];
	int			    currenttmu;
	int			    texEnv[2];
	int			    faceCulling;
	unsigned long	glStateBits;

    // Shadow state for the transforms
    float           projection[16];
    float           modelView[16];
} glstate_t;

extern glstate_t	        glState;		// outside of TR since it shouldn't be cleared during ref re-init

/*
====================================================================

RENDER BACKEND

====================================================================
*/

void GLRB_DriverInit( void );
void GLRB_RestoreTextureState( void );
void GLRB_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] );
void GLRB_GfxInfo_f( void );

/*
====================================================================

IMAGES

====================================================================
*/

typedef struct image_s image_t;

void GL_CreateImage( const image_t* image, const byte *pic, qboolean isLightmap );
void GL_DeleteImage( const image_t* image );
void GL_UpdateCinematic( const image_t* image, const byte* pic, int cols, int rows, qboolean dirty );

imageFormat_t GL_GetImageFormat( const image_t* image );
int GL_SumOfUsedImages( void );

void GL_SetImageBorderColor( const image_t* image, const float* borderColor );

int GL_ConvertImageFormat( imageFormat_t );

/*
====================================================================

SHADOW RENDERING

====================================================================
*/

void GLRB_ShadowSilhouette( const float* edges, int edgeCount );
void GLRB_ShadowFinish( void );

/*
====================================================================

MODEL RENDERING

====================================================================
*/

void GLRB_StageIteratorDepth( const shaderCommands_t *input );
void GLRB_StageIteratorGeneric( const shaderCommands_t *input );
void GLRB_StageIteratorVertexLitTexture( const shaderCommands_t *input );
void GLRB_StageIteratorLightmappedMultitexture( const shaderCommands_t *input );
void GLRB_DebugDrawTris( const shaderCommands_t *input );
void GLRB_DebugDrawNormals( const shaderCommands_t *input );

/*
====================================================================

PLATFORM SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_Init( void );
void		GLimp_Shutdown( void );
void		GLimp_BeginFrame(void); // @pjb
void		GLimp_EndFrame( void );

void        GLimp_MakeCurrent( qboolean current );

void		GLimp_LogComment( char *comment );

// NOTE TTimo linux works with float gamma value, not the gamma table
//   the params won't be used, getting the r_gamma cvar directly
void		GLimp_SetGamma( unsigned char red[256], 
						    unsigned char green[256],
							unsigned char blue[256] );

// @pjb: returns the window handle
HWND        GLimp_GetWindowHandle( void );

#endif