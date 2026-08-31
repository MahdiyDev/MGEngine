#include "resources.h"
#include "pathutil.h"

#include <mge_gui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------------------------------------------------------------------ helpers

static bool is_image(const char* name)
{
    const char* dot = strrchr(name, '.');
    if (dot == NULL)
        return false;
    char e[8] = { 0 };
    for (int i = 1; dot[i] && i < 7; i++)
        e[i - 1] = (dot[i] >= 'A' && dot[i] <= 'Z') ? dot[i] + 32 : dot[i];
    return strcmp(e, "png") == 0 || strcmp(e, "jpg") == 0 || strcmp(e, "jpeg") == 0 ||
        strcmp(e, "bmp") == 0 || strcmp(e, "tga") == 0 || strcmp(e, "gif") == 0 ||
        strcmp(e, "psd") == 0;
}

static void full_path(const Project* proj, const char* rel, char* out, int outSize)
{
    char root[RES_PATH_LEN];
    Project_Root(proj, root, sizeof(root));
    Path_Join(root, rel, out, (size_t)outSize);
}

// ------------------------------------------------------------------ thumbnails

void Resources_Init(Resources* r)
{
    memset(r, 0, sizeof(*r));
}

static void flush_thumbs(Resources* r)
{
    for (int i = 0; i < r->thumbCount; i++)
        Mge_UnloadTexture(r->thumbs[i].tex);
    r->thumbCount = 0;
}

void Resources_Shutdown(Resources* r)
{
    flush_thumbs(r);
}

static unsigned int thumb_for(Resources* r, const Project* proj, const char* rel)
{
    for (int i = 0; i < r->thumbCount; i++)
        if (strcmp(r->thumbs[i].rel, rel) == 0)
            return r->thumbs[i].tex.id;

    char full[RES_PATH_LEN * 2];
    full_path(proj, rel, full, (int)sizeof(full));
    Texture2D t = Mge_LoadTexture(full);
    if (t.id == 0)
        return 0;

    if (r->thumbCount == RES_MAX_THUMBS) { // evict oldest
        Mge_UnloadTexture(r->thumbs[0].tex);
        memmove(&r->thumbs[0], &r->thumbs[1], sizeof(r->thumbs[0]) * (RES_MAX_THUMBS - 1));
        r->thumbCount--;
    }
    snprintf(r->thumbs[r->thumbCount].rel, RES_PATH_LEN, "%s", rel);
    r->thumbs[r->thumbCount].tex = t;
    r->thumbCount++;
    return t.id;
}

// ------------------------------------------------------------------ file ops

// The folder that new items land in: the selection if it's a folder, its parent
// if it's a file, else `res/`.
static void target_dir(const Resources* r, char* out, int outSize)
{
    if (r->sel[0] == '\0') {
        snprintf(out, (size_t)outSize, "res");
    } else if (r->selIsDir) {
        snprintf(out, (size_t)outSize, "%s", r->sel);
    } else {
        Path_Dir(r->sel, out, (size_t)outSize);
        if (out[0] == '\0')
            snprintf(out, (size_t)outSize, "res");
    }
}

static void do_import(Resources* r, const Project* proj)
{
    char* src = Mge_OpenFileDialog("Import into res/", "Any file", "*.*");
    if (src == NULL)
        return;

    char base[128], tdir[RES_PATH_LEN], rel[RES_PATH_LEN], dst[RES_PATH_LEN * 2], dstDir[RES_PATH_LEN * 2];
    Path_Base(src, base, sizeof(base));
    target_dir(r, tdir, sizeof(tdir));
    Path_Join(tdir, base, rel, sizeof(rel));
    full_path(proj, tdir, dstDir, (int)sizeof(dstDir));
    Path_MakeDirs(dstDir);
    full_path(proj, rel, dst, (int)sizeof(dst));

    if (Path_CopyFile(src, dst))
        snprintf(r->sel, sizeof(r->sel), "%s", rel), r->selIsDir = false;
    else
        fprintf(stderr, "[editor] import failed: %s\n", src);
    free(src);
}

static void do_new_folder(Resources* r, const Project* proj, const char* name)
{
    char tdir[RES_PATH_LEN], rel[RES_PATH_LEN], full[RES_PATH_LEN * 2];
    target_dir(r, tdir, sizeof(tdir));
    Path_Join(tdir, name, rel, sizeof(rel));
    full_path(proj, rel, full, (int)sizeof(full));
    if (Path_MakeDirs(full)) {
        snprintf(r->sel, sizeof(r->sel), "%s", rel);
        r->selIsDir = true;
    }
}

static void do_rename(Resources* r, const Project* proj, const char* name)
{
    if (r->sel[0] == '\0')
        return;
    char parent[RES_PATH_LEN], rel[RES_PATH_LEN], from[RES_PATH_LEN * 2], to[RES_PATH_LEN * 2];
    Path_Dir(r->sel, parent, sizeof(parent));
    Path_Join(parent[0] ? parent : "res", name, rel, sizeof(rel));
    full_path(proj, r->sel, from, (int)sizeof(from));
    full_path(proj, rel, to, (int)sizeof(to));
    if (Path_Rename(from, to))
        snprintf(r->sel, sizeof(r->sel), "%s", rel);
    flush_thumbs(r);
}

static void do_delete(Resources* r, const Project* proj)
{
    if (r->sel[0] == '\0')
        return;
    char full[RES_PATH_LEN * 2];
    full_path(proj, r->sel, full, (int)sizeof(full));
    Path_Remove(full);
    r->sel[0] = '\0';
    flush_thumbs(r);
}

static void assign_slot(Resources* r, const Project* proj, Scene* s, const char* rel, int mapIndex)
{
    (void)r;
    if (s->selKind != SEL_OBJECT)
        return;
    char full[RES_PATH_LEN * 2];
    full_path(proj, rel, full, (int)sizeof(full));
    Texture2D t = Mge_LoadTextureEx(full, mapIndex == MATERIAL_MAP_DIFFUSE);
    if (t.id == 0)
        return;
    Object* o = &s->objects[s->selIndex];
    Mge_UnloadTexture(o->material.maps[mapIndex].texture);
    o->material.maps[mapIndex].texture = t;
    snprintf(s->texPath[s->selIndex][mapIndex], SCENE_TEXPATH_LEN, "%.*s", SCENE_TEXPATH_LEN - 1, rel);
    s->dirty = true;
}

// ------------------------------------------------------------------ tree

static void draw_children(Resources* r, const Project* proj, Scene* s, const char* rel);

static void draw_file(Resources* r, const Project* proj, Scene* s, const char* rel, const char* name)
{
    (void)s;
    bool selected = strcmp(r->sel, rel) == 0;

    if (is_image(name)) {
        Mge_GuiImage(thumb_for(r, proj, rel), 22.0f);
        Mge_GuiSameLine();
    }
    if (Mge_GuiSelectable(name, selected)) {
        snprintf(r->sel, sizeof(r->sel), "%s", rel);
        r->selIsDir = false;
    }
}

// The assign bar: when an image is selected here and a 3D object is selected in
// the scene, one button per material slot.
static void draw_assign_bar(Resources* r, const Project* proj, Scene* s)
{
    if (r->selIsDir || r->sel[0] == '\0' || s->selKind != SEL_OBJECT)
        return;
    char base[128];
    Path_Base(r->sel, base, sizeof(base));
    if (!is_image(base))
        return;

    static const char* const SLOTS[MATERIAL_MAP_COUNT] = { "diffuse", "specular", "normal", "height" };
    Mge_GuiLabel("assign to:");
    for (int m = 0; m < MATERIAL_MAP_COUNT; m++) {
        Mge_GuiSameLine();
        if (Mge_GuiButton(SLOTS[m]))
            assign_slot(r, proj, s, r->sel, m);
    }
}

static void draw_children(Resources* r, const Project* proj, Scene* s, const char* rel)
{
    char full[RES_PATH_LEN * 2];
    full_path(proj, rel, full, (int)sizeof(full));

    char dirs[128][128];
    int nd = Path_List(full, NULL, true, dirs, 128);
    for (int i = 0; i < nd; i++) {
        char childRel[RES_PATH_LEN];
        Path_Join(rel, dirs[i], childRel, sizeof(childRel));
        bool clicked = false;
        char id[RES_PATH_LEN + 140];
        snprintf(id, sizeof(id), "%.127s/##%.*s", dirs[i], RES_PATH_LEN - 1, childRel);
        bool open = Mge_GuiTreeNode(id, strcmp(r->sel, childRel) == 0, &clicked);
        if (clicked) {
            snprintf(r->sel, sizeof(r->sel), "%s", childRel);
            r->selIsDir = true;
        }
        if (open) {
            draw_children(r, proj, s, childRel);
            Mge_GuiTreePop();
        }
    }

    char files[256][128];
    int nf = Path_List(full, NULL, false, files, 256);
    Mge_GuiIndent();
    for (int i = 0; i < nf; i++) {
        char childRel[RES_PATH_LEN];
        Path_Join(rel, files[i], childRel, sizeof(childRel));
        draw_file(r, proj, s, childRel, files[i]);
    }
    Mge_GuiUnindent();
}

// ------------------------------------------------------------------ modals

static void name_modal(Resources* r, const Project* proj, Scene* s, const char* id, int kind)
{
    (void)s;
    if (!Mge_GuiBeginPopup(id))
        return;
    Mge_GuiLabel(kind == 1 ? "New name:" : "Folder name:");
    Mge_GuiSetNextItemWidth(200.0f);
    Mge_GuiInputText("##rname", r->nameBuf, (int)sizeof(r->nameBuf));
    Mge_GuiSpacing();
    if (r->nameBuf[0] && Mge_GuiButton("OK")) {
        if (kind == 1)
            do_rename(r, proj, r->nameBuf);
        else
            do_new_folder(r, proj, r->nameBuf);
        r->modal = 0;
        Mge_GuiClosePopup();
    }
    Mge_GuiSameLine();
    if (Mge_GuiButton("Cancel")) {
        r->modal = 0;
        Mge_GuiClosePopup();
    }
    Mge_GuiEndPopup();
}

static void delete_modal(Resources* r, const Project* proj)
{
    if (!Mge_GuiBeginPopup("Delete?"))
        return;
    char msg[RES_PATH_LEN + 32];
    snprintf(msg, sizeof(msg), "Delete \"%s\"%s?", r->sel, r->selIsDir ? " and its contents" : "");
    Mge_GuiLabel(msg);
    Mge_GuiSpacing();
    if (Mge_GuiButton("Delete")) {
        do_delete(r, proj);
        r->modal = 0;
        Mge_GuiClosePopup();
    }
    Mge_GuiSameLine();
    if (Mge_GuiButton("Cancel")) {
        r->modal = 0;
        Mge_GuiClosePopup();
    }
    Mge_GuiEndPopup();
}

// ------------------------------------------------------------------ panel

void Resources_Draw(Resources* r, Rectangle rect, Project* proj, Scene* s, int fps, int draws)
{
    if (strcmp(r->projectPath, proj->path) != 0) {
        flush_thumbs(r);
        r->sel[0] = '\0';
        snprintf(r->projectPath, sizeof(r->projectPath), "%s", proj->path);
    }

    if (!Mge_GuiBeginPanel("Resources", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }

    bool haveProject = proj->path[0] != '\0';
    bool haveSel = r->sel[0] != '\0';

    Mge_GuiLabel("RESOURCES");
    Mge_GuiSameLine();
    if (haveProject) {
        if (Mge_GuiButton("Import"))
            do_import(r, proj);
        Mge_GuiSameLine();
        if (Mge_GuiButton("New Folder")) {
            r->modal = 2;
            r->nameBuf[0] = '\0';
            Mge_GuiOpenPopup("New folder");
        }
        Mge_GuiSameLine();
        if (haveSel && Mge_GuiButton("Rename")) {
            r->modal = 1;
            char base[128];
            Path_Base(r->sel, base, sizeof(base));
            snprintf(r->nameBuf, sizeof(r->nameBuf), "%s", base);
            Mge_GuiOpenPopup("Rename");
        }
        if (haveSel) {
            Mge_GuiSameLine();
            if (Mge_GuiButton("Delete")) {
                r->modal = 3;
                Mge_GuiOpenPopup("Delete?");
            }
        }
    } else {
        Mge_GuiSameLine();
        Mge_GuiLabel("(save the project to use res/)");
    }

    char row[220];
    snprintf(row, sizeof(row), "%s%s   |   %d/%d obj, %d/%d lights   |   FPS %d  draws %d",
        proj->name, proj->dirty ? " *" : "",
        s->objectCount, SCENE_MAX_OBJECTS, s->lightCount, SCENE_MAX_LIGHTS, fps, draws);
    Mge_GuiLabel(row);

    draw_assign_bar(r, proj, s);
    Mge_GuiSeparator();

    if (haveProject)
        draw_children(r, proj, s, "res");

    name_modal(r, proj, s, "Rename", 1);
    name_modal(r, proj, s, "New folder", 2);
    delete_modal(r, proj);

    Mge_GuiEndPanel();
}
