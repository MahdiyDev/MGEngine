#include <cstdio>
#define CORE_INCLUDE
#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad//glad.h>

#include <cstdlib>

#if defined(_WIN32)
#include <minwindef.h>
#include <timeapi.h>
#endif

typedef struct {
    GLFWwindow* window; // GLFW window (graphic device)
} PlatformData;

extern CoreData CORE;

void InitPlatform(void);
void Close_Platform(void);
void Poll_Input_Events(void);
bool Mge_WindowShouldClose(void);
void Swap_Screen_Buffer(void);
void InitTimer(void);
// void SetFramebufferSizeCallback(int width, int height);

static PlatformData platform = { 0 };

static void Error_Callback(int error, const char* description);

// GLFW3 Error Callback, runs on GLFW3 error
static void Error_Callback(int error, const char* description)
{
    TRACE_LOG(LOG_WARNING, "GLFW: Error: %i Description: %s", error, description);
}

void InitPlatform(void)
{
    glfwSetErrorCallback(Error_Callback);

    int result = glfwInit();
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 0);

    if (result == GLFW_FALSE) {
        TRACE_LOG(LOG_ERROR, "GLFW: Failed to initialize GLFW");
        exit(EXIT_FAILURE);
    }
    TRACE_LOG(LOG_INFO, "GLFW: Initialized successfuly");
    // glfwDefaultWindowHints();

    platform.window = glfwCreateWindow(
        CORE.Window.screen.width,
        CORE.Window.screen.height,
        CORE.Window.title,
        glfwGetPrimaryMonitor(), NULL);

    if (!platform.window) {
        TRACE_LOG(LOG_WARNING, "GLFW: Failed to initialize Window");
        Close_Platform();
        exit(EXIT_FAILURE);
    }
    TRACE_LOG(LOG_INFO, "GLFW: Window Created");

    glfwMakeContextCurrent(platform.window);
    result = glfwGetError(NULL);

    if ((result != GLFW_NO_WINDOW_CONTEXT) && (result != GLFW_PLATFORM_ERROR)) {
        CORE.Window.ready = true;
    }

    MgeGL_Load_Extensions((void*)glfwGetProcAddress);

	glfwSwapInterval(0); // No v-sync by default

	InitTimer();

	CORE.Window.render.width = CORE.Window.screen.width;
    CORE.Window.render.height = CORE.Window.screen.height;

	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(monitor);
	CORE.Window.display.width = mode->width;
	CORE.Window.display.height = mode->height;
}

double Platform_GetTime(void)
{
	return glfwGetTime();
}

void Poll_Input_Events(void)
{
    glfwPollEvents();
    CORE.Window.shouldClose = glfwWindowShouldClose(platform.window);
    glfwSetWindowShouldClose(platform.window, GLFW_FALSE);
}

void Swap_Screen_Buffer(void)
{
    glfwSwapBuffers(platform.window);
}

bool Mge_WindowShouldClose(void)
{
    if (CORE.Window.ready) {
        return CORE.Window.shouldClose;
    }
    return true;
}

void Close_Platform(void)
{
    glfwDestroyWindow(platform.window);
    glfwTerminate();
    TRACE_LOG(LOG_INFO, "GLFW: terminated");
}

void Mge_ProcessInput(void)
{
    if(glfwGetKey(platform.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(platform.window, true);
}