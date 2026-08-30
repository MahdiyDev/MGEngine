// Light constructors + the uniform wiring in Mge_BeginLighting3D(Ex).
// No window / GL context: the MgeGL_* backend is stubbed and records every
// uniform the lighting code pushes so we can assert on names and values.

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_gl.h"
#include "test.h" // mlib repo-wide harness

static bool feq(float a, float b) { return fabsf(a - b) < 1e-4f; }

// ---- recording stub backend ----

typedef struct {
    char name[64];
    int kind; // 0 = float, 1 = int, 2 = vec3
    float f;
    int i;
    Vector3 v;
} Rec;

static Rec g_rec[512];
static int g_recCount;

static void rec_reset(void) { g_recCount = 0; }
static Rec* rec_find(const char* name)
{
    for (int k = g_recCount - 1; k >= 0; k--) // last write wins
        if (strcmp(g_rec[k].name, name) == 0)
            return &g_rec[k];
    return NULL;
}
static float rec_f(const char* name) { Rec* r = rec_find(name); return r ? r->f : -12345.0f; }
static int rec_i(const char* name) { Rec* r = rec_find(name); return r ? r->i : -12345; }
static Vector3 rec_v(const char* name) { Rec* r = rec_find(name); return r ? r->v : (Vector3){ -1, -1, -1 }; }
static bool rec_has(const char* name) { return rec_find(name) != NULL; }

Shader Mge_LoadShaderFromMemory(const char* vs, const char* fs)
{
    (void)vs;
    (void)fs;
    return (Shader){ .id = 42, .locs = NULL };
}
void MgeGL_SetShader(unsigned int id) { (void)id; }
void MgeGL_SetTexture(unsigned int id) { (void)id; }
void MgeGL_Draw(void) {}
unsigned int MgeGL_GetWhiteTexture(void) { return 1; }
unsigned int MgeGL_GetDefaultShaderId(void) { return 0; }

void MgeGL_Uniform1f(const char* name, float value)
{
    Rec* r = &g_rec[g_recCount++];
    snprintf(r->name, sizeof(r->name), "%s", name);
    r->kind = 0;
    r->f = value;
}
void MgeGL_Uniform1i(const char* name, const int value)
{
    Rec* r = &g_rec[g_recCount++];
    snprintf(r->name, sizeof(r->name), "%s", name);
    r->kind = 1;
    r->i = value;
}
void MgeGL_Uniform3fv(const char* name, Vector3 value)
{
    Rec* r = &g_rec[g_recCount++];
    snprintf(r->name, sizeof(r->name), "%s", name);
    r->kind = 2;
    r->v = value;
}

// ---- constructor tests ----

TEST(directional_light_defaults)
{
    Light l = Mge_MakeDirectionalLight((Vector3){ -1, -2, -1 }, (Vector3){ 1, 1, 0.9f });
    CHECK(l.type == LIGHT_DIRECTIONAL);
    CHECK(l.enabled);
    CHECK(feq(l.direction.y, -2.0f));
    CHECK(l.position.x == 0.0f && l.position.y == 0.0f && l.position.z == 0.0f);
    CHECK(feq(l.ambient, 0.12f));
    CHECK(feq(l.constant, 1.0f)); // divisor stays sane even though unused
}

TEST(point_light_defaults)
{
    Light l = Mge_MakePointLight((Vector3){ 3, 4, 5 }, (Vector3){ 1, 1, 1 });
    CHECK(l.type == LIGHT_POINT);
    CHECK(feq(l.position.z, 5.0f));
    CHECK(feq(l.ambient, 0.0f));
    CHECK(feq(l.constant, 1.0f));
    CHECK(l.linear > 0.0f && l.quadratic > 0.0f); // real distance falloff
}

TEST(spot_light_cutoffs)
{
    Light l = Mge_MakeSpotLight((Vector3){ 0, 5, 0 }, (Vector3){ 0, -1, 0 },
        (Vector3){ 1, 1, 1 }, 12.5f, 25.0f);
    CHECK(l.type == LIGHT_SPOT);
    CHECK(feq(l.position.y, 5.0f));
    CHECK(feq(l.direction.y, -1.0f));
    CHECK(feq(l.innerCutoff, cosf(12.5f * (float)DEG2RAD)));
    CHECK(feq(l.outerCutoff, cosf(25.0f * (float)DEG2RAD)));
    // wider angle -> smaller cosine; a soft edge means inner sits above outer
    CHECK(l.innerCutoff > l.outerCutoff);
    // inherits point-light attenuation
    CHECK(l.linear > 0.0f);
}

TEST(flashlight_tracks_camera)
{
    Camera3D cam = { .position = { 2, 1, 7 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    Light l = Mge_MakeFlashlight(cam, (Vector3){ 1, 1, 1 });
    CHECK(l.type == LIGHT_SPOT);
    CHECK(feq(l.position.x, 2.0f) && feq(l.position.z, 7.0f));
    CHECK(feq(l.direction.z, -1.0f));
    CHECK(feq(l.innerCutoff, cosf(12.5f * (float)DEG2RAD)));
    CHECK(l.innerCutoff > l.outerCutoff);
}

TEST(legacy_make_light_has_no_falloff)
{
    Light l = Mge_MakeLight((Vector3){ 1, 2, 3 }, (Vector3){ 1, 1, 1 });
    CHECK(l.type == LIGHT_POINT);
    CHECK(feq(l.ambient, 0.15f));
    CHECK(feq(l.linear, 0.0f) && feq(l.quadratic, 0.0f));
    CHECK(feq(l.constant, 1.0f));
}

// ---- Mge_BeginLighting3D(Ex) uniform wiring ----

TEST(begin_lighting_ex_uploads_every_light)
{
    Camera3D cam = { .position = { 0, 3, 9 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    Light lights[3] = {
        Mge_MakeDirectionalLight((Vector3){ -1, -1, 0 }, (Vector3){ 1, 1, 1 }),
        Mge_MakePointLight((Vector3){ 4, 5, 6 }, (Vector3){ 1, 0.5f, 0.25f }),
        Mge_MakeSpotLight((Vector3){ 0, 8, 0 }, (Vector3){ 0, -1, 0 }, (Vector3){ 1, 1, 1 }, 10.0f, 20.0f),
    };

    rec_reset();
    Mge_BeginLighting3DEx(lights, 3, cam);

    CHECK(rec_i("lightCount") == 3);
    CHECK(feq(rec_f("matSpecular"), 1.0f));
    CHECK(feq(rec_f("shininess"), 32.0f));

    Vector3 vp = rec_v("viewPos");
    CHECK(feq(vp.z, 9.0f));

    CHECK(rec_i("lights[0].type") == LIGHT_DIRECTIONAL);
    CHECK(rec_i("lights[1].type") == LIGHT_POINT);
    CHECK(rec_i("lights[2].type") == LIGHT_SPOT);
    CHECK(rec_i("lights[2].enabled") == 1);

    Vector3 p1 = rec_v("lights[1].position");
    CHECK(feq(p1.x, 4.0f) && feq(p1.y, 5.0f) && feq(p1.z, 6.0f));

    CHECK(rec_f("lights[1].quadratic") > 0.0f);
    CHECK(feq(rec_f("lights[2].innerCutoff"), cosf(10.0f * (float)DEG2RAD)));

    Mge_EndLighting3D();
}

TEST(begin_lighting_single_form_is_one_light)
{
    Camera3D cam = { .position = { 0, 0, 5 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    rec_reset();
    Mge_BeginLighting3D(Mge_MakePointLight((Vector3){ 1, 1, 1 }, (Vector3){ 1, 1, 1 }), cam);
    CHECK(rec_i("lightCount") == 1);
    CHECK(rec_has("lights[0].type"));
    CHECK(!rec_has("lights[1].type"));
    Mge_EndLighting3D();
}

TEST(begin_lighting_clamps_to_max)
{
    Camera3D cam = { .position = { 0, 0, 5 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    Light many[MGE_MAX_LIGHTS + 5];
    for (int i = 0; i < MGE_MAX_LIGHTS + 5; i++)
        many[i] = Mge_MakePointLight((Vector3){ (float)i, 0, 0 }, (Vector3){ 1, 1, 1 });

    rec_reset();
    Mge_BeginLighting3DEx(many, MGE_MAX_LIGHTS + 5, cam);

    CHECK(rec_i("lightCount") == MGE_MAX_LIGHTS);

    char last[64], past[64];
    snprintf(last, sizeof(last), "lights[%d].type", MGE_MAX_LIGHTS - 1);
    snprintf(past, sizeof(past), "lights[%d].type", MGE_MAX_LIGHTS);
    CHECK(rec_has(last));
    CHECK(!rec_has(past)); // nothing written past the shader's array bound

    Mge_EndLighting3D();
}

TEST(lighting_model_defaults_to_blinn_and_toggles)
{
    Camera3D cam = { .position = { 0, 0, 5 }, .target = { 0, 0, -1 }, .up = { 0, 1, 0 },
        .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    Light l = Mge_MakePointLight((Vector3){ 1, 1, 1 }, (Vector3){ 1, 1, 1 });

    CHECK(Mge_GetLightingModel() == LIGHTING_BLINN_PHONG); // default

    rec_reset();
    Mge_BeginLighting3D(l, cam);
    CHECK(rec_i("blinn") == 1); // halfway vector
    Mge_EndLighting3D();

    Mge_SetLightingModel(LIGHTING_PHONG);
    CHECK(Mge_GetLightingModel() == LIGHTING_PHONG);
    rec_reset();
    Mge_BeginLighting3D(l, cam);
    CHECK(rec_i("blinn") == 0); // classic reflect
    Mge_EndLighting3D();

    Mge_SetLightingModel(LIGHTING_BLINN_PHONG); // leave the default put back
}

int main(void)
{
    RUN(directional_light_defaults);
    RUN(point_light_defaults);
    RUN(spot_light_cutoffs);
    RUN(flashlight_tracks_camera);
    RUN(legacy_make_light_has_no_falloff);
    RUN(begin_lighting_ex_uploads_every_light);
    RUN(begin_lighting_single_form_is_one_light);
    RUN(begin_lighting_clamps_to_max);
    RUN(lighting_model_defaults_to_blinn_and_toggles);
    return test_summary();
}
