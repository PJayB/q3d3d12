/*
=========================================================

DDS LOADING

=========================================================
*/

extern "C" {
#   include "tr_local.h"
#   include "tr_layer.h"
}

// @pjb: DDS loading
#include "../d3d/DirectXTK/dds.h"

using namespace DirectX;

extern "C" void LoadDDS( const char *name, byte **pic, int *width, int *height )
{
	int		length;
	byte	*buffer;

	//
	// load the file
	//
	length = FS_ReadFile( ( char * ) name, (void **)&buffer);
	if (!buffer) {
		return;
	}

    if ( length < (int)( sizeof(DDS_HEADER) + sizeof(uint32_t) ) )
    {
        Com_Error( ERR_DROP, "LoadDDS: file not large enough to be a DDS texture" );
    }

    //
    // Extract information from the file
    //

    // DDS files always start with the same magic number ("DDS ")
    uint32_t dwMagicNumber = *( const uint32_t* )( buffer );
    if (dwMagicNumber != DDS_MAGIC)
    {
        Com_Error( ERR_DROP, "LoadDDS: invalid dds magic" );
    }

    const DDS_HEADER* pHeader = (const DDS_HEADER*) ( buffer + sizeof(uint32_t) );
    if (pHeader->size != sizeof(DDS_HEADER) ||
        pHeader->ddspf.size != sizeof(DDS_PIXELFORMAT))
    {
        Com_Error( ERR_DROP, "LoadDDS: invalid dds layout" );
    }

    // Check for DX10 extension
    if ((pHeader->ddspf.flags & DDS_FOURCC) &&
        (MAKEFOURCC( 'D', 'X', '1', '0' ) == pHeader->ddspf.fourCC))
    {
        // Must be long enough for both headers and magic value
        if (length < ( sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10) ) )
        {
            Com_Error( ERR_DROP, "LoadDDS: dds not long enough to store DX10 extension. Corrupt file?" );
        }
    }
    
    //
    // Allocate space for the entire DDS file - we're passing the whole thing, header
    // and all, through pic so that the graphics subsystem can have more information
    // when setting up the image.
    //
    *pic = (byte*) RI_Malloc( length );

    memcpy( *pic, buffer, length );

    *width = (int) pHeader->width;
    *height = (int) pHeader->height;

	FS_FreeFile( buffer );
}

