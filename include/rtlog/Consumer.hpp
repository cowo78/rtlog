/** \file
 *
 */

#pragma once

#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <thread>

#include "Formatter.hpp"
#include "../Traits.hpp"

namespace rtlog {

/** Base consumer class */
template<typename LOGGER_TRAITS, typename QUEUE_TRAITS>
class CLogConsumerBaseT
{
protected:
    moodycamel::ConcurrentQueue<rtlog::ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS>& m_Queue;
    std::atomic_bool m_Stop;

public:
    typedef moodycamel::ConcurrentQueue<rtlog::ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS> queue_type;

    CLogConsumerBaseT(queue_type& queue) :
        m_Queue(queue)
    {
        m_Stop.store(false);
    }

    virtual void consume() = 0;

    void stop()
    {
        m_Stop.store(true);
    }
};

/** Single file output consumer running in a new thread */
template<typename LOGGER_TRAITS, typename QUEUE_TRAITS>
class CLogConsumerSingleFileT : public CLogConsumerBaseT<LOGGER_TRAITS, QUEUE_TRAITS>
{
protected:
    std::chrono::microseconds m_PollInterval;
    rtlog::CFormatter m_Formatter;
    rtlog::ArgumentArrayT<LOGGER_TRAITS> m_ArgumentArray;
    std::thread m_ConsumerThread;
    std::string m_FileName;
    std::ofstream m_Stream;

public:
    typedef CLogConsumerBaseT<LOGGER_TRAITS, QUEUE_TRAITS> base_type;
    using queue_type = typename base_type::queue_type;

    CLogConsumerSingleFileT(const std::string& filename, queue_type& queue, uint32_t poll_interval_us) :
        base_type(queue),
        m_PollInterval(poll_interval_us), m_FileName(filename),
        m_Stream(filename, std::ofstream::binary|std::ofstream::trunc|std::ofstream::out)
    {
        // Create and start thread
        m_ConsumerThread = std::thread(std::bind(&CLogConsumerSingleFileT<LOGGER_TRAITS, QUEUE_TRAITS>::consume, this));
    }

    virtual void consume()
    {
        const typename LOGGER_TRAITS::CHAR_TYPE* p;
        while (!this->m_Stop.load(std::memory_order_acquire)) {
            while (this->m_Queue.try_dequeue(m_ArgumentArray)) {
                // Dequeue a log message block
                // It SHOULD be complete but it's not guaranteed
                p = m_Formatter.format(m_ArgumentArray);
                if (p)
                    m_Stream << p;
            }
            m_Stream.flush();
            // TODO: sleep only for remaining poll interval
            std::this_thread::sleep_for(m_PollInterval);
        }

        m_Stream.flush();
    }

    void stop()
    {
        this->m_Stop.store(true);
        m_ConsumerThread.join();
        m_Stream.close();
    }
};
using CLogConsumerSingleFile = CLogConsumerSingleFileT<rtlog::LoggerTraits, rtlog::ConcurrentQueueTraits>;

}  // namespace rtlog
