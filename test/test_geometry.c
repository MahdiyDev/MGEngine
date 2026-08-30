// Geometry-shader effect wrappers (mge_geometry.c). The GLSL is compiled only
// with a live context; here the MgeGL_* backend is stubbed to check the plumbing.

#include <stdbool.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h"

static unsigned int g_curShader;
static float g_lastFloat;
static Vector4 g_lastVec4;
static int g_drawCalls;

unsigned int MgeGL_LoadShader(const char* c, unsigned int type, const char* name)
{
    (void)c;
    (void)name;
    return type; // any non-zero id
}
unsigned int MgeGL_CreateShaderProgramGeo(unsigned int v, unsigned int g, unsigned int f)
{
    return (v && g && f) ? 100u : 0u;
}
unsigned int MgeGL_GetDefaultShaderId(void) { return 0; }
void MgeGL_SetShader(unsigned int id) { g_curShader = id; }
void MgeGL_Uniform1f(const char* name, float v) { (void)name; g_lastFloat = v; }
void MgeGL_Uniform4fv(const char* name, Vector4 v) { (void)name; g_lastVec4 = v; }
void MgeGL_Draw(void) { g_drawCalls++; }

TEST(begin_explode_switches_shader_and_sets_magnitude)
{
    g_curShader = 0;
    Mge_BeginExplode3D(0.5f);
    CHECK(g_curShader == 100);   // the geometry program
    CHECK(g_lastFloat == 0.5f);

    int d = g_drawCalls;
    Mge_EndExplode3D();
    CHECK(g_drawCalls == d + 1); // flushed
    CHECK(g_curShader == 0);     // back to default
}

TEST(begin_normals_sets_length_and_colour)
{
    g_curShader = 0;
    Mge_BeginNormals3D(0.2f, (Color){ 255, 128, 0, 255 });
    CHECK(g_curShader == 100);
    CHECK(g_lastFloat == 0.2f);
    CHECK(g_lastVec4.x == 1.0f);
    CHECK(g_lastVec4.y > 0.49f && g_lastVec4.y < 0.51f); // 128/255
    CHECK(g_lastVec4.w == 1.0f);

    Mge_EndNormals3D();
    CHECK(g_curShader == 0);
}

int main(void)
{
    RUN(begin_explode_switches_shader_and_sets_magnitude);
    RUN(begin_normals_sets_length_and_colour);
    return test_summary();
}
