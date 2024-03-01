#include "mge.h"
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
        glfwTerminate();
        TRACE_LOG(LOG_WARNING, "GLFW: Failed to initialize Window");
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(platform.window);
    result = glfwGetError(NULL);

    if ((result != GLFW_NO_WINDOW_CONTEXT) && (result != GLFW_PLATFORM_ERROR)) {
        CORE.Window.ready = true;
    }

    return EXIT_SUCCESS;
}

void Poll_Input_Events(void)
{
    glfwPollEvents();
    CORE.Window.shouldClose = glfwWindowShouldClose(platform.window);
    glfwSetWindowShouldClose(platform.window, GLFW_FALSE);
	Swap_Screen_Buffer();
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