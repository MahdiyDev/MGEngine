#pragma once

#include "mge_config.h"
#include <cstdint>
#include <stdarg.h>

#define MGE_VERSION "v0.1"

// Callbacks to hook some internal functions
typedef void (*Trace_Log_Callback)(int logLevel, const char* text, va_list args); // Logging: Redirect trace log messages

typedef enum {
    LOG_ALL = 0, // Display all logs
    LOG_TRACE, // Trace logging, intended for internal use only
    LOG_DEBUG, // Debug logging, used for internal debugging, it should be disabled on release builds
    LOG_INFO, // Info logging, used for program execution info
    LOG_WARNING, // Warning logging, used on recoverable failures
    LOG_ERROR, // Error logging, used on unrecoverable failures
    LOG_FATAL, // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    LOG_NONE // Disable logging
} TraceLogLevel;

void Trace_Log(int logType, const char* text, ...);

void Init_Window(uint32_t width, uint32_t height, const char* title);
void Poll_Input_Events(void);
bool Window_Should_Close(void);
void Close_Window(void);
