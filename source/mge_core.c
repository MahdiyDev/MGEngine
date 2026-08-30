#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CORE_INCLUDE
#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#include <glad/glad.h>

#if defined(_WIN32)
    #include <synchapi.h>
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
    #include <time.h>
#endif

extern void InitPlatform(void);
extern void Close_Platform(void);
extern double Platform_GetTime(void);
extern void Poll_Input_Events(void);
extern void Swap_Screen_Buffer(void);

static void SetupViewport(uint32_t width, uint32_t height);
static void InitTimer(void);
static void WaitTime(double seconds);
static float Get_Frame_Time(void);

CoreData CORE = { 0 };

#if defined(PLATFORM_DESKTOP)
    #include "platforms/mge_code_desktop.c"
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

    MgeGL_Init((int)CORE.Window.screen.width, (int)CORE.Window.screen.height);

    SetupViewport(CORE.Window.screen.width, CORE.Window.screen.height);

    CORE.Input.Mouse.scale = (Vector2){ 1.0f, 1.0f };

    CORE.Window.shouldClose = false;
    CORE.Time.frameCounter = 0;
    CORE.Time.deltaTime = 0;
}

static void SetupViewport(uint32_t width, uint32_t height)
{
    MgeGL_Viewport(0, 0, (int)width, (int)height);

    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_LoadIdentity();

    MgeGL_Ortho(0, width, height, 0, 0.0, 1.0);

    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();
}

static void InitTimer(void)
{
#if defined(_WIN32)
    timeBeginPeriod(1); // 1ms timer granularity
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
    struct timespec now = { 0 };
    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
        CORE.Time.base = (unsigned long long int)now.tv_sec * 1000000000LLU + (unsigned long long int)now.tv_nsec;
    } else {
        TRACE_LOG(LOG_WARNING, "TIMER: Hi-resolution timer not available");
    }
#endif

    CORE.Time.previous = Mge_GetTime();
}

static void WaitTime(double seconds)
{
    if (seconds < 0)
        return;

    double destinationTime = Mge_GetTime() + seconds;
    double sleepSeconds = seconds - seconds * 0.05; // reserve 5% for busy-waiting

#if defined(_WIN32)
    Sleep((unsigned long)(sleepSeconds * 1000.0));
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
    struct timespec req = { 0 };
    time_t sec = (time_t)sleepSeconds;
    long nsec = (long)((sleepSeconds - (double)sec) * 1000000000.0);
    req.tv_sec = sec;
    req.tv_nsec = nsec;
    while (nanosleep(&req, &req) == -1)
        continue;
#elif defined(__APPLE__)
    usleep((useconds_t)(sleepSeconds * 1000000.0));
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
    char* vertexShaderCode = NULL;
    char* fragmentShaderCode = NULL;

    if (vsFileName != NULL)
        vertexShaderCode = Mge_LoadFileText(vsFileName);
    if (fsFileName != NULL)
        fragmentShaderCode = Mge_LoadFileText(fsFileName);

    Shader shader = Mge_LoadShaderFromMemory(vertexShaderCode, fragmentShaderCode);

    Mge_UnLoadFileText(vertexShaderCode);
    Mge_UnLoadFileText(fragmentShaderCode);

    return shader;
}

void Mge_UnloadShader(Shader shader)
{
    if (shader.id != MgeGL_GetDefaultShaderId()) {
        MgeGL_UnloadShaderProgram(shader.id);
        free(shader.locs);
    }
}

Shader Mge_LoadShaderFromMemory(const char* vsCode, const char* fsCode)
{
    Shader shader = { 0 };

    unsigned int vertex = MgeGL_LoadShader(vsCode, GL_VERTEX_SHADER, "vertex");
    unsigned int fragment = MgeGL_LoadShader(fsCode, GL_FRAGMENT_SHADER, "fragment");
    shader.id = MgeGL_CreateShaderProgram(vertex, fragment);

    shader.locs = (int*)malloc(MGEGL_MAX_SHADER_LOCATIONS * sizeof(int));
    if (shader.locs != NULL) {
        for (int i = 0; i < MGEGL_MAX_SHADER_LOCATIONS; i++)
            shader.locs[i] = -1;
    }

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
    MgeGL_ResetDrawCalls();

    CORE.Time.current = Mge_GetTime();
    CORE.Time.update = CORE.Time.current - CORE.Time.previous;
    CORE.Time.previous = CORE.Time.current;

    CORE.Time.deltaTime = CORE.Time.current - CORE.Time.lastFrame;
    CORE.Time.lastFrame = CORE.Time.current;
}

void Mge_EndDrawing(void)
{
    MgeGL_Draw();

    Swap_Screen_Buffer();

    CORE.Time.current = Mge_GetTime();
    CORE.Time.draw = CORE.Time.current - CORE.Time.previous;
    CORE.Time.previous = CORE.Time.current;

    CORE.Time.frame = CORE.Time.update + CORE.Time.draw;

    if (CORE.Time.frame < CORE.Time.target) {
        WaitTime(CORE.Time.target - CORE.Time.frame);

        CORE.Time.current = Mge_GetTime();
        double waitTime = CORE.Time.current - CORE.Time.previous;
        CORE.Time.previous = CORE.Time.current;

        CORE.Time.frame += waitTime;
    }
    CORE.Time.frameCounter++;
    Poll_Input_Events();
}

void Mge_BeginMode3D(Camera3D camera)
{
    MgeGL_Draw();

    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_PushMatrix();
    MgeGL_LoadIdentity();

    float aspect = (float)CORE.Window.screen.width / (float)CORE.Window.screen.height;

    double clipNear = Mge_GetClipNear();
    double clipFar = Mge_GetClipFar();

    if (camera.projection == CAMERA_PERSPECTIVE) {
        double top = clipNear * tan(camera.fovy * 0.5 * DEG2RAD);
        double right = top * aspect;
        MgeGL_Frustum(-right, right, -top, top, clipNear, clipFar);
    } else if (camera.projection == CAMERA_ORTHOGRAPHIC) {
        double top = camera.fovy / 2.0;
        double right = top * aspect;
        MgeGL_Ortho(-right, right, -top, top, clipNear, clipFar);
    }

    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();

    Matrix matView = MatrixLookAt(camera.position, Vector3_Add(camera.position, camera.target), camera.up);
    MgeGL_MultMatrixf(MatrixToFloat(matView));

    MgeGL_EnableDepthTest();
}

void Mge_EndMode3D(void)
{
    MgeGL_Draw();

    MgeGL_MatrixMode(MGEGL_PROJECTION);
    MgeGL_PopMatrix();

    MgeGL_MatrixMode(MGEGL_MODELVIEW);
    MgeGL_LoadIdentity();

    MgeGL_DisableDepthTest();
}

void Mge_BeginShaderMode(Shader shader)
{
    MgeGL_SetShader(shader.id);
}

void Mge_EndShaderMode(void)
{
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

static float Get_Frame_Time(void)
{
    return (float)CORE.Time.frame;
}

void Mge_SetTargetFPS(int fps)
{
    if (fps < 1) {
        CORE.Time.target = 0.0;
    } else {
        CORE.Time.target = 1.0 / (double)fps;
    }
    TRACE_LOG(LOG_INFO, "TIMER: Target time per frame: %02.03f milliseconds", (float)CORE.Time.target * 1000.0f);
}

int Mge_GetDrawCalls(void)
{
    return MgeGL_GetDrawCalls(); // GL draws in the previous frame (lower = better batching)
}

int Mge_GetFps(void)
{
#define FPS_CAPTURE_FRAMES_COUNT 30
#define FPS_AVERAGE_TIME_SECONDS 0.5f
#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS / FPS_CAPTURE_FRAMES_COUNT)

    static int index = 0;
    static float history[FPS_CAPTURE_FRAMES_COUNT] = { 0 };
    static float average = 0, last = 0;
    float fpsFrame = Get_Frame_Time();

    if (CORE.Time.frameCounter == 0) {
        average = 0;
        last = 0;
        index = 0;
        for (int i = 0; i < FPS_CAPTURE_FRAMES_COUNT; i++)
            history[i] = 0;
    }

    if (fpsFrame == 0)
        return 0;

    if ((Mge_GetTime() - last) > FPS_STEP) {
        last = (float)Platform_GetTime();
        index = (index + 1) % FPS_CAPTURE_FRAMES_COUNT;
        average -= history[index];
        history[index] = fpsFrame / FPS_CAPTURE_FRAMES_COUNT;
        average += history[index];
    }

    if (average <= 0.0f)
        return 0;

    return (int)roundf(1.0f / average);
}

bool IsKeyPressed(int key)
{
    if ((key > 0) && (key < MAX_KEYBOARD_KEYS)) {
        return (CORE.Input.Keyboard.previousKeyState[key] == 0) && (CORE.Input.Keyboard.currentKeyState[key] == 1);
    }
    return false;
}

bool IsKeyPressedRepeat(int key)
{
    if ((key > 0) && (key < MAX_KEYBOARD_KEYS)) {
        return CORE.Input.Keyboard.keyRepeatInFrame[key] == 1;
    }
    return false;
}

bool IsKeyDown(int key)
{
    if ((key > 0) && (key < MAX_KEYBOARD_KEYS)) {
        return CORE.Input.Keyboard.currentKeyState[key] == 1;
    }
    return false;
}

bool IsKeyReleased(int key)
{
    if ((key > 0) && (key < MAX_KEYBOARD_KEYS)) {
        return (CORE.Input.Keyboard.previousKeyState[key] == 1) && (CORE.Input.Keyboard.currentKeyState[key] == 0);
    }
    return false;
}

bool IsKeyUp(int key)
{
    if ((key > 0) && (key < MAX_KEYBOARD_KEYS)) {
        return CORE.Input.Keyboard.currentKeyState[key] == 0;
    }
    return false;
}

float GetMouseX(void)
{
    return (CORE.Input.Mouse.currentPosition.x + CORE.Input.Mouse.offset.x) * CORE.Input.Mouse.scale.x;
}

float GetMouseY(void)
{
    return (CORE.Input.Mouse.currentPosition.y + CORE.Input.Mouse.offset.y) * CORE.Input.Mouse.scale.y;
}

Vector2 GetMousePosition(void)
{
    Vector2 position = { 0 };
    position.x = (CORE.Input.Mouse.currentPosition.x + CORE.Input.Mouse.offset.x) * CORE.Input.Mouse.scale.x;
    position.y = (CORE.Input.Mouse.currentPosition.y + CORE.Input.Mouse.offset.y) * CORE.Input.Mouse.scale.y;
    return position;
}

Vector2 GetMouseDelta(void)
{
    Vector2 d;
    d.x = CORE.Input.Mouse.currentPosition.x - CORE.Input.Mouse.previousPosition.x;
    d.y = CORE.Input.Mouse.currentPosition.y - CORE.Input.Mouse.previousPosition.y;
    return d;
}

bool IsMouseButtonPressed(int button)
{
    if ((button >= 0) && (button < MAX_MOUSE_BUTTONS)) {
        return (CORE.Input.Mouse.previousButtonState[button] == 0) && (CORE.Input.Mouse.currentButtonState[button] == 1);
    }
    return false;
}

bool IsMouseButtonDown(int button)
{
    if ((button >= 0) && (button < MAX_MOUSE_BUTTONS)) {
        return CORE.Input.Mouse.currentButtonState[button] == 1;
    }
    return false;
}

bool IsMouseButtonReleased(int button)
{
    if ((button >= 0) && (button < MAX_MOUSE_BUTTONS)) {
        return (CORE.Input.Mouse.previousButtonState[button] == 1) && (CORE.Input.Mouse.currentButtonState[button] == 0);
    }
    return false;
}

int Mge_GetScreenWidth(void)
{
    return (int)CORE.Window.screen.width;
}

int Mge_GetScreenHeight(void)
{
    return (int)CORE.Window.screen.height;
}

bool IsCursorHidden(void)
{
    return CORE.Input.Mouse.cursorHidden;
}

void Mge_ToggleCursor(void)
{
    if (CORE.Input.Mouse.cursorHidden)
        EnableCursor();
    else
        DisableCursor();
}
