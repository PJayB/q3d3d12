#ifndef __GFX_DRIVER_H__
#define __GFX_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../renderer/tr_local.h"
#include "../renderer/tr_layer.h"
#include "../qcommon/qcommon.h"

void D3D12_DriverInit( graphicsApi_t* api );
qboolean D3D12_DriverIsSupported(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
