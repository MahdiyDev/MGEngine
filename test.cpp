#include "mge.h"
#include "mge_utils.h"
#include <cstdio>

int main(int argc, char** argv)
{
    Init_Window(800, 600, "Hello this is test");

    while (!Window_Should_Close()) {
        Begin_Drawing();
        Clear_Background(DARKGRAY);
        End_Drawing();
    }

    Close_Window();
    return 0;
}