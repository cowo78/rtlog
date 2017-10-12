/** \file
 *
 */

#pragma once

#if defined(USE_INTERNAL_GETTID)

#include <boost/predef.h>

#if defined(BOOST_LIB_C_GNU) && defined(BOOST_OS_LINUX)

#if !defined(_GNU_SOURCE)
#   define _GNU_SOURCE
#endif
#include <pthread.h>
#include <sys/types.h>

namespace rtlog {
namespace details {

extern size_t __thread_id_offset_linux_glibc;

size_t __find_thread_id_offset_thread();

// Spawn a thread, search the pthread_self structure for the LWP ID and return it
size_t __find_thread_id_offset();

/** Horrible hack.
 *  The pthread_t structure is an opaque struct defined in glibc/nptl/descr.h
 *  Depending on various compilation options it may change in size and of course
 *  it's totally dependent on architecture.
 *  It's possible to use gettid() system call to get the LWP (Thread) ID but of course
 *  at the expense of a syscall.
 *  The basic idea is just to spawn a new thread, get the pthread_t opaque structure and search
 *  for the LWP ID obtained by the gettid() syscall.
 *  Of course YMMV and it's definitely possible to get a SEGV during startup
 *  Inlined since it's called every time a log message is enqueued
 */
inline pid_t pthread_gettid_np(void)
{
    return *(pid_t*)(((char*)pthread_self()) + __thread_id_offset_linux_glibc);
}

}  // namespace detail
}  // namespace rtlog

#endif  // BOOST_LIB_C_GNU && BOOST_OS_LINUX
#endif  // USE_INTERNAL_GETTID
