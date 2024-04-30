#include "mge.h"
#include "mge_utils.h"

#include <climits>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_TRACELOG_MSG_LENGTH
	#define MAX_TRACELOG_MSG_LENGTH 256 // Max length of one trace-log message
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static int logTypeLevel = LOG_INFO; // Minimum log type level
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

    unsigned int textSize = (unsigned int)strlen(text);
    memcpy(buffer + strlen(buffer), text, (textSize < (MAX_TRACELOG_MSG_LENGTH - 12)) ? textSize : (MAX_TRACELOG_MSG_LENGTH - 12));
    strcat(buffer, "\n");
    vprintf(buffer, args);
    fflush(stdout);

    va_end(args);

    if (logType == LOG_FATAL)
        exit(EXIT_FAILURE); // If fatal logging, exit program
#endif
}

char* Mge_LoadFileText(const char* fileName)
{
	FILE *fptr;
	size_t file_size = 0;
	char* file_data = NULL;

	if (fileName == NULL)
	{
		TRACE_LOG(LOG_WARNING, "FILEIO: File name provided is not valid");
		return file_data;
	}

	fptr = fopen(fileName, "rt");

	if (fptr != NULL)
	{
		fseek(fptr, 0, SEEK_END);
		file_size = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);

		if (file_size > 0)
		{
			file_data = (char*)malloc((file_size+1)*sizeof(char));

			if (file_data != NULL)
			{
				size_t count = fread(file_data, sizeof(char), file_size, fptr);
				if (count < file_size) file_data = (char*)realloc(file_data, count + 1);
				file_data[count] = '\0';

				TRACE_LOG(LOG_INFO, "FILEIO: [%s] Text file loaded successfully", fileName);
			} else TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to allocated memory for file reading", fileName);
		} else TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to read text file", fileName);
	} else TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to open text file", fileName);

	fclose(fptr);

	return file_data;
}

void Mge_UnLoadFileText(char* fileData)
{
	free(fileData);
}

unsigned char* Mge_LoadFileData(const char *fileName, size_t *dataSize)
{
	FILE *fptr;
	size_t file_size = 0;
	unsigned char* file_data = NULL;
	*dataSize = 0;

	if (fileName == NULL)
	{
		TRACE_LOG(LOG_WARNING, "FILEIO: File name provided is not valid");
		return file_data;
	}

	fptr = fopen(fileName, "rb");

	if (fptr != NULL)
	{
		fseek(fptr, 0, SEEK_END);
		file_size = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);

		if (file_size > 0)
		{
			file_data = (unsigned char*)malloc((file_size+1)*sizeof(unsigned char));

			if (file_data != NULL)
			{
				size_t count = fread(file_data, sizeof(char), file_size, fptr);

				if (count > INT_MAX)
				{
					TRACE_LOG(LOG_WARNING, "FILEIO: [%s] File is bigger than %d bytes, avoid using Mge_LoadFileData()", fileName, INT_MAX);

					free(file_data);
					file_data = NULL;
				}
				else
				{
					*dataSize = (int)count;

					if ((*dataSize) != file_size) TRACE_LOG(LOG_WARNING, "FILEIO: [%s] File partially loaded (%i bytes out of %i)", fileName, dataSize, count);
					else TRACE_LOG(LOG_INFO, "FILEIO: [%s] File loaded successfully", fileName);
                }
			} else TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to allocated memory for file reading", fileName);
		} else TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to read text file", fileName);
	} else TRACE_LOG(LOG_WARNING, "FILEIO: [%s] Failed to open text file", fileName);

	fclose(fptr);

	return file_data;
}

void Mge_UnloadFileData(unsigned char *data)
{
	free(data);
}

const char* Mge_GetFileExtension(const char *fileName)
{
	const char *dot = strrchr(fileName, '.');

	if (!dot || dot == fileName) return NULL;

	return dot;
}
