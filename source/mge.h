#pragma once

#include "mge_config.h"
#include "mge_math.h"
#include <cstdint>
#include <glad/glad.h>
#include <stdarg.h>
#include <vector>

#define MGE_VERSION "v0.1"

#if defined(__cplusplus)
#define CLITERAL(type) type
#else
#define CLITERAL(type) (type)
#endif

// Callbacks to hook some internal functions
typedef void (*Trace_Log_Callback)(int logLevel, const char* text, va_list args); // Logging: Redirect trace log messages

typedef enum {
    LOG_ALL = 0,		// Display all logs
    LOG_TRACE,			// Trace logging, intended for internal use only
    LOG_DEBUG,			// Debug logging, used for internal debugging, it should be disabled on release builds
    LOG_INFO,			// Info logging, used for program execution info
    LOG_WARNING,		// Warning logging, used on recoverable failures
    LOG_ERROR,			// Error logging, used on unrecoverable failures
    LOG_FATAL,			// Fatal logging, used to abort program: exit(EXIT_FAILURE)
    LOG_NONE			// Disable logging
} TraceLogLevel;

typedef struct Color {
    unsigned char r;	// Color red value
    unsigned char g;	// Color green value
    unsigned char b;	// Color blue value
    unsigned char a;	// Color alpha value
} Color;

typedef struct Rectangle {
	float x;			// Rectangle top-left corner position x
	float y;			// Rectangle top-left corner position y
	float width;		// Rectangle width
	float height;		// Rectangle height
} Rectangle;

#define RED \
    CLITERAL(Color) { 255, 0, 0, 255 }
#define GREEN \
    CLITERAL(Color) { 0, 255, 0, 255 }
#define BLUE \
    CLITERAL(Color) { 0, 0, 255, 255 }
#define BLACK \
    CLITERAL(Color) { 0, 0, 0, 255 }
#define BLACK \
    CLITERAL(Color) { 0, 0, 0, 255 }
#define LIGHTGRAY \
    CLITERAL(Color) { 200, 200, 200, 255 }
#define GRAY \
    CLITERAL(Color) { 130, 130, 130, 255 }
#define DARKGRAY \
    CLITERAL(Color) { 80, 80, 80, 255 }
#define WHITE \
    CLITERAL(Color) { 255, 255, 255, 255 }

void Trace_Log(int logType, const char* text, ...);

// Core
void Init_Window(uint32_t width, uint32_t height, const char* title);
bool Window_Should_Close(void);
void Close_Window(void);
float Get_Time(void);
int Get_Fps(void);
void Set_Target_FPS(int fps);

// Draw
void Begin_Drawing(void);
void End_Drawing(void);
void Clear_Background(Color color);
void MgeGL_Ortho(double left, double right, double bottom, double top, double znear, double zfar);

// Shapes
void Draw_Line(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
void Draw_LineV(Vector2 startPos, Vector2 endPos, Color color);
void Draw_Rectangle(int posX, int posY, int width, int height, Color color);
void Draw_RectangleV(Vector2 position, Vector2 size, Color color);
void Draw_RectangleRec(Rectangle rec, Color color);
void Draw_RectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);
void Draw_RectangleLines(int posX, int posY, int width, int height, Color color);
