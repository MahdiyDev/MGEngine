#include <cstdio>
#define CORE_INCLUDE
#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad//glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void MouseCursorPosCallback(GLFWwindow *window, double x, double y);

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
		NULL, NULL);

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

	glfwSetKeyCallback(platform.window, KeyCallback);
	glfwSetCursorPosCallback(platform.window, MouseCursorPosCallback);
	glfwSetInputMode(platform.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// ImGuiIO io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(platform.window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

double Platform_GetTime(void)
{
	return glfwGetTime();
}

void Poll_Input_Events(void)
{
	// Reset keys/chars pressed registered
	CORE.Input.Keyboard.keyPressedQueueCount = 0;
	CORE.Input.Keyboard.charPressedQueueCount = 0;

	// Register previous keys states
	for (int i = 0; i < MAX_KEYBOARD_KEYS; i++)
	{
		CORE.Input.Keyboard.previousKeyState[i] = CORE.Input.Keyboard.currentKeyState[i];
		CORE.Input.Keyboard.keyRepeatInFrame[i] = 0;
	}

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
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(platform.window);
	glfwTerminate();
	TRACE_LOG(LOG_INFO, "GLFW: terminated");
}

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key < 0) return;	// Security check, macOS fn key generates -1

	// WARNING: GLFW could return GLFW_REPEAT, we need to consider it as 1
	// to work properly with our implementation (IsKeyDown/IsKeyUp checks)
	if (action == GLFW_RELEASE) CORE.Input.Keyboard.currentKeyState[key] = 0;
	else if(action == GLFW_PRESS) CORE.Input.Keyboard.currentKeyState[key] = 1;
	else if(action == GLFW_REPEAT) CORE.Input.Keyboard.keyRepeatInFrame[key] = 1;

	// WARNING: Check if CAPS/NUM key modifiers are enabled and force down state for those keys
	if (((key == KEY_CAPS_LOCK) && ((mods & GLFW_MOD_CAPS_LOCK) > 0)) ||
		((key == KEY_NUM_LOCK) && ((mods & GLFW_MOD_NUM_LOCK) > 0))) CORE.Input.Keyboard.currentKeyState[key] = 1;

	// Check if there is space available in the key queue
	if ((CORE.Input.Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE) && (action == GLFW_PRESS))
	{
		// Add character to the queue
		CORE.Input.Keyboard.keyPressedQueue[CORE.Input.Keyboard.keyPressedQueueCount] = key;
		CORE.Input.Keyboard.keyPressedQueueCount++;
	}

	if(glfwGetKey(platform.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(platform.window, true);
}

static void MouseCursorPosCallback(GLFWwindow *window, double x, double y)
{
	CORE.Input.Mouse.currentPosition.x = (float)x;
	CORE.Input.Mouse.currentPosition.y = (float)y;
}
