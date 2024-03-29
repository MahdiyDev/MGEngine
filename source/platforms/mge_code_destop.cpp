#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>

typedef struct {
    GLFWwindow* window; // GLFW window (graphic device)
} PlatformData;

extern CoreData CORE;

int Init_Platform(void);
void Close_Platform(void);
void Poll_Input_Events(void);
bool Window_Should_Close(void);
void Swap_Screen_Buffer(void);

static PlatformData platform = { 0 };

static void Error_Callback(int error, const char* description);

// GLFW3 Error Callback, runs on GLFW3 error
static void Error_Callback(int error, const char* description)
{
    TRACE_LOG(LOG_WARNING, "GLFW: Error: %i Description: %s", error, description);
}

int Init_Platform(void)
{
    glfwSetErrorCallback(Error_Callback);

    int result = glfwInit();
	glfwWindowHint(GLFW_RESIZABLE, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 0);

    if (result == GLFW_FALSE) {
        TRACE_LOG(LOG_ERROR, "GLFW: Failed to initialize GLFW");
        return EXIT_FAILURE;
    }
    TRACE_LOG(LOG_INFO, "GLFW: Initialized successfuly");
    glfwDefaultWindowHints();

    platform.window = glfwCreateWindow(
        CORE.Window.display.width,
        CORE.Window.display.height,
        CORE.Window.title,
        NULL, NULL);

    if (!platform.window) {
        TRACE_LOG(LOG_WARNING, "GLFW: Failed to initialize Window");
        Close_Platform();
        return EXIT_FAILURE;
    }
    TRACE_LOG(LOG_INFO, "GLFW: Window Created");

    glfwMakeContextCurrent(platform.window);
    result = glfwGetError(NULL);

    if ((result != GLFW_NO_WINDOW_CONTEXT) && (result != GLFW_PLATFORM_ERROR)) {
        CORE.Window.ready = true;
    }

    MgeGL_Load_Extensions((void*)glfwGetProcAddress);

    return EXIT_SUCCESS;
}

float Platform_Get_Time(void)
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

bool Window_Should_Close(void)
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