#include "mge.h"
#include "mge_math.h"
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

	Vector2 origin = { 100, 100 };

	Mge_SetTargetFPS(60);

	float radius = 20.0f;

	Vector2 points[] = {
		Vector2 {origin.x, origin.y}, Vector2 {radius+origin.x, (radius/2)+origin.y}, Vector2 {radius+origin.x, -(radius/2)+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {radius+origin.x, -(radius/2)+origin.y}, Vector2 {origin.x, -radius+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {origin.x, -radius+origin.y}, Vector2 {-radius+origin.x, -(radius/2)+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {-radius+origin.x, -(radius/2)+origin.y}, Vector2 {-radius+origin.x, (radius/2)+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {-radius+origin.x, (radius/2)+origin.y}, Vector2 {origin.x, radius+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {origin.x, radius+origin.y}, Vector2 {radius+origin.x, (radius/2)+origin.y},
	};

	while (!Mge_WindowShouldClose()) {
		Mge_BeginDrawing();
			Mge_ClearBackground(GRAY);
			Draw_TriangleLines(Vector2 {100, 100}, Vector2 {150, 200}, Vector2 {50, 200}, GREEN);
			Draw_TriangleLines(Vector2 {100, 100+80}, Vector2 {150, 200+80}, Vector2 {50, 200+80}, GREEN);
			Draw_TriangleLines(Vector2 {100, 100+160}, Vector2 {150, 200+160}, Vector2 {50, 200+160}, GREEN);
			Draw_TriangleStrip(points, 18, BLUE);
		Mge_EndDrawing();
	}

	Mge_CloseWindow();
	return 0;
}
