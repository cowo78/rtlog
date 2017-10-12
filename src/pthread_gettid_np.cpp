/** \file
 *
 */

#include "../include/pthread_gettid_np.hpp"

#if defined(BOOST_LIB_C_GNU) && defined(BOOST_OS_LINUX) && defined(USE_INTERNAL_GETTID)

#if !defined(_GNU_SOURCE)
#   define _GNU_SOURCE
#endif
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <future>
#include <thread>

#include <boost/preprocessor/stringize.hpp>

namespace rtlog {
namespace details {

// Initialize the global private var
size_t __thread_id_offset_linux_glibc = rtlog::details::__find_thread_id_offset();

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

}   // namespace detail
}   // namespace rtlog

#endif  // BOOST_LIB_C_GNU && BOOST_OS_LINUX
