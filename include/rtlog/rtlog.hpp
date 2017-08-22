/** \file
 *
 */

#pragma once

#include "Argument.hpp"
#include "Formatter.hpp"
#include "../Singleton.hpp"
#include "../concurrentqueue.h"

namespace rtlog
{

/*
 * Ensure initialize() is called prior to any log statements.
 * log_directory - where to create the logs. For example - "/tmp/"
 * log_file_name - root of the file name. For example - "nanolog"
 * This will create log files of the form -
 * /tmp/nanolog.1.txt
 * /tmp/nanolog.2.txt
 * etc.
 * log_file_roll_size_mb - mega bytes after which we roll to next log file.
 */
void initialize(std::string const & log_directory, std::string const & log_file_name, uint32_t log_file_roll_size_mb);

struct ConcurrentQueueTraits : public moodycamel::ConcurrentQueueDefaultTraits
{
    static const size_t BLOCK_SIZE = 32;
    static const size_t MAX_SUBQUEUE_SIZE = 64;
};

template<std::size_t PARAM_SIZE, std::size_t BUFFER_SIZE, typename CHAR_TYPE, typename QUEUE_TRAITS>
class CLoggerT : public Singleton<CLoggerT<PARAM_SIZE, BUFFER_SIZE, CHAR_TYPE, QUEUE_TRAITS>>
{
    static_assert(PARAM_SIZE > 7);
    friend class Singleton<CLoggerT<PARAM_SIZE, BUFFER_SIZE, CHAR_TYPE, QUEUE_TRAITS>>;
    friend class std::default_delete<CLoggerT<PARAM_SIZE, BUFFER_SIZE, CHAR_TYPE, QUEUE_TRAITS>>;

    // Cannot access thread id private member and need an ostream to format thread::id
    // so just revert to pthread_t
    static_assert(std::is_same<std::thread::native_handle_type, pthread_t>::value);
    static_assert(std::is_integral<pthread_t>::value);
public:

    /** Save some arguments for later formatting */
    template<typename C, typename T0, typename... Args>
    inline bool write(
        std::chrono::time_point<C>&& time_point,
        pthread_t&& thread_id,
        rtlog::LogLevel&& level,
        const char *file, int line, const char *function,
        T0&& arg0, Args&&... args
    )
    {
        // TODO: enqueue only full messages
        static_assert(sizeof...(Args) <= (PARAM_SIZE - 6 - 1));
        arg_holders.enqueue(std::move(Argument(time_point)));
        arg_holders.enqueue(std::move(Argument(thread_id)));
        arg_holders.enqueue(std::move(Argument(level)));
        arg_holders.enqueue(std::move(Argument(file)));
        arg_holders.enqueue(std::move(Argument(line)));
        arg_holders.enqueue(std::move(Argument(function)));
        return _write(arg0, args...);
    }
    template<typename T0, typename... Args>
    inline bool _write(T0&& v0, Args&&... args)
    {
        // TODO: enqueue only full messages
        return (arg_holders.enqueue(std::move(Argument(v0))) && _write(args...));
    }
    template<typename T0>
    inline bool _write(T0&& v0)
    {
        // TODO: enqueue only full messages
        return (
            arg_holders.enqueue(std::move(Argument(v0))) &&
            // Add the final endline marked as NULL
            arg_holders.enqueue(std::move(Argument()))
        );
    }

    moodycamel::ConcurrentQueue<rtlog::Argument, QUEUE_TRAITS>& arg_holders;

protected:
    CLoggerT(moodycamel::ConcurrentQueue<rtlog::Argument, QUEUE_TRAITS>& queue) : arg_holders(queue) {}
    ~CLoggerT() {}

private:
};

/** Default logger */
using CLogger = CLoggerT<8, 1024, char, ConcurrentQueueTraits>;

inline bool is_logged(LogLevel level);
}   // namespace rtlog

// TODO: enqueue thread name instead of thread ID
#define RTLOG(LVL, ...) rtlog::CLogger::get().write( \
    std::move(std::chrono::high_resolution_clock::now()), \
    std::move(pthread_self()), \
    std::move(LVL), std::move(__FILE__), std::move(__LINE__), std::move(__func__), ##__VA_ARGS__)

#define LOG_INFO(...) rtlog::is_logged(rtlog::LogLevel::INFO) && RTLOG(rtlog::LogLevel::INFO, ##__VA_ARGS__)
#define LOG_WARN(...) rtlog::is_logged(rtlog::LogLevel::WARN) && RTLOG(rtlog::LogLevel::WARN, ##__VA_ARGS__)
#define LOG_CRIT(...) rtlog::is_logged(rtlog::LogLevel::CRIT) && RTLOG(rtlog::LogLevel::CRIT, ##__VA_ARGS__)
