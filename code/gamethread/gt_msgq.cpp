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

        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push( *msg );
    }

    bool MessageQueue::Pop( MSG* msg )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_queue.empty())
        {
            *msg = m_queue.front();
            m_queue.pop();
            return true;
        }
        return false;
    }
}
