#include "mge.h"
#include "mge_math.h"

int main(int argc, char** argv)
{
    Init_Window(800, 600, "Hello this is test");

    while (!Window_Should_Close()) {
        Begin_Drawing();
        	Clear_Background(GRAY);
			Draw_Line(Vector3{0,0,0}, Vector3{10,10,0}, RED);
        End_Drawing();
    }

    Close_Window();
    return 0;
}
