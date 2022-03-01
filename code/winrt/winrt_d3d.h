#ifndef __WINRT_D3D_H__
#define __WINRT_D3D_H__

#include <Unknwnbase.h>

extern "C" {

#ifdef Q_HAS_D3D11
    void D3D11_Window_Init();
    void D3D11_Window_Shutdown();
#endif

#ifdef Q_HAS_D3D12
    void D3D12_Window_Init();
    void D3D12_Window_Shutdown();
#endif

}

void D3D_WinRT_InitDeferral();
void D3D_WinRT_NotifyNewWindow( IUnknown* window, int logicalSizeX, int logicalSizeY );
void D3D_WinRT_NotifyWindowResize( int logicalSizeX, int logicalSizeY );
void D3D_WinRT_CleanupDeferral();

#endif
