// Blinn-Phong lighting for 3D surfaces: ambient + diffuse + specular, for any mix
// of directional / point / spot lights (up to MGE_MAX_LIGHTS at once). The
// specular model is switchable (Mge_SetLightingModel) between Blinn-Phong (the
// default halfway-vector form) and classic Phong (reflect + view dir).
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
    "uniform float matSpecular;\n"       // MATERIAL_MAP_SPECULAR.value  (highlight strength)
    "uniform vec3  matSpecularColor;\n"  // MATERIAL_MAP_SPECULAR.color  (tints the highlight, 0..1)
    "uniform float matDiffuse;\n"        // MATERIAL_MAP_DIFFUSE.value   (base-colour gain)
    "uniform float shininess;\n"
    "uniform int blinn;\n"           // 1 = Blinn-Phong halfway vector, 0 = classic Phong reflect
    "uniform int shadowsEnabled;\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "uniform sampler2D shadowMap;\n"
    "float ShadowFactor(vec3 N, vec3 L)\n"
    "{\n"
    "    if (shadowsEnabled == 0) return 0.0;\n"
    "    vec4 lp = lightSpaceMatrix * vec4(vFragPos, 1.0);\n"
    "    vec3 p = lp.xyz / lp.w;\n"
    "    p = p * 0.5 + 0.5;\n"                                // clip -> [0,1] texture space
    "    if (p.z > 1.0) return 0.0;\n"                        // past the light's far plane
    "    float bias = max(0.0025 * (1.0 - dot(N, L)), 0.0007);\n"
    "    float cur = p.z - bias;\n"
    "    float sh = 0.0;\n"
    "    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));\n"
    "    for (int x = -1; x <= 1; x++)\n"
    "        for (int y = -1; y <= 1; y++)\n"
    "            sh += cur > texture(shadowMap, p.xy + vec2(x, y) * texel).r ? 1.0 : 0.0;\n"
    "    return sh / 9.0;\n"                                  // 3x3 PCF
    "}\n"
    "uniform int pointShadowEnabled;\n"
    "uniform samplerCube pointShadowMap;\n"
    "uniform vec3 pointShadowLightPos;\n"
    "uniform float pointShadowFar;\n"
    "const vec3 pcfDisk[20] = vec3[](\n"
    "    vec3( 1, 1, 1), vec3( 1,-1, 1), vec3(-1,-1, 1), vec3(-1, 1, 1),\n"
    "    vec3( 1, 1,-1), vec3( 1,-1,-1), vec3(-1,-1,-1), vec3(-1, 1,-1),\n"
    "    vec3( 1, 1, 0), vec3( 1,-1, 0), vec3(-1,-1, 0), vec3(-1, 1, 0),\n"
    "    vec3( 1, 0, 1), vec3(-1, 0, 1), vec3( 1, 0,-1), vec3(-1, 0,-1),\n"
    "    vec3( 0, 1, 1), vec3( 0,-1, 1), vec3( 0,-1,-1), vec3( 0, 1,-1));\n"
    "float PointShadowFactor(vec3 fragPos)\n"
    "{\n"
    "    if (pointShadowEnabled == 0) return 0.0;\n"
    "    vec3 toLight = fragPos - pointShadowLightPos;\n"
    "    float cur = length(toLight);\n"
    "    if (cur > pointShadowFar) return 0.0;\n"
    "    float bias = 0.15;\n"
    "    float radius = (1.0 + cur / pointShadowFar) / 25.0;\n"
    "    float sh = 0.0;\n"
    "    for (int i = 0; i < 20; i++) {\n"
    "        float closest = texture(pointShadowMap, toLight + pcfDisk[i] * radius).r * pointShadowFar;\n"
    "        sh += (cur - bias > closest) ? 1.0 : 0.0;\n"
    "    }\n"
    "    return sh / 20.0;\n"
    "}\n"
    "uniform sampler2D normalMap;\n"
    "uniform int useNormalMap;\n"
    "uniform float normalStrength;\n"   // MATERIAL_MAP_NORMAL.value (0 = flat, 1 = as-authored, >1 = exaggerated)
    "uniform sampler2D heightMap;\n"
    "uniform int useParallax;\n"
    "uniform float heightScale;\n"      // MATERIAL_MAP_HEIGHT.value
    "uniform vec2 uvTiling;\n"          // Material.tiling  (uv' = uv*tiling + offset)
    "uniform vec2 uvOffset;\n"          // Material.offset
    "uniform int  triplanar;\n"         // Material.triplanar (diffuse projected from world XYZ)
    "uniform float triplanarScale;\n"   // Material.triplanarScale (world units per tile)
    // TBN from screen-space derivatives (no tangent vertex attribute needed).
    // Columns are tangent / bitangent / normal, so transpose() maps world->tangent.
    "mat3 CotangentFrame(vec3 N)\n"
    "{\n"
    "    vec3 dp1 = dFdx(vFragPos);\n"
    "    vec3 dp2 = dFdy(vFragPos);\n"
    "    vec2 duv1 = dFdx(vTexCoord);\n"
    "    vec2 duv2 = dFdy(vTexCoord);\n"
    "    vec3 dp2perp = cross(dp2, N);\n"
    "    vec3 dp1perp = cross(N, dp1);\n"
    "    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;\n"
    "    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;\n"
    "    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));\n"
    "    return mat3(T * invmax, B * invmax, N);\n"
    "}\n"
    "vec3 ApplyNormalMap(mat3 TBN, vec2 uv)\n"
    "{\n"
    "    vec3 m = texture(normalMap, uv).xyz * 2.0 - 1.0;\n"
    "    m.xy *= normalStrength;\n"          // scale the tangent-space tilt
    "    m = normalize(m);\n"
    "    return normalize(TBN * m);\n"
    "}\n"
    // Parallax-occlusion mapping (LearnOpenGL Advanced-Lighting/Parallax-Mapping).
    // viewTS is the fragment->eye direction in tangent space. The map is a DEPTH
    // map -- sampled directly: black (0) = surface, white (1) = deep groove
    // (e.g. bricks2_disp.jpg). Invert a height map before use.
    "vec2 ParallaxMapping(vec2 texCoords, vec3 viewTS)\n"
    "{\n"
    "    const float minLayers = 8.0;\n"
    "    const float maxLayers = 32.0;\n"
    "    float numLayers = mix(maxLayers, minLayers, clamp(abs(viewTS.z), 0.0, 1.0));\n"
    "    float layerDepth = 1.0 / numLayers;\n"
    "    float curLayerDepth = 0.0;\n"
    "    vec2 P = viewTS.xy / max(viewTS.z, 1e-3) * heightScale;\n"
    "    vec2 dTex = P / numLayers;\n"
    "    vec2 curTex = texCoords;\n"
    "    float curDepth = texture(heightMap, curTex).r;\n"
    "    for (int i = 0; i < 64; i++) {\n"
    "        if (curLayerDepth >= curDepth) break;\n"
    "        curTex -= dTex;\n"
    "        curDepth = texture(heightMap, curTex).r;\n"
    "        curLayerDepth += layerDepth;\n"
    "    }\n"
    "    vec2 prevTex = curTex + dTex;\n"
    "    float after  = curDepth - curLayerDepth;\n"
    "    float before = texture(heightMap, prevTex).r - curLayerDepth + layerDepth;\n"
    "    float w = after / (after - before);\n"
    "    return mix(curTex, prevTex, clamp(w, 0.0, 1.0));\n"
    "}\n"
    // --- triplanar: project from world XYZ, blend the 3 axis planes by |normal| ---
    "vec3 TriBlend(vec3 n)\n"
    "{\n"
    "    vec3 b = pow(abs(n), vec3(4.0));\n"
    "    return b / (b.x + b.y + b.z + 1e-5);\n"
    "}\n"
    // Per-plane parallax-occlusion march (offset-limiting -- no /viewDir.z, which
    // keeps the displacement stable and aligned at grazing angles). vt = the view
    // dir in this plane's (u, v, out) frame; the height map is a DEPTH map sampled
    // directly (0 = surface, 1 = deep groove -- same as ParallaxMapping).
    "vec2 TriPOM(vec2 uv, vec3 vt)\n"
    "{\n"
    "    if (useParallax == 0) return uv;\n"
    "    float grazing = clamp(abs(vt.z), 0.0, 1.0);\n"     // 1 = head-on
    "    float nl = mix(24.0, 8.0, grazing);\n"
    "    float ld = 1.0 / nl;\n"
    "    vec2 dUV = vt.xy * (heightScale / nl);\n"           // offset limiting
    "    float cur = 0.0;\n"
    "    float dep = texture(heightMap, uv).r;\n"
    "    for (int i = 0; i < 32; i++) {\n"
    "        if (cur >= dep) break;\n"
    "        uv -= dUV;\n"
    "        dep = texture(heightMap, uv).r;\n"
    "        cur += ld;\n"
    "    }\n"
    "    vec2 prev = uv + dUV;\n"
    "    float a = dep - cur;\n"
    "    float b = texture(heightMap, prev).r - cur + ld;\n"
    "    return mix(uv, prev, clamp(a / (a - b), 0.0, 1.0));\n"
    "}\n"
    // whiteout-blend triplanar normal mapping (Ben Golus). Per-plane tangent
    // normals are swizzled into world orientation and blended.
    "vec3 TriplanarNormal(vec2 uvX, vec2 uvY, vec2 uvZ, vec3 bw, vec3 N)\n"
    "{\n"
    "    vec3 nx = texture(normalMap, uvX).xyz * 2.0 - 1.0; nx.xy *= normalStrength;\n"
    "    vec3 ny = texture(normalMap, uvY).xyz * 2.0 - 1.0; ny.xy *= normalStrength;\n"
    "    vec3 nz = texture(normalMap, uvZ).xyz * 2.0 - 1.0; nz.xy *= normalStrength;\n"
    "    vec3 s = sign(N);\n"
    "    nx.z *= s.x; ny.z *= s.y; nz.z *= s.z;\n"
    "    return normalize(nx.zyx * bw.x + ny.xzy * bw.y + nz.xyz * bw.z);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(vNormal);\n"
    "    vec3 V = normalize(viewPos - vFragPos);\n"
    "    vec2 uv = vTexCoord * uvTiling + uvOffset;\n"
    "    vec4 base;\n"
    "    if (triplanar == 1) {\n"
    "        vec3 bw = TriBlend(N);\n"
    "        vec3 s = sign(N);\n"                            // face direction per axis
    "        vec3 wp = vFragPos / triplanarScale;\n"
    // per-plane view frame: (u-axis, v-axis, out) world components. The `out`
    // component carries the face sign so it is > 0 for the visible side.
    "        vec2 uvX = TriPOM(wp.zy + uvOffset, vec3(V.z, V.y, V.x * s.x));\n"
    "        vec2 uvY = TriPOM(wp.xz + uvOffset, vec3(V.x, V.z, V.y * s.y));\n"
    "        vec2 uvZ = TriPOM(wp.xy + uvOffset, vec3(V.x, V.y, V.z * s.z));\n"
    "        base = (texture(sampleTex, uvX) * bw.x + texture(sampleTex, uvY) * bw.y\n"
    "              + texture(sampleTex, uvZ) * bw.z) * vColor;\n"
    "        if (useNormalMap == 1) N = TriplanarNormal(uvX, uvY, uvZ, bw, N);\n"
    "    } else {\n"
    "        if (useNormalMap == 1 || useParallax == 1) {\n"
    "            mat3 TBN = CotangentFrame(N);\n"
    "            if (useParallax == 1) {\n"
    "                vec3 viewTS = normalize(transpose(TBN) * V);\n"
    "                uv = ParallaxMapping(uv, viewTS);\n"
    "            }\n"
    "            if (useNormalMap == 1) N = ApplyNormalMap(TBN, uv);\n"
    "        }\n"
    "        base = texture(sampleTex, uv) * vColor;\n"
    "    }\n"
    "    base.rgb *= matDiffuse;\n"                               // ...times the diffuse-map gain
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
    "        float sa;\n"
    "        if (blinn == 1) {\n"
    "            vec3 H = normalize(L + V);\n"
    "            sa = max(dot(N, H), 0.0);\n"
    "        } else {\n"
    "            vec3 R = reflect(-L, N);\n"
    "            sa = max(dot(V, R), 0.0);\n"
    "        }\n"
    "        float spec = (diff > 0.0) ? pow(sa, max(shininess, 1.0)) : 0.0;\n"
    "        vec3 amb = lights[i].ambient  * lights[i].color;\n"
    "        vec3 dif = lights[i].diffuse  * diff * lights[i].color;\n"
    "        vec3 spc = lights[i].specular * matSpecular * spec * lights[i].color * matSpecularColor;\n"
    "        float sh = 0.0;\n"                                   // only lights[0] casts
    "        if (i == 0) sh = (shadowsEnabled == 1) ? ShadowFactor(N, L) : PointShadowFactor(vFragPos);\n"
    "        lit += amb + (dif + spc) * atten * intensity * (1.0 - sh);\n"
    "    }\n"
    "    FragColor = vec4(lit * base.rgb, base.a);\n"
    "}\n";

static Shader s_shader = { 0 };
static bool s_loaded = false;
static bool s_active = false;
static LightingModel s_model = LIGHTING_BLINN_PHONG;

void Mge_SetLightingModel(LightingModel model)
{
    s_model = model;
}

LightingModel Mge_GetLightingModel(void)
{
    return s_model;
}

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
    MgeGL_Uniform1i("sampleTex", 0);      // diffuse
    MgeGL_Uniform1i("shadowMap", 1);      // directional/spot depth (2D)
    MgeGL_Uniform1i("pointShadowMap", 2); // point-light depth (cube); distinct units per sampler
    MgeGL_Uniform1i("normalMap", 3);      // tangent-space normal map
    MgeGL_Uniform1i("useNormalMap", 0);   // Mge_SetMaterial / Mge_DrawMesh turn this on
    MgeGL_Uniform1f("normalStrength", 1.0f);
    MgeGL_Uniform1i("heightMap", 4);      // parallax height map -> unit 4
    MgeGL_Uniform1i("useParallax", 0);    // Mge_SetMaterial turns this on
    MgeGL_Uniform1f("heightScale", 0.08f);
    MgeGL_Uniform2fv("uvTiling", (Vector2){ 1.0f, 1.0f });
    MgeGL_Uniform2fv("uvOffset", (Vector2){ 0.0f, 0.0f });
    MgeGL_Uniform1i("triplanar", 0);
    MgeGL_Uniform1f("triplanarScale", 1.0f);
    MgeGL_Uniform1f("matSpecular", 1.0f);
    MgeGL_Uniform3fv("matSpecularColor", (Vector3){ 1.0f, 1.0f, 1.0f });
    MgeGL_Uniform1f("matDiffuse", 1.0f);
    MgeGL_Uniform1f("shininess", 32.0f);
    MgeGL_Uniform1i("blinn", (s_model == LIGHTING_PHONG) ? 0 : 1);
    MgeGL_Uniform1i("shadowsEnabled", 0);      // Mge_BeginLighting3DShadowed turns this on
    MgeGL_Uniform1i("pointShadowEnabled", 0);  // Mge_BeginLighting3DPointShadowed turns this on
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

    // MATERIAL_MAP_NORMAL: per-pixel normals on unit 3 when the slot has a texture
    unsigned int normalId = material.maps[MATERIAL_MAP_NORMAL].texture.id;
    MgeGL_SetTextureSlot(3, normalId);
    MgeGL_Uniform1i("useNormalMap", (normalId != 0) ? 1 : 0);
    MgeGL_Uniform1f("normalStrength", material.maps[MATERIAL_MAP_NORMAL].value);

    // MATERIAL_MAP_HEIGHT: parallax-occlusion mapping on unit 4 when the slot is set
    unsigned int heightId = material.maps[MATERIAL_MAP_HEIGHT].texture.id;
    MgeGL_SetTextureSlot(4, heightId);
    MgeGL_Uniform1i("useParallax", (heightId != 0) ? 1 : 0);
    MgeGL_Uniform1f("heightScale", material.maps[MATERIAL_MAP_HEIGHT].value);

    // UV transform + triplanar projection. A raw (Material){...} literal leaves
    // tiling {0,0}; treat a zero axis as 1 so the texture doesn't collapse.
    Vector2 tiling = {
        (material.tiling.x != 0.0f) ? material.tiling.x : 1.0f,
        (material.tiling.y != 0.0f) ? material.tiling.y : 1.0f,
    };
    MgeGL_Uniform2fv("uvTiling", tiling);
    MgeGL_Uniform2fv("uvOffset", material.offset);
    MgeGL_Uniform1i("triplanar", material.triplanar ? 1 : 0);
    MgeGL_Uniform1f("triplanarScale", (material.triplanarScale > 1e-4f) ? material.triplanarScale : 1.0f);

    Color sc = material.maps[MATERIAL_MAP_SPECULAR].color;
    MgeGL_Uniform1f("matSpecular", material.maps[MATERIAL_MAP_SPECULAR].value);
    MgeGL_Uniform3fv("matSpecularColor",
        (Vector3){ sc.r / 255.0f, sc.g / 255.0f, sc.b / 255.0f });
    MgeGL_Uniform1f("matDiffuse", material.maps[MATERIAL_MAP_DIFFUSE].value);
    MgeGL_Uniform1f("shininess", material.shininess);
}

void Mge_EndLighting3D(void)
{
    if (!s_active)
        return;

    s_active = false;
    MgeGL_SetTexture(MgeGL_GetWhiteTexture()); // don't leak a material texture into unlit draws
    MgeGL_SetTextureSlot(3, 0);                // ...or a normal map
    MgeGL_SetTextureSlot(4, 0);                // ...or a height map
    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}
