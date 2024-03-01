#include "mge.h"
#include <cstdio>

int main(int argc, char** argv)
{
    Init_Window(800, 600, "Hello this is test");

    while (!Window_Should_Close()) {
        Poll_Input_Events();
    }

    Close_Window();
    return 0;
}
