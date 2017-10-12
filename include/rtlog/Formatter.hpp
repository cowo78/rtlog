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
    rtlog::Argument m_Argument;

public:
    typedef typename LOGGER_TRAITS::CHAR_TYPE char_type;
    constexpr static size_t buffer_size = LOGGER_TRAITS::BUFFER_SIZE;

    CFormatterT() : m_Writer(m_Buffer, LOGGER_TRAITS::BUFFER_SIZE) {};

    const char_type* get() const noexcept
    { return m_Writer.c_str(); }

    /** Performs a single message formatting and return internal pointer */
    const char_type* format(rtlog::ArgumentArrayT<LOGGER_TRAITS>& argument_array)
    {
        m_Writer.clear();
        for (auto& elem : argument_array) {
            if (elem.empty())
                break;  // Encountered an empty element before end marker, message incomplete

            m_Writer << elem;

            if (Argument::is_type<rtlog::_ArrayEndMarker>(elem))
                return m_Writer.c_str();  // End of message found
        }
        return NULL;  // No message enqueued
    }
};

using CFormatter = CFormatterT<rtlog::LoggerTraits, rtlog::ConcurrentQueueTraits>;

}  // namespace rtlog
