//@pjb : the API abstraction layer

#ifndef __TR_LAYER_H__
#define __TR_LAYER_H__

#include "tr_state.h"

extern const float s_identityMatrix[16];

typedef struct graphicsApi_s {
    void (* Shutdown)( void );
    void (* UnbindResources)( void );
    size_t (* LastError)( void );

    void (* CreateImage)( const image_t* image, const byte *pic, qboolean isLightmap );
    void (* DeleteImage)( const image_t* image );
    void (* CreateShader)( const shader_t* shader );
    void (* UpdateCinematic)( const image_t* image, const byte* pic, int cols, int rows, qboolean dirty );
    void (* DrawImage)( const image_t* image, const float* coords, const float* texcoords, const float* color );
    imageFormat_t (* GetImageFormat)( const image_t* image );
    void (* SetGamma)( float gamma, float intensity );
    int (* GetFrameImageMemoryUsage)( void );
    void (* GraphicsInfo)( void );
    void (* Clear)( unsigned long bits, const float* clearCol, unsigned long stencil, float depth );
    void (* SetProjectionMatrix)( const float* projMatrix );
    void (* GetProjectionMatrix)( float* projMatrix );
    void (* SetModelViewMatrix)( const float* modelViewMatrix );
    void (* GetModelViewMatrix)( float* modelViewMatrix );
    void (* SetViewport)( int left, int top, int width, int height );
    void (* Flush)( void );
    void (* SetState)( unsigned long stateMask );
    void (* ResetState2D)( void );
    void (* ResetState3D)( void );
    void (* SetPortalRendering)( qboolean enabled, const float* flipMatrix, const float* plane );
    void (* SetDepthRange)( float minRange, float maxRange );
    void (* BeginFrame)(void);
    void (* EndFrame)( void );
    void (* MakeCurrent)( qboolean current );
    void (* DrawSkyBox)( const skyboxDrawInfo_t* skybox, const float* eye_origin, const float* colorTint );
    void (* DrawStageDepth)( const shaderCommands_t *input );
    void (* DrawStageSky)( const shaderCommands_t *input );
    void (* DrawStageGeneric)( const shaderCommands_t *input );
    void (* DrawStageVertexLitTexture)( const shaderCommands_t *input );
    void (* DrawStageLightmappedMultitexture)( const shaderCommands_t *input );

    void (* DebugDrawAxis)( void );
    void (* DebugDrawTris)( const shaderCommands_t *input );
    void (* DebugDrawNormals)( const shaderCommands_t *input );
    void (* DebugDrawPolygon)( int color, int numPoints, const float* points );
    void (* DebugDraw)( void );
} graphicsApi_t;

extern graphicsApi_t tr_graphicsApi;

void R_ResetGraphicsLayer();

// @pjb: proxy uses this to validate the proxied drivers
void R_ValidateGraphicsLayer();

#define GFX_Shutdown tr_graphicsApi.Shutdown
#define GFX_UnbindResources tr_graphicsApi.UnbindResources
#define GFX_LastError tr_graphicsApi.LastError

#define GFX_CreateImage tr_graphicsApi.CreateImage
#define GFX_DeleteImage tr_graphicsApi.DeleteImage
#define GFX_CreateShader tr_graphicsApi.CreateShader
#define GFX_UpdateCinematic tr_graphicsApi.UpdateCinematic
#define GFX_DrawImage tr_graphicsApi.DrawImage
#define GFX_GetImageFormat tr_graphicsApi.GetImageFormat
#define GFX_SetGamma tr_graphicsApi.SetGamma
#define GFX_GetFrameImageMemoryUsage tr_graphicsApi.GetFrameImageMemoryUsage
#define GFX_GraphicsInfo tr_graphicsApi.GraphicsInfo
#define GFX_Clear tr_graphicsApi.Clear
#define GFX_SetProjectionMatrix tr_graphicsApi.SetProjectionMatrix
#define GFX_GetProjectionMatrix tr_graphicsApi.GetProjectionMatrix
#define GFX_SetModelViewMatrix tr_graphicsApi.SetModelViewMatrix
#define GFX_GetModelViewMatrix tr_graphicsApi.GetModelViewMatrix
#define GFX_SetViewport tr_graphicsApi.SetViewport
#define GFX_Flush tr_graphicsApi.Flush
#define GFX_SetState tr_graphicsApi.SetState
#define GFX_ResetState2D tr_graphicsApi.ResetState2D
#define GFX_ResetState3D tr_graphicsApi.ResetState3D
#define GFX_SetPortalRendering tr_graphicsApi.SetPortalRendering
#define GFX_SetDepthRange tr_graphicsApi.SetDepthRange
#define GFX_BeginFrame tr_graphicsApi.BeginFrame
#define GFX_EndFrame tr_graphicsApi.EndFrame
#define GFX_MakeCurrent tr_graphicsApi.MakeCurrent
#define GFX_DrawSkyBox tr_graphicsApi.DrawSkyBox
#define GFX_DrawStageDepth tr_graphicsApi.DrawStageDepth
#define GFX_DrawStageSky tr_graphicsApi.DrawStageSky
#define GFX_DrawStageGeneric tr_graphicsApi.DrawStageGeneric
#define GFX_DrawStageVertexLitTexture tr_graphicsApi.DrawStageVertexLitTexture
#define GFX_DrawStageLightmappedMultitexture tr_graphicsApi.DrawStageLightmappedMultitexture

#define GFX_DebugDrawAxis tr_graphicsApi.DebugDrawAxis
#define GFX_DebugDrawTris tr_graphicsApi.DebugDrawTris
#define GFX_DebugDrawNormals tr_graphicsApi.DebugDrawNormals
#define GFX_DebugDrawPolygon tr_graphicsApi.DebugDrawPolygon
#define GFX_DebugDraw tr_graphicsApi.DebugDraw

#endif
