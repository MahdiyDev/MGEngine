#include "inspector.h"
#include "pathutil.h"
#include "scene_build.h" // SceneBuild_FindSDK -- locate the bundled skybox

#include <mge_gui.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char* const WRAP_NAMES[4] = { "Repeat", "Clamp", "Mirror", "Mirror Once" };

// One material-map slot as a group: a texture thumbnail (opens a file picker,
// plus a clear "x"), then the slot's colour, scalar value and wrap mode.
// `showColor` is false for the normal / height maps, where a tint is meaningless.
// `valueMax` / `valueLabel` give the slot's `.value` its slot-specific meaning.
// `wrap` / `texPath` point at the editor's stored state for this slot.
// Returns true if anything changed this frame.
static bool material_slot(Material* mat, int mapIndex, unsigned char* wrap, char* texPath,
    const char* name, const char* id, bool showColor, float valueMax, const char* valueLabel)
{
    MaterialMap* m = &mat->maps[mapIndex];
    char lbl[32];
    bool changed = false;

    Mge_GuiSeparator();
    Mge_GuiLabel(name);

    if (Mge_GuiImageButton(id, m->texture.id, 56.0f)) {
        char* path = Mge_OpenImageDialog();
        if (path != NULL) {
            Mge_UnloadTexture(m->texture);
            m->texture = Mge_LoadTextureEx(path, mapIndex == MATERIAL_MAP_DIFFUSE);
            Mge_SetTextureWrap(m->texture, *wrap); // re-apply the chosen wrap
            snprintf(texPath, SCENE_TEXPATH_LEN, "%s", path);
            free(path);
            changed = true;
        }
    }
    if (m->texture.id != 0) {
        Mge_GuiSameLine();
        snprintf(lbl, sizeof(lbl), "x##%s", id);
        if (Mge_GuiButton(lbl)) {
            Mge_UnloadTexture(m->texture);
            m->texture = (Texture2D){ 0 };
            texPath[0] = '\0';
            changed = true;
        }
    }

    if (showColor) {
        snprintf(lbl, sizeof(lbl), "color##%s", id);
        changed |= Mge_GuiInputColor(lbl, &m->color);
    }
    snprintf(lbl, sizeof(lbl), "%s##%s", valueLabel, id);
    changed |= Mge_GuiSliderFloat(lbl, &m->value, 0.0f, valueMax);

    int w = *wrap;
    snprintf(lbl, sizeof(lbl), "wrap##%s", id);
    if (Mge_GuiCombo(lbl, &w, WRAP_NAMES, 4)) {
        *wrap = (unsigned char)w;
        Mge_SetTextureWrap(m->texture, w);
        changed = true;
    }
    return changed;
}

// A camera object: transform only. Position + rotation drive the game view when
// this camera is the scene's main camera (Environment > main camera).
static void inspect_camera(Scene* s, Project* proj)
{
    Object* o = &s->objects[s->selIndex];
    bool ch = false;

    Mge_GuiLabel("camera");
    ch |= Mge_GuiCheckbox("active", &o->active);
    ch |= Mge_GuiInputVec3("position", &o->transform.position);
    ch |= Mge_GuiInputVec3("rotation (pitch, yaw, roll)", &o->transform.rotation);
    Mge_GuiSeparator();

    bool isMain = (s->mainCamera == s->selIndex);
    if (Mge_GuiCheckbox("main camera (runs the game view)", &isMain)) {
        s->mainCamera = isMain ? s->selIndex : -1;
        ch = true;
    }
    Mge_GuiLabel("the editor always uses its own fly-camera.");
    (void)proj;
    if (ch)
        s->dirty = true;
}

static void inspect_object(Scene* s, Project* proj)
{
    Object* o = &s->objects[s->selIndex];
    if (o->kind == OBJECT_CAMERA) {
        inspect_camera(s, proj);
        return;
    }
    unsigned char* wrap = s->texWrap[s->selIndex];
    char (*tex)[SCENE_TEXPATH_LEN] = s->texPath[s->selIndex];
    bool ch = false;

    static const char* prims[3] = { "cube", "sphere", "plane" };
    ch |= Mge_GuiCheckbox("active", &o->active);
    if (o->kind == OBJECT_3D) {
        int prim = o->primitive;
        if (Mge_GuiCombo("primitive", &prim, prims, 3)) {
            o->primitive = (PrimitiveKind)prim;
            ch = true;
        }
    } else {
        Mge_GuiLabel("rect (2D)");
    }
    ch |= Mge_GuiInputVec3("position", &o->transform.position);
    ch |= Mge_GuiInputVec3("rotation", &o->transform.rotation);
    ch |= Mge_GuiInputVec3("size", &o->transform.scale);
    Mge_GuiSeparator();

    Mge_GuiLabel("material");
    ch |= Mge_GuiInputFloat("shininess", &o->material.shininess);
    ch |= Mge_GuiInputVec2("tiling", &o->material.tiling);   // uv * tiling + offset -- repeat without scaling
    ch |= Mge_GuiInputVec2("offset", &o->material.offset);
    ch |= Mge_GuiCheckbox("triplanar", &o->material.triplanar); // project diffuse from world XYZ
    if (o->material.triplanar)
        ch |= Mge_GuiInputFloat("triplanar scale", &o->material.triplanarScale);
    ch |= material_slot(&o->material, MATERIAL_MAP_DIFFUSE, &wrap[MATERIAL_MAP_DIFFUSE], tex[MATERIAL_MAP_DIFFUSE], "diffuse map", "diffuse", true, 2.0f, "gain");
    ch |= material_slot(&o->material, MATERIAL_MAP_SPECULAR, &wrap[MATERIAL_MAP_SPECULAR], tex[MATERIAL_MAP_SPECULAR], "specular map", "specular", true, 1.0f, "strength");
    ch |= material_slot(&o->material, MATERIAL_MAP_NORMAL, &wrap[MATERIAL_MAP_NORMAL], tex[MATERIAL_MAP_NORMAL], "normal map", "normal", false, 4.0f, "strength");
    ch |= material_slot(&o->material, MATERIAL_MAP_HEIGHT, &wrap[MATERIAL_MAP_HEIGHT], tex[MATERIAL_MAP_HEIGHT], "height map (parallax)", "height", false, 0.2f, "scale");

    if (ch)
        s->dirty = true;
}

static void inspect_light(Scene* s)
{
    Light* l = &s->lights[s->selIndex];
    static const char* kinds[3] = { "directional", "point", "spot" };
    bool ch = false;

    Mge_GuiLabel(kinds[l->type]);
    ch |= Mge_GuiCheckbox("enabled", &l->enabled);
    ch |= Mge_GuiInputColorRGB("color", &l->color);
    ch |= Mge_GuiSliderFloat("ambient", &l->ambient, 0.0f, 1.0f);
    ch |= Mge_GuiSliderFloat("diffuse", &l->diffuse, 0.0f, 2.0f);
    ch |= Mge_GuiSliderFloat("specular", &l->specular, 0.0f, 2.0f);
    if (l->type != LIGHT_DIRECTIONAL) {
        Mge_GuiSeparator();
        ch |= Mge_GuiInputVec3("position", &l->position);
        ch |= Mge_GuiInputFloat("linear", &l->linear);
        ch |= Mge_GuiInputFloat("quadratic", &l->quadratic);
    }
    if (l->type != LIGHT_POINT) {
        Mge_GuiSeparator();
        ch |= Mge_GuiInputVec3("direction", &l->direction);
    }

    if (ch)
        s->dirty = true;
}

static const char* const SKY_FACES[6] = { "right", "left", "top", "bottom", "front", "back" };
static char s_skyStatus[160];

static long file_size(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fclose(f);
    return n;
}

// Copy the 6 skybox faces from `srcDir` into <root>/res/skybox/ and point the
// scene at them. `srcDir` is a folder the user picked; it must hold
// right/left/top/bottom/front/back.jpg. A no-op copy when it already IS the
// project's skybox folder.
static void import_skybox(Scene* s, Project* proj, const char* srcDir)
{
    char root[512];
    Project_Root(proj, root, sizeof(root));
    if (root[0] == '\0') {
        snprintf(s_skyStatus, sizeof(s_skyStatus), "save the project first");
        return;
    }

    char dstDir[600];
    Path_Join(root, "res/skybox", dstDir, sizeof(dstDir));

    if (!Path_Equal(srcDir, dstDir)) {
        Path_MakeDirs(dstDir);
        int ok = 0;
        char missing[96] = { 0 };
        for (int i = 0; i < 6; i++) {
            char leaf[16], src[600], dst[700];
            snprintf(leaf, sizeof(leaf), "%s.jpg", SKY_FACES[i]);
            Path_Join(srcDir, leaf, src, sizeof(src));
            Path_Join(dstDir, leaf, dst, sizeof(dst));
            if (Path_CopyFile(src, dst) && file_size(dst) > 0)
                ok++;
            else
                snprintf(missing + strlen(missing), sizeof(missing) - strlen(missing), " %s", leaf);
        }
        if (ok == 6)
            snprintf(s_skyStatus, sizeof(s_skyStatus), "copied 6/6 faces into res/skybox/");
        else
            snprintf(s_skyStatus, sizeof(s_skyStatus), "copied %d/6 -- folder is missing:%s", ok, missing);
    } else {
        snprintf(s_skyStatus, sizeof(s_skyStatus), "using res/skybox/ in place");
    }

    snprintf(s->skyDir, SCENE_TEXPATH_LEN, "res/skybox");
    Scene_LoadSkybox(s, root);
    s->dirty = true;
}

// The Environment pseudo-entity: the sun (lights[0]), the skybox, and which
// camera object drives the built game's view.
static void inspect_environment(Scene* s, Project* proj)
{
    bool ch = false;

    Mge_GuiLabel("SUN (directional light)");
    Light* sun = &s->lights[0];
    ch |= Mge_GuiInputVec3("direction", &sun->direction);
    ch |= Mge_GuiInputColorRGB("color", &sun->color);
    ch |= Mge_GuiSliderFloat("ambient", &sun->ambient, 0.0f, 1.0f);
    ch |= Mge_GuiSliderFloat("diffuse", &sun->diffuse, 0.0f, 2.0f);
    ch |= Mge_GuiSliderFloat("specular", &sun->specular, 0.0f, 2.0f);

    Mge_GuiSeparator();
    Mge_GuiLabel("SKYBOX");
    Mge_GuiLabel(s->skyDir[0] ? s->skyDir : "(none)");
    if (Mge_GuiButton("choose folder...")) {
        char* dir = Mge_OpenFolderDialog("Pick a folder with right/left/top/bottom/front/back.jpg");
        if (dir != NULL) {
            import_skybox(s, proj, dir);
            free(dir);
        }
    }
    Mge_GuiSameLine();
    if (Mge_GuiButton("use engine default")) {
        char sdk[1024];
        if (SceneBuild_FindSDK(sdk, sizeof(sdk))) {
            char dir[1100];
            snprintf(dir, sizeof(dir), "%s/assets/skybox", sdk);
            import_skybox(s, proj, dir);
        } else {
            snprintf(s_skyStatus, sizeof(s_skyStatus), "engine SDK not found (set MGE_ENGINE)");
        }
    }
    Mge_GuiSameLine();
    if (Mge_GuiButton("reload")) {
        char root[512];
        Project_Root(proj, root, sizeof(root));
        Scene_LoadSkybox(s, root);
    }
    if (s_skyStatus[0])
        Mge_GuiLabel(s_skyStatus);

    Mge_GuiSeparator();
    Mge_GuiLabel("MAIN CAMERA (game view)");

    char names[SCENE_MAX_OBJECTS + 1][32];
    const char* ptrs[SCENE_MAX_OBJECTS + 1];
    int camObj[SCENE_MAX_OBJECTS + 1]; // combo row -> object index (-1 for "(none)")
    int count = 0, cur = 0;
    snprintf(names[count], sizeof(names[0]), "(none)");
    ptrs[count] = names[count];
    camObj[count] = -1;
    count++;
    for (int i = 0; i < s->objectCount; i++) {
        if (s->objects[i].kind != OBJECT_CAMERA)
            continue;
        snprintf(names[count], sizeof(names[0]), "%s", s->objectNames[i]);
        ptrs[count] = names[count];
        camObj[count] = i;
        if (i == s->mainCamera)
            cur = count;
        count++;
    }

    if (Mge_GuiCombo("camera", &cur, ptrs, count)) {
        s->mainCamera = camObj[cur];
        ch = true;
    }
    if (count == 1)
        Mge_GuiLabel("add a Camera in the hierarchy (+ add > Camera)");

    if (ch)
        s->dirty = true;
}

void Inspector_Draw(Rectangle rect, Scene* s, Project* proj)
{
    if (!Mge_GuiBeginPanel("Inspector", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }

    Mge_GuiLabel("INSPECTOR");
    Mge_GuiSeparator();

    if (s->selKind == SEL_ENV)
        inspect_environment(s, proj);
    else if (s->selKind == SEL_OBJECT)
        inspect_object(s, proj);
    else if (s->selKind == SEL_LIGHT)
        inspect_light(s);
    else
        Mge_GuiLabel("(nothing selected)");

    Mge_GuiEndPanel();
}
