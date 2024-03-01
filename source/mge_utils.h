#pragma once

#include "mge.h"

#if defined(SUPPORT_TRACELOG)
    #define TRACE_LOG(level, ...) Trace_Log(level, __VA_ARGS__)

    #if defined(SUPPORT_TRACELOG_DEBUG)
        #define TRACE_LOG_D(...) Trace_Log(LOG_DEBUG, __VA_ARGS__)
    #else
        #define TRACE_LOG_D(...) (void)0
    #endif
#else
    #define TRACE_LOG(level, ...) (void)0
    #define TRACE_LOG_D(...) (void)0
#endif
