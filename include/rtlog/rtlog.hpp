/** \file
 *
 */

#pragma once

#include "Argument.hpp"
#include "Consumer.hpp"
#include "Formatter.hpp"
#include "Levels.hpp"
#include "../concurrentqueue.h"
#include "../pthread_gettid_np.hpp"
#include "../Singleton.hpp"
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
    // The minimum number of arguments in case of relative time logged
    static_assert(LOGGER_TRAITS::PARAM_SIZE > 6);
    friend class Singleton<CLoggerT<LOGGER_TRAITS, QUEUE_TRAITS>>;
    friend class std::default_delete<CLoggerT<LOGGER_TRAITS, QUEUE_TRAITS>>;

    // We may be using pthread_t as Thread Identifier, so make sure it's a known value
    static_assert(std::is_same<std::thread::native_handle_type, pthread_t>::value);
    static_assert(std::is_integral<pthread_t>::value);
    static_assert(std::is_integral<pid_t>::value);

public:
    constexpr static std::size_t param_size = LOGGER_TRAITS::PARAM_SIZE;
    constexpr static std::size_t buffer_size = LOGGER_TRAITS::BUFFER_SIZE;
    constexpr static LogLevel DEFAULT_LEVEL = LOGGER_TRAITS::DEFAULT_LEVEL;

    typedef typename LOGGER_TRAITS::CHAR_TYPE char_type;
    typedef QUEUE_TRAITS queue_traits;
    typedef moodycamel::ConcurrentQueue<ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS> queue_type;

    /** Set the current logging level */
    void setLevel(LogLevel new_level) { m_LogLevel.store(new_level); }
    /** Access the underlying message queue */
    queue_type& getQueue() { return m_ArgumentQueue; }

    /** Enqueue some arguments for later formatting */
    template<typename TID, typename T0, typename... Args>
    inline bool write(TID&& thread_id, LogLevel&& level, const char_type* &&position, T0&& arg0, Args&&... args)
    {
        if (static_cast<std::underlying_type<LogLevel>::type>(level) < m_LogLevel.load(std::memory_order_relaxed))
            return true;

        // Make sure we have enough room
        static_assert(sizeof...(Args) < (LOGGER_TRAITS::PARAM_SIZE - 3 - 1));
        // Make sure all the POD are 0-initialized
        ArgumentArrayT<LOGGER_TRAITS> p = {};
        std::size_t enqueuedArguments = {};
        _write(p, enqueuedArguments, thread_id);
        _write(p, enqueuedArguments, level);
        _write(p, enqueuedArguments, position);
        _write(p, enqueuedArguments, arg0, args...);
        _write(p, enqueuedArguments, _ArrayEndMarker());

        return (m_ArgumentQueue.try_enqueue(std::move(p)));
    }

    /** Save some arguments for later formatting */
    template<typename C, typename TID, typename T0, typename... Args>
    inline bool write(
        std::chrono::time_point<C>&& time_point,
        TID&& thread_id,
        LogLevel&& level,
        const char* &&position,
        T0&& arg0, Args&&... args
    )
    {
        static_assert(sizeof...(Args) <= (LOGGER_TRAITS::PARAM_SIZE - 4 - 1));
        // Make sure all the POD are 0-initialized
        ArgumentArrayT<LOGGER_TRAITS> p = {};
        std::size_t enqueuedArguments = {};
        _write(p, enqueuedArguments, time_point);
        _write(p, enqueuedArguments, thread_id);
        _write(p, enqueuedArguments, level);
        _write(p, enqueuedArguments, position);
        _write(p, enqueuedArguments, arg0, args...);
        _write(p, enqueuedArguments, _ArrayEndMarker());

        return (m_ArgumentQueue.try_enqueue(std::move(p)));
    }

protected:
    CLoggerT(
        LogLevel level = DEFAULT_LEVEL
    ) : m_LogLevel(level) {}
    ~CLoggerT() {}

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

    /** Current log level */
    std::atomic<std::underlying_type<LogLevel>::type> m_LogLevel;

    /** Each queue element is made of an array of log message pieces */
    moodycamel::ConcurrentQueue<ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS> m_ArgumentQueue;
};

/** Default logger */
using CLogger = CLoggerT<rtlog::LoggerTraits, rtlog::ConcurrentQueueTraits>;

}   // namespace rtlog

#define RTLOG_POSITION() BOOST_PP_STRINGIZE([) __FILE__ BOOST_PP_STRINGIZE(:) BOOST_PP_STRINGIZE(__LINE__) BOOST_PP_STRINGIZE(])

/** pthread_self() is a simple function call implemented in assembler
 *  gettid is a full system call like getpid
 *  pthread_self returns the address of the pthread_t structure, not the real thread ID
 */
#if defined(USE_PTHREAD_SELF)
#   define RTLOG_THREAD_ID() pthread_self()
#elif defined(USE_SYS_GETTID)
#   define RTLOG_THREAD_ID() syscall(SYS_gettid)
#elif defined(USE_GETPID)
#   define RTLOG_THREAD_ID() getpid()
#elif defined(USE_INTERNAL_GETTID)
#   define RTLOG_THREAD_ID() rtlog::details::pthread_gettid_np()
#endif

#if defined(USE_TIMEPOINT)

#define RTLOG_NOW() std::chrono::high_resolution_clock::now()

#define RTLOG(LVL, ...)                                 \
    rtlog::CLogger::get().write(                        \
        std::move(RTLOG_NOW()),                         \
        std::move(RTLOG_THREAD_ID()),                   \
        std::move(LVL),                                 \
        std::move(RTLOG_POSITION()),                    \
        ##__VA_ARGS__                                   \
    )
#else
#define RTLOG(LVL, ...)                                 \
    rtlog::CLogger::get().write(                        \
        std::move(RTLOG_THREAD_ID()),                   \
        std::move(LVL),                                 \
        std::move(RTLOG_POSITION()),                    \
        ##__VA_ARGS__                                   \
    )
#endif  // USE_TIMEPOINT


#define LOG_INFO(...) \
    do { RTLOG(rtlog::LogLevel::INFO, ##__VA_ARGS__); } while (0);
#define LOG_WARN(...) \
    do { RTLOG(rtlog::LogLevel::WARN, ##__VA_ARGS__); } while (0);
#define LOG_CRIT(...) \
    do { RTLOG(rtlog::LogLevel::CRIT, ##__VA_ARGS__); } while (0);
