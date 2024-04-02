#include "mge.h"
#define MGEGL_IMLEMENTATION
#include "mge_gl.h"
#include "mge_utils.h"
#include <cstdint>

extern int Init_Platform(void);
extern void Close_Platform(void);
extern float Platform_Get_Time(void);

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
        bool ready;
        bool shouldClose;
    } Window;
} CoreData;

CoreData CORE = { 0 };

#if defined(PLATFORM_DESKTOP)
#include "platforms/mge_code_destop.cpp"
#endif

static void Setup_Viewport(uint32_t width, uint32_t height);

void Init_Window(uint32_t width, uint32_t height, const char* title)
{
    TRACE_LOG(LOG_INFO, "Initializing MGE %s", MGE_VERSION);
#if defined(PLATFORM_DESKTOP)
    TRACE_LOG(LOG_INFO, "Platform backend: DESKTOP (GLFW)");
#endif

    CORE.Window.display.width = width;
    CORE.Window.display.height = height;
    CORE.Window.title = title;

    Init_Platform();

	MgeGL_Init();

    Setup_Viewport(CORE.Window.display.width, CORE.Window.display.height);

    CORE.Window.shouldClose = false;
}

void Close_Window(void)
{
	MgeGL_Close();
    Close_Platform();
    CORE.Window.ready = false;
    TRACE_LOG(LOG_INFO, "Window closed successfully");
}

float Get_Time(void)
{
	return Platform_Get_Time();
}

void Clear_Background(Color color)
{
    MgeGL_Clear_Color(color.r, color.g, color.b, color.a);
    MgeGL_Clear_Screen_Buffers();
}

void Begin_Drawing(void)
{
}

void End_Drawing(void)
{
    Swap_Screen_Buffer();
    Poll_Input_Events();
}

void Setup_Viewport(uint32_t width, uint32_t height)
{
    MgeGL_Viewport(0, 0, width, height);
	MgeGL_Ortho(0, width, height, 0, 0.0f, 1.0f);
}
