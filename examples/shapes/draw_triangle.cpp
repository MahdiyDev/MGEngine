#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <cstdlib>
#include <signal.h>

#define ENABLE_DUAL_GPU

void signal_handler(int sig)
{
	Close_Window();
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
	Init_Window(WIDTH, HEIGHT, "Hello this is test");

	signal(SIGSEGV, signal_handler);
	signal(SIGINT, signal_handler);

	Rectangle rec = {
		.x = (float)WIDTH/2,
		.y = (float)HEIGHT/2,
		.width = 100,
		.height = 100,
	};
	Vector2 origin = { 0, 0 };

	Set_Target_FPS(60);

	Vector2 points[] = {
		Vector2 {100+origin.x, 100+origin.y}, Vector2 {200+origin.x, 150+origin.y}, Vector2 {200+origin.x, 50+origin.y},
		Vector2 {100+origin.x, 100+origin.y}, Vector2 {200+origin.x, 50+origin.y}, Vector2 {100+origin.x, 0+origin.y},
		Vector2 {100+origin.x, 100+origin.y}, Vector2 {100+origin.x, 0+origin.y}, Vector2 {0+origin.x, 50+origin.y},
		Vector2 {100+origin.x, 100+origin.y}, Vector2 {0+origin.x, 50+origin.y}, Vector2 {0+origin.x, 150+origin.y},
		Vector2 {100+origin.x, 100+origin.y}, Vector2 {0+origin.x, 150+origin.y}, Vector2 {100+origin.x, 200+origin.y},
		Vector2 {100+origin.x, 100+origin.y}, Vector2 {100+origin.x, 200+origin.y}, Vector2 {200+origin.x, 150+origin.y},
	};

	while (!Window_Should_Close()) {
		Begin_Drawing();
			float angle = -100.0f*Get_Time();
			Clear_Background(GRAY);
			// Draw_TriangleLines(Vector2 {100, 100}, Vector2 {150, 200}, Vector2 {50, 200}, GREEN);
			// Draw_TriangleLines(Vector2 {100, 100+80}, Vector2 {150, 200+80}, Vector2 {50, 200+80}, GREEN);
			// Draw_TriangleLines(Vector2 {100, 100+160}, Vector2 {150, 200+160}, Vector2 {50, 200+160}, GREEN);
			// printf("fps: %d\n", Get_Fps());
			Draw_TriangleStrip(points, 18, BLUE);
		End_Drawing();
	}

	Close_Window();
	return 0;
}
