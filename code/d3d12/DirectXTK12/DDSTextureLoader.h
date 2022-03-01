//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.h
//
// Functions for loading a DDS texture and creating a Direct3D 11 runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include "../d3d/DirectXTK/dds.h"

#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#pragma warning(pop)

namespace DirectX
{
    enum DDS_ALPHA_MODE
    {
        DDS_ALPHA_MODE_UNKNOWN       = 0,
        DDS_ALPHA_MODE_STRAIGHT      = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE        = 3,
        DDS_ALPHA_MODE_CUSTOM        = 4,
    };

    struct DDS_DESC
    {
        UINT Width;
        UINT Height;
        UINT Depth;
        UINT ArraySize;
        UINT MipCount;
        UINT SkipMips;
        UINT SubResourceCount;
        BOOL IsCubeMap;
        DXGI_FORMAT Format;
        DDS_ALPHA_MODE AlphaMode;
    };

    HRESULT GetTextureHeadersFromDDS( _In_ const uint8_t* bitData,
                                      _Out_ const DDS_HEADER** ppHeader,
                                      _Out_ const uint8_t** ppBits );

    HRESULT GetTextureInfoFromDDS( _In_ const DDS_HEADER* header,
                                   _In_ size_t maxsize,
                                   _Out_ DDS_DESC* pOut );

    HRESULT GetTextureDataFromDDS( _In_ const DDS_DESC* desc,
                                   _In_ const uint8_t* bitData,
                                   _Out_writes_(desc->SubResourceCount) D3D12_SUBRESOURCE_DATA* subresources );

    size_t BitsPerPixel(           _In_ DXGI_FORMAT fmt );
    DXGI_FORMAT MakeSRGB(          _In_ DXGI_FORMAT nonSrgb );
}
