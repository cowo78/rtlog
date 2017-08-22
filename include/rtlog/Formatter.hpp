/** \file
 *
 */

#pragma once

#include <boost/static_assert.hpp>

#include "Argument.hpp"
#include "../concurrentqueue.h"

namespace rtlog {

//~ template<unsigned int N>
//~ struct ArgumentHolder
//~ {
    //~ constexpr static unsigned int size = N;
    //~ LogLevel log_level;
    //~ Argument arguments[N];
//~ };

/** Pull rtlog::Argument instances from the ConcurrentQueue and format the result in an internal buffer.
 *  No memory allocations.
 *  Internal buffer size and char type as template parameters
 */
template<std::size_t BUFFER_SIZE, typename CHAR_TYPE>
class CFormatterT
{
protected:
    /** Actual formatter, fixed buffer */
    fmt::BasicArrayWriter<CHAR_TYPE> m_Writer;
    /** Formatter buffer */
    CHAR_TYPE m_Buffer[BUFFER_SIZE];
    Argument m_Argument;
    bool m_MessageCompleted;

public:
    typedef CHAR_TYPE char_type;
    constexpr static size_t buffer_size = BUFFER_SIZE;

    CFormatterT() : m_Writer(m_Buffer, BUFFER_SIZE), m_MessageCompleted(false) {};

    const char_type* get() const noexcept
    { return m_Writer.c_str(); }

    /** Performs a message formatting and return internal pointer */
    template<typename QUEUE_TRAITS>
    const char_type* format(moodycamel::ConcurrentQueue<rtlog::Argument, QUEUE_TRAITS>& queue)
    {
        if (m_MessageCompleted) {
            m_Writer.clear();
            m_MessageCompleted = false;
        }
        while (queue.try_dequeue(m_Argument)) {
            // TODO: should be writer << argument
            m_Argument.operator<<(m_Writer);
            if (m_Argument.empty()) {
                // Got a final nullptr, treated as EOL
                m_MessageCompleted = true;
                return m_Writer.c_str();
            }
        }
        return NULL;
    }

protected:
};

using CFormatter = CFormatterT<1024, char>;

}  // namespace rtlog

