/** \file
 *
 */

#pragma once

#include "Argument.hpp"
#include "../concurrentqueue.h"
#include "../Traits.hpp"

namespace rtlog {

/** Pull rtlog::Argument instances from the ConcurrentQueue and format the result in an internal buffer.
 *  No memory allocations.
 *  Internal buffer size and char type as template parameters
 */
template<typename LOGGER_TRAITS, typename QUEUE_TRAITS>
class CFormatterT
{
protected:
    /** Actual formatter, fixed buffer */
    fmt::BasicArrayWriter<typename LOGGER_TRAITS::CHAR_TYPE> m_Writer;
    /** Formatter buffer */
    typename LOGGER_TRAITS::CHAR_TYPE m_Buffer[LOGGER_TRAITS::BUFFER_SIZE];
    rtlog::ArgumentArrayT<LOGGER_TRAITS> m_ArgumentArray;
    rtlog::Argument m_Argument;

public:
    typedef typename LOGGER_TRAITS::CHAR_TYPE char_type;
    constexpr static size_t buffer_size = LOGGER_TRAITS::BUFFER_SIZE;

    CFormatterT() : m_Writer(m_Buffer, LOGGER_TRAITS::BUFFER_SIZE) {};

    const char_type* get() const noexcept
    { return m_Writer.c_str(); }

    /** Performs a single message formatting and return internal pointer */
    const char_type* format(moodycamel::ConcurrentQueue<rtlog::ArgumentArrayT<LOGGER_TRAITS>, QUEUE_TRAITS>& queue)
    {
        if (queue.try_dequeue(m_ArgumentArray)) {
            // Dequeue a log message block
            // It SHOULD be complete but it's not guaranteed
            m_Writer.clear();
            // TODO: should be writer << argument
            for (auto& elem : m_ArgumentArray) {
                if (elem.empty())
                    return NULL;  // Encountered an empty element before end marker, message incomplete

                elem.operator <<(m_Writer);

                if (Argument::is_type<rtlog::_ArrayEndMarker>(elem))
                    return m_Writer.c_str();  // End of message found
            }
        }
        return NULL;  // No message enqueued
    }
};

using CFormatter = CFormatterT<rtlog::logger_traits, rtlog::ConcurrentQueueTraits>;

}  // namespace rtlog
