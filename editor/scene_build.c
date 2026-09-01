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

// Assemble the compiler command line for a scene (no shell redirection appended).
// Also creates <sceneDir>/build and reports the produced .dll path. Returns false
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

    // gcc <cflags> -shared -std=c11 -I<sdk>/source <scene>/*.c -o <dll> -L<sdk>/build[/release] -lmgengine
    // (the compiler name is left unquoted so cmd.exe doesn't strip the first "path" quote)
    const char* libDir = release ? "build/release" : "build";
    int n = snprintf(cmd, (size_t)cmdSize,
        "%s %s -shared -std=c11 -DPLATFORM_DESKTOP -I\"%s/source\"",
        cc, cflags, sdk);
    for (int i = 0; i < nc && n < cmdSize - 300; i++)
        n += snprintf(cmd + n, (size_t)cmdSize - n, " \"%s/%s\"", sceneDir, names[i]);
    n += snprintf(cmd + n, (size_t)cmdSize - n,
        " -o \"%s\" -L\"%s/%s\" -lmgengine", outDll, sdk, libDir);
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

    char sceneDir[700];
    Project_SceneDir(proj, sceneName, sceneDir, sizeof(sceneDir));
    snprintf(job->logFile, sizeof(job->logFile), "%s/build/_compile.log", sceneDir);

    BuildLog_Line(log, "$ %s", cmd);
    BuildLog_Line(log, "-- compiling (separate process) --");

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
    if (job->logFile[0] != '\0')
        remove(job->logFile);
    memset(job, 0, sizeof(*job));
}
