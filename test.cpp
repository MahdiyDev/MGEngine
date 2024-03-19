#include "mge.h"

int main(int argc, char** argv)
{
    Init_Window(800, 600, "Hello this is test");

    while (!Window_Should_Close()) {
        Begin_Drawing();
        	Clear_Background(CLITERAL(Color) { 255, 32, 82, 255 });
        End_Drawing();
    }

    Close_Window();
    return 0;
}
