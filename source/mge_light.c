// Phong lighting for 3D surfaces: ambient + diffuse + specular.
//
// The renderer submits geometry in world space and uses `modelview` = the view
// matrix only (there is no per-object model matrix), so the fragment position and
// normal are already in world space -- no normal matrix is needed.
//
//   Mge_BeginLighting3D(light, camera);   // switches to the lighting shader
//       Mge_SetMaterial(mat);             // per-surface texture + shininess
//       Draw_Cube(...);                   // or Mge_DrawObject(obj)
//   Mge_EndLighting3D();                  // back to the default (unlit) shader

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

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
    "out vec4 FragColor;\n"
    "in vec4 vColor;\n"
    "in vec2 vTexCoord;\n"
    "in vec3 vFragPos;\n"
    "in vec3 vNormal;\n"
    "uniform sampler2D sampleTex;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 viewPos;\n"
    "uniform float ambientStrength;\n"
    "uniform float diffuseStrength;\n"
    "uniform float specularStrength;\n"
    "uniform float matSpecular;\n"   // per-material specular multiplier (MATERIAL_MAP_SPECULAR.value)
    "uniform float shininess;\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(vNormal);\n"
    "    vec3 L = normalize(lightPos - vFragPos);\n"
    "    vec3 V = normalize(viewPos - vFragPos);\n"
    "    vec3 R = reflect(-L, N);\n"
    "    float diff = max(dot(N, L), 0.0);\n"
    "    float spec = (diff > 0.0) ? pow(max(dot(V, R), 0.0), max(shininess, 1.0)) : 0.0;\n"
    "    vec3 ambient  = ambientStrength  * lightColor;\n"
    "    vec3 diffuse  = diffuseStrength  * diff * lightColor;\n"
    "    vec3 specular = specularStrength * matSpecular * spec * lightColor;\n"
    "    vec4 base = texture(sampleTex, vTexCoord) * vColor;\n"   // MATERIAL_MAP_DIFFUSE: texture * colour
    "    FragColor = vec4((ambient + diffuse + specular) * base.rgb, base.a);\n"
    "}\n";

static Shader s_shader = { 0 };
static bool s_loaded = false;
static bool s_active = false;

Light Mge_MakeLight(Vector3 position, Vector3 color)
{
    Light l = { 0 };
    l.position = position;
    l.color = color;
    l.ambient = 0.15f;
    l.diffuse = 1.0f;
    l.specular = 0.5f;
    return l;
}

void Mge_BeginLighting3D(Light light, Camera3D camera)
{
    if (!s_loaded) {
        s_shader = Mge_LoadShaderFromMemory(lightVertCode, lightFragCode);
        s_loaded = true;
    }

    MgeGL_SetShader(s_shader.id); // flushes the previous batch, makes this program active
    MgeGL_SetTexture(MgeGL_GetWhiteTexture()); // start untextured until a material says otherwise
    s_active = true;

    MgeGL_Uniform3fv("lightPos", light.position);
    MgeGL_Uniform3fv("lightColor", light.color);
    MgeGL_Uniform3fv("viewPos", camera.position);
    MgeGL_Uniform1f("ambientStrength", light.ambient);
    MgeGL_Uniform1f("diffuseStrength", light.diffuse);
    MgeGL_Uniform1f("specularStrength", light.specular);
    MgeGL_Uniform1f("matSpecular", 1.0f);
    MgeGL_Uniform1f("shininess", 32.0f);
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
