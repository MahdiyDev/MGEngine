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

	Rectangle rec = {
		.x = (float)WIDTH/2,
		.y = (float)HEIGHT/2,
		.width = 100,
		.height = 100,
	};
	Vector2 origin = { rec.width/2, rec.height/2 };

    while (!Mge_WindowShouldClose()) {
		float angle = -10.0f*Mge_GetTime();

        Mge_BeginDrawing();
        	Mge_ClearBackground(GRAY);
			Draw_Line(rec.x, rec.y, rec.x-0, rec.y-100, RED);
			Draw_Line(rec.x, rec.y, rec.x+100, rec.y-0, BLUE);
			Draw_RectanglePro(rec, origin, 1*angle, WHITE);
        Mge_EndDrawing();
    }

    Mge_CloseWindow();
    return 0;
}
