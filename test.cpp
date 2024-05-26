#include "mge.h"
#include "mge_utils.h"

#include <cstdlib>
#include <signal.h>

#define ENABLE_DUAL_GPU

void signal_handler(int sig)
{
	Mge_CloseWindow();
	if (sig == SIGSEGV) {
		TRACE_LOG(LOG_ERROR, "Invalid access to storage.");
	} else if (sig == SIGINT) {
		TRACE_LOG(LOG_INFO, "<Ctrl-C> recived. exiting...");
	}
	exit(sig);
}

int main(int argc, char** argv)
{
	int WIDTH = 800, HEIGHT = 600;
	Mge_InitWindow(WIDTH, HEIGHT, "Hello this is test");

	signal(SIGSEGV, signal_handler);
	signal(SIGINT, signal_handler);

	Mge_SetTargetFPS(60);
	float i = 0;
	while (!Mge_WindowShouldClose()) {
		Mge_BeginDrawing();
		// Mge_BeginMode3D(Camera3D {});
			Mge_ClearBackground(GRAY);
			Draw_RectangleRec(Rectangle {150,150,100,100}, RED);
			Draw_RectangleRec(Rectangle {200,200,100,100}, GREEN);
			Draw_TriangleLines(Vector2 {100, 100}, Vector2 {150, 200}, Vector2 {50, 200}, GREEN);
		// Mge_EndMode3D();
		Mge_EndDrawing();
	}

	Mge_CloseWindow();
	return 0;
}
