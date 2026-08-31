#include "scene_io.h"
#include "pathutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------------------------------------------------------------------ helpers

static const char* PRIM_NAMES[3] = { "cube", "sphere", "plane" };
static const char* LIGHT_NAMES[3] = { "directional", "point", "spot" };

static int name_index(const char* s, const char* const* table, int count, int fallback)
{
    for (int i = 0; i < count; i++)
        if (strcmp(s, table[i]) == 0)
            return i;
    return fallback;
}

// pull the text between the first and last '"' of `line` into `out`
static bool quoted(const char* line, char* out, size_t outSize)
{
    const char* a = strchr(line, '"');
    const char* b = (a != NULL) ? strrchr(line, '"') : NULL;
    if (a == NULL || b == NULL || b <= a)
        return false;
    size_t n = (size_t)(b - a - 1);
    if (n >= outSize)
        n = outSize - 1;
    memcpy(out, a + 1, n);
    out[n] = '\0';
    return true;
}

// ------------------------------------------------------------------ writer

static void wv3(FILE* f, const char* k, Vector3 v)
{
    fprintf(f, "  %s %g %g %g\n", k, (double)v.x, (double)v.y, (double)v.z);
}
static void wv2(FILE* f, const char* k, Vector2 v)
{
    fprintf(f, "  %s %g %g\n", k, (double)v.x, (double)v.y);
}
static void wf(FILE* f, const char* k, float v) { fprintf(f, "  %s %g\n", k, (double)v); }
static void wi(FILE* f, const char* k, int v) { fprintf(f, "  %s %d\n", k, v); }

// Scene name: the folder holding it when the file is the canonical
// `scene.mgscene` (project layout), otherwise the file's own stem.
static void scene_name_for(const char* path, char* out, size_t outSize)
{
    char base[64];
    Path_Base(path, base, sizeof(base));
    Path_StripExt(base);
    if (strcmp(base, "scene") == 0) {
        char dir[512];
        Path_Dir(path, dir, sizeof(dir));
        if (dir[0] != '\0') {
            Path_Base(dir, out, outSize);
            return;
        }
    }
    snprintf(out, outSize, "%s", base);
}

// Resolve `stored` (absolute, or relative to `base`) to a path we can read now.
static void resolve_tex(const char* stored, const char* base, char* out, size_t n)
{
    if (stored[0] == '\0')
        out[0] = '\0';
    else if (Path_IsAbsolute(stored) || base[0] == '\0')
        snprintf(out, n, "%s", stored);
    else
        Path_Join(base, stored, out, n);
}

bool Scene_Save(Scene* s, const char* path, Camera3D camera, const char* projectRoot)
{
    char dir[512];
    Path_Dir(path, dir, sizeof(dir));

    const char* root = (projectRoot != NULL) ? projectRoot : "";

    // copy any outside textures into <root>/res/ and store them root-relative
    if (root[0] != '\0') {
        char res[600];
        Path_Join(root, "res", res, sizeof(res));
        Path_MakeDirs(res);

        for (int i = 0; i < s->objectCount; i++) {
            for (int m = 0; m < MATERIAL_MAP_COUNT; m++) {
                char* stored = s->texPath[i][m];
                if (stored[0] == '\0')
                    continue;

                char src[1024];
                resolve_tex(stored, root, src, sizeof(src));

                char base[SCENE_TEXPATH_LEN - 8];
                Path_Base(stored, base, sizeof(base));
                char rel[SCENE_TEXPATH_LEN];
                snprintf(rel, sizeof(rel), "res/%s", base);

                char dst[1024];
                Path_Join(root, rel, dst, sizeof(dst));

                if (!Path_Equal(src, dst)) // already the project copy -> just normalise
                    Path_CopyFile(src, dst);
                snprintf(stored, SCENE_TEXPATH_LEN, "%s", rel);
            }
        }
    }

    FILE* f = fopen(path, "wb");
    if (f == NULL)
        return false;

    scene_name_for(path, s->name, sizeof(s->name));

    fprintf(f, "mgescene 1\n");
    fprintf(f, "name \"%s\"\n\n", s->name);

    fprintf(f, "camera\n");
    wv3(f, "position", camera.position);
    wv3(f, "target", camera.target);
    wv3(f, "up", camera.up);
    wf(f, "fov", camera.fovy);
    fprintf(f, "\n");

    fprintf(f, "render\n");
    wi(f, "shadows", s->shadowsOn ? 1 : 0);
    wv3(f, "shadowCenter", s->shadowCenter);
    wf(f, "shadowRadius", s->shadowRadius);
    wi(f, "hdr", s->hdrOn ? 1 : 0);
    wi(f, "tonemap", s->toneMap);
    wf(f, "exposure", s->exposure);
    wi(f, "bloom", s->bloomOn ? 1 : 0);
    wf(f, "bloomThreshold", s->bloom.threshold);
    wf(f, "bloomIntensity", s->bloom.intensity);
    wi(f, "msaa", Mge_IsMSAAEnabled() ? 1 : 0);
    fprintf(f, "\n");

    for (int i = 0; i < s->objectCount; i++) {
        const Object* o = &s->objects[i];
        fprintf(f, "object \"%s\"\n", s->objectNames[i]);
        fprintf(f, "  primitive %s\n", PRIM_NAMES[o->primitive % 3]);
        wi(f, "active", o->active ? 1 : 0);
        wv3(f, "position", o->transform.position);
        wv3(f, "rotation", o->transform.rotation);
        wv3(f, "scale", o->transform.scale);
        wf(f, "shininess", o->material.shininess);
        wv2(f, "tiling", o->material.tiling);
        wv2(f, "offset", o->material.offset);
        wi(f, "triplanar", o->material.triplanar ? 1 : 0);
        wf(f, "triplanarScale", o->material.triplanarScale);
        for (int m = 0; m < MATERIAL_MAP_COUNT; m++) {
            Color c = o->material.maps[m].color;
            fprintf(f, "  m%d.color %d %d %d %d\n", m, c.r, c.g, c.b, c.a);
            fprintf(f, "  m%d.value %g\n", m, (double)o->material.maps[m].value);
            fprintf(f, "  m%d.wrap %d\n", m, s->texWrap[i][m]);
            if (s->texPath[i][m][0] != '\0')
                fprintf(f, "  m%d.texture \"%s\"\n", m, s->texPath[i][m]);
        }
        fprintf(f, "\n");
    }

    for (int i = 0; i < s->lightCount; i++) {
        const Light* l = &s->lights[i];
        fprintf(f, "light \"%s\"\n", s->lightNames[i]);
        fprintf(f, "  type %s\n", LIGHT_NAMES[l->type % 3]);
        wi(f, "enabled", l->enabled ? 1 : 0);
        wv3(f, "position", l->position);
        wv3(f, "direction", l->direction);
        wv3(f, "color", l->color);
        wf(f, "ambient", l->ambient);
        wf(f, "diffuse", l->diffuse);
        wf(f, "specular", l->specular);
        wf(f, "constant", l->constant);
        wf(f, "linear", l->linear);
        wf(f, "quadratic", l->quadratic);
        wf(f, "innerCutoff", l->innerCutoff);
        wf(f, "outerCutoff", l->outerCutoff);
        fprintf(f, "\n");
    }

    fclose(f);

    snprintf(s->path, sizeof(s->path), "%s", path);
    s->dirty = false;

    // scaffold a scene-code template once (Phase 4 compiles it)
    char code[600];
    char codeName[160];
    snprintf(codeName, sizeof(codeName), "%s.c", s->name);
    Path_Join(dir, codeName, code, sizeof(code));
    FILE* cf = fopen(code, "rb");
    if (cf != NULL) {
        fclose(cf);
    } else {
        cf = fopen(code, "wb");
        if (cf != NULL) {
            fprintf(cf,
                "// %s -- scene logic. Every .c in this folder is compiled into the scene's\n"
                "// module, which the editor hot-reloads when you edit it. The editor owns\n"
                "// object / light storage; reach it through the MgeSceneCtx.\n"
                "#include <mge.h>\n"
                "#include <mge_math.h>\n\n"
                "void MgeScene_Init(MgeSceneCtx* ctx) { (void)ctx; }\n\n"
                "void MgeScene_Update(MgeSceneCtx* ctx, float dt)\n"
                "{\n"
                "    // example: spin every object\n"
                "    for (int i = 0; i < *ctx->objectCount; i++)\n"
                "        ctx->objects[i].transform.rotation.y += 40.0f * dt;\n"
                "}\n\n"
                "void MgeScene_Shutdown(MgeSceneCtx* ctx) { (void)ctx; }\n",
                s->name);
            fclose(cf);
        }
    }

    return true;
}

// ------------------------------------------------------------------ parser

static void clear_entities(Scene* s)
{
    memset(s->objects, 0, sizeof(s->objects));
    memset(s->objectNames, 0, sizeof(s->objectNames));
    memset(s->texWrap, 0, sizeof(s->texWrap));
    memset(s->texPath, 0, sizeof(s->texPath));
    memset(s->lights, 0, sizeof(s->lights));
    memset(s->lightNames, 0, sizeof(s->lightNames));
    s->objectCount = 0;
    s->lightCount = 0;
    s->selKind = SEL_NONE;
    s->selIndex = 0;
}

enum { SEC_TOP, SEC_CAMERA, SEC_RENDER, SEC_OBJECT, SEC_LIGHT };

bool Scene_Load(Scene* s, const char* path, Camera3D* outCamera)
{
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return false;

    char line[1024];
    if (fgets(line, sizeof(line), f) == NULL || strncmp(line, "mgescene", 8) != 0) {
        fclose(f);
        return false;
    }

    clear_entities(s);

    Camera3D cam = { .up = { 0, 1, 0 }, .fovy = 60.0f, .projection = CAMERA_PERSPECTIVE };
    int sec = SEC_TOP;
    Object* obj = NULL;   // current object being filled
    Light* lit = NULL;    // current light
    int objIdx = -1;

    while (fgets(line, sizeof(line), f) != NULL) {
        char* p = strchr(line, '#'); // strip comments
        if (p != NULL)
            *p = '\0';
        // trim
        char* a = line;
        while (*a == ' ' || *a == '\t' || *a == '\r' || *a == '\n')
            a++;
        size_t len = strlen(a);
        while (len > 0 && (a[len - 1] == ' ' || a[len - 1] == '\t' || a[len - 1] == '\r' || a[len - 1] == '\n'))
            a[--len] = '\0';
        if (a[0] == '\0')
            continue;

        char key[64] = { 0 };
        sscanf(a, "%63s", key);
        const char* rest = a + strlen(key);
        while (*rest == ' ' || *rest == '\t')
            rest++;

        // --- section headers ---
        if (strcmp(key, "name") == 0 && sec == SEC_TOP) {
            char nm[64];
            if (quoted(a, nm, sizeof(nm)))
                snprintf(s->name, sizeof(s->name), "%s", nm);
            continue;
        }
        if (strcmp(key, "camera") == 0) { sec = SEC_CAMERA; continue; }
        if (strcmp(key, "render") == 0) { sec = SEC_RENDER; continue; }
        if (strcmp(key, "object") == 0) {
            if (s->objectCount >= SCENE_MAX_OBJECTS) { obj = NULL; sec = SEC_OBJECT; continue; }
            objIdx = s->objectCount++;
            obj = &s->objects[objIdx];
            *obj = Mge_MakeShape3D(PRIM_CUBE, (Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 }, (Color){ 255, 255, 255, 255 });
            quoted(a, s->objectNames[objIdx], sizeof(s->objectNames[objIdx]));
            sec = SEC_OBJECT;
            continue;
        }
        if (strcmp(key, "light") == 0) {
            if (s->lightCount >= SCENE_MAX_LIGHTS) { lit = NULL; sec = SEC_LIGHT; continue; }
            int li = s->lightCount++;
            lit = &s->lights[li];
            *lit = Mge_MakePointLight((Vector3){ 0, 0, 0 }, (Vector3){ 1, 1, 1 });
            quoted(a, s->lightNames[li], sizeof(s->lightNames[li]));
            sec = SEC_LIGHT;
            continue;
        }

        // --- fields ---
        float x, y, z, w;
        int iv;

        if (sec == SEC_CAMERA) {
            if (sscanf(rest, "%f %f %f", &x, &y, &z) == 3) {
                if (strcmp(key, "position") == 0) cam.position = (Vector3){ x, y, z };
                else if (strcmp(key, "target") == 0) cam.target = (Vector3){ x, y, z };
                else if (strcmp(key, "up") == 0) cam.up = (Vector3){ x, y, z };
            } else if (strcmp(key, "fov") == 0) {
                cam.fovy = (float)atof(rest);
            }
            continue;
        }
        if (sec == SEC_RENDER) {
            if (strcmp(key, "shadows") == 0) s->shadowsOn = atoi(rest) != 0;
            else if (strcmp(key, "shadowCenter") == 0 && sscanf(rest, "%f %f %f", &x, &y, &z) == 3)
                s->shadowCenter = (Vector3){ x, y, z };
            else if (strcmp(key, "shadowRadius") == 0) s->shadowRadius = (float)atof(rest);
            else if (strcmp(key, "hdr") == 0) s->hdrOn = atoi(rest) != 0;
            else if (strcmp(key, "tonemap") == 0) s->toneMap = atoi(rest);
            else if (strcmp(key, "exposure") == 0) s->exposure = (float)atof(rest);
            else if (strcmp(key, "bloom") == 0) s->bloomOn = atoi(rest) != 0;
            else if (strcmp(key, "bloomThreshold") == 0) s->bloom.threshold = (float)atof(rest);
            else if (strcmp(key, "bloomIntensity") == 0) s->bloom.intensity = (float)atof(rest);
            else if (strcmp(key, "msaa") == 0) Mge_SetMSAAEnabled(atoi(rest) != 0);
            continue;
        }
        if (sec == SEC_OBJECT && obj != NULL) {
            if (strcmp(key, "primitive") == 0)
                obj->primitive = (PrimitiveKind)name_index(rest, PRIM_NAMES, 3, PRIM_CUBE);
            else if (strcmp(key, "active") == 0)
                obj->active = atoi(rest) != 0;
            else if (strcmp(key, "position") == 0 && sscanf(rest, "%f %f %f", &x, &y, &z) == 3)
                obj->transform.position = (Vector3){ x, y, z };
            else if (strcmp(key, "rotation") == 0 && sscanf(rest, "%f %f %f", &x, &y, &z) == 3)
                obj->transform.rotation = (Vector3){ x, y, z };
            else if (strcmp(key, "scale") == 0 && sscanf(rest, "%f %f %f", &x, &y, &z) == 3)
                obj->transform.scale = (Vector3){ x, y, z };
            else if (strcmp(key, "shininess") == 0)
                obj->material.shininess = (float)atof(rest);
            else if (strcmp(key, "tiling") == 0 && sscanf(rest, "%f %f", &x, &y) == 2)
                obj->material.tiling = (Vector2){ x, y };
            else if (strcmp(key, "offset") == 0 && sscanf(rest, "%f %f", &x, &y) == 2)
                obj->material.offset = (Vector2){ x, y };
            else if (strcmp(key, "triplanar") == 0)
                obj->material.triplanar = atoi(rest) != 0;
            else if (strcmp(key, "triplanarScale") == 0)
                obj->material.triplanarScale = (float)atof(rest);
            else if (key[0] == 'm' && key[1] >= '0' && key[1] <= '3' && key[2] == '.') {
                int mi = key[1] - '0';
                const char* sub = key + 3;
                if (strcmp(sub, "color") == 0 && sscanf(rest, "%f %f %f %f", &x, &y, &z, &w) == 4)
                    obj->material.maps[mi].color = (Color){ (unsigned char)x, (unsigned char)y, (unsigned char)z, (unsigned char)w };
                else if (strcmp(sub, "value") == 0)
                    obj->material.maps[mi].value = (float)atof(rest);
                else if (strcmp(sub, "wrap") == 0)
                    s->texWrap[objIdx][mi] = (unsigned char)atoi(rest);
                else if (strcmp(sub, "texture") == 0)
                    quoted(a, s->texPath[objIdx][mi], SCENE_TEXPATH_LEN);
            }
            continue;
        }
        if (sec == SEC_LIGHT && lit != NULL) {
            if (strcmp(key, "type") == 0)
                lit->type = (LightType)name_index(rest, LIGHT_NAMES, 3, LIGHT_POINT);
            else if (strcmp(key, "enabled") == 0) lit->enabled = atoi(rest) != 0;
            else if (strcmp(key, "position") == 0 && sscanf(rest, "%f %f %f", &x, &y, &z) == 3)
                lit->position = (Vector3){ x, y, z };
            else if (strcmp(key, "direction") == 0 && sscanf(rest, "%f %f %f", &x, &y, &z) == 3)
                lit->direction = (Vector3){ x, y, z };
            else if (strcmp(key, "color") == 0 && sscanf(rest, "%f %f %f", &x, &y, &z) == 3)
                lit->color = (Vector3){ x, y, z };
            else if (strcmp(key, "ambient") == 0) lit->ambient = (float)atof(rest);
            else if (strcmp(key, "diffuse") == 0) lit->diffuse = (float)atof(rest);
            else if (strcmp(key, "specular") == 0) lit->specular = (float)atof(rest);
            else if (strcmp(key, "constant") == 0) lit->constant = (float)atof(rest);
            else if (strcmp(key, "linear") == 0) lit->linear = (float)atof(rest);
            else if (strcmp(key, "quadratic") == 0) lit->quadratic = (float)atof(rest);
            else if (strcmp(key, "innerCutoff") == 0) lit->innerCutoff = (float)atof(rest);
            else if (strcmp(key, "outerCutoff") == 0) lit->outerCutoff = (float)atof(rest);
            (void)iv;
            continue;
        }
    }

    fclose(f);

    if (s->lightCount == 0) { // a scene must keep a sun for the shadow pass
        s->lights[0] = Mge_MakeDirectionalLight((Vector3){ -0.5f, -1.0f, -0.4f }, (Vector3){ 0.7f, 0.7f, 0.8f });
        snprintf(s->lightNames[0], sizeof(s->lightNames[0]), "Sun");
        s->lightCount = 1;
    }

    snprintf(s->path, sizeof(s->path), "%s", path);
    scene_name_for(path, s->name, sizeof(s->name)); // folder wins for `scene.mgscene`
    s->dirty = false;
    if (outCamera != NULL)
        *outCamera = cam;
    return true;
}
