#ifndef LOG__
#define LOG__
#pragma once

#include "core/core.hpp"

#ifndef LOG_LEVEL
#error To Include core/log.hpp you must define LOGLEVEL
#endif


#define LOG_LOW    250
#define LOG_MEDIUM 500
#define LOG_HIGH   750
#define LOG_CRITICAL 999
#define LOG_ALWAYS 1000

#if LOG_LEVEL == LOG_ALWAYS
#ifndef NDEBUG
#warning Logging only with LOG_ALWAYS is redundant
#endif
#endif

namespace core {
    namespace log {
        static constexpr core::size level = LOG_LEVEL;
        static constexpr core::size low = LOG_LOW;
        static constexpr core::size medium = LOG_MEDIUM;
        static constexpr core::size high = LOG_HIGH;
        static constexpr core::size always = LOG_ALWAYS;
    };
};

#endif
