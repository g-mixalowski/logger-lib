#ifndef LOGGERLIB_EXPORT_HPP_
#define LOGGERLIB_EXPORT_HPP_

// include the needed export_*.hpp file (static/shared build)
#ifndef LOGGERLIB_STATIC_DEFINE
#include <loggerlib/export_shared.hpp>
#else
#include <loggerlib/export_static.hpp>
#endif

#endif // LOGGERLIB_EXPORT_HPP_