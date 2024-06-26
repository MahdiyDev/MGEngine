#include "imgui.h"
#include "mge_math.h"
#include <cstddef>
#include <cstdlib>
#include <limits.h>
#include <math.h>
#define CORE_INCLUDE
#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"
#include <cmath>
#include <cstdint>

#if defined(_WIN32)
#include <synchapi.h>
// void __stdcall Sleep(unsigned long msTimeout);			  // Required for: WaitTime()
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

	CORE.Input.Mouse.scale = Vector2 { 1.0f, 1.0f };

	CORE.Window.shouldClose = false;
	CORE.Time.frameCounter = 0;
	CORE.Time.deltaTime = 0;
}

static void SetupViewport(uint32_t width, uint32_t height)
{
	MgeGL_Viewport(0, 0, width, height);

	MgeGL_MatrixMode(MGEGL_PROJECTION);		// Switch to projection matrix
	MgeGL_LoadIdentity();					  // Reset current matrix (projection)

	MgeGL_Ortho(0, width, height, 0, 0.0f, 1.0f);

	MgeGL_MatrixMode(MGEGL_MODELVIEW);		 // Switch back to modelview matrix
	MgeGL_LoadIdentity();					  // Reset current matrix (modelview)
}

void InitTimer(void)
{
	// Setting a higher resolution can improve the accuracy of time-out intervals in wait functions.
	// However, it can also reduce overall system performance, because the thread scheduler switches tasks more often.
	// High resolutions can also prevent the CPU power management system from entering power-saving modes.
	// Setting a higher resolution does not improve the accuracy of the high-resolution performance counter.
#if defined(_WIN32)
	timeBeginPeriod(1);				 // Setup high-resolution timer to 1ms (granularity of 1-2 ms)
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

	CORE.Time.previous = Mge_GetTime();	 // Get time as double
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

Shader Mge_LoadShader(const char* vsFileName, const char* fsFileName)
{
	Shader shader = {0};
	char* vertexShaderCode = NULL;
	char* fragmentShaderCode = NULL;

	if (vsFileName != NULL) vertexShaderCode = Mge_LoadFileText(vsFileName);
	if (fsFileName != NULL) fragmentShaderCode = Mge_LoadFileText(fsFileName);

	shader = Mge_LoadShaderFromMemory(vertexShaderCode, fragmentShaderCode);

	Mge_UnLoadFileText(vertexShaderCode);
	Mge_UnLoadFileText(fragmentShaderCode);

	return shader;
}

void Mge_UnloadShader(Shader shader)
{
	if (shader.id != MgeGL_GetDefaultShaderId())
	{
		MgeGL_UnloadShaderProgram(shader.id);

		free(shader.locs);
	}
}

Shader Mge_LoadShaderFromMemory(const char *vsCode, const char *fsCode)
{
	Shader shader = {0};

	unsigned int vertex = MgeGL_LoadShader(vsCode, GL_VERTEX_SHADER, "vertex");
	unsigned int fragment = MgeGL_LoadShader(fsCode, GL_FRAGMENT_SHADER, "fragment");
	shader.id = MgeGL_CreateShaderProgram(vertex, fragment);

	shader.locs = (int *)malloc(MGEGL_MAX_SHADER_LOCATIONS * sizeof(int));

	// All locations reset to -1 (no location)
	for (int i = 0; i < MGEGL_MAX_SHADER_LOCATIONS; i++) shader.locs[i] = -1;

	// shader.locs[SHADER_LOC_VERTEX_POSITION]

	return shader;
}

double Mge_GetTime(void)
{
	return Platform_GetTime();
}

double Mge_GetDeltaTime(void)
{
	return CORE.Time.deltaTime;
}

void Mge_ClearBackground(Color color)
{
	MgeGL_ClearColor(color);
	MgeGL_ClearScreenBuffers();
}

void Mge_BeginDrawing(void)
{
	// Fps calculation
	CORE.Time.current = Mge_GetTime();
	CORE.Time.update = CORE.Time.current - CORE.Time.previous;
	CORE.Time.previous = CORE.Time.current;

	// Delta time calculation
	CORE.Time.deltaTime = CORE.Time.current - CORE.Time.lastFrame;
	CORE.Time.lastFrame = CORE.Time.current;
}

void Mge_EndDrawing(void)
{
	// ImGui::Render();
	// ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	MgeGL_Draw();

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

		CORE.Time.frame += waitTime;	// Total frame time: update + draw + wait
	}
	CORE.Time.frameCounter++;
	Poll_Input_Events();
}

Vector3 transform = Vector3 {0, 0, -6.0f};

void Mge_BeginMode3D(Camera3D& camera)
{
	MgeGL_Draw();
		
	MgeGL_MatrixMode(MGEGL_PROJECTION);
	MgeGL_PushMatrix();
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

	// ImGui_ImplOpenGL3_NewFrame();
	// ImGui_ImplGlfw_NewFrame();
	// ImGui::NewFrame();
	// ImGui::Begin("MgeGL_Translatef");
	// 	ImGui::Text("camera.position");
	// 	ImGui::DragFloat("camera.position.x", &camera.position.x, 0.01f);
	// 	ImGui::DragFloat("camera.position.y", &camera.position.y, 0.01f);
	// 	ImGui::DragFloat("camera.position.z", &camera.position.z, 0.01f);

	// 	ImGui::Text("camera.target");
	// 	ImGui::DragFloat("camera.target.x", &camera.target.x, 0.01f);
	// 	ImGui::DragFloat("camera.target.y", &camera.target.y, 0.01f);
	// 	ImGui::DragFloat("camera.target.z", &camera.target.z, 0.01f);
	// ImGui::End();
	// ImGui::EndFrame();

	MgeGL_MatrixMode(MGEGL_MODELVIEW);
	MgeGL_LoadIdentity();

	Matrix matView = MatrixLookAt(camera.position, camera.position + camera.target, camera.up);
	MgeGL_MultMatrixf(MatrixToFloat(matView));	  // Multiply modelview matrix by view matrix (camera)

	MgeGL_EnableDepthTest();
}

void Mge_EndMode3D(void)
{
	MgeGL_Draw();

	MgeGL_MatrixMode(MGEGL_PROJECTION);	// Switch to projection matrix
	MgeGL_PopMatrix();					 // Restore previous matrix (projection) from matrix stack

	MgeGL_MatrixMode(MGEGL_MODELVIEW);	 // Switch back to modelview matrix
	MgeGL_LoadIdentity();				  // Reset current matrix (modelview)

	MgeGL_DisableDepthTest();
}

void Mge_BeginShaderMode(Shader shader)
{
	MgeGL_SetShader(shader.id);
}

void Mge_EndShaderMode()
{
	MgeGL_SetShader(MgeGL_GetDefaultShaderId());
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

#define FPS_CAPTURE_FRAMES_COUNT	30	  // 30 captures
#define FPS_AVERAGE_TIME_SECONDS   0.5f	 // 500 milliseconds
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

bool IsKeyPressed(int key)
{
	bool pressed = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if (
			(CORE.Input.Keyboard.previousKeyState[key] == 0) && 
			(CORE.Input.Keyboard.currentKeyState[key] == 1)
		) {
			pressed = true;
		};
	}

	return pressed;
}

// Check if a key has been pressed again
bool IsKeyPressedRepeat(int key)
{
	bool repeat = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if (CORE.Input.Keyboard.keyRepeatInFrame[key] == 1) repeat = true;
	}

	return repeat;
}

// Check if a key is being pressed (key held down)
bool IsKeyDown(int key)
{
	bool down = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if (CORE.Input.Keyboard.currentKeyState[key] == 1) down = true;
	}

	return down;
}

// Check if a key has been released once
bool IsKeyReleased(int key)
{
	bool released = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if ((CORE.Input.Keyboard.previousKeyState[key] == 1) && (CORE.Input.Keyboard.currentKeyState[key] == 0)) released = true;
	}

	return released;
}

// Check if a key is NOT being pressed (key not held down)
bool IsKeyUp(int key)
{
	bool up = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if (CORE.Input.Keyboard.currentKeyState[key] == 0) up = true;
	}

	return up;
}

// Get mouse position X
float GetMouseX(void)
{
	return (CORE.Input.Mouse.currentPosition.x + CORE.Input.Mouse.offset.x)*CORE.Input.Mouse.scale.x;
}

// Get mouse position Y
float GetMouseY(void)
{
	return (CORE.Input.Mouse.currentPosition.y + CORE.Input.Mouse.offset.y)*CORE.Input.Mouse.scale.y;
}

// Get mouse position XY
Vector2 GetMousePosition(void)
{
	Vector2 position = { 0 };

	position.x = (CORE.Input.Mouse.currentPosition.x + CORE.Input.Mouse.offset.x)*CORE.Input.Mouse.scale.x;
	position.y = (CORE.Input.Mouse.currentPosition.y + CORE.Input.Mouse.offset.y)*CORE.Input.Mouse.scale.y;

	return position;
}
