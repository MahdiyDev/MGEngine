#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <cstdlib>
#include <signal.h>

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

	float angle1 = -0.5f*Get_Time();
	float angle2 = angle1/4*Get_Time();
	Vector2 line_start1 = Vector2 { 400, 400 };
	Vector2 line_end1 = Vector2 { 450, 450 };
	Vector2 line_start2 = line_start1;
	Vector2 line_end2 = line_end1;

	while (!Window_Should_Close()) {
		Begin_Drawing();
			Clear_Background(GRAY);
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
		End_Drawing();
	}

	Close_Window();
	return 0;
}
