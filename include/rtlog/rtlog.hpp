/** \file
 *
 */

#pragma once

#include "Argument.hpp"
#include "Formatter.hpp"
#include "Consumer.hpp"
#include "../Singleton.hpp"
#include "../concurrentqueue.h"
#include "../Traits.hpp"

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

template<typename LOGGER_TRAITS, typename QUEUE_TRAITS>
class CLoggerT : public Singleton<CLoggerT<LOGGER_TRAITS, QUEUE_TRAITS>>
{
    static_assert(LOGGER_TRAITS::PARAM_SIZE > 7);
    friend class Singleton<CLoggerT<LOGGER_TRAITS, QUEUE_TRAITS>>;
    friend class std::default_delete<CLoggerT<LOGGER_TRAITS, QUEUE_TRAITS>>;

    // Cannot access thread id private member and need an ostream to format thread::id
    // so just revert to pthread_t
    static_assert(std::is_same<std::thread::native_handle_type, pthread_t>::value);
    static_assert(std::is_integral<pthread_t>::value);
    static_assert(std::is_integral<pid_t>::value);

public:
    constexpr static std::size_t param_size = LOGGER_TRAITS::PARAM_SIZE;
    constexpr static std::size_t buffer_size = LOGGER_TRAITS::BUFFER_SIZE;
    typedef typename LOGGER_TRAITS::CHAR_TYPE char_type;
    typedef QUEUE_TRAITS queue_traits;

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
        static_assert(sizeof...(Args) <= (LOGGER_TRAITS::PARAM_SIZE - 6 - 1));
        // Make sure all the POD are 0-initialized
        ArgumentArrayT<LOGGER_TRAITS> p = {};
        std::size_t enqueuedArguments = {};
        _write(p, enqueuedArguments, time_point);
        _write(p, enqueuedArguments, thread_id);
        _write(p, enqueuedArguments, level);
        _write(p, enqueuedArguments, file);
        _write(p, enqueuedArguments, line);
        _write(p, enqueuedArguments, function);
        _write(p, enqueuedArguments, arg0, args...);
        _write(p, enqueuedArguments, _ArrayEndMarker());

        return (m_ArgumentQueue.try_enqueue(std::move(p)));
    }
    template<typename T0, typename... Args>
    inline bool _write(ArgumentArrayT<LOGGER_TRAITS>& p, std::size_t& queue_pos, T0&& v0, Args&&... args)
    {
        p[queue_pos++] = v0;
        return _write(p, queue_pos, args...);
    }
    template<typename T0>
    inline bool _write(ArgumentArrayT<LOGGER_TRAITS>& p, std::size_t& queue_pos, T0&& v0)
    {
        p[queue_pos++] = v0;
        return true;
    }

protected:
    CLoggerT(moodycamel::ConcurrentQueue<ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS>& queue) :
        m_ArgumentQueue(queue)
    {}
    ~CLoggerT() {}

    /** Each queue element is made of an array of log message pieces */
    moodycamel::ConcurrentQueue<ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS>& m_ArgumentQueue;
};

/** Default logger */
using CLogger = CLoggerT<rtlog::logger_traits, rtlog::ConcurrentQueueTraits>;

inline bool is_logged(LogLevel level);
}   // namespace rtlog

// TODO: enqueue PID instead of thread ID
#define RTLOG(LVL, ...)                                 \
    rtlog::CLogger::get().write(                        \
        std::move(                                      \
            std::chrono::high_resolution_clock::now()), \
            std::move(pthread_self()),                  \
            std::move(LVL),                             \
            std::move(__FILE__),                        \
            std::move(__LINE__),                        \
            std::move(__func__),                        \
            ##__VA_ARGS__                               \
    )

#define LOG_INFO(...) \
    do { rtlog::is_logged(rtlog::LogLevel::INFO) && RTLOG(rtlog::LogLevel::INFO, ##__VA_ARGS__); } while (0);
#define LOG_WARN(...) \
    do { rtlog::is_logged(rtlog::LogLevel::WARN) && RTLOG(rtlog::LogLevel::WARN, ##__VA_ARGS__); } while (0);
#define LOG_CRIT(...) \
    do { rtlog::is_logged(rtlog::LogLevel::CRIT) && RTLOG(rtlog::LogLevel::CRIT, ##__VA_ARGS__); } while (0);
