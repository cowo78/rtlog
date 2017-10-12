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

template<typename LOGGER_TRAITS, typename QUEUE_TRAITS>
class CLogConsumerT
{
protected:
    moodycamel::ConcurrentQueue<rtlog::ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS>& m_Queue;
    std::chrono::microseconds m_PollInterval;
    rtlog::CFormatter m_Formatter;
    std::thread m_ConsumerThread;
    std::atomic_bool m_Stop;
    std::string m_FileName;
    std::ofstream m_Stream;

public:
    typedef moodycamel::ConcurrentQueue<rtlog::ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS> queue_type;

    CLogConsumerT(const std::string& filename, queue_type& queue, uint32_t poll_interval_us) :
        m_Queue(queue), m_PollInterval(poll_interval_us), m_FileName(filename),
        m_Stream(filename, std::ofstream::binary|std::ofstream::trunc|std::ofstream::out)
    {
        m_Stop.store(false);
        m_ConsumerThread = std::thread(std::bind(&CLogConsumerT<LOGGER_TRAITS, QUEUE_TRAITS>::consume, this));
    }

    void consume()
    {
        const char* p;
        while (!m_Stop.load(std::memory_order_acquire)) {
            while ((p = m_Formatter.format(m_Queue)) != NULL)
                m_Stream << p;

            std::this_thread::sleep_for(m_PollInterval);
        }

        while ((p = m_Formatter.format(m_Queue)) != NULL)
            m_Stream << p;
        m_Stream.flush();
    }

    void stop()
    {
        m_Stop.store(true);
        m_ConsumerThread.join();
        m_Stream.close();
    }
};
using CLogConsumer = CLogConsumerT<rtlog::LoggerTraits, rtlog::ConcurrentQueueTraits>;

}  // namespace rtlog
