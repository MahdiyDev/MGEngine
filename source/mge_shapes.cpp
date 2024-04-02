#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"
#include <math.h>

void Draw_Line(int startPosX, int startPosY, int endPosX, int endPosY, Color color)
{
	MgeGL_Begin(MGEGL_LINES);
		MgeGL_Color4ub(color.r, color.g, color.b, color.a);
        MgeGL_Vertex2i(startPosX, startPosY);
        MgeGL_Vertex2i(endPosX, endPosY);
	MgeGL_End();
}

void Draw_LineV(Vector2 startPos, Vector2 endPos, Color color)
{
	MgeGL_Begin(MGEGL_LINES);
		MgeGL_Color4ub(color.r, color.g, color.b, color.a);
        MgeGL_Vertex2f(startPos.x, startPos.y);
        MgeGL_Vertex2f(endPos.x, endPos.y);
	MgeGL_End();
}

void Draw_Rectangle(int posX, int posY, int width, int height, Color color)
{
    Draw_RectangleV((Vector2){ (float)posX, (float)posY }, (Vector2){ (float)width, (float)height }, color);
}

void Draw_RectangleV(Vector2 position, Vector2 size, Color color)
{
    Draw_Rectangle_Pro((Rectangle){ position.x, position.y, size.x, size.y }, (Vector2){ 0.0f, 0.0f }, 0.0f, color);
}

void Draw_RectangleRec(Rectangle rec, Color color)
{
    Draw_Rectangle_Pro(rec, (Vector2){ 0.0f, 0.0f }, 0.0f, color);
}

void Draw_Rectangle_Pro(Rectangle rec, Vector2 origin, float rotation, Color color)
{
	Vector2 topLeft = { 0 };
	Vector2 topRight = { 0 };
	Vector2 bottomLeft = { 0 };
	Vector2 bottomRight = { 0 };

	if (rotation == 0.0f)
	{
		float x = rec.x - origin.x;
		float y = rec.y - origin.y;
		topLeft = (Vector2){ x, y };
		topRight = (Vector2){ x + rec.width, y };
		bottomLeft = (Vector2){ x, y + rec.height };
		bottomRight = (Vector2){ x + rec.width, y + rec.height };
	}
	else
    {
		float sinRotation = sinf(rotation*DEG2RAD);
		float cosRotation = cosf(rotation*DEG2RAD);
		float x = rec.x;
		float y = rec.y;
		float dx = -origin.x;
		float dy = -origin.y;

		topLeft.x = x + dx*cosRotation - dy*sinRotation;
		topLeft.y = y + dx*sinRotation + dy*cosRotation;

		topRight.x = x + (dx + rec.width)*cosRotation - dy*sinRotation;
		topRight.y = y + (dx + rec.width)*sinRotation + dy*cosRotation;

		bottomLeft.x = x + dx*cosRotation - (dy + rec.height)*sinRotation;
        bottomLeft.y = y + dx*sinRotation + (dy + rec.height)*cosRotation;

		bottomRight.x = x + (dx + rec.width)*cosRotation - (dy + rec.height)*sinRotation;
		bottomRight.y = y + (dx + rec.width)*sinRotation + (dy + rec.height)*cosRotation;
    }

	MgeGL_Begin(MGEGL_TRIANGLES);
		MgeGL_Color4ub(color.r, color.g, color.b, color.a);

		MgeGL_Vertex2f(topLeft.x, topLeft.y);
		MgeGL_Vertex2f(bottomLeft.x, bottomLeft.y);
		MgeGL_Vertex2f(topRight.x, topRight.y);

		MgeGL_Vertex2f(topRight.x, topRight.y);
		MgeGL_Vertex2f(bottomLeft.x, bottomLeft.y);
		MgeGL_Vertex2f(bottomRight.x, bottomRight.y);
	MgeGL_End();
}
