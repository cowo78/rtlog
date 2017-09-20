/** \file
 *
 */

#pragma once

#include <array>

#include <boost/any.hpp>

#include <fmt/format.h>

#include "../Traits.hpp"

#ifndef DBG
#define DBG() printf("%s:%d - %s\n", __FILE__, __LINE__, static_cast<const char*>(__PRETTY_FUNCTION__))
#endif

namespace rtlog {

enum LogLevel : int { INFO = 0, WARN = 1, CRIT = 2 };

template<LogLevel L> struct LogLevelType {};
using LogInfoType = LogLevelType<LogLevel::INFO>;
using LogWarnType = LogLevelType<LogLevel::WARN>;
using LogCritType = LogLevelType<LogLevel::CRIT>;

template<LogLevel L> struct LogLevelSignature;
template<> struct LogLevelSignature<LogLevel::INFO> { constexpr static const char* signature = "INFO"; };
template<> struct LogLevelSignature<LogLevel::WARN> { constexpr static const char* signature = "WARN"; };
template<> struct LogLevelSignature<LogLevel::CRIT> { constexpr static const char* signature = "CRIT"; };


/** All supported types */
enum class E_ARG_TYPE : uint8_t
{
    NULL_TYPE,
    INT64_TYPE, UINT64_TYPE,
    INT32_TYPE, UINT32_TYPE,
    INT16_TYPE, UINT16_TYPE,
    INT8_TYPE, UINT8_TYPE,
    C_STR_TYPE, CHAR_TYPE,
    LOG_INFO_TYPE, LOG_WARN_TYPE, LOG_CRIT_TYPE, LOG_LEVEL_TYPE,
    TIMEPOINT_TYPE,
    END_MARKER_TYPE,
    UNKNOWN_TYPE
};

struct _ArrayEndMarker {};

/** C++ type to E_ARG_TYPE enum type trait */
template<typename T, int N = 0> struct ArgType;

template<> struct ArgType<std::nullptr_t, 0> { constexpr static E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::NULL_TYPE; };
template<> struct ArgType<int8_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::INT8_TYPE; };
template<> struct ArgType<uint8_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::UINT8_TYPE; };
template<> struct ArgType<int16_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::INT16_TYPE; };
template<> struct ArgType<uint16_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::UINT16_TYPE; };
template<> struct ArgType<int32_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::INT32_TYPE; };
template<> struct ArgType<uint32_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::UINT32_TYPE; };
template<> struct ArgType<int64_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::INT64_TYPE; };
template<> struct ArgType<uint64_t, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::UINT64_TYPE; };
template<int N> struct ArgType<const char (&)[N]> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::C_STR_TYPE; };
template<int N> struct ArgType<const char [N]> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::C_STR_TYPE; };
template<> struct ArgType<const char*> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::C_STR_TYPE; };
template<> struct ArgType<char, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::CHAR_TYPE; };
template<> struct ArgType<LogLevel, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::LOG_LEVEL_TYPE; };
template<> struct ArgType<LogInfoType, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::LOG_INFO_TYPE; };
template<> struct ArgType<LogWarnType, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::LOG_WARN_TYPE; };
template<> struct ArgType<LogCritType, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::LOG_CRIT_TYPE; };
template<> struct ArgType<std::chrono::time_point<std::chrono::high_resolution_clock>, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::TIMEPOINT_TYPE; };
//~ template<> struct ArgType<std::chrono::time_point<std::chrono::system_clock>, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::TIMEPOINT_TYPE; };
//~ template<> struct ArgType<std::chrono::time_point<std::chrono::steady_clock>, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::TIMEPOINT_TYPE; };
template<> struct ArgType<rtlog::_ArrayEndMarker, 0> { static constexpr E_ARG_TYPE ARG_TYPE = E_ARG_TYPE::END_MARKER_TYPE; };

/** E_ARG_TYPE enum to C++ type type trait */
template<E_ARG_TYPE T> struct TypeArg;
template<> struct TypeArg<E_ARG_TYPE::NULL_TYPE> { typedef std::nullptr_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::INT64_TYPE> { typedef int64_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::UINT64_TYPE> { typedef uint64_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::INT32_TYPE> { typedef int32_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::UINT32_TYPE> { typedef uint32_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::INT16_TYPE> { typedef int16_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::UINT16_TYPE> { typedef uint16_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::INT8_TYPE> { typedef int8_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::UINT8_TYPE> { typedef uint8_t TYPE; };
template<> struct TypeArg<E_ARG_TYPE::C_STR_TYPE> { typedef const char* TYPE; };
template<> struct TypeArg<E_ARG_TYPE::CHAR_TYPE> { typedef char TYPE; };
template<> struct TypeArg<E_ARG_TYPE::LOG_LEVEL_TYPE> { typedef LogLevel TYPE; };
template<> struct TypeArg<E_ARG_TYPE::TIMEPOINT_TYPE> { typedef std::chrono::time_point<std::chrono::high_resolution_clock> TYPE; };
template<> struct TypeArg<E_ARG_TYPE::END_MARKER_TYPE> { typedef _ArrayEndMarker TYPE; };


/** Efficiently store an arbitrary argument as boost::any and save the type for later usage.
 *  Supports stream insertion based on saved type.
 */
class Argument : protected boost::any
{
private:
    E_ARG_TYPE m_Type;

public:
    Argument() : boost::any(), m_Type(E_ARG_TYPE::NULL_TYPE) {}
    template<typename ValueType>
    Argument(ValueType&& v) : boost::any(std::move(v)), m_Type(ArgType<typename std::remove_reference<ValueType>::type>::ARG_TYPE) {}

    template<typename ValueType> Argument& operator=(ValueType&& v)
    {
        m_Type = ArgType<typename std::remove_reference<ValueType>::type>::ARG_TYPE;
        boost::any(static_cast<ValueType&&>(v)).swap(*this);
        return *this;
    }
    using boost::any::operator=;

    Argument& swap(Argument& arg)
    {
        E_ARG_TYPE x(m_Type);
        m_Type = arg.m_Type;
        arg.m_Type = x;
        boost::swap(*this, arg);
        return *this;
    }

    template<typename STREAM> STREAM& operator<<(STREAM& os) const
    {
        // TODO: use FormatInt for integer types
        switch (m_Type) {
            case E_ARG_TYPE::NULL_TYPE:
                return os;
            case E_ARG_TYPE::INT64_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::INT64_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::UINT64_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::UINT64_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::INT32_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::INT32_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::UINT32_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::UINT32_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::INT16_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::INT16_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::UINT16_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::UINT16_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::INT8_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::INT8_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::UINT8_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::UINT8_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::CHAR_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::CHAR_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::C_STR_TYPE:
                os.operator<<(boost::any_cast<TypeArg<E_ARG_TYPE::C_STR_TYPE>::TYPE>(*this));
                break;
            case E_ARG_TYPE::LOG_LEVEL_TYPE:
                switch (boost::any_cast<TypeArg<E_ARG_TYPE::LOG_LEVEL_TYPE>::TYPE>(*this)) {
                    case LogLevel::INFO:
                        os.operator<<(LogLevelSignature<LogLevel::INFO>::signature);
                        break;
                    case LogLevel::WARN:
                        os.operator<<(LogLevelSignature<LogLevel::WARN>::signature);
                        break;
                    case LogLevel::CRIT:
                        os.operator<<(LogLevelSignature<LogLevel::CRIT>::signature);
                        break;
                }
                break;
            case E_ARG_TYPE::TIMEPOINT_TYPE:
                // TODO: better time formatting
                os.operator<<(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        boost::any_cast<TypeArg<E_ARG_TYPE::TIMEPOINT_TYPE>::TYPE>(*this).time_since_epoch()
                    ).count()
                );
                break;
            case E_ARG_TYPE::END_MARKER_TYPE:
                // TODO: widen
                os.operator<<("|\n");
                return os;
            default:
                break;
        }
        // TODO: widen
        os.operator <<(" ");
        return os;
    }

    // TODO: for some reason it does not compile as standard function on gcc 7.2.0
    template<typename T>
    inline static bool is_type(const Argument& arg) { return arg.m_Type == ArgType<T, 0>::ARG_TYPE; }

    using boost::any::empty;
};


/** Holds a log message split in base components, still to be formatted */
template<typename LOGGER_TRAITS>
class ArgumentArrayT : public std::array<Argument, LOGGER_TRAITS::PARAM_SIZE> {};

using ArgumentArray = ArgumentArrayT<rtlog::logger_traits>;

}  // namespace rtlog
