// Geometry-shader effects: exploding triangles and vertex-normal visualization.
// Both drive the batcher's geometry (Draw_Cube / Mge_DrawMesh / Mge_DrawModel):
// the vertex shader works in view space, the geometry shader emits primitives
// and applies `projection` -- both matrices come from the batcher.

#include "mge.h"
#include "mge_gl.h"

#include <glad/glad.h>

// ---- explode: displace each triangle along its own face normal ----

static const char* explodeVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec4 aColor;\n"
    "layout(location = 2) in vec2 aTexCoord;\n"
    "out VS { vec4 color; vec2 uv; } vs;\n"
    "uniform mat4 modelview;\n"
    "void main() { vs.color = aColor; vs.uv = aTexCoord; gl_Position = modelview * vec4(aPos, 1.0); }\n";

static const char* explodeGeom =
    "#version 330 core\n"
    "layout(triangles) in;\n"
    "layout(triangle_strip, max_vertices = 3) out;\n"
    "in VS { vec4 color; vec2 uv; } gs[];\n"
    "out vec4 vColor;\n"
    "out vec2 vUV;\n"
    "uniform mat4 projection;\n"
    "uniform float magnitude;\n"
    "void main()\n"
    "{\n"
    "    vec3 a = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;\n"
    "    vec3 b = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;\n"
    "    vec3 n = normalize(cross(a, b));\n"
    "    for (int i = 0; i < 3; i++) {\n"
    "        vColor = gs[i].color;\n"
    "        vUV = gs[i].uv;\n"
    "        gl_Position = projection * (gl_in[i].gl_Position + vec4(n * magnitude, 0.0));\n"
    "        EmitVertex();\n"
    "    }\n"
    "    EndPrimitive();\n"
    "}\n";

static const char* explodeFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec4 vColor;\n"
    "in vec2 vUV;\n"
    "uniform sampler2D tex;\n"
    "void main() { FragColor = texture(tex, vUV) * vColor; }\n";

// ---- normals: a short line at every vertex ----

static const char* normalsVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 3) in vec3 aNormal;\n"
    "out VS { vec3 normal; } vs;\n"
    "uniform mat4 modelview;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = modelview * vec4(aPos, 1.0);\n"
    "    vs.normal = mat3(modelview) * aNormal;\n"   // view-space (modelview is view-only here)
    "}\n";

static const char* normalsGeom =
    "#version 330 core\n"
    "layout(triangles) in;\n"
    "layout(line_strip, max_vertices = 6) out;\n"
    "in VS { vec3 normal; } gs[];\n"
    "uniform mat4 projection;\n"
    "uniform float len;\n"
    "void line(int i)\n"
    "{\n"
    "    gl_Position = projection * gl_in[i].gl_Position;\n"
    "    EmitVertex();\n"
    "    vec3 n = (length(gs[i].normal) > 0.0) ? normalize(gs[i].normal) : vec3(0.0);\n"
    "    gl_Position = projection * (gl_in[i].gl_Position + vec4(n * len, 0.0));\n"
    "    EmitVertex();\n"
    "    EndPrimitive();\n"
    "}\n"
    "void main() { line(0); line(1); line(2); }\n";

static const char* normalsFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 color;\n"
    "void main() { FragColor = color; }\n";

static unsigned int s_explodeProgram = 0;
static unsigned int s_normalsProgram = 0;

static void EnsureResources(void)
{
    if (s_explodeProgram != 0)
        return;

    s_explodeProgram = MgeGL_CreateShaderProgramGeo(
        MgeGL_LoadShader(explodeVert, GL_VERTEX_SHADER, "explode vertex"),
        MgeGL_LoadShader(explodeGeom, GL_GEOMETRY_SHADER, "explode geometry"),
        MgeGL_LoadShader(explodeFrag, GL_FRAGMENT_SHADER, "explode fragment"));
    s_normalsProgram = MgeGL_CreateShaderProgramGeo(
        MgeGL_LoadShader(normalsVert, GL_VERTEX_SHADER, "normals vertex"),
        MgeGL_LoadShader(normalsGeom, GL_GEOMETRY_SHADER, "normals geometry"),
        MgeGL_LoadShader(normalsFrag, GL_FRAGMENT_SHADER, "normals fragment"));
}

void Mge_BeginExplode3D(float magnitude)
{
    EnsureResources();
    MgeGL_SetShader(s_explodeProgram);
    MgeGL_Uniform1f("magnitude", magnitude);
}

void Mge_EndExplode3D(void)
{
    MgeGL_Draw();
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

void Mge_BeginNormals3D(float length, Color color)
{
    EnsureResources();
    MgeGL_SetShader(s_normalsProgram);
    MgeGL_Uniform1f("len", length);
    MgeGL_Uniform4fv("color", (Vector4){
        color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f });
}

void Mge_EndNormals3D(void)
{
    MgeGL_Draw();
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
