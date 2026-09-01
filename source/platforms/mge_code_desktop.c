// Desktop platform backend (GLFW). Included into mge_core.c.

#include <stdio.h>
#include <stdlib.h>

#define CORE_INCLUDE
#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#if defined(_WIN32)
    #include <timeapi.h>
#endif

typedef struct {
    GLFWwindow* window; // GLFW window (graphic device)
} PlatformData;

extern CoreData CORE;

// NOTE: this file is #included into mge_core.c; InitTimer() is defined there.
static void InitTimer(void);

static PlatformData platform = { 0 };

static void Error_Callback(int error, const char* description);
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void MouseCursorPosCallback(GLFWwindow* window, double x, double y);
static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
static void Framebuffer_Size_Callback(GLFWwindow* window, int w, int h);

// GLFW3 Error Callback, runs on GLFW3 error
static void Error_Callback(int error, const char* description)
{
    TRACE_LOG(LOG_WARNING, "GLFW: Error: %i Description: %s", error, description);
}

void InitPlatform(void)
{
    glfwSetErrorCallback(Error_Callback);

    int result = glfwInit();
    if (result == GLFW_FALSE) {
        TRACE_LOG(LOG_ERROR, "GLFW: Failed to initialize GLFW");
        exit(EXIT_FAILURE);
    }
    TRACE_LOG(LOG_INFO, "GLFW: Initialized successfully");

    glfwWindowHint(GLFW_RESIZABLE, Mge_GetWindowResizable() ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_SAMPLES, Mge_GetRequestedMSAA()); // MSAA; 0 = off (see Mge_SetMSAA)
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);         // so GL_FRAMEBUFFER_SRGB works (see Mge_SetGammaCorrection)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, Mge_GetDebugOutput() ? GLFW_TRUE : GLFW_FALSE);

    platform.window = glfwCreateWindow(
        (int)CORE.Window.screen.width,
        (int)CORE.Window.screen.height,
        CORE.Window.title,
        NULL, NULL);

    if (!platform.window) {
        TRACE_LOG(LOG_ERROR, "GLFW: Failed to create window");
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

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor != NULL) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (mode != NULL) {
            CORE.Window.display.width = (uint32_t)mode->width;
            CORE.Window.display.height = (uint32_t)mode->height;
        }
    }

    glfwSetKeyCallback(platform.window, KeyCallback);
    glfwSetCursorPosCallback(platform.window, MouseCursorPosCallback);
    glfwSetMouseButtonCallback(platform.window, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(platform.window, Framebuffer_Size_Callback);
    if (Mge_GetWindowResizable())
        glfwSetWindowSizeLimits(platform.window, 640, 400, GLFW_DONT_CARE, GLFW_DONT_CARE);
}

double Platform_GetTime(void)
{
    return glfwGetTime();
}

void* Mge_GetWindowHandle(void)
{
    return platform.window;
}

void Poll_Input_Events(void)
{
    CORE.Input.Keyboard.keyPressedQueueCount = 0;
    CORE.Input.Keyboard.charPressedQueueCount = 0;

    CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;

    for (int i = 0; i < MAX_KEYBOARD_KEYS; i++) {
        CORE.Input.Keyboard.previousKeyState[i] = CORE.Input.Keyboard.currentKeyState[i];
        CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;
    }
    for (int i = 0; i < MAX_MOUSE_BUTTONS; i++) {
        CORE.Input.Mouse.previousButtonState[i] = CORE.Input.Mouse.currentButtonState[i];
    }

    glfwPollEvents();
    if (glfwWindowShouldClose(platform.window))
        CORE.Window.shouldClose = true;
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

void Mge_SetWindowShouldClose(bool value)
{
    CORE.Window.shouldClose = value;
    if (platform.window != NULL)
        glfwSetWindowShouldClose(platform.window, value ? GLFW_TRUE : GLFW_FALSE);
}

void Mge_SetWindowSize(int width, int height)
{
    if (platform.window == NULL || width <= 0 || height <= 0)
        return;
    glfwSetWindowSize(platform.window, width, height); // -> Framebuffer_Size_Callback
}

// the windowed-mode rect, saved when we go fullscreen so it can be restored
static struct { int x, y, w, h; bool saved; } s_windowedRect;
static bool s_vsync = false;

void Mge_SetVSync(bool enabled)
{
    s_vsync = enabled;
    if (platform.window != NULL)
        glfwSwapInterval(enabled ? 1 : 0);
}

bool Mge_IsVSyncEnabled(void) { return s_vsync; }

int Mge_GetMonitorRefreshRate(void)
{
    GLFWmonitor* mon = platform.window ? glfwGetWindowMonitor(platform.window) : NULL;
    if (mon == NULL)
        mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = mon ? glfwGetVideoMode(mon) : NULL;
    return (mode != NULL && mode->refreshRate > 0) ? mode->refreshRate : 0;
}

bool Mge_IsFullscreen(void)
{
    return platform.window != NULL && glfwGetWindowMonitor(platform.window) != NULL;
}

void Mge_ToggleFullscreen(void)
{
    if (platform.window == NULL)
        return;

    if (glfwGetWindowMonitor(platform.window) == NULL) {
        glfwGetWindowPos(platform.window, &s_windowedRect.x, &s_windowedRect.y);
        glfwGetWindowSize(platform.window, &s_windowedRect.w, &s_windowedRect.h);
        s_windowedRect.saved = true;

        GLFWmonitor* mon = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = mon ? glfwGetVideoMode(mon) : NULL;
        if (mon == NULL || mode == NULL)
            return;
        glfwSetWindowMonitor(platform.window, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        int x = s_windowedRect.saved ? s_windowedRect.x : 100;
        int y = s_windowedRect.saved ? s_windowedRect.y : 100;
        int w = (s_windowedRect.saved && s_windowedRect.w > 0) ? s_windowedRect.w : 1280;
        int h = (s_windowedRect.saved && s_windowedRect.h > 0) ? s_windowedRect.h : 720;
        glfwSetWindowMonitor(platform.window, NULL, x, y, w, h, 0);
    }

    glfwSwapInterval(s_vsync ? 1 : 0); // glfwSetWindowMonitor can reset the swap interval

    // a non-resizable window doesn't get Framebuffer_Size_Callback -- sync now
    int fw = 0, fh = 0;
    glfwGetFramebufferSize(platform.window, &fw, &fh);
    if (fw > 0 && fh > 0) {
        CORE.Window.screen.width = (uint32_t)fw;
        CORE.Window.screen.height = (uint32_t)fh;
        CORE.Window.render.width = (uint32_t)fw;
        CORE.Window.render.height = (uint32_t)fh;
        SetupViewport((uint32_t)fw, (uint32_t)fh);
    }
}

void Close_Platform(void)
{
    glfwDestroyWindow(platform.window);
    glfwTerminate();
    TRACE_LOG(LOG_INFO, "GLFW: terminated");
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)window;
    (void)scancode;
    if (key < 0 || key >= MAX_KEYBOARD_KEYS)
        return; // macOS fn key generates -1

    if (action == GLFW_RELEASE)
        CORE.Input.Keyboard.currentKeyState[key] = 0;
    else if (action == GLFW_PRESS)
        CORE.Input.Keyboard.currentKeyState[key] = 1;
    else if (action == GLFW_REPEAT)
        CORE.Input.Keyboard.keyRepeatInFrame[key] = 1;

    if (((key == KEY_CAPS_LOCK) && ((mods & GLFW_MOD_CAPS_LOCK) > 0)) ||
        ((key == KEY_NUM_LOCK) && ((mods & GLFW_MOD_NUM_LOCK) > 0)))
        CORE.Input.Keyboard.currentKeyState[key] = 1;

    if ((CORE.Input.Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE) && (action == GLFW_PRESS)) {
        CORE.Input.Keyboard.keyPressedQueue[CORE.Input.Keyboard.keyPressedQueueCount] = key;
        CORE.Input.Keyboard.keyPressedQueueCount++;
    }

    if (glfwGetKey(platform.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(platform.window, GLFW_TRUE);
}

static void MouseCursorPosCallback(GLFWwindow* window, double x, double y)
{
    (void)window;
    CORE.Input.Mouse.currentPosition.x = (float)x;
    CORE.Input.Mouse.currentPosition.y = (float)y;
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void)window;
    (void)mods;
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return;
    CORE.Input.Mouse.currentButtonState[button] = (action == GLFW_PRESS) ? 1 : 0;
}

// window / framebuffer resized (only fires when the window is resizable). Fires
// from glfwPollEvents, which the engine calls at the end of Mge_EndDrawing -- so
// no batch is open and it is safe to reset the viewport here.
static void Framebuffer_Size_Callback(GLFWwindow* window, int w, int h)
{
    (void)window;
    if (w <= 0 || h <= 0) // minimised
        return;
    CORE.Window.screen.width = (uint32_t)w;
    CORE.Window.screen.height = (uint32_t)h;
    CORE.Window.render.width = (uint32_t)w;
    CORE.Window.render.height = (uint32_t)h;
    SetupViewport((uint32_t)w, (uint32_t)h);
}

void ShowCursor(void)
{
    glfwSetInputMode(platform.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    CORE.Input.Mouse.cursorHidden = false;
}

void HideCursor(void)
{
    glfwSetInputMode(platform.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    CORE.Input.Mouse.cursorHidden = true;
}

void EnableCursor(void)
{
    glfwSetInputMode(platform.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    SetMousePosition((int)CORE.Window.screen.width / 2, (int)CORE.Window.screen.height / 2);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(platform.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    CORE.Input.Mouse.cursorHidden = false;
}

void DisableCursor(void)
{
    glfwSetInputMode(platform.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    SetMousePosition((int)CORE.Window.screen.width / 2, (int)CORE.Window.screen.height / 2);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(platform.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    CORE.Input.Mouse.cursorHidden = true;
}

void SetMousePosition(int x, int y)
{
    CORE.Input.Mouse.currentPosition = (Vector2){ (float)x, (float)y };
    CORE.Input.Mouse.previousPosition = CORE.Input.Mouse.currentPosition;

    glfwSetCursorPos(platform.window, CORE.Input.Mouse.currentPosition.x, CORE.Input.Mouse.currentPosition.y);
}
