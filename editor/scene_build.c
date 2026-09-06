#include "scene_build.h"
#include "pathutil.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
    #include <direct.h>
    #include <windows.h>
    #define POPEN _popen
    #define PCLOSE _pclose
    #define GETCWD _getcwd
    #define DLL_EXT ".dll"
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <signal.h>
    #include <sys/wait.h>
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
    if (!file_exists(a))
        return false;
    // recognised by the presence of a built engine lib in either config
    const char* libs[] = {
        "build/libmgengine.dll.a", "build/libmgengine.so",
        "build/release/libmgengine.dll.a", "build/release/libmgengine.so",
    };
    for (size_t i = 0; i < sizeof(libs) / sizeof(libs[0]); i++) {
        Path_Join(dir, libs[i], b, sizeof(b));
        if (file_exists(b))
            return true;
    }
    return false;
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

#if defined(_WIN32)
// Write a Win32 VERSIONINFO resource for the scene DLL and compile it to a COFF
// .res with windres. Returns true and fills `resOut` with the .res path on
// success; on any failure logs one note and returns false (metadata is
// best-effort -- a build never fails over it). The resource gives freshly-built
// scene DLLs real PE metadata, which quiets Windows Defender / SmartScreen
// heuristics (real trust still needs code signing).
static bool build_scene_res(const char* buildDir, const char* projName, const char* sdk,
    const char* outPath, bool isApp, BuildLog* log, char* resOut, int resOutSize)
{
    char leaf[128];
    Path_Base(outPath, leaf, sizeof(leaf));

    char rcPath[820];
    snprintf(rcPath, sizeof(rcPath), "%s/_scene.rc", buildDir);
    FILE* f = fopen(rcPath, "w");
    if (f == NULL) {
        BuildLog_Line(log, "note: could not write %s -- %s gets no version info", rcPath,
            isApp ? "the exe" : "the scene DLL");
        return false;
    }
    fprintf(f,
        "1 VERSIONINFO\n"
        "FILEVERSION 1,0,0,0\nPRODUCTVERSION 1,0,0,0\n"
        "FILEFLAGSMASK 0x3fL\nFILEFLAGS 0x0L\nFILEOS 0x40004L\nFILETYPE 0x%sL\nFILESUBTYPE 0x0L\n"
        "BEGIN\n"
        " BLOCK \"StringFileInfo\"\n BEGIN\n  BLOCK \"040904b0\"\n  BEGIN\n"
        "   VALUE \"CompanyName\", \"MGEngine\"\n"
        "   VALUE \"ProductName\", \"%s\"\n"
        "   VALUE \"FileDescription\", \"%s%s\"\n"
        "   VALUE \"FileVersion\", \"1.0.0.0\"\n"
        "   VALUE \"ProductVersion\", \"1.0.0.0\"\n"
        "   VALUE \"InternalName\", \"%s\"\n"
        "   VALUE \"OriginalFilename\", \"%s\"\n"
        "   VALUE \"LegalCopyright\", \"MGEngine\"\n"
        "  END\n END\n"
        " BLOCK \"VarFileInfo\"\n BEGIN\n  VALUE \"Translation\", 0x409, 1200\n END\n"
        "END\n",
        isApp ? "1" : "2", projName, projName, isApp ? "" : " scene module", leaf, leaf);
    // an application also carries the shared manifest (DPI / longPathAware / supportedOS)
    if (isApp) {
        char man[1024];
        snprintf(man, sizeof(man), "%s/resources/app.manifest", sdk);
        struct stat st;
        if (stat(man, &st) == 0)
            fprintf(f, "1 24 \"%s\"\n", man);
    }
    fclose(f);

    const char* wr = getenv("WINDRES");
    if (wr == NULL || wr[0] == '\0')
        wr = "windres";
    snprintf(resOut, (size_t)resOutSize, "%s/_scene.res", buildDir);

    char wcmd[2600];
    snprintf(wcmd, sizeof(wcmd), "%s -O coff \"%s\" \"%s\" 2>nul", wr, rcPath, resOut);
    if (system(wcmd) != 0) {
        BuildLog_Line(log, "note: windres unavailable -- built without version info");
        remove(rcPath);
        resOut[0] = '\0';
        return false;
    }
    return true;
}
#endif

// The scene's source directory: <root>/scenes/<name>, or -- for the project-level
// shared game module (sceneName == NULL) -- <root>/source.
static void module_src_dir(const Project* proj, const char* sceneName, char* out, int outSize)
{
    if (sceneName != NULL)
        Project_SceneDir(proj, sceneName, out, (size_t)outSize);
    else
        Project_SourceDir(proj, out, (size_t)outSize);
}

// Assemble the compiler command line for a scene module (no shell redirection
// appended). `sceneName == NULL` builds the project-level shared module from
// <root>/source/*.c (its .dll is what Play mode loads for a static-game project).
// Creates <srcDir>/build and reports the produced .dll path. Returns false
// (writing the reason into `log`) when the build can't be set up.
static bool build_command(const Project* proj, const char* sceneName, bool release,
    BuildLog* log, char* outDll, int outDllSize, char* cmd, int cmdSize)
{
    char sdk[1024];
    if (!SceneBuild_FindSDK(sdk, sizeof(sdk))) {
        BuildLog_Line(log, "error: engine SDK not found. Set MGE_ENGINE to the MGEngine repo,");
        BuildLog_Line(log, "       or run the editor from inside a build/ next to source/.");
        return false;
    }

    char srcDir[700];
    module_src_dir(proj, sceneName, srcDir, sizeof(srcDir));
    if (srcDir[0] == '\0') {
        BuildLog_Line(log, "error: save the project first");
        return false;
    }
    const char* stem = (sceneName != NULL) ? sceneName : proj->name;

    char names[64][128];
    int nc = Path_List(srcDir, ".c", false, names, 64);
    if (nc <= 0) {
        BuildLog_Line(log, "error: no .c files in %s", srcDir);
        return false;
    }

    char buildDir[760];
    Path_Join(srcDir, "build", buildDir, sizeof(buildDir));
    Path_MakeDirs(buildDir);

    snprintf(outDll, (size_t)outDllSize, "%s/%s_%s" DLL_EXT,
        buildDir, stem, release ? "release" : "debug");

    const char* cc = getenv("CC");
    if (cc == NULL || cc[0] == '\0')
        cc = "gcc";
    const char* cflags = release ? proj->cflagsRelease : proj->cflagsDebug;

    // gcc <cflags> -shared -std=c11 -I<sdk>/source <src>/*.c -o <dll> -L<sdk>/build[/release] -lmgengine
    // (the compiler name is left unquoted so cmd.exe doesn't strip the first "path" quote)
    const char* libDir = release ? "build/release" : "build";
    int n = snprintf(cmd, (size_t)cmdSize,
        "%s %s -shared -std=c11 -DPLATFORM_DESKTOP -I\"%s/source\"",
        cc, cflags, sdk);
    for (int i = 0; i < nc && n < cmdSize - 300; i++)
        n += snprintf(cmd + n, (size_t)cmdSize - n, " \"%s/%s\"", srcDir, names[i]);
    n += snprintf(cmd + n, (size_t)cmdSize - n,
        " -o \"%s\" -L\"%s/%s\" -lmgengine", outDll, sdk, libDir);

#if defined(_WIN32)
    char res[820];
    if (build_scene_res(buildDir, proj->name, sdk, outDll, false, log, res, sizeof(res)))
        n += snprintf(cmd + n, (size_t)cmdSize - n, " \"%s\"", res);
#endif
    return true;
}

// The player's data layer -- the editor .c files runtime/player.c links (kept in
// sync with PLAYER_SRC in the Makefile). Compiled straight into a static-game exe.
static const char* const PLAYER_DATA_LAYER[] = {
    "editor/scene.c", "editor/scene_io.c", "editor/project.c", "editor/project_io.c",
    "editor/pathutil.c", "editor/editor_camera.c", "editor/scene_runtime.c",
};

// Assemble the link command for a static-game project's executable: the SDK
// player + its data layer + <root>/source/*.c, all linked into <outExe> with the
// game code baked in (-DMGE_STATIC_GAME). No scene .dll is produced. Creates
// <root>/source/build for scratch. Returns false (reason in `log`) on setup error.
static bool build_exe_command(const Project* proj, bool release, BuildLog* log,
    const char* outExe, char* cmd, int cmdSize)
{
    char sdk[1024];
    if (!SceneBuild_FindSDK(sdk, sizeof(sdk))) {
        BuildLog_Line(log, "error: engine SDK not found (set MGE_ENGINE)");
        return false;
    }

    char srcDir[700];
    Project_SourceDir(proj, srcDir, sizeof(srcDir));
    char names[64][128];
    int nc = Path_List(srcDir, ".c", false, names, 64);
    if (nc <= 0) {
        BuildLog_Line(log, "error: no .c files in %s", srcDir);
        return false;
    }

    char buildDir[760];
    Path_Join(srcDir, "build", buildDir, sizeof(buildDir));
    Path_MakeDirs(buildDir);

    const char* cc = getenv("CC");
    if (cc == NULL || cc[0] == '\0')
        cc = "gcc";
    const char* cflags = release ? proj->cflagsRelease : proj->cflagsDebug;
    const char* libDir = release ? "build/release" : "build";

    int n = snprintf(cmd, (size_t)cmdSize,
        "%s %s -std=c11 -DPLATFORM_DESKTOP -DMGE_STATIC_GAME"
        " -I\"%s/source\" -I\"%s/editor\" -I\"%s/vendor/mlib\""
        " \"%s/runtime/player.c\"",
        cc, cflags, sdk, sdk, sdk, sdk);
    for (size_t i = 0; i < sizeof(PLAYER_DATA_LAYER) / sizeof(PLAYER_DATA_LAYER[0]); i++)
        n += snprintf(cmd + n, (size_t)cmdSize - n, " \"%s/%s\"", sdk, PLAYER_DATA_LAYER[i]);
    for (int i = 0; i < nc && n < cmdSize - 400; i++)
        n += snprintf(cmd + n, (size_t)cmdSize - n, " \"%s/%s\"", srcDir, names[i]);
    n += snprintf(cmd + n, (size_t)cmdSize - n,
        " -o \"%s\" -L\"%s/%s\" -lmgengine", outExe, sdk, libDir);
#if defined(_WIN32)
    n += snprintf(cmd + n, (size_t)cmdSize - n, " -mwindows");
#else
    n += snprintf(cmd + n, (size_t)cmdSize - n, " -lm -Wl,-rpath,'$ORIGIN'");
#endif
    if (release)
        n += snprintf(cmd + n, (size_t)cmdSize - n, " -s");

#if defined(_WIN32)
    char res[820];
    if (build_scene_res(buildDir, proj->name, sdk, outExe, true, log, res, sizeof(res)))
        n += snprintf(cmd + n, (size_t)cmdSize - n, " \"%s\"", res);
#endif
    return true;
}

bool SceneBuild_Compile(const Project* proj, const char* sceneName, bool release,
    BuildLog* log, char* outDll, int outDllSize)
{
    char cmd[8192];
    if (!build_command(proj, sceneName, release, log, outDll, outDllSize, cmd, sizeof(cmd)))
        return false;

    BuildLog_Line(log, "$ %s", cmd);
    strncat(cmd, " 2>&1", sizeof(cmd) - strlen(cmd) - 1); // popen only captures stdout

    FILE* pipe = POPEN(cmd, "r");
    if (pipe == NULL) {
        BuildLog_Line(log, "error: could not run the compiler");
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

// ------------------------------------------------------------- async compile

// stream whatever the child has appended to its log file into the BuildLog
static void pump_log(SceneBuildJob* job)
{
    FILE* f = fopen(job->logFile, "rb");
    if (f == NULL)
        return;
    if (fseek(f, job->logCopied, SEEK_SET) == 0) {
        char buf[1024];
        size_t r;
        while ((r = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
            buf[r] = '\0';
            BuildLog_Append(job->log, buf);
            job->logCopied += (long)r;
        }
    }
    fclose(f);
}

#if defined(_WIN32)
static void* spawn_child(const char* cmd, const char* logFile)
{
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE h = CreateFileA(logFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return NULL;

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = h;
    si.hStdError = h;

    char full[9000];
    snprintf(full, sizeof(full), "cmd.exe /d /c %s", cmd);

    PROCESS_INFORMATION pi = { 0 };
    BOOL ok = CreateProcessA(NULL, full, NULL, NULL, TRUE, CREATE_NO_WINDOW,
        NULL, NULL, &si, &pi);
    CloseHandle(h);
    if (!ok)
        return NULL;
    CloseHandle(pi.hThread);
    return (void*)pi.hProcess;
}

static bool child_done(void* proc, bool* ok)
{
    HANDLE h = (HANDLE)proc;
    if (WaitForSingleObject(h, 0) != WAIT_OBJECT_0)
        return false;
    DWORD code = 1;
    GetExitCodeProcess(h, &code);
    *ok = (code == 0);
    return true;
}

static void child_reap(void* proc, bool finished)
{
    HANDLE h = (HANDLE)proc;
    if (!finished)
        TerminateProcess(h, 1);
    CloseHandle(h);
}
#else
static void* spawn_child(const char* cmd, const char* logFile)
{
    pid_t pid = fork();
    if (pid < 0)
        return NULL;
    if (pid == 0) {
        int fd = open(logFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { dup2(fd, 1); dup2(fd, 2); close(fd); }
        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        _exit(127);
    }
    pid_t* box = malloc(sizeof(*box));
    if (box != NULL)
        *box = pid;
    return box;
}

static bool child_done(void* proc, bool* ok)
{
    pid_t pid = *(pid_t*)proc;
    int st = 0;
    if (waitpid(pid, &st, WNOHANG) != pid)
        return false;
    *ok = (WIFEXITED(st) && WEXITSTATUS(st) == 0);
    return true;
}

static void child_reap(void* proc, bool finished)
{
    pid_t pid = *(pid_t*)proc;
    if (!finished) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
    free(proc);
}
#endif

bool SceneBuild_Start(SceneBuildJob* job, const Project* proj, const char* sceneName,
    bool release, BuildLog* log)
{
    memset(job, 0, sizeof(*job));
    job->log = log;

    char cmd[8192];
    if (!build_command(proj, sceneName, release, log, job->outDll, sizeof(job->outDll),
            cmd, sizeof(cmd)))
        return false;

    char srcDir[700];
    module_src_dir(proj, sceneName, srcDir, sizeof(srcDir));
    snprintf(job->logFile, sizeof(job->logFile), "%s/build/_compile.log", srcDir);

    BuildLog_Line(log, "$ %s", cmd);
    BuildLog_Line(log, "-- compiling (separate process) --");

    job->proc = spawn_child(cmd, job->logFile);
    if (job->proc == NULL) {
        BuildLog_Line(log, "error: could not start the compiler process");
        return false;
    }
    return true;
}

bool SceneBuild_StartExe(SceneBuildJob* job, const Project* proj, bool release,
    BuildLog* log, const char* outExe)
{
    memset(job, 0, sizeof(*job));
    job->log = log;
    snprintf(job->outDll, sizeof(job->outDll), "%s", outExe); // Clear() derives the scratch dir from this

    char cmd[8192];
    if (!build_exe_command(proj, release, log, outExe, cmd, sizeof(cmd)))
        return false;

    char srcDir[700];
    Project_SourceDir(proj, srcDir, sizeof(srcDir));
    snprintf(job->logFile, sizeof(job->logFile), "%s/build/_link.log", srcDir);

    BuildLog_Line(log, "$ %s", cmd);
    BuildLog_Line(log, "-- linking the game exe (separate process) --");

    job->proc = spawn_child(cmd, job->logFile);
    if (job->proc == NULL) {
        BuildLog_Line(log, "error: could not start the compiler process");
        return false;
    }
    return true;
}

bool SceneBuild_Poll(SceneBuildJob* job)
{
    if (job->proc == NULL)
        return true;
    if (job->finished)
        return true;

    pump_log(job);

    bool ok = false;
    if (!child_done(job->proc, &ok))
        return false;

    pump_log(job); // final flush
    job->ok = ok;
    job->finished = true;
    if (ok)
        BuildLog_Line(job->log, "-- build ok: %s --", job->outDll);
    else
        BuildLog_Line(job->log, "-- build FAILED --");
    return true;
}

void SceneBuild_Clear(SceneBuildJob* job)
{
    if (job->proc != NULL)
        child_reap(job->proc, job->finished);
    if (job->logFile[0] != '\0') {
        remove(job->logFile);
        // the generated version-info scratch (Windows) lands in the build dir
        // beside the log file -- for both a scene .dll and a static-game .exe
        char dir[820], p[900];
        Path_Dir(job->logFile, dir, sizeof(dir));
        Path_Join(dir, "_scene.rc", p, sizeof(p));
        remove(p);
        Path_Join(dir, "_scene.res", p, sizeof(p));
        remove(p);
    }
    memset(job, 0, sizeof(*job));
}
