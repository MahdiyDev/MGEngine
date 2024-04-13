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
	Vector2 origin = { 100, 100 };

	Set_Target_FPS(60);

	float radius = 20.0f;

	Vector2 points[] = {
		Vector2 {origin.x, origin.y}, Vector2 {radius+origin.x, (radius/2)+origin.y}, Vector2 {radius+origin.x, -(radius/2)+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {radius+origin.x, -(radius/2)+origin.y}, Vector2 {origin.x, -radius+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {origin.x, -radius+origin.y}, Vector2 {-radius+origin.x, -(radius/2)+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {-radius+origin.x, -(radius/2)+origin.y}, Vector2 {-radius+origin.x, (radius/2)+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {-radius+origin.x, (radius/2)+origin.y}, Vector2 {origin.x, radius+origin.y},
		Vector2 {origin.x, origin.y}, Vector2 {origin.x, radius+origin.y}, Vector2 {radius+origin.x, (radius/2)+origin.y},
	};

	while (!Window_Should_Close()) {
		Begin_Drawing();
			float angle = -100.0f*Get_Time();
			Clear_Background(GRAY);
			// Draw_TriangleLines(Vector2 {100, 100}, Vector2 {150, 200}, Vector2 {50, 200}, GREEN);
			// Draw_TriangleLines(Vector2 {100+100, 100}, Vector2 {150+100, 200}, Vector2 {50+100, 200}, GREEN);
			// Draw_TriangleLines(Vector2 {100+50, 100+100}, Vector2 {150+50, 200+100}, Vector2 {50+50, 200+100}, GREEN);
			// // printf("fps: %d\n", Get_Fps());
			// Draw_TriangleStrip(points, 18, BLUE);
			Draw_Rectangle(300, 300, 100, 100, RED);
			Draw_RectangleLines(300, 300, 100, 100, RED);
		End_Drawing();
	}

	Close_Window();
	return 0;
}
