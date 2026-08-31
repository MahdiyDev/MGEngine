// Physically-based rendering -- Cook-Torrance metallic/roughness BRDF, with
// optional image-based ambient lighting. A separate path from mge_light.c's
// Blinn-Phong: PBRMaterial (albedo / normal / metallic / roughness / ao) and its
// own shader.  LearnOpenGL PBR/Theory + PBR/Lighting + PBR/IBL.

#include "mge.h"
#include "mge_gl.h"

#include <glad/glad.h>
#include <stdio.h>

static const char* pbrVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec4 aColor;\n"
    "layout(location = 2) in vec2 aTexCoord;\n"
    "layout(location = 3) in vec3 aNormal;\n"
    "out vec4 vColor;\n"
    "out vec2 vUV;\n"
    "out vec3 vFragPos;\n"
    "out vec3 vNormal;\n"
    "uniform mat4 modelview;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * modelview * vec4(aPos, 1.0);\n"
    "    vColor = aColor; vUV = aTexCoord; vFragPos = aPos; vNormal = aNormal;\n"
    "}\n";

static const char* pbrFrag =
    "#version 330 core\n"
    "#define MAX_LIGHTS 8\n"
    "#define PI 3.14159265359\n"
    "out vec4 FragColor;\n"
    "in vec4 vColor;\n"
    "in vec2 vUV;\n"
    "in vec3 vFragPos;\n"
    "in vec3 vNormal;\n"
    "struct Light { int type; int enabled; vec3 position; vec3 direction; vec3 color;\n"
    "               float ambient; float diffuse; float specular;\n"
    "               float constant; float linear; float quadratic;\n"
    "               float innerCutoff; float outerCutoff; };\n"
    "uniform Light lights[MAX_LIGHTS];\n"
    "uniform int lightCount;\n"
    "uniform vec3 camPos;\n"
    // material
    "uniform sampler2D albedoMap;   uniform int useAlbedoMap;   uniform vec3 albedoColor;\n"
    "uniform sampler2D normalMap;   uniform int useNormalMap;\n"
    "uniform sampler2D metallicMap; uniform int useMetallicMap; uniform float metallicValue;\n"
    "uniform sampler2D roughMap;    uniform int useRoughMap;    uniform float roughnessValue;\n"
    "uniform sampler2D aoMap;       uniform int useAoMap;       uniform float aoValue;\n"
    // IBL
    "uniform int useIBL;\n"
    "uniform samplerCube irradianceMap;\n"
    "uniform samplerCube prefilterMap;\n"
    "uniform sampler2D brdfLUT;\n"
    "uniform float prefilterMaxLod;\n"
    "\n"
    "vec3 getNormal()\n"
    "{\n"
    "    vec3 N = normalize(vNormal);\n"
    "    if (useNormalMap == 0) return N;\n"
    // TBN from screen-space derivatives (same trick as the Blinn-Phong path)
    "    vec3 dp1 = dFdx(vFragPos), dp2 = dFdy(vFragPos);\n"
    "    vec2 du1 = dFdx(vUV), du2 = dFdy(vUV);\n"
    "    vec3 dp2p = cross(dp2, N), dp1p = cross(N, dp1);\n"
    "    vec3 T = dp2p * du1.x + dp1p * du2.x;\n"
    "    vec3 B = dp2p * du1.y + dp1p * du2.y;\n"
    "    float inv = inversesqrt(max(dot(T,T), dot(B,B)));\n"
    "    mat3 TBN = mat3(T * inv, B * inv, N);\n"
    "    vec3 m = texture(normalMap, vUV).xyz * 2.0 - 1.0;\n"
    "    return normalize(TBN * m);\n"
    "}\n"
    "\n"
    "float DistributionGGX(vec3 N, vec3 H, float a)\n"
    "{\n"
    "    float a2 = a * a;\n"
    "    float ndh = max(dot(N, H), 0.0);\n"
    "    float d = ndh * ndh * (a2 - 1.0) + 1.0;\n"
    "    return a2 / max(PI * d * d, 1e-7);\n"
    "}\n"
    "float GeometrySchlickGGX(float ndv, float k)\n"
    "{\n"
    "    return ndv / (ndv * (1.0 - k) + k);\n"
    "}\n"
    "float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)\n"
    "{\n"
    "    float k = (rough + 1.0) * (rough + 1.0) / 8.0;\n"      // direct-light k
    "    return GeometrySchlickGGX(max(dot(N, V), 0.0), k) * GeometrySchlickGGX(max(dot(N, L), 0.0), k);\n"
    "}\n"
    "vec3 fresnelSchlick(float ct, vec3 F0)\n"
    "{\n"
    "    return F0 + (1.0 - F0) * pow(clamp(1.0 - ct, 0.0, 1.0), 5.0);\n"
    "}\n"
    "vec3 fresnelSchlickRough(float ct, vec3 F0, float rough)\n"
    "{\n"
    "    return F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(clamp(1.0 - ct, 0.0, 1.0), 5.0);\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec3 albedo = (useAlbedoMap == 1 ? texture(albedoMap, vUV).rgb : albedoColor) * vColor.rgb;\n"
    "    float metallic  = useMetallicMap == 1 ? texture(metallicMap, vUV).r : metallicValue;\n"
    "    float roughness = useRoughMap    == 1 ? texture(roughMap, vUV).r    : roughnessValue;\n"
    "    float ao        = useAoMap       == 1 ? texture(aoMap, vUV).r       : aoValue;\n"
    "    vec3 N = getNormal();\n"
    "    vec3 V = normalize(camPos - vFragPos);\n"
    "    vec3 R = reflect(-V, N);\n"
    "    vec3 F0 = mix(vec3(0.04), albedo, metallic);\n"
    "\n"
    "    vec3 Lo = vec3(0.0);\n"
    "    for (int i = 0; i < lightCount && i < MAX_LIGHTS; i++) {\n"
    "        if (lights[i].enabled == 0) continue;\n"
    "        vec3 L; vec3 radiance = lights[i].color * lights[i].diffuse;\n"
    "        if (lights[i].type == 0) { L = normalize(-lights[i].direction); }\n"
    "        else {\n"
    "            vec3 toL = lights[i].position - vFragPos; float d = length(toL);\n"
    "            L = toL / max(d, 1e-4);\n"
    "            radiance /= max(lights[i].constant + lights[i].linear * d + lights[i].quadratic * d * d, 1e-4);\n"
    "        }\n"
    "        if (lights[i].type == 2) {\n"
    "            float th = dot(L, normalize(-lights[i].direction));\n"
    "            float e = max(lights[i].innerCutoff - lights[i].outerCutoff, 1e-4);\n"
    "            radiance *= clamp((th - lights[i].outerCutoff) / e, 0.0, 1.0);\n"
    "        }\n"
    "        vec3 H = normalize(V + L);\n"
    "        float NDF = DistributionGGX(N, H, roughness);\n"
    "        float G   = GeometrySmith(N, V, L, roughness);\n"
    "        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);\n"
    "        vec3  spec = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 1e-4);\n"
    "        vec3  kD = (vec3(1.0) - F) * (1.0 - metallic);\n"
    "        Lo += (kD * albedo / PI + spec) * radiance * max(dot(N, L), 0.0);\n"
    "    }\n"
    "\n"
    "    vec3 ambient;\n"
    "    if (useIBL == 1) {\n"
    "        vec3 F = fresnelSchlickRough(max(dot(N, V), 0.0), F0, roughness);\n"
    "        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);\n"
    "        vec3 diffuse = texture(irradianceMap, N).rgb * albedo;\n"
    "        vec3 pref = textureLod(prefilterMap, R, roughness * prefilterMaxLod).rgb;\n"
    "        vec2 ab = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;\n"
    "        vec3 specular = pref * (F * ab.x + ab.y);\n"
    "        ambient = (kD * diffuse + specular) * ao;\n"
    "    } else {\n"
    "        ambient = vec3(0.03) * albedo * ao;\n"
    "    }\n"
    "    FragColor = vec4(ambient + Lo, 1.0);\n"  // linear HDR -- tone-map downstream
    "}\n";

static Shader s_shader = { 0 };
static bool s_loaded = false;
static bool s_active = false;

static void ensure(void)
{
    if (s_loaded)
        return;
    s_shader = Mge_LoadShaderFromMemory(pbrVert, pbrFrag);
    s_loaded = true;
}

static void begin_common(const Light* lights, int count, Camera3D camera)
{
    ensure();
    MgeGL_SetShader(s_shader.id);
    MgeGL_SetTexture(MgeGL_GetWhiteTexture());
    s_active = true;

    if (count < 0) count = 0;
    if (count > MGE_MAX_LIGHTS) count = MGE_MAX_LIGHTS;

    MgeGL_Uniform3fv("camPos", camera.position);
    MgeGL_Uniform1i("lightCount", count);
    MgeGL_Uniform1i("albedoMap", 0);
    MgeGL_Uniform1i("normalMap", 1);
    MgeGL_Uniform1i("metallicMap", 2);
    MgeGL_Uniform1i("roughMap", 3);
    MgeGL_Uniform1i("aoMap", 4);
    MgeGL_Uniform1i("irradianceMap", 5);
    MgeGL_Uniform1i("prefilterMap", 6);
    MgeGL_Uniform1i("brdfLUT", 7);
    MgeGL_Uniform1i("useIBL", 0);
    for (int i = 0; i < count; i++)
        Mge_UploadLightUniforms(&lights[i], i);
}

void Mge_BeginPBR3D(const Light* lights, int count, Camera3D camera)
{
    begin_common(lights, count, camera);
}

void Mge_BeginPBR3DIBL(const Light* lights, int count, Camera3D camera, Environment env)
{
    begin_common(lights, count, camera);
    if (env.irradiance == 0)
        return;
    MgeGL_SetTextureSlot(5, 0); // slots 5/6 are cubemaps -- bind by hand
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.irradiance);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.prefilter);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, env.brdfLUT);
    glActiveTexture(GL_TEXTURE0);
    MgeGL_Uniform1i("useIBL", 1);
    MgeGL_Uniform1f("prefilterMaxLod", (float)(env.prefilterMips - 1));
}

void Mge_SetPBRMaterial(PBRMaterial m)
{
    if (!s_active)
        return;
    MgeGL_Draw();

    MgeGL_SetTexture(m.albedo.id); // unit 0 through the batcher
    MgeGL_SetTextureSlot(1, m.normal.id);
    MgeGL_SetTextureSlot(2, m.metallic.id);
    MgeGL_SetTextureSlot(3, m.roughness.id);
    MgeGL_SetTextureSlot(4, m.ao.id);

    MgeGL_Uniform1i("useAlbedoMap", m.albedo.id != 0);
    MgeGL_Uniform1i("useNormalMap", m.normal.id != 0);
    MgeGL_Uniform1i("useMetallicMap", m.metallic.id != 0);
    MgeGL_Uniform1i("useRoughMap", m.roughness.id != 0);
    MgeGL_Uniform1i("useAoMap", m.ao.id != 0);
    MgeGL_Uniform3fv("albedoColor", m.albedoColor);
    MgeGL_Uniform1f("metallicValue", m.metallicValue);
    MgeGL_Uniform1f("roughnessValue", m.roughnessValue);
    MgeGL_Uniform1f("aoValue", (m.aoValue > 0.0f) ? m.aoValue : 1.0f);
}

void Mge_EndPBR3D(void)
{
    if (!s_active)
        return;
    s_active = false;
    MgeGL_Draw();
    for (int s = 1; s <= 4; s++)
        MgeGL_SetTextureSlot(s, 0);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    MgeGL_SetTexture(MgeGL_GetWhiteTexture());
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
