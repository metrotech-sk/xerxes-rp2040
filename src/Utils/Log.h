#ifndef __LOG_HPP
#define __LOG_HPP

#include <string>
#include <iostream>
#include <sstream>
#include "pico/time.h"
#include "stdio.h"
#include <cstdlib>
#include <cstring>

// log level is set in CMakeLists.txt
// #define _LOG_LEVEL 1 = ERROR
// #define _LOG_LEVEL 2 = WARNING
// #define _LOG_LEVEL 3 = INFO
// #define _LOG_LEVEL 4 = DEBUG

// macro to strip file path from __FILE__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define _CLR_BOLD "\033[1m"
#define _CLR_B_RED "\033[1;31m"
#define _CLR_B_YLW "\033[1;33m"
#define _CLR_B_GREY "\033[1;2m"
#define _CLR_RED "\033[31m"
#define _CLR_YLW "\033[33m"
#define _CLR_GREY "\033[2m"
#define _CLR_RST "\033[0m"

#ifdef NDEBUG
// do not log in release mode
#define xlog(level, msg) \
    do                   \
    {                    \
    } while (0)
#else
// define xerxes_log(level, msg) to printf as a macro in format
// [Time][Log level] — [File]:[Line] [Function] — [Text]
// e.g. [0.000000][INFO] — [src/main.cpp:123 main] — Hello World!
// sleep_ms(1) is used to prevent the log messages from being printed in the
// wrong order, enough for 200 bytes with baudrate 1500000
#define xlog(level, msg)                                                                                                                                                                            \
    do                                                                                                                                                                                              \
    {                                                                                                                                                                                               \
        std::stringstream ss;                                                                                                                                                                       \
        double timestamp = time_us_64() / 1000000.0f;                                                                                                                                               \
        ss << _CLR_GREY << "[" << timestamp << "][" << _CLR_RST << level << _CLR_GREY << "] — [" << __FILENAME__ << ":" << __LINE__ << " " << __func__ << "()] — " << _CLR_RST << msg << std::endl; \
        printf(ss.str().c_str()); /* printf is threadsafe, std::cout is not ! */                                                                                                                    \
    } while (0)
#endif // NDEBUG

// always log errors
#define xlog_error(msg)                                                       \
    do                                                                        \
    {                                                                         \
        xlog(_CLR_B_RED << "ERROR" << _CLR_RST, _CLR_RED << msg << _CLR_RST); \
    } while (0)

// convenience macros
#define xlog_err(msg) xlog_error(msg)
#define xloge(msg) xlog_error(msg)

// log warnings if log level is warning or higher
#if (_LOG_LEVEL >= 2)
#define xlog_warning(msg)                                                       \
    do                                                                          \
    {                                                                           \
        xlog(_CLR_B_YLW << "WARNING" << _CLR_RST, _CLR_YLW << msg << _CLR_RST); \
    } while (0)
#else
#define xlog_warning(msg) \
    do                    \
    {                     \
    } while (0)
#endif // _LOG_LEVEL

// convenience macros
#define xlog_warn(msg) xlog_warning(msg)
#define xlogw(msg) xlog_warning(msg)

#if (_LOG_LEVEL >= 3)
#define xlog_info(msg)                              \
    do                                              \
    {                                               \
        xlog(_CLR_BOLD << "INFO" << _CLR_RST, msg); \
    } while (0)
#else
#define xlog_info(msg) \
    do                 \
    {                  \
    } while (0)
#endif // _LOG_LEVEL

// convenience macros
#define xlogi(msg) xlog_info(msg)

// log debug messages if log level is debug
#if (_LOG_LEVEL >= 4)
#define xlog_debug(msg)                                                         \
    do                                                                          \
    {                                                                           \
        xlog(_CLR_B_GREY << "DEBUG" << _CLR_RST, _CLR_GREY << msg << _CLR_RST); \
    } while (0)
#else
#define xlog_debug(msg) \
    do                  \
    {                   \
    } while (0)
#endif // _LOG_LEVEL

// convenience macros
#define xlogd(msg) xlog_debug(msg)
#define xlog_dbg(msg) xlog_debug(msg)

#if (_LOG_LEVEL >= 5)
#define xlog_trace(msg)                                                         \
    do                                                                          \
    {                                                                           \
        xlog(_CLR_B_GREY << "TRACE" << _CLR_RST, _CLR_GREY << msg << _CLR_RST); \
    } while (0)
#else
#define xlog_trace(msg) \
    do                  \
    {                   \
    } while (0)
#endif // _LOG_LEVEL

// convenience macros
#define xlogt(msg) xlog_trace(msg)
#define xlog_trc(msg) xlog_trace(msg)

#endif /* LOG_HPP */