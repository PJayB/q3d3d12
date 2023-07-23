#include "../d3d/Direct3DCommon.h"

#ifdef Q_HAS_D3D12
#   include "../d3d12/D3D12Core.h"
#   include "../d3d12/D3D12QAPI.h"
#   include "../d3d12/Device.h"
#   include "../d3d12/SwapChain.h"
#   include "../d3d12/DescriptorHeaps.h"
#   include "../d3d12/GlobalCommandList.h"
#endif

extern "C" {
#   include "resource.h"
#   include "win_local.h"
}

#define	WINDOW_CLASS_NAME	"Quake 3: Arena (DirectX 12)"

static HWND g_hWnd = nullptr;

//----------------------------------------------------------------------------
// WndProc: Intercepts window events before passing them on to the game.
//----------------------------------------------------------------------------
static LONG WINAPI Direct3DWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
        // If our window was closed, lose our cached handle
		if (hWnd == g_hWnd) {
            g_hWnd = NULL;
        }
		
        // We want to pass this message on to the MainWndProc
        break;
	case WM_SIZE:

        // @pjb: todo: recreate swapchain?
        // @pjb: actually fuck that. disable window sizing.

        break;

    default:
        break;
    }

    return MainWndProc( hWnd, uMsg, wParam, lParam );
}

//----------------------------------------------------------------------------
// Register the window class.
//----------------------------------------------------------------------------
static BOOL RegisterWindowClass()
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC) Direct3DWndProc;
    wc.cbClsExtra = 0;
    wc.hInstance = g_wv.hInstance;
    wc.hIcon = LoadIcon( g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    return ::RegisterClass( &wc ) != 0;
}

//----------------------------------------------------------------------------
// Creates a window to render our game in.
//----------------------------------------------------------------------------
static HWND CreateGameWindow( int x, int y, int width, int height, bool fullscreen )
{
    UINT exStyle;
    UINT style;

	if ( fullscreen )
	{ 
		exStyle = WS_EX_TOPMOST;
		style = WS_POPUP|WS_VISIBLE|WS_SYSMENU;
	}
	else
	{
		exStyle = 0;
		style = WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE|WS_SYSMENU;
	}
  
    RECT rect = { x, y, x + width, y + height };
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    // Make sure it's on-screen 
    if ( rect.top < 0 ) 
    {
        rect.bottom -= rect.top;
        rect.top = 0;
    }
    if ( rect.left < 0 ) 
    {
        rect.right -= rect.left;
        rect.left = 0;
    }

    // @pjb: todo: right and bottom edges of the monitor

    return CreateWindowEx(
        exStyle, 
        WINDOW_CLASS_NAME,
        "Quake 3: Arena (DirectX 12)",
        style, 
        rect.left, 
        rect.top, 
        rect.right - rect.left, 
        rect.bottom - rect.top, 
        NULL, 
        NULL, 
        g_wv.hInstance, 
        NULL );
}

#ifdef Q_HAS_D3D12
//----------------------------------------------------------------------------
// Creates a window, initializes the driver and sets up Direct3D 12 state.
//----------------------------------------------------------------------------
D3D_PUBLIC void D3D12_Window_Init( void )
{
	RI_Printf( PRINT_ALL, "Initializing DX12 subsystem\n" );

    if ( !RegisterWindowClass() )
    {
        //Com_Error( ERR_FATAL, "Failed to register DX12 window class.\n" );
        //return;
    }


    // @pjb: todo: fullscreen stuff
    bool fullscreen = r_fullscreen->integer != 0;

    cvar_t* vid_xpos = Cvar_Get ("vid_xpos", "", 0);
    cvar_t* vid_ypos = Cvar_Get ("vid_ypos", "", 0);

    g_hWnd = CreateGameWindow( 
        vid_xpos->integer,
        vid_ypos->integer,
        vdConfig.vidWidth, 
        vdConfig.vidHeight,
        fullscreen);
    if ( !g_hWnd )
    {
        Com_Error( ERR_FATAL, "Failed to create Direct3D 12 window.\n" );
        return;
    }

	QD3D12::Device::Init();

    DXGI_SWAP_CHAIN_DESC1 scDesc;
	ZeroMemory( &scDesc, sizeof(scDesc) );
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    scDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    QD3D12::SwapChain::GetDescFromConfig( &scDesc );
    
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsd;
    ZeroMemory( &fsd, sizeof(fsd) );
    fsd.Windowed = r_fullscreen->integer <= 0;
    fsd.Scaling = DXGI_MODE_SCALING_STRETCHED;

	QDXGISwapChain* swapChain = nullptr;
    HRESULT hr = QD3D12::SwapChain::CreateSwapChain(
		QD3D12::Device::GetQ(),
		g_hWnd, 
		&scDesc, 
		&fsd, 
		&swapChain);
    if (FAILED(hr))
    {
        // @pjb: todo: if swapchain desc is too fancy, fall back
        Com_Error( ERR_FATAL, "Failed to create Direct3D 12 swapchain: %s.\n", QD3D::HResultToString(hr) );
        return;
    }

	QD3D12::SwapChain::Init(swapChain);
	QD3D12::GlobalCommandList::KickOff();

    ::ShowWindow( g_hWnd, SW_SHOW );
    ::UpdateWindow( g_hWnd );
	::SetForegroundWindow( g_hWnd );
	::SetFocus( g_hWnd );
}

//----------------------------------------------------------------------------
// Cleans up and stops D3D12, and closes the window.
//----------------------------------------------------------------------------
D3D_PUBLIC void D3D12_Window_Shutdown( void )
{
    QD3D12::SwapChain::Destroy();
    QD3D12::Device::Destroy();

    ::UnregisterClass( WINDOW_CLASS_NAME, g_wv.hInstance );
    ::DestroyWindow( g_hWnd );

    g_hWnd = NULL;
}
#endif

//----------------------------------------------------------------------------
// Returns the window handle
//----------------------------------------------------------------------------
D3D_PUBLIC HWND D3D12_Window_GetWindowHandle( void )
{
    return g_hWnd;
}

