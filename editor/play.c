#include "play.h"

#include <mge_gui.h>
#include <stdio.h>
#include <string.h>

void Play_Init(Play* p)
{
    memset(p, 0, sizeof(*p));
}

// ---- helpers ----

static const char* active_name(const Project* proj)
{
    if (proj->activeScene < 0 || proj->activeScene >= proj->sceneCount)
        return NULL;
    return proj->scenes[proj->activeScene];
}

static MgeSceneCtx make_ctx(Scene* s, EditorCamera* cam)
{
    MgeSceneCtx c = { 0 };
    c.objects = s->objects;
    c.objectCount = &s->objectCount;
    c.maxObjects = SCENE_MAX_OBJECTS;
    c.lights = s->lights;
    c.lightCount = &s->lightCount;
    c.maxLights = SCENE_MAX_LIGHTS;
    c.camera = cam->cam;
    c.selected = (s->selKind == SEL_OBJECT) ? s->selIndex : -1;
    return c;
}

// restore `s` from the pre-play snapshot, keeping the live GPU resources
static void restore(Scene* s, const Scene* snap)
{
    ShadowMap sh = s->shadow;
    Cubemap sky = s->sky;
    RenderTexture rt = s->hdrRT;
    BloomFX bl = s->bloom;
    *s = *snap;
    s->shadow = sh;
    s->sky = sky;
    s->hdrRT = rt;
    s->bloom = bl;
}

// compile the active scene; on success `dll` (>= 600) gets its path. Requires a
// saved project.
static bool build(Play* p, const Project* proj, char* dll, int dllSize)
{
    const char* name = active_name(proj);
    BuildLog_Reset(&p->log);
    if (proj->path[0] == '\0' || name == NULL) {
        BuildLog_Line(&p->log, "save the project first (Project > Save Project)");
        return false;
    }
    return SceneBuild_Compile(proj, name, false, &p->log, dll, dllSize);
}

static void reload(Play* p, const Project* proj, Scene* s, EditorCamera* cam, const char* dll)
{
    MgeSceneCtx ctx = make_ctx(s, cam);
    SceneRuntime_Shutdown(&p->rt, &ctx);
    SceneRuntime_Unload(&p->rt);

    char err[256];
    if (SceneRuntime_Load(&p->rt, dll, err, sizeof(err))) {
        SceneRuntime_Init(&p->rt, &ctx);
        BuildLog_Line(&p->log, "-- reloaded --");
    } else {
        BuildLog_Line(&p->log, "reload error: %s", err);
    }

    const char* name = active_name(proj);
    char dir[700];
    Project_SceneDir(proj, name, dir, sizeof(dir));
    p->rt.sourceDigest = SceneRuntime_SourceDigest(dir);
}

// ---- public ----

bool Play_Action(Play* p, TopbarAction a, Project* proj, Scene* s, EditorCamera* cam)
{
    char dll[600];

    switch (a) {
    case TOPBAR_BUILD:
        if (build(p, proj, dll, sizeof(dll)) && p->playing)
            reload(p, proj, s, cam, dll);
        p->showConsole = true;
        return true;

    case TOPBAR_PLAY:
        if (p->playing)
            return true;
        if (!build(p, proj, dll, sizeof(dll))) {
            p->showConsole = true;
            return true;
        }
        {
            char err[256];
            if (!SceneRuntime_Load(&p->rt, dll, err, sizeof(err))) {
                BuildLog_Line(&p->log, "load error: %s", err);
                p->showConsole = true;
                return true;
            }
        }
        p->snapshot = *s;
        {
            MgeSceneCtx ctx = make_ctx(s, cam);
            SceneRuntime_Init(&p->rt, &ctx);
        }
        {
            char dir[700];
            Project_SceneDir(proj, active_name(proj), dir, sizeof(dir));
            p->rt.sourceDigest = SceneRuntime_SourceDigest(dir);
        }
        p->playing = true;
        return true;

    case TOPBAR_STOP:
        if (!p->playing)
            return true;
        {
            MgeSceneCtx ctx = make_ctx(s, cam);
            SceneRuntime_Shutdown(&p->rt, &ctx);
        }
        SceneRuntime_Unload(&p->rt);
        restore(s, &p->snapshot);
        p->playing = false;
        return true;

    default:
        return false;
    }
}

void Play_Frame(Play* p, Project* proj, Scene* s, EditorCamera* cam)
{
    if (!p->playing)
        return;

    const char* name = active_name(proj);
    if (name == NULL)
        return;

    char dir[700];
    Project_SceneDir(proj, name, dir, sizeof(dir));

    long d = SceneRuntime_SourceDigest(dir);
    if (d != p->rt.sourceDigest) {
        char dll[600];
        BuildLog_Reset(&p->log);
        BuildLog_Line(&p->log, "-- source changed, rebuilding --");
        if (SceneBuild_Compile(proj, name, false, &p->log, dll, sizeof(dll)))
            reload(p, proj, s, cam, dll);
        else
            p->rt.sourceDigest = d; // failed build: don't respin on the same source
        p->showConsole = true;
    }

    MgeSceneCtx ctx = make_ctx(s, cam);
    SceneRuntime_Update(&p->rt, &ctx, (float)Mge_GetDeltaTime());
}

void Play_DrawConsole(Play* p, Rectangle rect)
{
    if (!Mge_GuiBeginPanel("Console", rect.x, rect.y, rect.width, rect.height)) {
        Mge_GuiEndPanel();
        return;
    }
    Mge_GuiLabel("CONSOLE");
    Mge_GuiSameLine();
    if (Mge_GuiButton("clear"))
        BuildLog_Reset(&p->log);
    Mge_GuiSameLine();
    if (Mge_GuiButton("close"))
        p->showConsole = false;
    Mge_GuiSeparator();
    Mge_GuiLogBox("##buildlog", p->log.text);
    Mge_GuiEndPanel();
}

void Play_Shutdown(Play* p, Scene* s, EditorCamera* cam)
{
    if (p->playing) {
        MgeSceneCtx ctx = make_ctx(s, cam);
        SceneRuntime_Shutdown(&p->rt, &ctx);
        p->playing = false;
    }
    SceneRuntime_Unload(&p->rt);
    BuildLog_Free(&p->log);
}
