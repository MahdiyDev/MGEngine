// Phong lighting for 3D surfaces: ambient + diffuse + specular, for any mix of
// directional / point / spot lights (up to MGE_MAX_LIGHTS at once).
//
// The renderer submits geometry in world space and uses `modelview` = the view
// matrix only (there is no per-object model matrix), so the fragment position and
// normal are already in world space -- no normal matrix is needed.
//
//   Light lights[] = { Mge_MakeDirectionalLight(...), Mge_MakePointLight(...) };
//   Mge_BeginLighting3DEx(lights, 2, camera);   // switches to the lighting shader
//       Mge_SetMaterial(mat);                   // per-surface texture + shininess
//       Draw_Cube(...);                         // or Mge_DrawObject(obj)
//   Mge_EndLighting3D();                        // back to the default (unlit) shader

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

#include <math.h>
#include <stdio.h>

static const char* lightVertCode =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec4 aColor;\n"
    "layout(location = 2) in vec2 aTexCoord;\n"
    "layout(location = 3) in vec3 aNormal;\n"
    "out vec4 vColor;\n"
    "out vec2 vTexCoord;\n"
    "out vec3 vFragPos;\n"
    "out vec3 vNormal;\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * modelview * vec4(aPos, 1.0);\n"
    "    vColor = aColor;\n"
    "    vTexCoord = aTexCoord;\n"
    "    vFragPos = aPos;\n"   // geometry is already in world space
    "    vNormal = aNormal;\n"
    "}\n";

static const char* lightFragCode =
    "#version 330 core\n"
    "#define MAX_LIGHTS 8\n"
    "out vec4 FragColor;\n"
    "in vec4 vColor;\n"
    "in vec2 vTexCoord;\n"
    "in vec3 vFragPos;\n"
    "in vec3 vNormal;\n"
    "struct Light {\n"
    "    int type;          // 0 dir, 1 point, 2 spot\n"
    "    int enabled;\n"
    "    vec3 position;\n"
    "    vec3 direction;\n"
    "    vec3 color;\n"
    "    float ambient;\n"
    "    float diffuse;\n"
    "    float specular;\n"
    "    float constant;\n"
    "    float linear;\n"
    "    float quadratic;\n"
    "    float innerCutoff;\n"
    "    float outerCutoff;\n"
    "};\n"
    "uniform Light lights[MAX_LIGHTS];\n"
    "uniform int lightCount;\n"
    "uniform vec3 viewPos;\n"
    "uniform sampler2D sampleTex;\n"
    "uniform float matSpecular;\n"   // MATERIAL_MAP_SPECULAR.value
    "uniform float shininess;\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(vNormal);\n"
    "    vec3 V = normalize(viewPos - vFragPos);\n"
    "    vec4 base = texture(sampleTex, vTexCoord) * vColor;\n"   // MATERIAL_MAP_DIFFUSE: texture * colour
    "    vec3 lit = vec3(0.0);\n"
    "    for (int i = 0; i < lightCount && i < MAX_LIGHTS; i++) {\n"
    "        if (lights[i].enabled == 0) continue;\n"
    "        vec3 L;\n"
    "        float atten = 1.0;\n"
    "        if (lights[i].type == 0) {\n"                        // directional
    "            L = normalize(-lights[i].direction);\n"
    "        } else {\n"                                          // point / spot
    "            vec3 toLight = lights[i].position - vFragPos;\n"
    "            float dist = length(toLight);\n"
    "            L = toLight / max(dist, 1e-4);\n"
    "            atten = 1.0 / (lights[i].constant + lights[i].linear * dist + lights[i].quadratic * dist * dist);\n"
    "        }\n"
    "        float intensity = 1.0;\n"
    "        if (lights[i].type == 2) {\n"                        // spot cone (soft edge)
    "            float theta = dot(L, normalize(-lights[i].direction));\n"
    "            float eps = max(lights[i].innerCutoff - lights[i].outerCutoff, 1e-4);\n"
    "            intensity = clamp((theta - lights[i].outerCutoff) / eps, 0.0, 1.0);\n"
    "        }\n"
    "        float diff = max(dot(N, L), 0.0);\n"
    "        vec3 R = reflect(-L, N);\n"
    "        float spec = (diff > 0.0) ? pow(max(dot(V, R), 0.0), max(shininess, 1.0)) : 0.0;\n"
    "        vec3 amb = lights[i].ambient  * lights[i].color;\n"
    "        vec3 dif = lights[i].diffuse  * diff * lights[i].color;\n"
    "        vec3 spc = lights[i].specular * matSpecular * spec * lights[i].color;\n"
    "        lit += amb + (dif + spc) * atten * intensity;\n"
    "    }\n"
    "    FragColor = vec4(lit * base.rgb, base.a);\n"
    "}\n";

static Shader s_shader = { 0 };
static bool s_loaded = false;
static bool s_active = false;

// ----- light construction (no GL context needed) -----

Light Mge_MakeDirectionalLight(Vector3 direction, Vector3 color)
{
    Light l = { 0 };
    l.type = LIGHT_DIRECTIONAL;
    l.enabled = true;
    l.direction = direction;
    l.color = color;
    l.ambient = 0.12f;
    l.diffuse = 1.0f;
    l.specular = 0.5f;
    l.constant = 1.0f; // attenuation unused, but keep the divisor sane
    return l;
}

Light Mge_MakePointLight(Vector3 position, Vector3 color)
{
    Light l = { 0 };
    l.type = LIGHT_POINT;
    l.enabled = true;
    l.position = position;
    l.color = color;
    l.ambient = 0.0f;
    l.diffuse = 1.0f;
    l.specular = 0.5f;
    l.constant = 1.0f;   // classic "reaches ~50 units" falloff
    l.linear = 0.09f;
    l.quadratic = 0.032f;
    return l;
}

Light Mge_MakeSpotLight(Vector3 position, Vector3 direction, Vector3 color,
    float innerAngleDeg, float outerAngleDeg)
{
    Light l = Mge_MakePointLight(position, color);
    l.type = LIGHT_SPOT;
    l.direction = direction;
    l.innerCutoff = cosf(innerAngleDeg * (float)DEG2RAD);
    l.outerCutoff = cosf(outerAngleDeg * (float)DEG2RAD);
    return l;
}

Light Mge_MakeFlashlight(Camera3D camera, Vector3 color)
{
    // camera.target is the (already forward) look direction in this engine
    return Mge_MakeSpotLight(camera.position, camera.target, color, 12.5f, 20.0f);
}

Light Mge_MakeLight(Vector3 position, Vector3 color)
{
    // legacy helper: a point light with no distance falloff
    Light l = Mge_MakePointLight(position, color);
    l.ambient = 0.15f;
    l.linear = 0.0f;
    l.quadratic = 0.0f;
    return l;
}

// ----- binding lights for a draw pass -----

static void UploadLight(const Light* l, int i)
{
    char name[48];
#define U(field) (snprintf(name, sizeof(name), "lights[%d]." field, i), name)
    MgeGL_Uniform1i(U("type"), (int)l->type);
    MgeGL_Uniform1i(U("enabled"), l->enabled ? 1 : 0);
    MgeGL_Uniform3fv(U("position"), l->position);
    MgeGL_Uniform3fv(U("direction"), l->direction);
    MgeGL_Uniform3fv(U("color"), l->color);
    MgeGL_Uniform1f(U("ambient"), l->ambient);
    MgeGL_Uniform1f(U("diffuse"), l->diffuse);
    MgeGL_Uniform1f(U("specular"), l->specular);
    MgeGL_Uniform1f(U("constant"), l->constant);
    MgeGL_Uniform1f(U("linear"), l->linear);
    MgeGL_Uniform1f(U("quadratic"), l->quadratic);
    MgeGL_Uniform1f(U("innerCutoff"), l->innerCutoff);
    MgeGL_Uniform1f(U("outerCutoff"), l->outerCutoff);
#undef U
}

void Mge_BeginLighting3DEx(const Light* lights, int count, Camera3D camera)
{
    if (!s_loaded) {
        s_shader = Mge_LoadShaderFromMemory(lightVertCode, lightFragCode);
        s_loaded = true;
    }

    MgeGL_SetShader(s_shader.id); // flushes the previous batch, makes this program active
    MgeGL_SetTexture(MgeGL_GetWhiteTexture()); // start untextured until a material says otherwise
    s_active = true;

    if (count < 0)
        count = 0;
    if (count > MGE_MAX_LIGHTS)
        count = MGE_MAX_LIGHTS;

    MgeGL_Uniform3fv("viewPos", camera.position);
    MgeGL_Uniform1f("matSpecular", 1.0f);
    MgeGL_Uniform1f("shininess", 32.0f);
    MgeGL_Uniform1i("lightCount", count);

    for (int i = 0; i < count; i++)
        UploadLight(&lights[i], i);
}

void Mge_BeginLighting3D(Light light, Camera3D camera)
{
    Mge_BeginLighting3DEx(&light, 1, camera);
}

void Mge_SetMaterial(Material material)
{
    if (!s_active)
        return;

    MgeGL_Draw(); // flush surfaces queued with the previous material

    // MATERIAL_MAP_DIFFUSE: bind its texture (white 1x1 when unset). The map's
    // colour is applied through the per-vertex colour of the geometry you draw.
    MgeGL_SetTexture(material.maps[MATERIAL_MAP_DIFFUSE].texture.id);

    MgeGL_Uniform1f("matSpecular", material.maps[MATERIAL_MAP_SPECULAR].value);
    MgeGL_Uniform1f("shininess", material.shininess);
}

void Mge_EndLighting3D(void)
{
    if (!s_active)
        return;

    s_active = false;
    MgeGL_SetTexture(MgeGL_GetWhiteTexture()); // don't leak a material texture into unlit draws
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
