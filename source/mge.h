#pragma once

#include "mge_config.h"
#include "mge_math.h"
#include <cstdarg>
#include <cstdint>

#define MGE_VERSION "v0.0.1"

#if defined(__cplusplus)
	#define CLITERAL(type) type
#else
	#define CLITERAL(type) (type)
#endif

void Trace_Log(int logType, const char* text, ...);
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

// Camera projection
typedef enum {
	CAMERA_PERSPECTIVE = 0,  // Perspective projection
	CAMERA_ORTHOGRAPHIC      // Orthographic projection
} CameraProjection;

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

#define RED			CLITERAL(Color) { 255, 0, 0, 255 }
#define GREEN		CLITERAL(Color) { 0, 255, 0, 255 }
#define BLUE		CLITERAL(Color) { 0, 0, 255, 255 }
#define DARKGREEN	CLITERAL(Color) { 51, 77, 77, 255 }
#define BLACK		CLITERAL(Color) { 0, 0, 0, 255 }
#define GRAY		CLITERAL(Color) { 130, 130, 130, 255 }
#define LIGHTGRAY	CLITERAL(Color) { 200, 200, 200, 255 }
#define DARKGRAY	CLITERAL(Color) { 80, 80, 80, 255 }
#define WHITE		CLITERAL(Color) { 255, 255, 255, 255 }
#define YELLOW		CLITERAL(Color) { 255, 255, 0, 255 }

typedef struct Camera3D {
	Vector3 position;       // Camera position
	Vector3 target;         // Camera target it looks-at
	Vector3 up;             // Camera up vector (rotation over its axis)
	float fovy;             // Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic
	int projection;         // Camera projection: CAMERA_PERSPECTIVE or CAMERA_ORTHOGRAPHIC
} Camera3D;

#ifdef CORE_INCLUDE
typedef struct {
    int x;
    int y;
} Point;
typedef struct {
    uint32_t width;
    uint32_t height;
} Size;

typedef struct CoreData {
    struct {
        const char* title;
        Size display;
        Size screen;
        Size render;
        bool ready;
        bool shouldClose;
    } Window;

	struct {
		double frame;
		unsigned int frameCounter;
		unsigned long long int base;
		double current;
		double draw;
		double previous;
		double target;
		double update;
	} Time;
} CoreData;
#endif

// Core
void Mge_InitWindow(uint32_t width, uint32_t height, const char* title);
bool Mge_WindowShouldClose(void);
void Mge_CloseWindow(void);
double Mge_GetTime(void);
int Mge_GetFps(void);
void Mge_SetTargetFPS(int fps);

void Mge_ProcessInput();
void Mge_ClearBackground(Color color);
void Mge_BeginDrawing();
void Mge_EndDrawing();
void Mge_BeginMode3D(Camera3D camera);
void Mge_EndMode3D(void);

// Shapes
void Draw_Line(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
void Draw_LineV(Vector2 startPos, Vector2 endPos, Color color);
void Draw_Rectangle(int posX, int posY, int width, int height, Color color);
void Draw_RectangleV(Vector2 position, Vector2 size, Color color);
void Draw_RectangleRec(Rectangle rec, Color color);
void Draw_RectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);
void Draw_RectangleLines(int posX, int posY, int width, int height, Color color);
void Draw_Triangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void Draw_Triangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void Draw_TriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void Draw_TriangleFan(Vector2 *points, int pointCount, Color color);
void Draw_TriangleStrip(Vector2 *points, int pointCount, Color color);
