// follow with
// strace -o log -ff -tt
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

template
fmt::BasicWriter<char>& operator<<(fmt::BasicWriter<char>& os, Argument const& arg);

#if defined(BOOST_LIB_C_GNU) && defined(BOOST_OS_LINUX) && defined(USE_INTERNAL_GETTID)
size_t __thread_id_offset_linux_glibc = rtlog::__find_thread_id_offset();
#endif

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

    unsigned int max_threads(std::thread::hardware_concurrency());
    std::vector<std::future<void>> thread_futures;
    for (unsigned int thread_index(0); thread_index < max_threads; thread_index++) {
        thread_futures.push_back(
            std::move(
                std::async(
                    [&uniform_dist, &e1, thread_index] ()
                    {
                        for (int i{0}; i < 100; i++) {
                            for (int j{0}; j < 10; j++) {
                                std::chrono::microseconds sleep_time(uniform_dist(e1));
                                LOG_INFO("Thread idx", thread_index, (i*10)+j, sleep_time.count());
                                std::this_thread::sleep_for(sleep_time);
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                    } // lambda
                ) // async
            ) // move
        );  // push_back
    }
    LOG_INFO("Base thread wait");
    for (auto& f : thread_futures)
        f.wait();

    LOG_INFO("Children joined");
    consumer.stop();

    return 0;
}
