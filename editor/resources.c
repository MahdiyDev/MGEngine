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

// ------------------------------------------------------------------ selection

static void select_only(Resources* r, const char* rel, bool isDir)
{
    snprintf(r->sel, sizeof(r->sel), "%s", rel);
    r->selIsDir = isDir;
    r->multiCount = 0;
}

// ctrl-click: toggle `rel` in/out of the multi-selection
static void select_toggle(Resources* r, const char* rel, bool isDir)
{
    if (r->sel[0] == '\0') {
        select_only(r, rel, isDir);
        return;
    }
    if (strcmp(r->sel, rel) == 0)
        return;
    for (int i = 0; i < r->multiCount; i++) {
        if (strcmp(r->multi[i], rel) == 0) { // already in -> drop it
            memmove(&r->multi[i], &r->multi[i + 1], sizeof(r->multi[0]) * (r->multiCount - i - 1));
            r->multiCount--;
            return;
        }
    }
    if (r->multiCount < RES_MAX_SEL)
        snprintf(r->multi[r->multiCount++], RES_PATH_LEN, "%s", rel);
}

static bool is_selected(const Resources* r, const char* rel)
{
    if (strcmp(r->sel, rel) == 0)
        return true;
    for (int i = 0; i < r->multiCount; i++)
        if (strcmp(r->multi[i], rel) == 0)
            return true;
    return false;
}

// gather primary + extras into `out` (>= RES_MAX_SEL+1); returns count
static int selected_list(const Resources* r, char (*out)[RES_PATH_LEN])
{
    int n = 0;
    if (r->sel[0] != '\0')
        snprintf(out[n++], RES_PATH_LEN, "%s", r->sel);
    for (int i = 0; i < r->multiCount; i++)
        snprintf(out[n++], RES_PATH_LEN, "%s", r->multi[i]);
    return n;
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
        select_only(r, rel, false);
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
    if (Path_MakeDirs(full))
        select_only(r, rel, true);
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
    char list[RES_MAX_SEL + 1][RES_PATH_LEN];
    int n = selected_list(r, list);
    for (int i = 0; i < n; i++) {
        char full[RES_PATH_LEN * 2];
        full_path(proj, list[i], full, (int)sizeof(full));
        Path_Remove(full);
    }
    r->sel[0] = '\0';
    r->multiCount = 0;
    flush_thumbs(r);
}

static void do_copy(Resources* r)
{
    char list[RES_MAX_SEL + 1][RES_PATH_LEN];
    int n = selected_list(r, list);
    r->clipCount = 0;
    for (int i = 0; i < n && r->clipCount < RES_MAX_SEL; i++)
        snprintf(r->clip[r->clipCount++], RES_PATH_LEN, "%.*s", RES_PATH_LEN - 1, list[i]);
}

static void do_paste(Resources* r, const Project* proj)
{
    if (r->clipCount == 0)
        return;
    char tdir[RES_PATH_LEN];
    target_dir(r, tdir, sizeof(tdir));

    for (int i = 0; i < r->clipCount; i++) {
        char name[200];
        Path_Base(r->clip[i], name, sizeof(name));

        char rel[RES_PATH_LEN], srcFull[RES_PATH_LEN * 2], dstFull[RES_PATH_LEN * 2];
        Path_Join(tdir, name, rel, sizeof(rel));
        full_path(proj, r->clip[i], srcFull, (int)sizeof(srcFull));
        full_path(proj, rel, dstFull, (int)sizeof(dstFull));

        // don't clobber: append " copy" until the name is free
        for (int guard = 0; guard < 16 && Path_MTime(dstFull) > 0; guard++) {
            const char* dot = strrchr(name, '.');
            int stemLen = dot ? (int)(dot - name) : (int)strlen(name);
            if (stemLen > 150)
                stemLen = 150;
            char stem[160];
            memcpy(stem, name, (size_t)stemLen);
            stem[stemLen] = '\0';
            char ext[16];
            snprintf(ext, sizeof(ext), "%.15s", dot ? dot : "");
            snprintf(name, sizeof(name), "%s copy%s", stem, ext);
            Path_Join(tdir, name, rel, sizeof(rel));
            full_path(proj, rel, dstFull, (int)sizeof(dstFull));
        }
        if (!Path_Equal(srcFull, dstFull))
            Path_CopyTree(srcFull, dstFull);
        select_only(r, rel, Path_IsDir(dstFull));
    }
    flush_thumbs(r);
}

// Move `srcRel` into the folder `dstDirRel` (drag & drop). Rejects moving a
// folder into itself / a descendant, and a no-op self move.
static void do_move(Resources* r, const Project* proj, const char* srcRel, const char* dstDirRel)
{
    char base[128];
    Path_Base(srcRel, base, sizeof(base));

    char srcDir[RES_PATH_LEN], destRel[RES_PATH_LEN];
    Path_Dir(srcRel, srcDir, sizeof(srcDir));
    if (Path_Equal(srcDir[0] ? srcDir : "res", dstDirRel))
        return; // already there
    Path_Join(dstDirRel, base, destRel, sizeof(destRel));
    if (Path_Equal(destRel, srcRel))
        return;
    // dst inside src? (moving a folder under itself)
    size_t sl = strlen(srcRel);
    if (strncmp(dstDirRel, srcRel, sl) == 0 && (dstDirRel[sl] == '/' || dstDirRel[sl] == '\0'))
        return;

    char from[RES_PATH_LEN * 2], to[RES_PATH_LEN * 2], toDir[RES_PATH_LEN * 2];
    full_path(proj, srcRel, from, (int)sizeof(from));
    full_path(proj, dstDirRel, toDir, (int)sizeof(toDir));
    Path_MakeDirs(toDir);
    full_path(proj, destRel, to, (int)sizeof(to));
    if (Path_MTime(to) > 0)
        return; // name taken
    if (Path_Rename(from, to))
        select_only(r, destRel, Path_IsDir(to));
    flush_thumbs(r);
}

// ------------------------------------------------------------------ tree

static void draw_children(Resources* r, const Project* proj, const char* rel);

static void row_context(Resources* r, const Project* proj, const char* rel, bool isDir)
{
    char id[RES_PATH_LEN + 8];
    snprintf(id, sizeof(id), "ctx##%s", rel);
    if (!Mge_GuiBeginContextMenu(id))
        return;
    if (!is_selected(r, rel))
        select_only(r, rel, isDir); // right-click selects
    if (Mge_GuiMenuItem("New Folder")) {
        r->modal = 2;
        r->nameBuf[0] = '\0';
        Mge_GuiOpenPopup("New folder");
    }
    if (Mge_GuiMenuItem("Rename")) {
        r->modal = 1;
        char b[128];
        Path_Base(r->sel, b, sizeof(b));
        snprintf(r->nameBuf, sizeof(r->nameBuf), "%s", b);
        Mge_GuiOpenPopup("Rename");
    }
    if (Mge_GuiMenuItem("Copy"))
        do_copy(r);
    if (r->clipCount > 0 && Mge_GuiMenuItem("Paste"))
        do_paste(r, proj);
    if (Mge_GuiMenuItem("Delete")) {
        r->modal = 3;
        Mge_GuiOpenPopup("Delete?");
    }
    Mge_GuiEndContextMenu();
}

static void draw_file(Resources* r, const Project* proj, const char* rel, const char* name)
{
    if (is_image(name)) {
        Mge_GuiImage(thumb_for(r, proj, rel), 22.0f);
        Mge_GuiSameLine();
    }
    if (Mge_GuiSelectable(name, is_selected(r, rel))) {
        bool add = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (add)
            select_toggle(r, rel, false);
        else
            select_only(r, rel, false);
    }
    Mge_GuiDragSource(rel, name);
    row_context(r, proj, rel, false);
}

static void draw_children(Resources* r, const Project* proj, const char* rel)
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
        bool open = Mge_GuiTreeNode(id, is_selected(r, childRel), &clicked);
        if (clicked) {
            bool add = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            if (add)
                select_toggle(r, childRel, true);
            else
                select_only(r, childRel, true);
        }
        Mge_GuiDragSource(childRel, dirs[i]);
        char got[RES_PATH_LEN];
        if (Mge_GuiDropTarget(got, (int)sizeof(got)))
            do_move(r, proj, got, childRel);
        row_context(r, proj, childRel, true);

        if (open) {
            draw_children(r, proj, childRel);
            Mge_GuiTreePop();
        }
    }

    char files[256][128];
    int nf = Path_List(full, NULL, false, files, 256);
    Mge_GuiIndent();
    for (int i = 0; i < nf; i++) {
        char childRel[RES_PATH_LEN];
        Path_Join(rel, files[i], childRel, sizeof(childRel));
        draw_file(r, proj, childRel, files[i]);
    }
    Mge_GuiUnindent();
}

// ------------------------------------------------------------------ modals

static void name_modal(Resources* r, const Project* proj, const char* id, int kind)
{
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
    char msg[RES_PATH_LEN + 48];
    int n = 1 + r->multiCount;
    if (n > 1)
        snprintf(msg, sizeof(msg), "Delete %d items (folders include their contents)?", n);
    else
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
        r->multiCount = 0;
        r->clipCount = 0;
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
        if (haveSel) {
            Mge_GuiSameLine();
            if (Mge_GuiButton("Rename")) {
                r->modal = 1;
                char base[128];
                Path_Base(r->sel, base, sizeof(base));
                snprintf(r->nameBuf, sizeof(r->nameBuf), "%s", base);
                Mge_GuiOpenPopup("Rename");
            }
            Mge_GuiSameLine();
            if (Mge_GuiButton("Copy"))
                do_copy(r);
            Mge_GuiSameLine();
            if (Mge_GuiButton("Delete")) {
                r->modal = 3;
                Mge_GuiOpenPopup("Delete?");
            }
        }
        if (r->clipCount > 0) {
            Mge_GuiSameLine();
            if (Mge_GuiButton("Paste"))
                do_paste(r, proj);
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
    Mge_GuiSeparator();

    if (haveProject) {
        // a drop onto the "res/" header moves an item to the top level
        Mge_GuiLabel("res/  (drag rows here for the top level; ctrl-click multi)");
        char got[RES_PATH_LEN];
        if (Mge_GuiDropTarget(got, (int)sizeof(got)))
            do_move(r, proj, got, "res");
        draw_children(r, proj, "res");
    }

    // Ctrl+C / Ctrl+V while the panel is up
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (haveProject && ctrl && !Mge_GuiWantsKeyboard()) {
        if (IsKeyPressed(KEY_C) && haveSel)
            do_copy(r);
        if (IsKeyPressed(KEY_V) && r->clipCount > 0)
            do_paste(r, proj);
    }

    name_modal(r, proj, "Rename", 1);
    name_modal(r, proj, "New folder", 2);
    delete_modal(r, proj);

    Mge_GuiEndPanel();
}
