/** \file
 *
 */

#pragma once

#include "concurrentqueue.h"
#include "rtlog/Levels.hpp"

namespace rtlog {

/** Configuration parameters for the concurrent queue */
struct ConcurrentQueueTraits : public moodycamel::ConcurrentQueueDefaultTraits
{
    static const std::size_t BLOCK_SIZE = 32;
    static const std::size_t MAX_SUBQUEUE_SIZE = 64;
};

/** Configuration parameters for the logger itself */
struct LoggerTraits
{
    /** Number of available parameters in RTLOG_ macros */
    constexpr static std::size_t PARAM_SIZE = 16;
    /** Formatting buffer size */
    constexpr static std::size_t BUFFER_SIZE = 1024;
    /** Default logger level */
    constexpr static LogLevel DEFAULT_LEVEL = LogLevel::INFO;

    /** Messages character type */
    typedef char CHAR_TYPE;
};

}  // namespace rtlog
