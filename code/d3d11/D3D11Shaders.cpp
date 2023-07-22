#include "D3D11Shaders.h"

#define HLSL_PATH "hlsl"
#define HLSL_EXTENSION "cso"

ID3D11PixelShader* LoadPixelShader( const char* name )
{
    void* blob = nullptr;
    int len = FS_ReadFile( va( "%s/%s.%s", HLSL_PATH, name, HLSL_EXTENSION ), &blob );
    if ( !len || !blob  ) {
        RI_Error( ERR_FATAL, "Failed to load pixel shader '%s'\n", name );
    }

    ID3D11PixelShader* shader = nullptr;
    HRESULT hr = g_pDevice->CreatePixelShader(
        blob,
        len,
        nullptr,
        &shader );
    if ( FAILED( hr ) ) {
        RI_Error( ERR_FATAL, "Failed to load pixel shader '%s': 0x%08X\n", name, hr );
    }

    FS_FreeFile( blob );

    return shader;
}

ID3D11VertexShader* LoadVertexShader( const char* name, d3dVertexShaderBlob_t* blobOut )
{
    void* blob = nullptr;
    int len = FS_ReadFile( va( "%s/%s.%s", HLSL_PATH, name, HLSL_EXTENSION ), &blob );
    if ( !len || !blob ) {
        RI_Error( ERR_FATAL, "Failed to load vertex shader '%s'\n", name );
    }

    ID3D11VertexShader* shader = nullptr;
    HRESULT hr = g_pDevice->CreateVertexShader(
        blob,
        len,
        nullptr,
        &shader );
    if ( FAILED( hr ) ) {
        RI_Error( ERR_FATAL, "Failed to create vertex shader '%s': 0x%08X\n", name, hr );
    }

    if ( blobOut )
    {
        blobOut->blob = blob;
        blobOut->len = len;
    }
    else
    {
        FS_FreeFile( blob );
    }

    return shader;
}

void InitShaders()
{

}

void DestroyShaders()
{

}

D3D_PUBLIC void D3D11API_CreateShader( const shader_t* )
{
    // Not required.
}
