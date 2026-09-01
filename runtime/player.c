// MGEngine standalone player -- runs a built project.
//
//   <exe> [project.mgproject]
//
// Loads the project, mounts <name>.pak.NNN, opens the startup scene's data +
// textures, loads its compiled module (<scene>.dll next to the exe), and runs
// MgeScene_Update each frame while drawing the scene. Reuses the editor's data
// layer (scene.c / scene_io.c / project*.c / editor_camera.c / scene_runtime.c);
// no GUI.
#include <mge.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #define CHDIR _chdir
#else
    #include <unistd.h>
    #define CHDIR chdir
#endif

#include "scene.h"
#include "project.h"
#include "project_io.h"
#include "scene_io.h"
#include "editor_camera.h"
#include "scene_runtime.h"
#include "pathutil.h"

// run from the executable's own directory so relative paths (the pak, the scene
// .dlls) resolve when launched from elsewhere. argv[0] is a path when the exe is
// double-clicked or run with a path; a bare-name PATH launch just stays put.
static void chdir_to_exe(const char* argv0)
{
    char dir[1024];
    Path_Dir(argv0, dir, sizeof(dir));
    if (dir[0] != '\0')
        (void)CHDIR(dir);
}

static MgeSceneCtx make_ctx(Scene* s)
{
    MgeSceneCtx c = { 0 };
    c.objects = s->objects;
    c.objectCount = &s->objectCount;
    c.maxObjects = SCENE_MAX_OBJECTS;
    c.lights = s->lights;
    c.lightCount = &s->lightCount;
    c.maxLights = SCENE_MAX_LIGHTS;
    c.selected = -1;
    return c;
}

int main(int argc, char** argv)
{
    chdir_to_exe(argv[0]);

    const char* projPath = (argc > 1) ? argv[1] : "project.mgproject";

    // mount the data pak FIRST, under a fixed name, so everything below -- the
    // project file included -- resolves out of it. A staged bundle keeps it in
    // packs/; loose-file dev runs have no pak (ok to miss).
    char projDir[512], pakStem[700], packsDir[600];
    Path_Dir(projPath, projDir, sizeof(projDir));
    const char* base = projDir[0] ? projDir : ".";
    Path_Join(base, "packs", packsDir, sizeof(packsDir));
    Path_Join(packsDir, "data", pakStem, sizeof(pakStem));
    if (!Mge_MountPak(pakStem)) {
        Path_Join(base, "data", pakStem, sizeof(pakStem)); // flat fallback
        Mge_MountPak(pakStem);
    }

    // project.mgproject: from the pak in a shipped bundle, loose on disk in dev
    Project project;
    if (!Project_Load(&project, projPath)) {
        fprintf(stderr, "player: cannot load %s\n", projPath);
        return 1;
    }

    Mge_SetMSAA(project.msaa);
    Mge_InitWindow((uint32_t)(project.windowW > 0 ? project.windowW : 1280),
        (uint32_t)(project.windowH > 0 ? project.windowH : 720), project.name);
    Mge_SetTargetFPS(project.targetFps > 0 ? project.targetFps : 60);

    Scene scene;
    Scene_Init(&scene, project.windowW, project.windowH);

    EditorCamera camera;
    EditorCamera_Init(&camera);

    int idx = Project_FindScene(&project, project.startupScene);
    if (idx < 0)
        idx = 0;
    const char* sceneName = project.scenes[idx];

    char sceneFile[700];
    Project_SceneFile(&project, sceneName, sceneFile, sizeof(sceneFile));
    Camera3D c;
    if (Scene_Load(&scene, sceneFile, &c)) {
        char root[512];
        Project_Root(&project, root, sizeof(root));
        Scene_LoadMaterialTextures(&scene, root);
        Scene_LoadSkybox(&scene, root);
        EditorCamera_SetPose(&camera, c);
    } else {
        fprintf(stderr, "player: cannot load scene '%s'\n", sceneName);
    }

    // the built game views the scene through its main camera object (a scene
    // module can move that object to move the camera). Only when a scene has no
    // main camera does the player fall back to a free-fly debug camera.
    bool flyCam = !Scene_MainCamera(&scene, NULL);
    if (flyCam)
        DisableCursor();
    else
        EnableCursor();

    // the scene module: a staged bundle keeps it as scenes/scene.<index>.dll
    // (index into project.scenes[] -- names aren't shipped); a loose dev run
    // keeps it flat as <scene>.dll next to the exe
    char dll[700], scenesDir[600], modName[32];
    snprintf(modName, sizeof(modName), "scene.%d", idx);
    Path_Join(base, "scenes", scenesDir, sizeof(scenesDir));
    Path_Join(scenesDir, modName, dll, sizeof(dll));
    strncat(dll, ".dll", sizeof(dll) - strlen(dll) - 1);
    if (Path_MTime(dll) == 0) {
        Path_Join(base, sceneName, dll, sizeof(dll)); // flat loose fallback
        strncat(dll, ".dll", sizeof(dll) - strlen(dll) - 1);
    }

    SceneRuntime rt = { 0 };
    MgeSceneCtx ctx = make_ctx(&scene);
    char err[256];
    if (SceneRuntime_Load(&rt, dll, err, sizeof(err)))
        SceneRuntime_Init(&rt, &ctx);
    else
        fprintf(stderr, "player: no scene module (%s): %s\n", dll, err);

    // headless smoke hook: render N frames, screenshot, exit (CI / `make` checks)
    const char* shot = getenv("MGE_PLAYER_SHOT");
    int frame = 0;
    while (!Mge_WindowShouldClose()) {
        Camera3D view;
        if (!Scene_MainCamera(&scene, &view)) {
            EditorCamera_Update(&camera, false, false); // no main camera -> debug fly-cam
            view = camera.cam;
        }

        ctx = make_ctx(&scene);
        ctx.camera = view;
        SceneRuntime_Update(&rt, &ctx, (float)Mge_GetDeltaTime());

        // the module may have moved the camera object this frame
        if (Scene_MainCamera(&scene, &view))
            ctx.camera = view;

        Mge_BeginDrawing();
        Scene_Draw(&scene, view, false, false); // no editor gizmos in the shipped game
        Mge_EndDrawing();
        if (shot && ++frame == 60) {
            Mge_TakeScreenshot(shot);
            break;
        }
    }

    SceneRuntime_Shutdown(&rt, &ctx);
    SceneRuntime_Unload(&rt);
    Scene_Shutdown(&scene);
    Mge_CloseWindow();
    Mge_UnmountPaks();
    return 0;
}
