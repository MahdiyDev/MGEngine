#include <stdio.h>
#include <raylib.h>

int main(void)
{
	InitWindow(800, 600, "const char *title");
	while (!WindowShouldClose()) {
		BeginDrawing();
			printf("fps: %d\n", GetFPS());
			DrawRectangle(10, 10, 300, 300, RED);
		EndDrawing();
	}
	CloseWindow();
}
