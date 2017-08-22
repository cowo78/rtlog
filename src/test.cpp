#include "../include/stdafx.h"
#include "../include/rtlog/Formatter.hpp"
#include "../include/rtlog/rtlog.hpp"

#ifndef DBG
#define DBG() printf("%s:%d - %s\n", __FILE__, __LINE__, static_cast<const char*>(__PRETTY_FUNCTION__))
#endif

namespace rtlog
{

std::atomic<unsigned int> loglevel = {0};
bool is_logged(LogLevel level)
{
    return static_cast<unsigned int>(level) >= loglevel.load(std::memory_order_relaxed);
}

}  // namespace rtlog


template<typename QUEUE_TRAITS>
class CLogConsumerT
{
protected:
    moodycamel::ConcurrentQueue<rtlog::Argument, QUEUE_TRAITS>& m_Queue;
    std::chrono::microseconds m_PollInterval;
    rtlog::CFormatter m_Formatter;
    std::thread m_ConsumerThread;
    std::atomic_bool m_Stop;

public:
    CLogConsumerT(moodycamel::ConcurrentQueue<rtlog::Argument, QUEUE_TRAITS>& queue, uint32_t poll_interval) :
        m_Queue(queue), m_PollInterval(poll_interval)
    {
        m_Stop.store(false);
        m_ConsumerThread = std::thread(std::bind(&CLogConsumerT<QUEUE_TRAITS>::consume, this));
    }

    void consume()
    {
        const char* p;
        while (!m_Stop.load(std::memory_order_acquire)) {
            p = m_Formatter.format(m_Queue);
            if (p)
                std::cout << p;
            else
                std::this_thread::sleep_for(m_PollInterval);
        }

        while ((p = m_Formatter.format(m_Queue)) != NULL)
            std::cout << p;
    }

    void stop()
    {
        m_Stop.store(true);
        m_ConsumerThread.join();
    }
};
using CLogConsumer = CLogConsumerT<rtlog::ConcurrentQueueTraits>;

int main(int argc, char* argv[])
{
    moodycamel::ConcurrentQueue<rtlog::Argument, rtlog::ConcurrentQueueTraits> main_queue;
    rtlog::CFormatter formatter;

    rtlog::CLogger::initialize(main_queue);
    CLogConsumer consumer(main_queue, 600);
    //~ std::thread consumer_thread(consumer, std::ref(formatter), std::ref(main_queue));

    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<unsigned int> uniform_dist(10, 200);

    for (int i{0}; i < 100; i++) {
        for (int j{0}; j < 10; j++) {
            LOG_INFO("User message", (i*10)+j);
            std::this_thread::sleep_for(
                std::chrono::microseconds(uniform_dist(e1))
            );
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    consumer.stop();


    //~ consumer_thread.join();
    //~ std::cout << rtlog::CLogger::get().format();

    //~ CFormatter l;
    //~ l.write("asd {}", "CCC");
    //~ l.format();
    //~ l.write("asd {}", -13, 13u);
    //~ l.format();
    //~ l.write("asd {}");
    //~ l.format();

    return 0;
}
