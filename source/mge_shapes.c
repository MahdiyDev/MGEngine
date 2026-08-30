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
    Draw_RectanglePro((Rectangle){ position.x, position.y, size.x, size.y }, (Vector2){ 0.0f, 0.0f }, 0.0f, color);
}

void Draw_RectangleRec(Rectangle rec, Color color)
{
    Draw_RectanglePro(rec, (Vector2){ 0.0f, 0.0f }, 0.0f, color);
}

void Draw_RectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color)
{
    Vector2 topLeft = { 0 };
    Vector2 topRight = { 0 };
    Vector2 bottomLeft = { 0 };
    Vector2 bottomRight = { 0 };

    if (rotation == 0.0f) {
        float x = rec.x - origin.x;
        float y = rec.y - origin.y;
        topLeft = (Vector2){ x, y };
        topRight = (Vector2){ x + rec.width, y };
        bottomLeft = (Vector2){ x, y + rec.height };
        bottomRight = (Vector2){ x + rec.width, y + rec.height };
    } else {
        float sinRotation = sinf(rotation * DEG2RAD);
        float cosRotation = cosf(rotation * DEG2RAD);
        float x = rec.x;
        float y = rec.y;
        float dx = -origin.x;
        float dy = -origin.y;

        topLeft.x = x + dx * cosRotation - dy * sinRotation;
        topLeft.y = y + dx * sinRotation + dy * cosRotation;

        topRight.x = x + (dx + rec.width) * cosRotation - dy * sinRotation;
        topRight.y = y + (dx + rec.width) * sinRotation + dy * cosRotation;

        bottomLeft.x = x + dx * cosRotation - (dy + rec.height) * sinRotation;
        bottomLeft.y = y + dx * sinRotation + (dy + rec.height) * cosRotation;

        bottomRight.x = x + (dx + rec.width) * cosRotation - (dy + rec.height) * sinRotation;
        bottomRight.y = y + (dx + rec.width) * sinRotation + (dy + rec.height) * cosRotation;
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

void Draw_RectangleLines(int posX, int posY, int width, int height, Color color)
{
    float x = (float)posX;
    float y = (float)posY;
    float w = (float)width;
    float h = (float)height;

    MgeGL_Begin(MGEGL_LINES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);

    MgeGL_Vertex2f(x, y);
    MgeGL_Vertex2f(x + w, y);

    MgeGL_Vertex2f(x + w, y);
    MgeGL_Vertex2f(x + w, y + h);

    MgeGL_Vertex2f(x + w, y + h);
    MgeGL_Vertex2f(x, y + h);

    MgeGL_Vertex2f(x, y + h);
    MgeGL_Vertex2f(x, y);
    MgeGL_End();
}

void Draw_Triangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color)
{
    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    MgeGL_Vertex2f(v1.x, v1.y);
    MgeGL_Vertex2f(v2.x, v2.y);
    MgeGL_Vertex2f(v3.x, v3.y);
    MgeGL_End();
}

void Draw_TriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color color)
{
    MgeGL_Begin(MGEGL_LINES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    MgeGL_Vertex2f(v1.x, v1.y);
    MgeGL_Vertex2f(v2.x, v2.y);

    MgeGL_Vertex2f(v2.x, v2.y);
    MgeGL_Vertex2f(v3.x, v3.y);

    MgeGL_Vertex2f(v3.x, v3.y);
    MgeGL_Vertex2f(v1.x, v1.y);
    MgeGL_End();
}

void Draw_TriangleFan(Vector2* points, int pointCount, Color color)
{
    if (pointCount < 3)
        return;

    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    for (int i = 1; i < pointCount - 1; i++) {
        MgeGL_Vertex2f(points[0].x, points[0].y);
        MgeGL_Vertex2f(points[i].x, points[i].y);
        MgeGL_Vertex2f(points[i + 1].x, points[i + 1].y);
    }
    MgeGL_End();
}

void Draw_TriangleStrip(Vector2* points, int pointCount, Color color)
{
    if (pointCount < 3)
        return;

    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    for (int i = 2; i < pointCount; i++) {
        if ((i % 2) == 0) {
            MgeGL_Vertex2f(points[i].x, points[i].y);
            MgeGL_Vertex2f(points[i - 2].x, points[i - 2].y);
            MgeGL_Vertex2f(points[i - 1].x, points[i - 1].y);
        } else {
            MgeGL_Vertex2f(points[i].x, points[i].y);
            MgeGL_Vertex2f(points[i - 1].x, points[i - 1].y);
            MgeGL_Vertex2f(points[i - 2].x, points[i - 2].y);
        }
    }
    MgeGL_End();
}

// 2D arrow: a shaft line from `start` to `end` plus a filled triangular head.
void Draw_Arrow(Vector2 start, Vector2 end, float headSize, Color color)
{
    Draw_LineV(start, end, color);

    Vector2 dir = { end.x - start.x, end.y - start.y };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.0001f || headSize <= 0.0f)
        return;
    dir.x /= len;
    dir.y /= len;

    Vector2 perp = { -dir.y, dir.x };
    Vector2 base = { end.x - dir.x * headSize, end.y - dir.y * headSize };
    Vector2 left = { base.x + perp.x * headSize * 0.5f, base.y + perp.y * headSize * 0.5f };
    Vector2 right = { base.x - perp.x * headSize * 0.5f, base.y - perp.y * headSize * 0.5f };
    Draw_Triangle(end, left, right, color);
}

// 3D arrow (call inside Mge_BeginMode3D): shaft line + a 4-line head.
void Draw_Arrow3D(Vector3 start, Vector3 end, Color color)
{
    Vector3 dir = Vector3_Subtract(end, start);
    float len = Vector3_Length(dir);
    if (len < 0.0001f)
        return;
    dir = Vector3_Scale(dir, 1.0f / len);

    float head = len * 0.18f;
    if (head > 0.5f)
        head = 0.5f;

    Vector3 ref = (fabsf(dir.y) < 0.99f) ? (Vector3){ 0.0f, 1.0f, 0.0f } : (Vector3){ 1.0f, 0.0f, 0.0f };
    Vector3 n1 = Vector3Normalize(Vector3Cross(dir, ref));
    Vector3 n2 = Vector3Cross(dir, n1);
    Vector3 neck = Vector3_Subtract(end, Vector3_Scale(dir, head));

    MgeGL_Begin(MGEGL_LINES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    MgeGL_Vertex3f(start.x, start.y, start.z);
    MgeGL_Vertex3f(end.x, end.y, end.z);

    float r = head * 0.4f;
    Vector3 fins[4] = {
        Vector3_Add(neck, Vector3_Scale(n1, r)),
        Vector3_Add(neck, Vector3_Scale(n1, -r)),
        Vector3_Add(neck, Vector3_Scale(n2, r)),
        Vector3_Add(neck, Vector3_Scale(n2, -r)),
    };
    for (int i = 0; i < 4; i++) {
        MgeGL_Vertex3f(end.x, end.y, end.z);
        MgeGL_Vertex3f(fins[i].x, fins[i].y, fins[i].z);
    }
    MgeGL_End();
}

// true if any component is non-zero (skip the rotation maths for the common case)
static bool has_rotation(Vector3 deg)
{
    return deg.x != 0.0f || deg.y != 0.0f || deg.z != 0.0f;
}

void Draw_CubeEx(Vector3 center, Vector3 size, Vector3 rotationDeg, Color color)
{
    float x = size.x * 0.5f, y = size.y * 0.5f, z = size.z * 0.5f;

    const bool rot = has_rotation(rotationDeg);
    const Matrix R = rot
        ? Matrix_RotateXYZ((Vector3){ rotationDeg.x * DEG2RAD, rotationDeg.y * DEG2RAD, rotationDeg.z * DEG2RAD })
        : Matrix_Identity();

    // 6 faces, each: outward normal + its 4 corner OFFSETS from centre (bottom-
    // left, bottom-right, top-right, top-left seen from outside). Rotated about
    // the centre, then translated. Per-corner UVs so a texture maps once per face.
    const struct {
        float n[3];
        float v[4][3];
    } faces[6] = {
        { { 0, 0, 1 },  { { -x, -y, z }, { x, -y, z }, { x, y, z }, { -x, y, z } } },     // +Z
        { { 0, 0, -1 }, { { x, -y, -z }, { -x, -y, -z }, { -x, y, -z }, { x, y, -z } } }, // -Z
        { { 1, 0, 0 },  { { x, -y, z }, { x, -y, -z }, { x, y, -z }, { x, y, z } } },     // +X
        { { -1, 0, 0 }, { { -x, -y, -z }, { -x, -y, z }, { -x, y, z }, { -x, y, -z } } }, // -X
        { { 0, 1, 0 },  { { -x, y, z }, { x, y, z }, { x, y, -z }, { -x, y, -z } } },     // +Y
        { { 0, -1, 0 }, { { -x, -y, -z }, { x, -y, -z }, { x, -y, z }, { -x, -y, z } } }, // -Y
    };
    const float uv[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
    const int tri[6] = { 0, 1, 2, 0, 2, 3 };

    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    for (int f = 0; f < 6; f++) {
        Vector3 n = { faces[f].n[0], faces[f].n[1], faces[f].n[2] };
        if (rot) {
            Vector4 rn = Vector4_Transform((Vector4){ n.x, n.y, n.z, 0.0f }, R);
            n = (Vector3){ rn.x, rn.y, rn.z };
        }
        MgeGL_Normal3f(n.x, n.y, n.z);
        for (int i = 0; i < 6; i++) {
            int k = tri[i];
            Vector3 o = { faces[f].v[k][0], faces[f].v[k][1], faces[f].v[k][2] };
            Vector3 p = rot ? Vector3_RotateAround((Vector3){ center.x + o.x, center.y + o.y, center.z + o.z }, center, R)
                            : (Vector3){ center.x + o.x, center.y + o.y, center.z + o.z };
            MgeGL_TexCoord2f(uv[k][0], uv[k][1]);
            MgeGL_Vertex3f(p.x, p.y, p.z);
        }
    }
    MgeGL_TexCoord2f(0.0f, 0.0f);
    MgeGL_Normal3f(0.0f, 0.0f, 1.0f);
    MgeGL_End();
}

void Draw_Cube(Vector3 center, Vector3 size, Color color)
{
    Draw_CubeEx(center, size, (Vector3){ 0.0f, 0.0f, 0.0f }, color);
}

void Draw_CubeWiresEx(Vector3 center, Vector3 size, Vector3 rotationDeg, Color color)
{
    float x = size.x * 0.5f, y = size.y * 0.5f, z = size.z * 0.5f;

    const bool rot = has_rotation(rotationDeg);
    const Matrix R = rot
        ? Matrix_RotateXYZ((Vector3){ rotationDeg.x * DEG2RAD, rotationDeg.y * DEG2RAD, rotationDeg.z * DEG2RAD })
        : Matrix_Identity();

    Vector3 c[8] = {
        { -x, -y, -z }, { x, -y, -z }, { x, y, -z }, { -x, y, -z },
        { -x, -y, z }, { x, -y, z }, { x, y, z }, { -x, y, z },
    };
    for (int i = 0; i < 8; i++) {
        c[i] = (Vector3){ center.x + c[i].x, center.y + c[i].y, center.z + c[i].z };
        if (rot)
            c[i] = Vector3_RotateAround(c[i], center, R);
    }

    int e[12][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
        { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };

    MgeGL_Begin(MGEGL_LINES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);
    for (int i = 0; i < 12; i++) {
        MgeGL_Vertex3f(c[e[i][0]].x, c[e[i][0]].y, c[e[i][0]].z);
        MgeGL_Vertex3f(c[e[i][1]].x, c[e[i][1]].y, c[e[i][1]].z);
    }
    MgeGL_End();
}

void Draw_CubeWires(Vector3 center, Vector3 size, Color color)
{
    Draw_CubeWiresEx(center, size, (Vector3){ 0.0f, 0.0f, 0.0f }, color);
}

void Draw_SphereEx(Vector3 center, float radius, int rings, int slices, Color color)
{
    if (rings < 2)
        rings = 2;
    if (slices < 3)
        slices = 3;

    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(color.r, color.g, color.b, color.a);

    for (int i = 0; i < rings; i++) {
        float lat0 = PI * (-0.5f + (float)i / (float)rings);
        float lat1 = PI * (-0.5f + (float)(i + 1) / (float)rings);
        float y0 = sinf(lat0), cr0 = cosf(lat0);
        float y1 = sinf(lat1), cr1 = cosf(lat1);

        for (int j = 0; j < slices; j++) {
            float lng0 = 2.0f * PI * (float)j / (float)slices;
            float lng1 = 2.0f * PI * (float)(j + 1) / (float)slices;
            float cx0 = cosf(lng0), sz0 = sinf(lng0);
            float cx1 = cosf(lng1), sz1 = sinf(lng1);

            // unit-sphere corners; the position doubles as the normal
            Vector3 n[4] = {
                { cx0 * cr0, y0, sz0 * cr0 }, { cx1 * cr0, y0, sz1 * cr0 },
                { cx1 * cr1, y1, sz1 * cr1 }, { cx0 * cr1, y1, sz0 * cr1 },
            };
            const int tri[6] = { 0, 1, 2, 0, 2, 3 };
            for (int k = 0; k < 6; k++) {
                Vector3 u = n[tri[k]];
                MgeGL_Normal3f(u.x, u.y, u.z);
                MgeGL_TexCoord2f(0.0f, 0.0f);
                MgeGL_Vertex3f(center.x + u.x * radius, center.y + u.y * radius, center.z + u.z * radius);
            }
        }
    }

    MgeGL_TexCoord2f(0.0f, 0.0f);
    MgeGL_Normal3f(0.0f, 0.0f, 1.0f);
    MgeGL_End();
}

void Draw_Sphere(Vector3 center, float radius, Color color)
{
    Draw_SphereEx(center, radius, 8, 14, color);
}
