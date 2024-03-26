#include "mge.h"
#include "mge_math.h"

int main(int argc, char** argv)
{
    int WIDTH = 800, HEIGHT = 600;
    Init_Window(WIDTH, HEIGHT, "Hello this is test");

    float angle1 = -0.005f*Get_Time();
    float angle2 = angle1/4*Get_Time();
	Vector2 line_start1 = Vector2 { 0, 0 };
	Vector2 line_end1 = Vector2 { 0.5f, 0.5f };
	Vector2 line_start2 = line_start1;
	Vector2 line_end2 = line_end1;

    while (!Window_Should_Close()) {
        Begin_Drawing();
        Clear_Background(GRAY);
			Vector2 result1 = Vector2_Rotate(Vector2 { line_end1.x, line_end1.y }, angle1);
			line_end1 = Vector2{result1.x, result1.y};
			Vector2 result2 = Vector2_Rotate(Vector2 { line_end2.x, line_end2.y }, angle2);
			line_end2 = Vector2{result2.x, result2.y};
			Draw_Line(line_start1, line_end1, RED);
			Draw_Line(line_start2, line_end2, BLUE);
        End_Drawing();
    }

    Close_Window();
    return 0;
}
