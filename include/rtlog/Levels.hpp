/** \file
 *  Supported log levels and related stuff
 */

#pragma once

namespace rtlog {

enum LogLevel : unsigned int
{
    INFO = 0,
    WARN = 1,
    CRIT = 2
};

// Specific types for each log level
template<LogLevel L> struct LogLevelType {};
using LogInfoType = LogLevelType<LogLevel::INFO>;
using LogWarnType = LogLevelType<LogLevel::WARN>;
using LogCritType = LogLevelType<LogLevel::CRIT>;

// Specific string signature for each log level type
template<LogLevel L> struct LogLevelSignature;
template<> struct LogLevelSignature<LogLevel::INFO> { constexpr static const char* signature = "INFO"; };
template<> struct LogLevelSignature<LogLevel::WARN> { constexpr static const char* signature = "WARN"; };
template<> struct LogLevelSignature<LogLevel::CRIT> { constexpr static const char* signature = "CRIT"; };

}  // namespace rtlog
