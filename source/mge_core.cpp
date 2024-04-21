#include "mge_math.h"
#include <math.h>
#define CORE_INCLUDE
#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"
#include <cmath>
#include <cstdint>

#if defined(_WIN32)
#include <synchapi.h>
// void __stdcall Sleep(unsigned long msTimeout);              // Required for: WaitTime()
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
#include <time.h>
#endif

extern void InitPlatform(void);
extern void Close_Platform(void);
extern double Platform_GetTime(void);

static void SetupViewport(uint32_t width, uint32_t height);
static void InitTimer(void);

CoreData CORE = { 0 };

#if defined(PLATFORM_DESKTOP)
	#include "platforms/mge_code_destop.cpp"
#endif

void Mge_InitWindow(uint32_t width, uint32_t height, const char* title)
{
    TRACE_LOG(LOG_INFO, "Initializing MGE %s", MGE_VERSION);
#if defined(PLATFORM_DESKTOP)
    TRACE_LOG(LOG_INFO, "Platform backend: DESKTOP (GLFW)");
#endif

    CORE.Window.screen.width = width;
    CORE.Window.screen.height = height;
    CORE.Window.title = title;

    InitPlatform();

	MgeGL_Init(CORE.Window.screen.width, CORE.Window.screen.height);

    SetupViewport(CORE.Window.screen.width, CORE.Window.screen.height);

    CORE.Window.shouldClose = false;
	CORE.Time.frameCounter = 0;
}

static void SetupViewport(uint32_t width, uint32_t height)
{
	MgeGL_Viewport(0, 0, width, height);

	MgeGL_MatrixMode(MGEGL_PROJECTION);        // Switch to projection matrix
	MgeGL_LoadIdentity();                      // Reset current matrix (projection)

	MgeGL_Ortho(0, width, height, 0, 0.0f, 1.0f);

	MgeGL_MatrixMode(MGEGL_MODELVIEW);         // Switch back to modelview matrix
	MgeGL_LoadIdentity();                      // Reset current matrix (modelview)
}

void InitTimer(void)
{
    // Setting a higher resolution can improve the accuracy of time-out intervals in wait functions.
    // However, it can also reduce overall system performance, because the thread scheduler switches tasks more often.
    // High resolutions can also prevent the CPU power management system from entering power-saving modes.
    // Setting a higher resolution does not improve the accuracy of the high-resolution performance counter.
#if defined(_WIN32)
	timeBeginPeriod(1);                 // Setup high-resolution timer to 1ms (granularity of 1-2 ms)
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
    struct timespec now = { 0 };

	if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)  // Success
	{
		CORE.Time.base = (unsigned long long int)now.tv_sec*1000000000LLU + (unsigned long long int)now.tv_nsec;
	} else {
		TRACE_LOG(LOG_WARNING, "TIMER: Hi-resolution timer not available");
	}
#endif

    CORE.Time.previous = Mge_GetTime();     // Get time as double
}

void WaitTime(double seconds)
{
	if (seconds < 0) return;

	double destinationTime = Mge_GetTime() + seconds;

	double sleepSeconds = seconds - seconds*0.05;  // NOTE: We reserve a percentage of the time for busy waiting

    // System halt functions
    #if defined(_WIN32)
        Sleep((unsigned long)(sleepSeconds*1000.0));
    #endif
    #if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        struct timespec req = { 0 };
        time_t sec = sleepSeconds;
        long nsec = (sleepSeconds - sec)*1000000000L;
        req.tv_sec = sec;
        req.tv_nsec = nsec;

        // NOTE: Use nanosleep() on Unix platforms... usleep() it's deprecated.
        while (nanosleep(&req, &req) == -1) continue;
    #endif
    #if defined(__APPLE__)
        usleep(sleepSeconds*1000000.0);
    #endif

	while (Mge_GetTime() < destinationTime) { }
}

void Mge_CloseWindow(void)
{
	MgeGL_Close();
    Close_Platform();
    CORE.Window.ready = false;
    TRACE_LOG(LOG_INFO, "Window closed successfully");
}

double Mge_GetTime(void)
{
	return Platform_GetTime();
}

void Mge_ClearBackground(Color color)
{
    MgeGL_ClearColor(color);
    MgeGL_ClearScreenBuffers();
}

void Mge_BeginDrawing(void)
{
	CORE.Time.current = Mge_GetTime();
	CORE.Time.update = CORE.Time.current - CORE.Time.previous;
	CORE.Time.previous = CORE.Time.current;
	Mge_ProcessInput();
}

void Mge_EndDrawing(void)
{
	Swap_Screen_Buffer();

	CORE.Time.current = Mge_GetTime();
	CORE.Time.draw = CORE.Time.current - CORE.Time.previous;
	CORE.Time.previous = CORE.Time.current;

	CORE.Time.frame = CORE.Time.update + CORE.Time.draw;

	// Wait for some milliseconds...
	if (CORE.Time.frame < CORE.Time.target)
	{
		WaitTime(CORE.Time.target - CORE.Time.frame);

		CORE.Time.current = Mge_GetTime();
		double waitTime = CORE.Time.current - CORE.Time.previous;
		CORE.Time.previous = CORE.Time.current;

		CORE.Time.frame += waitTime;    // Total frame time: update + draw + wait
	}
	CORE.Time.frameCounter++;
	Poll_Input_Events();
}

void Mge_BeginMode3D(Camera3D camera)
{
	// MgeGL_Draw();
	MgeGL_MatrixMode(MGEGL_PROJECTION);
	MgeGL_LoadIdentity();

	float aspect = (float)CORE.Window.screen.width/(float)CORE.Window.screen.height;

	if (camera.projection == CAMERA_PERSPECTIVE)
	{
		double top = MGE_CULL_DISTANCE_NEAR*tan(camera.fovy*0.5*DEG2RAD);
		double right = top*aspect;

		MgeGL_Frustum(-right, right, -top, top, MGE_CULL_DISTANCE_NEAR, MGE_CULL_DISTANCE_FAR);
	}
	else if (camera.projection == CAMERA_ORTHOGRAPHIC)
    {
		// Setup orthographic projection
		double top = camera.fovy/2.0;
		double right = top*aspect;

		MgeGL_Ortho(-right, right, -top,top, MGE_CULL_DISTANCE_NEAR, MGE_CULL_DISTANCE_FAR);
    }
	MgeGL_Translatef(0, 0, -6.0f);
	// MgeGL_Rotatef(45.0f*Mge_GetTime(), 1.0f, 1.0f, 0);

	MgeGL_MatrixMode(MGEGL_MODELVIEW);
	MgeGL_LoadIdentity();

	// Vector3 direction = Vector3Normalize(Vector3Subtract(camera.position, camera.target));
	float radius = 10.0f;
	float camX = sin(Mge_GetTime()) * radius;
	float camZ = cos(Mge_GetTime()) * radius;

	Matrix matView = MatrixLookAt(
		Vector3 {camX, 0.0f, camZ},
		Vector3 {0.0f, 0.0f, 0.0f},
		Vector3 {0.0f, 1.0f, 0.0f}
	);
	// Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
	MgeGL_MultMatrixf(MatrixToFloat(matView));      // Multiply modelview matrix by view matrix (camera)

	MgeGL_EnableDepthTest();
}

void Mge_EndMode3D(void)
{
	MgeGL_DisableDepthTest();
}

float Get_Frame_Time(void)
{
    return (float)CORE.Time.frame;
}

void Mge_SetTargetFPS(int fps)
{
	if (fps < 1) { CORE.Time.target = 0.0; }
	else { CORE.Time.target = 1.0/(double)fps; }
	TRACE_LOG(LOG_INFO, "TIMER: Target time per frame: %02.03f milliseconds", (float)CORE.Time.target*1000.0f);
}

int Mge_GetFps(void)
{
	int fps = 0;

	#define FPS_CAPTURE_FRAMES_COUNT    30      // 30 captures
	#define FPS_AVERAGE_TIME_SECONDS   0.5f     // 500 milliseconds
	#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS/FPS_CAPTURE_FRAMES_COUNT)

	static int index = 0;
	static float history[FPS_CAPTURE_FRAMES_COUNT] = { 0 };
	static float average = 0, last = 0;
	float fpsFrame = Get_Frame_Time();

    // if we reset the window, reset the FPS info
	if (CORE.Time.frameCounter == 0)
	{
		average = 0;
		last = 0;
		index = 0;

		for (int i = 0; i < FPS_CAPTURE_FRAMES_COUNT; i++) history[i] = 0;
	}

	if (fpsFrame == 0) return 0;

	if ((Mge_GetTime() - last) > FPS_STEP)
	{
		last = (float)Platform_GetTime();
		index = (index + 1)%FPS_CAPTURE_FRAMES_COUNT;
		average -= history[index];
		history[index] = fpsFrame/FPS_CAPTURE_FRAMES_COUNT;
		average += history[index];
	}

	fps = (int)std::roundf(1.0f/average);

	return fps;
}
/*
#include <cstdio>
#include <cstdlib>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

typedef struct CoreData {
	struct {
		GLFWwindow* handle;
	} window;
} CoreData;

CoreData CORE = { 0 };

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// static void Error_Callback(int error, const char* description);

// // GLFW3 Error Callback, runs on GLFW3 error
// static void Error_Callback(int error, const char* description)
// {
//     TRACE_LOG(LOG_WARNING, "GLFW: Error: %i Description: %s", error, description);
// }

void Mge_InitWindow(uint32_t width, uint32_t height, const char* title)
{
	int result = glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 0);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	if (result == GLFW_FALSE)
	{
		TRACE_LOG(LOG_ERROR, "GLFW: Failed to initialize GLFW");
		exit(EXIT_FAILURE);
	}
	TRACE_LOG(LOG_INFO, "GLFW: Initialized successfuly");

	CORE.window.handle = glfwCreateWindow(800, 600, title, NULL, NULL);
	if (CORE.window.handle == NULL)
	{
		printf("Failed to create GLFW window");
		glfwTerminate();
		exit(1);
	}
	glfwMakeContextCurrent(CORE.window.handle);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD");
		exit(1);
	}

	glfwSetFramebufferSizeCallback(CORE.window.handle, framebuffer_size_callback);

	MgeGL_Init();
}

void Mge_CloseWindow()
{
	MgeGL_Close();
	glfwTerminate();
}

void Mge_BeginDrawing()
{
	Mge_ProcessInput();
}

void Mge_EndDrawing()
{
	glfwSwapBuffers(CORE.window.handle);
	glfwPollEvents();
}

double Mge_GetTime()
{
	return glfwGetTime();
}

bool Mge_WindowShouldClose()
{
	return glfwWindowShouldClose(CORE.window.handle);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void Mge_ProcessInput()
{
    if(glfwGetKey(CORE.window.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(CORE.window.handle, true);
}
*/