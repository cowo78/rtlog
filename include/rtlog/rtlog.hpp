/** \file
 *
 */

#pragma once

#include <boost/predef.h>
#include <boost/preprocessor/stringize.hpp>

#include <sys/types.h>
#include <sys/syscall.h>

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
        const char* &&position,
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
        _write(p, enqueuedArguments, position);
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


#if defined(BOOST_LIB_C_GNU) && defined(BOOST_OS_LINUX) && defined(USE_INTERNAL_GETTID)
size_t __find_thread_id_offset_thread()
{
    pid_t tid((pid_t)syscall(SYS_gettid));
    uint8_t* threadmem = (uint8_t*)pthread_self();
    size_t offset(0);
    do {
        if (*(pid_t*)(threadmem + offset) == tid)
            break;
    } while (++offset);  // Yes, SEGV ahead

    return offset;
}

// Spawn a thread, search the pthread_self structure for the LWP ID and return it
size_t __find_thread_id_offset()
{
    return std::async(__find_thread_id_offset_thread).get();
}

extern size_t __thread_id_offset_linux_glibc;

/** Horrible hack.
 *  The pthread_t structure is an opaque struct defined in glibc/nptl/descr.h
 *  Depending on various compilation options it may change in size and of course
 *  it's totally dependent on architecture.
 *  It's possible to use gettid() system call to get the LWP (Thread) ID but of course
 *  at the expense of a syscall.
 *  The basic idea is just to spawn a new thread, get the pthread_t opaque structure and search
 *  for the LWP ID obtained by the gettid() syscall.
 *  Of course YMMV and it's definitely possible to get a SEGV during startup
 */
inline pid_t pthread_gettid_np(void)
{
    return *(pid_t*)(((char*)pthread_self()) + __thread_id_offset_linux_glibc);
}

#endif  // BOOST_LIB_C_GNU && BOOST_OS_LINUX
}   // namespace rtlog

#define RTLOG_NOW() std::chrono::high_resolution_clock::now()
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
#   define RTLOG_THREAD_ID() rtlog::pthread_gettid_np()
#endif


#define RTLOG(LVL, ...)                                 \
    rtlog::CLogger::get().write(                        \
        std::move(RTLOG_NOW()),                         \
        std::move(RTLOG_THREAD_ID()),                   \
        std::move(LVL),                                 \
        std::move(RTLOG_POSITION()),                    \
        ##__VA_ARGS__                                   \
    )


#define LOG_INFO(...) \
    do { rtlog::is_logged(rtlog::LogLevel::INFO) && RTLOG(rtlog::LogLevel::INFO, ##__VA_ARGS__); } while (0);
#define LOG_WARN(...) \
    do { rtlog::is_logged(rtlog::LogLevel::WARN) && RTLOG(rtlog::LogLevel::WARN, ##__VA_ARGS__); } while (0);
#define LOG_CRIT(...) \
    do { rtlog::is_logged(rtlog::LogLevel::CRIT) && RTLOG(rtlog::LogLevel::CRIT, ##__VA_ARGS__); } while (0);
