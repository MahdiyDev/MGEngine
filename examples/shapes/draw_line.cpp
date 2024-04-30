#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <cstdlib>
#include <signal.h>

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

	Mge_SetTargetFPS(60);

	signal(SIGSEGV, signal_handler);
	signal(SIGINT, signal_handler);

	float angle1 = -0.5f*Mge_GetTime();
	float angle2 = angle1/4*Mge_GetTime();
	Vector2 line_start1 = Vector2 { 400, 400 };
	Vector2 line_end1 = Vector2 { 450, 450 };
	Vector2 line_start2 = line_start1;
	Vector2 line_end2 = line_end1;

	while (!Mge_WindowShouldClose()) {
		Mge_BeginDrawing();
			Mge_ClearBackground(GRAY);
			Vector2 result1 = Vector2_Rotate(Vector2 { line_end1.x, line_end1.y }, angle1);
			line_end1 = Vector2{result1.x, result1.y};
			Vector2 result2 = Vector2_Rotate(Vector2 { line_end2.x, line_end2.y }, angle2);
			line_end2 = Vector2{result2.x, result2.y};

			// Cloc lines
			Draw_LineV(line_start1, line_end1, RED);
			Draw_LineV(line_start2, line_end2, BLUE);

			// Rectangle with lines
			Draw_Line(100, 100, 200, 100, BLUE);
			Draw_Line(200, 100, 200, 200, BLUE);
			Draw_Line(200, 200, 100, 200, BLUE);
			Draw_Line(100, 200, 100, 100, BLUE);
		Mge_EndDrawing();
	}

	Mge_CloseWindow();
	return 0;
}
