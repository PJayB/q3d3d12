/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// net_wins.c

extern "C" {
#   include "../qshared/q_shared.h"
#   include "../qcommon/qcommon.h"
#   include "../win/win_shared.h"
}

#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#pragma warning(disable : 4273)
#include <vector>
#include <atomic>
#pragma warning(default : 4273)

using namespace Windows::Xbox::Networking;
using namespace Windows::Networking::Connectivity;

static std::atomic<bool> bNetworkConnected = false;

std::vector<SecureDeviceAssociationTemplate^> sdaTemplates;

static WSADATA	winsockdata;
static qboolean	winsockInitialized = qfalse;
static qboolean networkingEnabled = qfalse;

static cvar_t	*net_noudp;

static SOCKET	ip_socket;
static SOCKET	socks_socket;

#define	MAX_IPS		16
static	int		numIP;
static	byte	localIP[MAX_IPS][4];

//=============================================================================


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


/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void ) {
	int		code;

	code = WSAGetLastError();
	switch( code ) {
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}

void NetadrToSockadr( netadr_t *a, void* ptr ) {
	if( a->type == NA_BROADCAST ) {
        struct sockaddr_in* s = (struct sockaddr_in*) ptr;
	    memset( s, 0, sizeof(*s) );
		s->sin_family = AF_INET;
		s->sin_port = a->port;
		s->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if( a->type == NA_IP ) {
        if ( NET_IsIpv6NetAdr( a->ip ) ) {
            struct sockaddr_in6* s = (struct sockaddr_in6*) ptr;
	        memset( s, 0, sizeof(*s) );
		    s->sin6_family = AF_INET6;
            Com_Memcpy( &s->sin6_addr, a->ip, sizeof(a->ip) );
		    s->sin6_port = a->port;
        } else {
            struct sockaddr_in* s = (struct sockaddr_in*) ptr;
	        memset( s, 0, sizeof(*s) );
            s->sin_family = AF_INET;
            Com_Memcpy( &s->sin_addr, a->ip, sizeof(int) );
            s->sin_port = a->port;
        }
	}
    else {
        assert(0);
    }
}


void SockadrToNetadr( struct sockaddr_in6 *s, netadr_t *a ) {
    Com_Memset( a->ip, 0, sizeof(a->ip) );

	if (s->sin6_family == AF_INET) {
		a->type = NA_IP;
        Com_Memcpy( a->ip, &((struct sockaddr_in *)s)->sin_addr, sizeof(int) );
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
	else if (s->sin6_family == AF_INET6) {
		a->type = NA_IP;
        Com_Memcpy( a->ip, &s->sin6_addr, sizeof(a->ip) );
		a->port = s->sin6_port;
	}
	else {
        assert(0);
	}
}

qboolean NET_GetHostByName( const char *s, int port, void* sadr ) {
    struct addrinfo hints;
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET6;

    char portStr[10];
    sprintf_s( portStr, _countof(portStr), "%d", port );

    ADDRINFOA* info = nullptr;
    if ( GetAddrInfoA( s, portStr, nullptr, &info ) != 0 ) {
		Com_Printf( "NET_GetHostByName: %s\n", NET_ErrorString() );
        return qfalse;
    }

    // Pick the first address
    qboolean gotResult = qfalse;
    for ( ADDRINFOA* ptr = info; ptr != nullptr; ptr = ptr->ai_next )
    {
        if ( ptr->ai_family == AF_INET6 ) {
            Com_Memcpy( sadr, ptr->ai_addr, ptr->ai_addrlen );
            gotResult = qtrue;
            break;
        }
        else if ( ptr->ai_family == AF_INET ) {
            const sockaddr_in* ipv4 = (sockaddr_in*) ptr->ai_addr;
            sockaddr_in6* ipv6 = (sockaddr_in6*) sadr;

            ipv6->sin6_family = AF_INET6;
            ipv6->sin6_port = ipv4->sin_port;
            Com_Memset( &ipv6->sin6_addr, 0, sizeof( ipv6->sin6_addr ) );

            ipv6->sin6_addr.u.Word[5] = 0xFFFFU;
            
            Com_Memcpy( &ipv6->sin6_addr.u.Word[6], &ipv4->sin_addr, sizeof( int ) );
            
            gotResult = qtrue;
            break;
        }
    }    

    FreeAddrInfoA(info);

    return gotResult;
}

/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
=============
*/
qboolean Sys_StringToSockaddr( const char *s, int port, struct sockaddr_in6 *sadr ) {

	memset( sadr, 0, sizeof( *sadr ) );

	sadr->sin6_family = AF_INET;
	sadr->sin6_port = 0;

	if( !NET_StringToIpv6( s, (byte*) &sadr->sin6_addr ) ) {

        return NET_GetHostByName( s, port, sadr );
	}
	
	return qtrue;
}

/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
=============
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a ) {
	struct sockaddr_in6 sadr;

    // @pjb: HACK
    int port = PORT_SERVER;
    if ( strstr( s, "update" ) != 0 )
        port = PORT_UPDATE;
	
	if ( !Sys_StringToSockaddr( s, port, &sadr ) ) {
		return qfalse;
	}
	
	SockadrToNetadr( &sadr, a );
	return qtrue;
}

//=============================================================================

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
int	recvfromCount;

qboolean Sys_GetPacket( netadr_t *net_from, msg_t *net_message ) {
	int 	ret;
	struct sockaddr_in6 from;
	int		fromlen;
	int		net_socket;
	int		err;

    // @pjb: can we init the sockets yet? Pretty please?
    if ( ip_socket == 0 && bNetworkConnected && net_noudp && !net_noudp->integer )
    {
	    NET_Config( qtrue );
    }

	net_socket = ip_socket;
	if( !net_socket ) {
		return qfalse;
	}

	fromlen = sizeof(from);
	recvfromCount++;		// performance check
	ret = recvfrom( net_socket, (char *) net_message->data, net_message->maxsize, 0, (struct sockaddr *)&from, &fromlen );
	if (ret == SOCKET_ERROR)
	{
		err = WSAGetLastError();

		if( err == WSAEWOULDBLOCK || err == WSAECONNRESET ) {
			return qfalse;
		}
		Com_Printf( "NET_GetPacket: %s\n", NET_ErrorString() );
		return qfalse;
	}

	if ( net_socket == ip_socket ) {
		memset( ((struct sockaddr_in *)&from)->sin_zero, 0, 8 );
	}

	SockadrToNetadr( &from, net_from );
	net_message->readcount = 0;

	if( ret == net_message->maxsize ) {
		Com_Printf( "Oversize packet from %s\n", NET_AdrToString (*net_from) );
		return qfalse;
	}

	net_message->cursize = ret;
	return qtrue;
}

//=============================================================================

static char socksBuf[4096];

/*
==================
Sys_SendPacket
==================
*/
void Sys_SendPacket( int length, const void *data, netadr_t to ) {
	int				ret;
	struct sockaddr_in6	addr;
	SOCKET			net_socket;

    // Drop packets that are being sent too early
    if ( !bNetworkConnected )
        return;

	if( to.type == NA_BROADCAST ) {
		net_socket = ip_socket;
	}
	else if( to.type == NA_IP ) {
		net_socket = ip_socket;
	}
	else if( to.type == NA_IPX ) {
		return; // @pjb: ignore IPX packets
	}
	else if( to.type == NA_BROADCAST_IPX ) {
		return; // @pjb: ignore IPX packets
	}
	else {
		Com_Error( ERR_FATAL, "Sys_SendPacket: bad address type" );
		return;
	}

	if( !net_socket ) {
		return;
	}

	NetadrToSockadr( &to, &addr );

	ret = sendto( net_socket, (char *) data, length, 0, (const sockaddr*) &addr, sizeof(addr) );
    if( ret == SOCKET_ERROR ) {
		int err = WSAGetLastError();

		// wouldblock is silent
		if( err == WSAEWOULDBLOCK ) {
			return;
		}

		// some PPP links do not allow broadcasts and return an error
		if( ( err == WSAEADDRNOTAVAIL ) && ( to.type == NA_BROADCAST ) ) {
			return;
		}

		Com_Printf( "NET_SendPacket: %s\n", NET_ErrorString() );
	}
}


//=============================================================================

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean Sys_IsLANAddress( netadr_t adr ) {
	int		i;

	if( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	if( adr.type == NA_IPX ) {
		return qtrue;
	}

	if( adr.type != NA_IP ) {
		return qfalse;
	}

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte

	if( adr.ip[0] == 127 && adr.ip[1] == 0 && adr.ip[2] == 0 && adr.ip[3] == 1 ) {
		return qtrue;
	}

	// Class A
	if( (adr.ip[0] & 0x80) == 0x00 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if( adr.ip[0] == localIP[i][0] ) {
				return qtrue;
			}
		}
		// the RFC1918 class a block will pass the above test
		return qfalse;
	}

	// Class B
	if( (adr.ip[0] & 0xc0) == 0x80 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] ) {
				return qtrue;
			}
			// also check against the RFC1918 class b blocks
			if( adr.ip[0] == 172 && localIP[i][0] == 172 && (adr.ip[1] & 0xf0) == 16 && (localIP[i][1] & 0xf0) == 16 ) {
				return qtrue;
			}
		}
		return qfalse;
	}

	// Class C
	for ( i = 0 ; i < numIP ; i++ ) {
		if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2] ) {
			return qtrue;
		}
		// also check against the RFC1918 class c blocks
		if( adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168 ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP(void) {
	int i;

	for (i = 0; i < numIP; i++) {
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
	}
}


//=============================================================================


/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket( char *net_interface, int port ) {
	SOCKET				newsocket;
	struct sockaddr_in6	address;
	qboolean			_true = qtrue;
	int					i = 1;
	int					err;

	if( net_interface ) {
		Com_Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	}
	else {
		Com_Printf( "Opening IP socket: localhost:%i\n", port );
	}

	if( ( newsocket = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
		err = WSAGetLastError();
		if( err != WSAEAFNOSUPPORT ) {
			Com_Printf( "WARNING: UDP_OpenSocket: socket: %s\n", NET_ErrorString() );
		}
		return 0;
	}

    // set sockets options for exclusive IPv6.
    int v6only = 0;
    setsockopt(
        newsocket,
        IPPROTO_IPV6,
        IPV6_V6ONLY,
        (char*) &v6only,
        sizeof( v6only )
        );

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, (u_long *) &_true ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		return 0;
	}

	// make it broadcast capable
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
		return 0;
	}

	ZeroMemory( &address, sizeof( address ) );

	if( net_interface && net_interface[0] && Q_stricmp(net_interface, "localhost") ) {
		Sys_StringToSockaddr( net_interface, port, &address );
	}

	if( port == PORT_ANY ) {
		address.sin6_port = 0;
	}
	else {
		address.sin6_port = htons( (short)port );
	}

	address.sin6_family = AF_INET6;

	if( bind( newsocket, (const sockaddr *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	return newsocket;
}


/*
=====================
NET_GetLocalAddress
=====================
*/
void NET_GetLocalAddress( void ) {
	char				hostname[256];
	struct hostent		*hostInfo;
	int					error;
	char				*p;
	int					ip;
	int					n;

	if( gethostname( hostname, 256 ) == SOCKET_ERROR ) {
		error = WSAGetLastError();
		return;
	}

	hostInfo = gethostbyname( hostname );
	if( !hostInfo ) {
		error = WSAGetLastError();
		return;
	}

	Com_Printf( "Hostname: %s\n", hostInfo->h_name );
	n = 0;
	while( ( p = hostInfo->h_aliases[n++] ) != NULL ) {
		Com_Printf( "Alias: %s\n", p );
	}

	if ( hostInfo->h_addrtype != AF_INET ) {
		return;
	}

	numIP = 0;
	while( ( p = hostInfo->h_addr_list[numIP] ) != NULL && numIP < MAX_IPS ) {
		ip = ntohl( *(int *)p );
		localIP[ numIP ][0] = p[0];
		localIP[ numIP ][1] = p[1];
		localIP[ numIP ][2] = p[2];
		localIP[ numIP ][3] = p[3];
		Com_Printf( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );
		numIP++;
	}
}

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP( void ) {
	cvar_t	*ip;
	int		port;
	int		i;

	ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH );
	port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH )->integer;

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one
	for( i = 0 ; i < 10 ; i++ ) {
		ip_socket = NET_IPSocket( ip->string, port + i );
		if ( ip_socket ) {
			Cvar_SetValue( "net_port", port + i );
			NET_GetLocalAddress();
			return;
		}
	}
	Com_Printf( "WARNING: Couldn't allocate IP port\n");
}


//===================================================================


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
		if ( ip_socket && ip_socket != INVALID_SOCKET ) {
			closesocket( ip_socket );
			ip_socket = 0;
		}

		if ( socks_socket && socks_socket != INVALID_SOCKET ) {
			closesocket( socks_socket );
			socks_socket = 0;
		}
	}

	if( start ) {
		if (! net_noudp->integer ) {
			NET_OpenIP();
		}
	}
}

/*
====================
XBO_UpdateNetworkStatus
====================
*/
void XBO_UpdateNetworkStatus()
{
    // should try/catch around this code    
    ConnectionProfile^ internetConnectionProfile = NetworkInformation::GetInternetConnectionProfile();    
    if (internetConnectionProfile != nullptr)    
    {        
        auto i = internetConnectionProfile->GetNetworkConnectivityLevel();        
        if (Windows::Networking::Connectivity::NetworkConnectivityLevel::XboxLiveAccess == i)        
        {            
            bNetworkConnected = true;            
            return;        
        }    
    }    

    bNetworkConnected = false;
}

/*
====================
XBO_OnNetworkStatusChanged
====================
*/
void XBO_OnNetworkStatusChanged( Platform::Object^ sender )
{
    XBO_UpdateNetworkStatus();
}

/*
====================
NET_Init
====================
*/
void NET_Init( void ) {
	int		r;

	r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r ) {
		Com_Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
		return;
	}

    sdaTemplates.push_back( SecureDeviceAssociationTemplate::GetTemplateByName( "MasterTrafficUdp" ) );
    sdaTemplates.push_back( SecureDeviceAssociationTemplate::GetTemplateByName( "UpdateTrafficUdp" ) );
    sdaTemplates.push_back( SecureDeviceAssociationTemplate::GetTemplateByName( "ServerTrafficUdp" ) );

    // Warm up the network driver
    NetworkInformation::NetworkStatusChanged += ref new NetworkStatusChangedEventHandler( XBO_OnNetworkStatusChanged, Platform::CallbackContext::Same );
    XBO_UpdateNetworkStatus();

	winsockInitialized = qtrue;
	Com_Printf( "Winsock Initialized\n" );

	// this is really just to get the cvars registered
	NET_GetCvars();
}


/*
====================
NET_Shutdown
====================
*/
C_EXPORT void NET_Shutdown( void ) {
	if ( !winsockInitialized ) {
		return;
	}
	NET_Config( qfalse );
	WSACleanup();
	winsockInitialized = qfalse;
    sdaTemplates.clear();
}


/*
====================
NET_Sleep

sleeps msec or until net socket is ready
====================
*/
C_EXPORT void NET_Sleep( int msec ) {
}


/*
====================
NET_Restart_f
====================
*/
C_EXPORT void NET_Restart( void ) {
	NET_Config( networkingEnabled );
}
