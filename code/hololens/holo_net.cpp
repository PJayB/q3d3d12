extern "C" {
#   include "../qshared/q_shared.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <ppl.h>
#include <ppltasks.h>
#include <assert.h>
#include <map>

#include "../gamethread/gt_shared.h"
#include "../winrt/winrt_utils.h"
#include "holo_net.h"

using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;

static qboolean networkingEnabled = qfalse;

static cvar_t	*net_noudp;

DatagramSocket^ g_Socket = nullptr;
std::map<netadr_t, IOutputStream^> g_StreamOutCache;
GameThread::MessageQueue g_NetMsgQueue;

static const USHORT c_MulticastAddress[] = { 0xFF02, 0, 0, 0, 0, 0, 0, 2 }; // All routers
static const USHORT c_BroadcastAddress[] = { 0xFFFF, 0xFFFF, 0, 0, 0, 0, 0, 0 };

// For the g_StreamOutCache map
static bool operator < (const netadr_t& a, const netadr_t& b)
{
    return memcmp( &a, &b, sizeof( netadr_t ) ) < 0;
}

/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
2001:0db8:85a3:0000:0000:8a2e:0370:7334
=============
*/
bool NET_StringToIpv6( const char* const s, BYTE* addressBytes )
{
    USHORT address[8];
    USHORT group = 0;
    int groupSplit = -1;
    int groupIndex = 0;
    char previous = 0;
    const char* cur = s;
    for ( ; *cur; ++cur )
    {
        // Check if there are too many characters
        if ( groupIndex == 8 )
            return false;

        // Check for :: abbreviations
        if ( *cur == ':' && previous == ':' )
        {
            // Can't have two sets of ::
            if ( groupSplit != -1 )
                return false;
            
            // Can't have this at the end
            if ( groupIndex == 7 )
                return false;

            // We'll keep filling all the groups in sequence, then shuffle them up in a minute
            groupSplit = groupIndex;
        }
        // Check for group delimiter
        else if ( *cur == ':' )
        {
            // Push the current group (if there is one)
            address[groupIndex++] = group;
            group = 0;
        }
        else
        {
            // If we have stuff in the top 4 bits, this is a bad address
            if ( group & 0xF000 )
                return false;

            byte nibble = 0;

            // Check for a valid character
            if ( *cur >= 'A' && *cur <= 'Z' )
                nibble = *cur - 'A' + 10;
            else if ( *cur >= 'a' && *cur <= 'z' )
                nibble = *cur - 'a' + 10;
            else if ( *cur >= '0' && *cur <= '9' )
                nibble = *cur - '0';
            else
                return false;

            group = (group << 4) | nibble;
        }
        
        previous = *cur;
    }

    if ( previous == 0 )
        return false;

    // Push the final group
    address[groupIndex++] = group;

    // Group delimiter? Shuffle them up!
    if ( groupSplit != -1 )
    {
        int last;
        for ( last = 7; groupIndex >= groupSplit; --last) 
            address[last] = address[--groupIndex];
        for ( int empty = groupSplit; empty <= last; ++empty )
            address[empty] = 0;
    }

    memcpy( addressBytes, address, sizeof( address ) );

    return true;
}

bool NET_Ipv6ToString( const BYTE* address, char* str, size_t len )
{
    if ( len < 8 * 5 )
        return false;

    const USHORT* addr16 = reinterpret_cast<const USHORT*>( address );

    sprintf_s( str, len, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
        addr16[0], addr16[1], addr16[2], addr16[3], 
        addr16[4], addr16[5], addr16[6], addr16[7] );

    return true;
}

bool NET_StringToIpv4( const char* s, BYTE* addressBytes )
{
    BYTE address[4];
    UINT v = 0;
    UINT i = 0;
    bool groupValid = false;

    for ( ; *s; ++s )
    {
        // Too many characters?
        if ( i == 4 )
            return false;

        // Push a new group
        if ( *s == '.' )
        {
            if ( !groupValid )
                return false;
            if ( v > 255 )
                return false;
            address[i++] = v;
            v = 0;
            groupValid = false;
        }
        else
        {
            byte nibble = 0;

            // Check for a valid character
            if ( *s >= '0' && *s <= '9' )
                nibble = *s - '0';
            else
                return false;

            v = (v * 10) + nibble;
            groupValid = true;
        }
    }

    // push the last group
    if ( !groupValid )
        return false;
    if ( v > 255 )
        return false;
    address[i++] = v;

    memcpy( addressBytes, address, sizeof( address ) );
    return true;
}

bool NET_Ipv4ToString( const BYTE* address, char* str, size_t len )
{
    if ( len < 3 * 5 )
        return false;

    const USHORT* addr16 = reinterpret_cast<const USHORT*>( address );

    sprintf_s( str, len, "%d.%d.%d.%d",
        address[0], address[1],
        address[2], address[3] );

    return true;
}

bool NET_IsIpv6NetAdr( const void* ip )
{
    return 
        ((const UINT*) ip)[1] != 0 ||
        ((const UINT*) ip)[2] != 0 ||
        ((const UINT*) ip)[3] != 0;
}


C_EXPORT qboolean Sys_StringToAdr( const char *s, netadr_t *a ) {

    // @pjb: We won't support IPX any more here.
    if ( !NET_StringToIpv6( s, a->ip ) )
        if ( !NET_StringToIpv4( s, a->ip ) )
            return qfalse;
    a->type = NA_IP;
    a->port = 0;
    return qtrue;
}

/*
=============
NET_GetMessageArguments
=============
*/
DatagramSocketMessageReceivedEventArgs^ NET_GetMessageArguments( const GameThread::MSG* msg ) 
{
    IUnknown* iface = reinterpret_cast<IUnknown*>( msg->Param0 );
    return WinRT::GetType<DatagramSocketMessageReceivedEventArgs>( iface );
}

/*
==================
NET_OpenNewStream
==================
*/
IOutputStream^ NET_OpenNewStream( const netadr_t* to )
{
    HANDLE hEvent = CreateEventEx( nullptr, nullptr, 0, EVENT_ALL_ACCESS );

    IOutputStream^ stream = nullptr;

    char ipAddress[64];
    if ( NET_IsIpv6NetAdr( to->ip ) )
        NET_Ipv6ToString( to->ip, ipAddress, sizeof( ipAddress ) );
    else 
        NET_Ipv4ToString( to->ip, ipAddress, sizeof( ipAddress ) );

    HostName^ remoteHostName = ref new HostName( WinRT::CopyString( ipAddress ) );

    concurrency::create_task( g_Socket->GetOutputStreamAsync( remoteHostName, BigShort( to->port ).ToString() ) ).then(
        [hEvent, &stream] ( concurrency::task<IOutputStream^> t )
    {
        try
        {
            stream = t.get();
        }
        catch ( Platform::Exception^ ex )
        {
            OutputDebugStringW( ex->Message->Data() );
            stream = nullptr;
        }

        SetEvent( hEvent );
    } );
    
    WaitForSingleObjectEx( hEvent, INFINITE, FALSE );
    CloseHandle( hEvent );

    return stream;
}

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
C_EXPORT qboolean Sys_GetPacket( netadr_t *net_from, msg_t *net_message ) {
    
    qboolean hasPacket = qfalse;

    // Pump one message
    GameThread::MSG msg;
    if ( !g_NetMsgQueue.Pop( &msg ) )
        return qfalse;

    // It must be a packet event
    if ( msg.Message != NET_MSG_INCOMING_MESSAGE ) 
    {
        assert(0);
        return qfalse;
    }

    auto args = NET_GetMessageArguments( &msg );
    auto dataReader = args->GetDataReader();

    // How much data is there in the buffer?
    net_message->cursize = min( net_message->maxsize, dataReader->UnconsumedBufferLength );

    // @pjb: oh Jesus the perf

    // Read the data into the packet
    auto buf = ref new Platform::Array<byte>( net_message->cursize );
    dataReader->ReadBytes( buf );

    Com_Memcpy( net_message->data, buf->Data, net_message->cursize );

    Com_Memset( net_from, 0, sizeof( *net_from ) );

    // Oh my.
    char seriousHack[256];
    WinRT::CopyString( args->RemoteAddress->DisplayName, seriousHack, sizeof( seriousHack ) );
    NET_StringToAdr( seriousHack, net_from );
    
    // Get the address of the sender
    UINT port = 0;
    swscanf_s( args->RemotePort->Data(), L"%d", &port );

    net_from->port = BigShort( port );

    return qtrue;
}

/*
==================
Sys_SendPacketToAddress
==================
*/
void Sys_SendPacketToAddress( int length, const void *data, const netadr_t& to )
{
    IOutputStream^ stream = nullptr;

    // Look up the address in the output stream cache
    auto cache = g_StreamOutCache.find( to );
    if ( cache == std::end( g_StreamOutCache ) )
    {
        // Not cached. Cache it now!
        stream = NET_OpenNewStream( &to );
        g_StreamOutCache[to] = stream;
    }
    else
    {
        stream = cache->second;
    }

    // Barf :(
    if ( stream == nullptr )
        return;

    // @pjb: todo: the efficiency of this really concerns me
    try
    {
        DataWriter^ dw = ref new DataWriter();
        dw->WriteBytes( ref new Platform::Array<byte>( (byte*) data, length ) );
        stream->WriteAsync( dw->DetachBuffer() );
    }
    catch ( Platform::Exception^ ex )
    {
        OutputDebugStringW( ex->Message->Data() );
    }
}

/*
==================
Sys_SendPacket
==================
*/
C_EXPORT void Sys_SendPacket( int length, const void *data, netadr_t to )
{
    // Ignore IPX requests
    if ( g_Socket == nullptr || to.type == NA_IPX || to.type == NA_BROADCAST_IPX )
        return;

    if ( to.type == NA_BROADCAST )
    {
        // Multicast IPv6
        static_assert( sizeof( c_MulticastAddress ) == sizeof( to.ip ), "Whoops" );
        Com_Memcpy( to.ip, c_MulticastAddress, sizeof( c_MulticastAddress ) );
        
        Sys_SendPacketToAddress( length, data, to );

        // Broadcast IPv4
        static_assert( sizeof( c_BroadcastAddress ) == sizeof( to.ip ), "Whoops" );
        Com_Memcpy( to.ip, c_BroadcastAddress, sizeof( c_BroadcastAddress ) );
        
        Sys_SendPacketToAddress( length, data, to );
    }
    else 
    {
        Sys_SendPacketToAddress( length, data, to );
    }
}

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
C_EXPORT qboolean Sys_IsLANAddress( netadr_t adr ) {
    // @pjb: todo
    return qtrue;
}

/*
==================
Sys_ShowIP
==================
*/
C_EXPORT void Sys_ShowIP(void) {
    char hostnameTmp[256];
    char adapterIdTmp[256];
    // @pjb: todo: fatal error C1001: An internal error has occurred in the compiler.
    // for each ( HostName^ localHostInfo in NetworkInformation::GetHostNames() )
    auto hostNames = NetworkInformation::GetHostNames();
    for ( int i = 0; i < hostNames->Size; ++i )
    {
        auto localHostInfo = hostNames->GetAt(i);
        if ( localHostInfo->IPInformation != nullptr )
        {
            WinRT::CopyString( localHostInfo->DisplayName, hostnameTmp, sizeof( hostnameTmp ) );
            WinRT::CopyString( localHostInfo->IPInformation->NetworkAdapter->NetworkAdapterId.ToString(), adapterIdTmp, sizeof( adapterIdTmp ) );

            Com_Printf( "Address: %s / Adapter: %s\n", hostnameTmp, adapterIdTmp );
        }
    }
}

/*
====================
NET_OnMessageReceived

DO NOT USE ANY QUAKE FUNCTIONS. (Sys_Milliseconds is an exception.)
This function MUST interop with the game through the message queue.
====================
*/
void NET_OnMessageReceived( 
    DatagramSocket^ socket, 
    DatagramSocketMessageReceivedEventArgs^ args )
{
    GameThread::MSG msg;
    INIT_MSG( msg );
    msg.Message = NET_MSG_INCOMING_MESSAGE;
    msg.Param0 = (size_t) WinRT::GetPointer( args );
    g_NetMsgQueue.Post( &msg );
}

/*
=====================
NET_StartListeningOnPort
=====================
*/
HRESULT NET_StartListeningOnPort( HostName^ localHost, int port )
{
    Platform::String^ serviceName = port.ToString();

    // Sync primitives
    HANDLE bindEndPointComplete = CreateEventEx( nullptr, nullptr, 0, EVENT_ALL_ACCESS );
    HRESULT status = E_FAIL;

#ifdef _WIN32_WINNT_WINBLUE
    g_Socket->Control->DontFragment = true;
    g_Socket->Control->InboundBufferSizeInBytes = 2048;
#endif

    // Listen on socket
    g_Socket->MessageReceived += ref new TypedEventHandler<DatagramSocket^, DatagramSocketMessageReceivedEventArgs^>( 
        NET_OnMessageReceived );

    // @pjb: todo: on Blue we can do .wait()

    if ( localHost == nullptr )
    {
        // Try to bind to a specific address.
        concurrency::create_task( g_Socket->BindServiceNameAsync( serviceName ) ).then(
            [bindEndPointComplete, &status] ( concurrency::task<void> previousTask )
        {
            try
            {
                // Try getting an exception.
                previousTask.get();
                status = S_OK;
            }
            catch ( Platform::Exception^ exception )
            {
                Com_Printf( "Couldn't not bind: %S\n", exception->Message->Data() );
                status = exception->HResult;
            }

            SetEvent( bindEndPointComplete );
        });
    }
    else
    {
        // Try to bind to a specific address.
        concurrency::create_task( g_Socket->BindEndpointAsync( localHost, serviceName ) ).then(
            [bindEndPointComplete, &status] ( concurrency::task<void> previousTask)
        {
            try
            {
                // Try getting an exception.
                previousTask.get();
                status = S_OK;
            }
            catch ( Platform::Exception^ exception )
            {
                Com_Printf( "Couldn't not bind: %S\n", exception->Message->Data() );
                status = exception->HResult;
            }

            SetEvent( bindEndPointComplete );
        });
    }

    WaitForSingleObjectEx( bindEndPointComplete, INFINITE, FALSE );
    CloseHandle( bindEndPointComplete );

    // Join multicast group
    char multicastGroupStr[256];
    NET_Ipv6ToString( (const BYTE*) c_MulticastAddress, multicastGroupStr, sizeof( multicastGroupStr ) );
    g_Socket->JoinMulticastGroup( ref new HostName( WinRT::CopyString( multicastGroupStr ) ) );

    return status;
}

/*
====================
NET_StartListening
====================
*/
void NET_StartListening( void ) {

	cvar_t* ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH );
	int port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH )->integer;

    HostName^ localHost = nullptr;

    auto hostNames = NetworkInformation::GetHostNames();
    if ( !hostNames->Size )
    {
        Com_Printf( "No adapters.\n" );
        return;
    }

    // First, see if there's an adapter that uses this port
    char hostNameTmp[256];
    for ( int i = 0; *ip->string && localHost == nullptr && i < hostNames->Size; ++i )
    {
        auto localHostInfo = hostNames->GetAt(i);
        if ( localHostInfo->IPInformation != nullptr )
        {
            WinRT::CopyString( localHostInfo->DisplayName, hostNameTmp, sizeof( hostNameTmp ) );
            if ( Q_strncmp( ip->string, hostNameTmp, sizeof(hostNameTmp) ) == 0 )
            {
                localHost = localHostInfo;
            }
        }
    }

    if ( localHost != nullptr )
    {
        WinRT::CopyString( localHost->DisplayName, hostNameTmp, sizeof( hostNameTmp ) );
        Com_Printf( "Found localHost %s.\n", hostNameTmp );
        Cvar_Set( "net_ip", hostNameTmp );
    }

    g_Socket = ref new DatagramSocket();
        
	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one
	for( int i = 0 ; i < 10 ; i++ ) {
		if ( SUCCEEDED( NET_StartListeningOnPort( localHost, port + i ) ) ) {
			Cvar_SetValue( "net_port", port + i );
			return;
		}
	}
	Com_Printf( "WARNING: Couldn't allocate IP port\n");
}
/*
====================
NET_StopListening
====================
*/
void NET_StopListening( void ) 
{
    // Must handle all messages before we clean up as 
    // they could contain pointers that need to be 
    // freed.
    GameThread::MSG msg;
    while ( g_NetMsgQueue.Pop( &msg ) ) 
    {
        // Discard message arguments
        NET_GetMessageArguments( &msg );
    }

    // Clear the output stream cache
    g_StreamOutCache.clear();

    // Close the socket
    g_Socket = nullptr;
}

/*
====================
NET_GetCvars
====================
*/
static qboolean NET_GetCvars( void ) {
	qboolean	modified;

	modified = qfalse;

	if( net_noudp && net_noudp->modified ) {
		modified = qtrue;
	}
	net_noudp = Cvar_Get( "net_noudp", "0", CVAR_LATCH | CVAR_ARCHIVE );

	return modified;
}

/*
====================
NET_Config
====================
*/
void NET_Config( qboolean enableNetworking ) {

	qboolean	modified;
	qboolean	stop;
	qboolean	start;

	// get any latched changes to cvars
	modified = NET_GetCvars();

	if( net_noudp->integer ) {
		enableNetworking = qfalse;
	}

	// if enable state is the same and no cvars were modified, we have nothing to do
	if( enableNetworking == networkingEnabled && !modified ) {
		return;
	}

	if( enableNetworking == networkingEnabled ) {
		if( enableNetworking ) {
			stop = qtrue;
			start = qtrue;
		}
		else {
			stop = qfalse;
			start = qfalse;
		}
	}
	else {
		if( enableNetworking ) {
			stop = qfalse;
			start = qtrue;
		}
		else {
			stop = qtrue;
			start = qfalse;
		}
		networkingEnabled = enableNetworking;
	}

	if( stop ) {
		NET_StopListening();
	}

	if( start ) {
		NET_StartListening();
	}
}


/*
====================
NET_Init
====================
*/
C_EXPORT void NET_Init( void ) {

    // @pjb: todo

    // Print the local addresses and adapters
	Com_Printf( "Init WinRT Sockets\n" );

	Com_Printf( "... [INFO] Set net_ip to specify a use a specific local address.\n" );
	Com_Printf( "... [INFO] Set net_port to specify a port to listen on.\n" );

    Sys_ShowIP();

    // Configure the addresses
	NET_Config( qtrue );
}

/*
====================
NET_Shutdown
====================
*/
C_EXPORT void NET_Shutdown( void ) {
	NET_Config( qfalse );
	Com_Printf( "Shutdown WinRT Sockets\n" );
}

/*
====================
NET_Sleep

sleeps msec or until net socket is ready
====================
*/
C_EXPORT void NET_Sleep( int msec ) {
    // @pjb: nothing to do here.
}

/*
====================
NET_Restart_f
====================
*/
C_EXPORT void NET_Restart( void ) {
	NET_Config( networkingEnabled );
}
