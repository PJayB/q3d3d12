#include "tr_local.h"
#include "tr_layer.h"

graphicsApi_t tr_graphicsApi;

void R_ResetGraphicsLayer()
{
    memset(&tr_graphicsApi, 0, sizeof(tr_graphicsApi));
}

void R_ValidateGraphicsLayer()
{
    // Validate all pointers are not null
    const graphicsApi_t* layer = &tr_graphicsApi;
    for (
        size_t* fptr = (size_t*) layer;
        fptr != (size_t*) (layer + 1);
        ++fptr)
    {
        if (!*fptr)
        {
            RI_Error(ERR_FATAL, "Incomplete graphics API implementation at offset %d\n", 
                (int)(fptr - (size_t*)layer));
        }
    }
}
