#include "gt_msgq.h"

#include <assert.h>

namespace GameThread
{
    MessageQueue::MessageQueue()
    {
    }

    void MessageQueue::Post( const MSG* msg )
    {
        assert( msg->TimeStamp != 0 );
        m_msgs.push( *msg );
    }

    bool MessageQueue::Pop( MSG* msg )
    {
        return m_msgs.try_pop( *msg );
    }
}
