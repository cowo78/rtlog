/** \file
 *
 */

#pragma once

#include "concurrentqueue.h"

namespace rtlog {

struct ConcurrentQueueTraits : public moodycamel::ConcurrentQueueDefaultTraits
{
    static const std::size_t BLOCK_SIZE = 32;
    static const std::size_t MAX_SUBQUEUE_SIZE = 64;
};

struct logger_traits
{
    constexpr static std::size_t PARAM_SIZE = 16;
    constexpr static std::size_t BUFFER_SIZE = 1024;

    typedef char CHAR_TYPE;
};

}  // namespace rtlog
