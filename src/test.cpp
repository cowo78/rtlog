#include "../include/stdafx.h"
#include "../include/rtlog/Formatter.hpp"
#include "../include/rtlog/Consumer.hpp"
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


int main(int argc, char* argv[])
{
    moodycamel::ConcurrentQueue<rtlog::ArgumentArray, rtlog::ConcurrentQueueTraits> main_queue;
    rtlog::CFormatter formatter;

    rtlog::CLogger::initialize(main_queue);
    rtlog::CLogConsumer consumer("test.log", main_queue, 600);

    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<unsigned int> uniform_dist(10, 200);


    for (int i{0}; i < 100; i++) {
        for (int j{0}; j < 10; j++) {
            std::chrono::microseconds sleep_time(uniform_dist(e1));
            LOG_INFO("User message", (i*10)+j, sleep_time.count());
            std::this_thread::sleep_for(sleep_time);
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
