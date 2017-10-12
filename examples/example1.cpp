// follow with
// strace -o log -ff -tt
#include "../include/stdafx.h"
#include "../include/rtlog/Consumer.hpp"
#include "../include/rtlog/rtlog.hpp"


int main(int argc, char* argv[])
{
    auto& logger = rtlog::CLogger::initialize(rtlog::LogLevel::INFO);
    rtlog::CLogConsumerSingleFile consumer("test.log", logger.getQueue(), 600);

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
