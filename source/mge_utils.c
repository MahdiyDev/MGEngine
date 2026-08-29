#include "mge.h"
#include "mge_utils.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// mlib: byte buffer used for file loading
#define STRING_IMPLEMENTATION
#include "string/string.h"

#ifndef MAX_TRACELOG_MSG_LENGTH
    #define MAX_TRACELOG_MSG_LENGTH 256 // Max length of one trace-log message
#endif

static int logTypeLevel = LOG_INFO;        // Minimum log type level
static Trace_Log_Callback traceLog = NULL; // TraceLog callback function pointer

// Show trace log messages (LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG)
void Trace_Log(int logType, const char* text, ...)
{
#if defined(SUPPORT_TRACELOG)
    // Message has level below current threshold, don't emit
    if (logType < logTypeLevel)
        return;

    va_list args;
    va_start(args, text);

    if (traceLog) {
        traceLog(logType, text, args);
        va_end(args);
        return;
    }

    char buffer[MAX_TRACELOG_MSG_LENGTH] = { 0 };

    switch (logType) {
    case LOG_TRACE:
        strcpy(buffer, "TRACE: ");
        break;
    case LOG_DEBUG:
        strcpy(buffer, "DEBUG: ");
        break;
    case LOG_INFO:
        strcpy(buffer, "INFO: ");
        break;
    case LOG_WARNING:
        strcpy(buffer, "WARNING: ");
        break;
    case LOG_ERROR:
        strcpy(buffer, "ERROR: ");
        break;
    case LOG_FATAL:
        strcpy(buffer, "FATAL: ");
        break;
    default:
        break;
    }

    size_t offset = strlen(buffer);
    size_t room = MAX_TRACELOG_MSG_LENGTH - offset - 2; // leave space for '\n' and '\0'
    size_t textSize = strlen(text);
    if (textSize > room)
        textSize = room;
    memcpy(buffer + offset, text, textSize);
    buffer[offset + textSize] = '\n';
    buffer[offset + textSize + 1] = '\0';

    vprintf(buffer, args);
    fflush(stdout);

    va_end(args);

    if (logType == LOG_FATAL)
        exit(EXIT_FAILURE); // If fatal logging, exit program
#else
    (void)logType;
    (void)text;
#endif
}

// Load a text file into a NUL-terminated, caller-owned buffer (free with Mge_UnLoadFileText).
char* Mge_LoadFileText(const char* fileName)
{
    if (fileName == NULL) {
        TRACE_LOG(LOG_WARNING, "FILEIO: File name provided is not valid");
        return NULL;
    }

    string_builder sb = { 0 };
    if (!sb_read_file(&sb, fileName)) {
        TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to read text file", fileName);
        sb_free(&sb);
        return NULL;
    }

    sb_add_c(&sb, '\0'); // so the result is a valid C string
    TRACE_LOG(LOG_INFO, "FILEIO: [%s] Text file loaded successfully", fileName);
    return sb.items;
}

void Mge_UnLoadFileText(char* fileData)
{
    free(fileData);
}

// Load a binary file. *dataSize receives the byte count. Free with Mge_UnloadFileData.
unsigned char* Mge_LoadFileData(const char* fileName, size_t* dataSize)
{
    *dataSize = 0;

    if (fileName == NULL) {
        TRACE_LOG(LOG_WARNING, "FILEIO: File name provided is not valid");
        return NULL;
    }

    string_builder sb = { 0 };
    if (!sb_read_file(&sb, fileName)) {
        TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to read file", fileName);
        sb_free(&sb);
        return NULL;
    }

    if (sb.count > (size_t)INT_MAX) {
        TRACE_LOG(LOG_WARNING, "FILEIO: [%s] File is bigger than %d bytes, avoid using Mge_LoadFileData()", fileName, INT_MAX);
        sb_free(&sb);
        return NULL;
    }

    *dataSize = sb.count;
    TRACE_LOG(LOG_INFO, "FILEIO: [%s] File loaded successfully (%zu bytes)", fileName, sb.count);
    return (unsigned char*)sb.items;
}

void Mge_UnloadFileData(unsigned char* data)
{
    free(data);
}

const char* Mge_GetFileExtension(const char* fileName)
{
    const char* dot = strrchr(fileName, '.');

    if (!dot || dot == fileName)
        return NULL;

    return dot;
}
