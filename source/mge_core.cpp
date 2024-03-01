#include "mge.h"
#include "mge_utils.h"
#include <cstdint>

extern int Init_Platform(void);
extern void Close_Platform(void);

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

    CORE.Window.shouldClose = false;
}

void Close_Window(void)
{
    Close_Platform();
    CORE.Window.ready = false;
    TRACE_LOG(LOG_INFO, "Window closed successfully");
}
