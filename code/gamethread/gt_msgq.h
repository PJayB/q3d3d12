#pragma once

#include <stdint.h>
#include <concurrent_queue.h>

// Convenience macro for setting up a message
#define INIT_MSG( msg )        { ZeroMemory( &(msg), sizeof( msg ) ); (msg).TimeStamp = Sys_Milliseconds(); }

namespace GameThread
{

struct MSG
{
    size_t Message;
    size_t Param0;
    size_t Param1;
    size_t TimeStamp;
};

class MessageQueue
{
public:
    MessageQueue();
    void Post( const MSG* msg );
    bool Pop( MSG* msg );

private:
    
    concurrency::concurrent_queue<MSG> m_msgs;
};

}
