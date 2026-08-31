#include "scene_build.h"
#include "pathutil.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
    #include <direct.h>
    #define POPEN _popen
    #define PCLOSE _pclose
    #define GETCWD _getcwd
    #define DLL_EXT ".dll"
#else
    #include <unistd.h>
    #define POPEN popen
    #define PCLOSE pclose
    #define GETCWD getcwd
    #define DLL_EXT ".so"
#endif

// ------------------------------------------------------------------ BuildLog

void BuildLog_Reset(BuildLog* b)
{
    b->len = 0;
    if (b->text != NULL)
        b->text[0] = '\0';
}

void BuildLog_Free(BuildLog* b)
{
    free(b->text);
    b->text = NULL;
    b->len = b->cap = 0;
}

void BuildLog_Append(BuildLog* b, const char* s)
{
    int add = (int)strlen(s);
    if (b->len + add + 1 > b->cap) {
        int cap = b->cap ? b->cap * 2 : 4096;
        while (cap < b->len + add + 1)
            cap *= 2;
        char* t = realloc(b->text, (size_t)cap);
        if (t == NULL)
            return;
        b->text = t;
        b->cap = cap;
    }
    memcpy(b->text + b->len, s, (size_t)add + 1);
    b->len += add;
}

void BuildLog_Line(BuildLog* b, const char* fmt, ...)
{
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    BuildLog_Append(b, line);
    BuildLog_Append(b, "\n");
}

// ------------------------------------------------------------------ SDK lookup

static bool file_exists(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static bool is_sdk(const char* dir)
{
    char a[1024], b[1024];
    Path_Join(dir, "source/mge.h", a, sizeof(a));
    Path_Join(dir, "build/libmgengine.dll.a", b, sizeof(b));
    if (file_exists(a) && file_exists(b))
        return true;
    // POSIX import-lib-less
    Path_Join(dir, "build/libmgengine.so", b, sizeof(b));
    return file_exists(a) && file_exists(b);
}

bool SceneBuild_FindSDK(char* out, int outSize)
{
    const char* env = getenv("MGE_ENGINE");
    if (env != NULL && env[0] != '\0' && is_sdk(env)) {
        snprintf(out, (size_t)outSize, "%s", env);
        return true;
    }

    char cwd[1024];
    if (GETCWD(cwd, sizeof(cwd)) == NULL)
        return false;
    for (size_t i = 0; cwd[i]; i++)
        if (cwd[i] == '\\')
            cwd[i] = '/';

    // walk up from the working directory
    char dir[1024];
    snprintf(dir, sizeof(dir), "%s", cwd);
    for (int up = 0; up < 6; up++) {
        if (is_sdk(dir)) {
            snprintf(out, (size_t)outSize, "%s", dir);
            return true;
        }
        char* slash = strrchr(dir, '/');
        if (slash == NULL || slash == dir)
            break;
        *slash = '\0';
    }
    return false;
}

// ------------------------------------------------------------------ compile

bool SceneBuild_Compile(const Project* proj, const char* sceneName, bool release,
    BuildLog* log, char* outDll, int outDllSize)
{
    char sdk[1024];
    if (!SceneBuild_FindSDK(sdk, sizeof(sdk))) {
        BuildLog_Line(log, "error: engine SDK not found. Set MGE_ENGINE to the MGEngine repo,");
        BuildLog_Line(log, "       or run the editor from inside a build/ next to source/.");
        return false;
    }

    char sceneDir[700];
    Project_SceneDir(proj, sceneName, sceneDir, sizeof(sceneDir));
    if (sceneDir[0] == '\0') {
        BuildLog_Line(log, "error: save the project first");
        return false;
    }

    char names[64][128];
    int nc = Path_List(sceneDir, ".c", false, names, 64);
    if (nc <= 0) {
        BuildLog_Line(log, "error: no .c files in %s", sceneDir);
        return false;
    }

    char buildDir[760];
    Path_Join(sceneDir, "build", buildDir, sizeof(buildDir));
    Path_MakeDirs(buildDir);

    snprintf(outDll, (size_t)outDllSize, "%s/%s_%s" DLL_EXT,
        buildDir, sceneName, release ? "release" : "debug");

    const char* cc = getenv("CC");
    if (cc == NULL || cc[0] == '\0')
        cc = "gcc";
    const char* cflags = release ? proj->cflagsRelease : proj->cflagsDebug;

    // gcc <cflags> -shared -std=c11 -I<sdk>/source <scene>/*.c -o <dll> -L<sdk>/build -lmgengine
    // (the compiler name is left unquoted so cmd.exe doesn't strip the first "path" quote)
    char cmd[8192];
    int n = snprintf(cmd, sizeof(cmd),
        "%s %s -shared -std=c11 -DPLATFORM_DESKTOP -I\"%s/source\"",
        cc, cflags, sdk);
    for (int i = 0; i < nc && n < (int)sizeof(cmd) - 300; i++)
        n += snprintf(cmd + n, sizeof(cmd) - n, " \"%s/%s\"", sceneDir, names[i]);
    n += snprintf(cmd + n, sizeof(cmd) - n,
        " -o \"%s\" -L\"%s/build\" -lmgengine 2>&1", outDll, sdk);

    BuildLog_Line(log, "$ %s", cmd);

    FILE* pipe = POPEN(cmd, "r");
    if (pipe == NULL) {
        BuildLog_Line(log, "error: could not run the compiler (%s)", cc);
        return false;
    }
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe) != NULL)
        BuildLog_Append(log, buf);
    int rc = PCLOSE(pipe);

    if (rc != 0) {
        BuildLog_Line(log, "-- build FAILED (exit %d) --", rc);
        return false;
    }
    BuildLog_Line(log, "-- build ok: %s --", outDll);
    return true;
}
