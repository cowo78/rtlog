/** \file
 *
 */

#include "../include/stdafx.h"
#include "../include/rtlog/rtlog.hpp"

namespace rtlog
{

// Make sure the implementation is instantiated
template
fmt::BasicWriter<typename LoggerTraits::CHAR_TYPE>& operator<<(fmt::BasicWriter<typename LoggerTraits::CHAR_TYPE>& os, Argument const& arg);


}  // namespace rtlog
