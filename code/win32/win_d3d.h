#ifndef __DX12_PUBLIC_H__
#define __DX12_PUBLIC_H__

#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void D3D12_Window_Init( void );
void D3D12_Window_Shutdown( void );
HWND D3D12_Window_GetWindowHandle( void );

#ifdef __cplusplus
} // extern "C"
#endif

#endif
