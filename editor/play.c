#include "play.h"
#include "release.h"

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

static MgeSceneCtx make_ctx(Scene* s, Camera3D cam)
{
    MgeSceneCtx c = { 0 };
    c.objects = s->objects;
    c.objectCount = &s->objectCount;
    c.maxObjects = SCENE_MAX_OBJECTS;
    c.lights = s->lights;
    c.lightCount = &s->lightCount;
    c.maxLights = SCENE_MAX_LIGHTS;
    c.camera = cam;
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

// Kick off a background compile of the active scene. `purpose` is what Play_Frame
// should do once it finishes (JOB_BUILD / JOB_PLAY / JOB_RELOAD). Needs a saved
// project.
static void start_job(Play* p, const Project* proj, int purpose)
{
    const char* name = active_name(proj);
    if (proj->path[0] == '\0' || name == NULL) {
        BuildLog_Reset(&p->log);
        BuildLog_Line(&p->log, "save the project first (Project > Save Project)");
        p->showConsole = true;
        return;
    }
    if (p->jobPurpose != JOB_NONE) {
        BuildLog_Line(&p->log, "-- a build is already running --");
        p->showConsole = true;
        return;
    }
    BuildLog_Reset(&p->log);
    if (!SceneBuild_Start(&p->job, proj, name, false, &p->log)) {
        SceneBuild_Clear(&p->job);
        p->showConsole = true;
        return;
    }
    p->jobPurpose = purpose;
    p->showConsole = true;
}

static void cancel_job(Play* p)
{
    if (p->jobPurpose == JOB_NONE)
        return;
    SceneBuild_Clear(&p->job);
    p->jobPurpose = JOB_NONE;
}

static void reload(Play* p, const Project* proj, Scene* s, const char* dll)
{
    MgeSceneCtx ctx = make_ctx(s, p->viewCam);
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

// A background compile just finished: act on p->job.ok / p->job.outDll.
static void finish_job(Play* p, Project* proj, Scene* s)
{
    bool ok = p->job.ok;
    char dll[768];
    snprintf(dll, sizeof(dll), "%s", p->job.outDll);
    int purpose = p->jobPurpose;
    long digest = p->jobDigest;

    SceneBuild_Clear(&p->job);
    p->jobPurpose = JOB_NONE;
    p->showConsole = true;

    if (purpose == JOB_PLAY) {
        if (!ok)
            return;
        char err[256];
        if (!SceneRuntime_Load(&p->rt, dll, err, sizeof(err))) {
            BuildLog_Line(&p->log, "load error: %s", err);
            return;
        }
        p->snapshot = *s;
        MgeSceneCtx ctx = make_ctx(s, p->viewCam);
        SceneRuntime_Init(&p->rt, &ctx);
        char dir[700];
        Project_SceneDir(proj, active_name(proj), dir, sizeof(dir));
        p->rt.sourceDigest = SceneRuntime_SourceDigest(dir);
        p->playing = true;
        return;
    }

    if (purpose == JOB_BUILD) {
        if (ok && p->playing)
            reload(p, proj, s, dll);
        return;
    }

    if (purpose == JOB_RELOAD) {
        if (ok)
            reload(p, proj, s, dll);
        else
            p->rt.sourceDigest = digest; // failed build: don't respin on the same source
        return;
    }
}

// ---- public ----

bool Play_Action(Play* p, TopbarAction a, Project* proj, Scene* s)
{
    switch (a) {
    case TOPBAR_BUILD:
        start_job(p, proj, JOB_BUILD);
        return true;

    case TOPBAR_BUILD_RELEASE:
        if (p->playing)
            Play_Action(p, TOPBAR_STOP, proj, s);
        cancel_job(p);
        Release_Build(proj, &p->log); // synchronous: ships every scene at once
        p->showConsole = true;
        return true;

    case TOPBAR_PLAY:
        if (p->playing || p->jobPurpose == JOB_PLAY)
            return true;
        start_job(p, proj, JOB_PLAY);
        return true;

    case TOPBAR_STOP:
        if (p->jobPurpose == JOB_PLAY)
            cancel_job(p); // abort a pending start
        if (!p->playing)
            return true;
        {
            MgeSceneCtx ctx = make_ctx(s, p->viewCam);
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

void Play_Frame(Play* p, Project* proj, Scene* s)
{
    // advance an in-flight compile (started by Build / Play / a hot-reload)
    if (p->jobPurpose != JOB_NONE && SceneBuild_Poll(&p->job))
        finish_job(p, proj, s);

    if (!p->playing)
        return;

    const char* name = active_name(proj);
    if (name == NULL)
        return;

    char dir[700];
    Project_SceneDir(proj, name, dir, sizeof(dir));

    // hot-reload: on a source change, kick one background rebuild
    if (p->jobPurpose == JOB_NONE) {
        long d = SceneRuntime_SourceDigest(dir);
        if (d != p->rt.sourceDigest) {
            BuildLog_Reset(&p->log);
            BuildLog_Line(&p->log, "-- source changed, rebuilding --");
            if (SceneBuild_Start(&p->job, proj, name, false, &p->log)) {
                p->jobPurpose = JOB_RELOAD;
                p->jobDigest = d;
                p->showConsole = true;
            } else {
                SceneBuild_Clear(&p->job);
                p->rt.sourceDigest = d;
            }
        }
    }

    MgeSceneCtx ctx = make_ctx(s, p->viewCam);
    SceneRuntime_Update(&p->rt, &ctx, (float)Mge_GetDeltaTime());
}

bool Play_DrawOverlay(Play* p, float screenW, int fps)
{
    bool stop = false;
    if (!Mge_GuiBeginPanel("##playbar", 0.0f, 0.0f, screenW, 34.0f)) {
        Mge_GuiEndPanel();
        return false;
    }
    if (Mge_GuiButton("[ Stop ]"))
        stop = true;
    Mge_GuiSameLine();
    if (Mge_GuiButton(p->showConsole ? "[Console]" : "Console"))
        p->showConsole = !p->showConsole;
    Mge_GuiSameLine();

    char label[96];
    snprintf(label, sizeof(label), "PLAYING  --  Esc to stop  |  FPS %d", fps);
    Mge_GuiLabel(label);
    Mge_GuiEndPanel();
    return stop;
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

void Play_Shutdown(Play* p, Scene* s)
{
    if (p->jobPurpose != JOB_NONE)
        SceneBuild_Clear(&p->job);
    if (p->playing) {
        MgeSceneCtx ctx = make_ctx(s, p->viewCam);
        SceneRuntime_Shutdown(&p->rt, &ctx);
        p->playing = false;
    }
    SceneRuntime_Unload(&p->rt);
    BuildLog_Free(&p->log);
}
