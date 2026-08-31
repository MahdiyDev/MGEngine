// project.mgproject round-trip + Project helpers (editor/project.c + project_io.c).
// Hermetic: no engine .o, no GL.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "test.h"

#include "../editor/project.h"
#include "../editor/project_io.h"
#include "../editor/pathutil.h"

#if defined(_WIN32)
    #include <direct.h>
    #define RMDIR(p) _rmdir(p)
#else
    #include <unistd.h>
    #define RMDIR(p) rmdir(p)
#endif

TEST(default_project)
{
    Project p;
    Project_Default(&p);
    CHECK(strcmp(p.name, "untitled") == 0);
    CHECK(p.path[0] == '\0');
    CHECK(p.sceneCount == 1 && strcmp(p.scenes[0], "untitled") == 0);
    CHECK(p.activeScene == 0);
    CHECK(p.windowW == 1280 && p.windowH == 720);
    CHECK(strcmp(p.startupScene, "untitled") == 0);
}

TEST(scene_list_ops)
{
    Project p;
    Project_Default(&p);

    CHECK(Project_AddScene(&p, "level1"));
    CHECK(Project_AddScene(&p, "menu"));
    CHECK(p.sceneCount == 3);
    CHECK(!Project_AddScene(&p, "level1"));   // duplicate
    CHECK(!Project_AddScene(&p, "bad name")); // space is invalid
    CHECK(Project_FindScene(&p, "menu") == 2);
    CHECK(Project_FindScene(&p, "nope") == -1);

    Project_RemoveScene(&p, 0); // drop "untitled"
    CHECK(p.sceneCount == 2);
    CHECK(strcmp(p.scenes[0], "level1") == 0);

    CHECK(Project_ValidSceneName("a-b_1"));
    CHECK(!Project_ValidSceneName(""));
    CHECK(!Project_ValidSceneName("a/b"));
    CHECK(!Project_ValidSceneName("build"));  // reserved -- collides with the layout
    CHECK(!Project_ValidSceneName("res"));
    CHECK(!Project_ValidSceneName("scenes"));
}

TEST(path_helpers)
{
    Project p;
    Project_Default(&p);
    char out[600];

    Project_Root(&p, out, sizeof(out));
    CHECK(out[0] == '\0'); // in-memory

    snprintf(p.path, sizeof(p.path), "C:/games/mygame/project.mgproject");
    Project_Root(&p, out, sizeof(out));
    CHECK(strcmp(out, "C:/games/mygame") == 0);

    Project_SceneDir(&p, "level1", out, sizeof(out));
    CHECK(strcmp(out, "C:/games/mygame/scenes/level1") == 0);

    Project_SceneFile(&p, "level1", out, sizeof(out));
    CHECK(strcmp(out, "C:/games/mygame/scenes/level1/scene.mgscene") == 0);
}

TEST(project_mgproject_round_trip)
{
    Path_MakeDirs("project_io_tmp");
    const char* path = "project_io_tmp/project.mgproject";

    Project a;
    Project_Default(&a);
    strcpy(a.output, "coolgame");
    a.windowW = 1920;
    a.windowH = 1080;
    a.targetFps = 144;
    a.msaa = 8;
    strcpy(a.cflagsDebug, "-O0 -g -DDBG");
    Project_AddScene(&a, "level1");
    Project_AddScene(&a, "boss");
    strcpy(a.startupScene, "level1");

    CHECK(Project_Save(&a, path));
    CHECK(strcmp(a.name, "project_io_tmp") == 0); // = the folder name
    CHECK(!a.dirty);

    Project b;
    CHECK(Project_Load(&b, path));
    CHECK(strcmp(b.name, "project_io_tmp") == 0);
    CHECK(b.windowW == 1920 && b.windowH == 1080);
    CHECK(b.targetFps == 144 && b.msaa == 8);
    CHECK(strcmp(b.output, "coolgame") == 0);
    CHECK(strcmp(b.cflagsDebug, "-O0 -g -DDBG") == 0);
    CHECK(b.sceneCount == 3);
    CHECK(strcmp(b.scenes[0], "untitled") == 0);
    CHECK(strcmp(b.scenes[1], "level1") == 0);
    CHECK(strcmp(b.scenes[2], "boss") == 0);
    CHECK(strcmp(b.startupScene, "level1") == 0);
    CHECK(b.activeScene == 1); // startup scene's index
    CHECK(strcmp(b.path, path) == 0);
    CHECK(!b.dirty);

    remove(path);
}

TEST(load_rejects_a_non_project_file)
{
    FILE* f = fopen("project_io_tmp/bogus.mgproject", "wb");
    CHECK(f != NULL);
    fprintf(f, "mgescene 1\n");
    fclose(f);

    Project p;
    CHECK(!Project_Load(&p, "project_io_tmp/bogus.mgproject"));
    remove("project_io_tmp/bogus.mgproject");
}

int main(void)
{
    RUN(default_project);
    RUN(scene_list_ops);
    RUN(path_helpers);
    RUN(project_mgproject_round_trip);
    RUN(load_rejects_a_non_project_file);

    RMDIR("project_io_tmp");
    return test_summary();
}
