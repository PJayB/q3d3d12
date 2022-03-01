#pragma once

enum NET_MSG
{
    NET_MSG_INCOMING_MESSAGE
};

extern GameThread::MessageQueue g_NetMsgQueue;

/*
    Async Events
*/

void NET_OnMessageReceived( 
    Windows::Networking::Sockets::DatagramSocket^ socket, 
    Windows::Networking::Sockets::DatagramSocketMessageReceivedEventArgs^ args );

/*
    Thread-safe Quake functions
*/
extern "C" int Sys_Milliseconds( void );
