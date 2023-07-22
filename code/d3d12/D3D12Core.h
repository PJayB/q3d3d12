#pragma once

#include "../d3d/Direct3DCommon.h"

#ifdef _XBOX_ONE
#	include <d3d11_x.h>
#	include <d3d12_x.h>
#	include <d3dx12_x.h>
#else
#	include <d3d12.h>
#	include "DirectXTK12/d3dx12.h"
#endif

#define QD3D12_CONSTANT_BUFFER_ALIGNMENT	256
#define QD3D12_INDEX_BUFFER_ALIGNMENT		4096
#define QD3D12_VERTEX_BUFFER_ALIGNMENT		4096

// TODO: make these cvars?
#define QD3D12_NUM_BUFFERS					2
#define QD3D12_ESTIMATED_DRAW_CALLS			128
#define QD3D12_UPLOAD_SCRATCH_SIZE			(8*1024*1024)

#define QD3D12_NUM_CBV_SRV_UAV_DESCRIPTORS  2048
#define QD3D12_NUM_SAMPLER_DESCRIPTORS      2048
#define QD3D12_NUM_RTV_DESCRIPTORS          64
#define QD3D12_NUM_DSV_DESCRIPTORS          64

#define QD3D12_RESOURCE_STATE_INITIAL       D3D12_RESOURCE_STATE_COMMON

#ifdef _XBOX_ONE
#	define QD3D12_NODE_MASK					D3D12XBOX_NODE_MASK
#else
#	define QD3D12_NODE_MASK                 1
#endif

#if defined(_DEBUG) || defined(PROFILE)
#	define QD3D12_PROFILE_NAMES				1
#else
#	define QD3D12_PROFILE_NAMES				0
#endif

namespace QD3D12
{
	//----------------------------------------------------------------------------
	// Set a resource name
	//----------------------------------------------------------------------------
	HRESULT SetResourceName(
		_In_ ID3D12Resource* resource,
		_In_ const char* fmt,
		_In_ ...);
	HRESULT SetResourceName(
		_In_ ID3D12Resource* resource,
		_In_ const wchar_t* fmt,
		_In_ ...);

	//----------------------------------------------------------------------------
	// Set a resource barrier
	//----------------------------------------------------------------------------
	void TransitionResource(
		_In_ ID3D12GraphicsCommandList* commandList,
		_In_ ID3D12Resource* resource,
		_In_ D3D12_RESOURCE_STATES stateBefore,
		_In_ D3D12_RESOURCE_STATES stateAfter);

	//----------------------------------------------------------------------------
	// Set GPU offsets for descriptor heaps
	//----------------------------------------------------------------------------
	template<UINT startIndex, UINT count> 
	inline void SetGraphicsRootDescriptorTables(
		_In_ ID3D12GraphicsCommandList* pList,
		const D3D12_GPU_DESCRIPTOR_HANDLE* pHandles)
	{
		for (UINT i = 0; i < count; ++i)
		{
			pList->SetGraphicsRootDescriptorTable(startIndex + i, pHandles[i]);
		}
	}

	//----------------------------------------------------------------------------
	// Utility for setting a CB
	//----------------------------------------------------------------------------
    template<UINT Slot> void SetGenericRootConstantBufferView(
        _In_ ID3D12GraphicsCommandList* pList,
        _In_ D3D12_GPU_VIRTUAL_ADDRESS CB)
    {
        assert((CB & 0xFF) == 0);
        pList->SetGraphicsRootConstantBufferView(Slot, CB);
    }

    //----------------------------------------------------------------------------
    // Utility for setting many CBs
    //----------------------------------------------------------------------------
    template<UINT CBCount> void SetGenericRootConstantBufferViews(
        _In_ ID3D12GraphicsCommandList* pList,
        _In_ const D3D12_GPU_VIRTUAL_ADDRESS* pCBs)
    {
        for (UINT i = 0; i < CBCount; ++i)
        {
            assert((pCBs[i] & 0xFF) == 0);
            pList->SetGraphicsRootConstantBufferView(i, pCBs[i]);
        }
    }
}

extern "C" 
{
    void D3D12API_Shutdown( void );
    void D3D12API_UnbindResources( void );
    size_t D3D12API_LastError( void );

    void D3D12API_CreateImage( const image_t* image, const byte *pic, qboolean isLightmap );
    void D3D12API_DeleteImage( const image_t* image );
    void D3D12API_CreateShader( const shader_t* shader );
    void D3D12API_UpdateCinematic( const image_t* image, const byte* pic, int cols, int rows, qboolean dirty );
    void D3D12API_DrawImage( const image_t* image, const float* coords, const float* texcoords, const float* color );
    imageFormat_t D3D12API_GetImageFormat( const image_t* image );
    void D3D12API_SetGamma( float gamma, float intensity );
    int D3D12API_GetFrameImageMemoryUsage( void );
    void D3D12API_GraphicsInfo( void );
    void D3D12API_Clear( unsigned long bits, const float* clearCol, unsigned long stencil, float depth );
    void D3D12API_SetProjectionMatrix( const float* projMatrix );
    void D3D12API_GetProjectionMatrix( float* projMatrix );
    void D3D12API_SetModelViewMatrix( const float* modelViewMatrix );
    void D3D12API_GetModelViewMatrix( float* modelViewMatrix );
    void D3D12API_SetViewport( int left, int top, int width, int height );
    void D3D12API_Flush( void );
    void D3D12API_SetState( unsigned long stateMask );
    void D3D12API_ResetState2D( void );
    void D3D12API_ResetState3D( void );
    void D3D12API_SetPortalRendering( qboolean enabled, const float* flipMatrix, const float* plane );
    void D3D12API_SetDepthRange( float minRange, float maxRange );
    void D3D12API_BeginFrame(void);
    void D3D12API_EndFrame( void );
    void D3D12API_MakeCurrent( qboolean current );
    void D3D12API_DrawSkyBox( const skyboxDrawInfo_t* skybox, const float* eye_origin, const float* colorTint );
    void D3D12API_DrawStageDepth( const shaderCommands_t *input );
    void D3D12API_DrawStageSky( const shaderCommands_t *input );
    void D3D12API_DrawStageGeneric( const shaderCommands_t *input );
    void D3D12API_DrawStageVertexLitTexture( const shaderCommands_t *input );
    void D3D12API_DrawStageLightmappedMultitexture( const shaderCommands_t *input );

    void D3D12API_DebugDrawAxis( void );
    void D3D12API_DebugDrawTris( const shaderCommands_t *input );
    void D3D12API_DebugDrawNormals( const shaderCommands_t *input );
    void D3D12API_DebugDrawPolygon( int color, int numPoints, const float* points );
    void D3D12API_DebugDraw( void );
}
