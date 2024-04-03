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

	Rectangle rec = {
		.x = (float)WIDTH/2,
		.y = (float)HEIGHT/2,
		.width = 100,
		.height = 100,
	};
	Vector2 origin = { 0, 0 };

	// float angle = -10.0f*Get_Time();
    while (!Window_Should_Close()) {

        Begin_Drawing();
        	Clear_Background(GRAY);
			Draw_Line(rec.x, rec.y, rec.x-0, rec.y-100, RED);
			Draw_Line(rec.x, rec.y, rec.x-100, rec.y-0, BLUE);
			Draw_RectanglePro(rec, origin, 0, WHITE);
			Draw_RectangleLines(rec.x, rec.y, 100, 100, BLUE);
        End_Drawing();
    }

    Close_Window();
    return 0;
}
