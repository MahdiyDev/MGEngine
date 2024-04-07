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

extern int Init_Platform(void);
extern void Close_Platform(void);
extern float Platform_Get_Time(void);

static void Setup_Viewport(uint32_t width, uint32_t height);
static void Init_Timer(void);

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

	struct {
		double frame;
		unsigned int frameCounter;
		unsigned long long int base;
		double current;
		double draw;
		double previous;
		double target;
		double update;
	} Time;
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

	MgeGL_Init();

    Setup_Viewport(CORE.Window.display.width, CORE.Window.display.height);

    CORE.Window.shouldClose = false;
	CORE.Time.frameCounter = 0;
}

void Init_Timer(void)
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

    CORE.Time.previous = Get_Time();     // Get time as double
}

void Wait_Time(double seconds)
{
	if (seconds < 0) return;

	double destinationTime = Get_Time() + seconds;

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

	while (Get_Time() < destinationTime) { }
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
	CORE.Time.current = Get_Time();
	CORE.Time.update = CORE.Time.current - CORE.Time.previous;
	CORE.Time.previous = CORE.Time.current;
}

void End_Drawing(void)
{
	Swap_Screen_Buffer();

	CORE.Time.current = Get_Time();
	CORE.Time.draw = CORE.Time.current - CORE.Time.previous;
	CORE.Time.previous = CORE.Time.current;

	CORE.Time.frame = CORE.Time.update + CORE.Time.draw;

	// Wait for some milliseconds...
	if (CORE.Time.frame < CORE.Time.target)
	{
		Wait_Time(CORE.Time.target - CORE.Time.frame);

		CORE.Time.current = Get_Time();
		double waitTime = CORE.Time.current - CORE.Time.previous;
		CORE.Time.previous = CORE.Time.current;

		CORE.Time.frame += waitTime;    // Total frame time: update + draw + wait
	}
	CORE.Time.frameCounter++;
	Poll_Input_Events();
}

void Setup_Viewport(uint32_t width, uint32_t height)
{
    MgeGL_Viewport(0, 0, width, height);
	MgeGL_Ortho(0, width, height, 0, 0.0f, 1.0f);
}

float Get_Frame_Time(void)
{
    return (float)CORE.Time.frame;
}

void Set_Target_FPS(int fps)
{
	if (fps < 1) { CORE.Time.target = 0.0; }
	else { CORE.Time.target = 1.0/(double)fps; }
	TRACE_LOG(LOG_INFO, "TIMER: Target time per frame: %02.03f milliseconds", (float)CORE.Time.target*1000.0f);
}

int Get_Fps(void)
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

	if ((Get_Time() - last) > FPS_STEP)
	{
		last = (float)Platform_Get_Time();
		index = (index + 1)%FPS_CAPTURE_FRAMES_COUNT;
		average -= history[index];
		history[index] = fpsFrame/FPS_CAPTURE_FRAMES_COUNT;
		average += history[index];
	}

	fps = (int)std::roundf(1.0f/average);

	return fps;
}
