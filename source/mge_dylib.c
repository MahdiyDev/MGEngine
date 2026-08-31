// Dynamic library loading -- the host side of the hot-reloadable scene-module
// contract (see MgeSceneCtx in mge.h). Its own translation unit so <windows.h>
// / <dlfcn.h> stay out of the renderer and the stubbed-GL unit tests.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// No "mge.h": on Windows <windows.h> collides with the engine's `Rectangle` /
// `ShowCursor` names. These four prototypes are the whole surface.
void* Mge_LoadLibrary(const char* path);
void* Mge_GetSymbol(void* handle, const char* name);
void Mge_FreeLibrary(void* handle);
const char* Mge_GetDylibError(void);

static char s_err[512];

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void set_last_error(const char* what)
{
    DWORD e = GetLastError();
    char msg[256] = { 0 };
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, e,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg, sizeof(msg), NULL);
    for (char* c = msg; *c; c++)
        if (*c == '\r' || *c == '\n')
            *c = ' ';
    snprintf(s_err, sizeof(s_err), "%s: [%lu] %s", what, (unsigned long)e, msg);
}

void* Mge_LoadLibrary(const char* path)
{
    s_err[0] = '\0';
    HMODULE h = LoadLibraryA(path);
    if (h == NULL)
        set_last_error("LoadLibrary");
    return (void*)h;
}

void* Mge_GetSymbol(void* handle, const char* name)
{
    s_err[0] = '\0';
    if (handle == NULL) {
        snprintf(s_err, sizeof(s_err), "GetSymbol: null handle");
        return NULL;
    }
    FARPROC p = GetProcAddress((HMODULE)handle, name);
    if (p == NULL)
        set_last_error("GetProcAddress");
    return (void*)(intptr_t)p;
}

void Mge_FreeLibrary(void* handle)
{
    if (handle != NULL)
        FreeLibrary((HMODULE)handle);
}

#else

#include <dlfcn.h>

void* Mge_LoadLibrary(const char* path)
{
    s_err[0] = '\0';
    void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (h == NULL)
        snprintf(s_err, sizeof(s_err), "dlopen: %s", dlerror());
    return h;
}

void* Mge_GetSymbol(void* handle, const char* name)
{
    s_err[0] = '\0';
    if (handle == NULL) {
        snprintf(s_err, sizeof(s_err), "dlsym: null handle");
        return NULL;
    }
    dlerror();
    void* p = dlsym(handle, name);
    const char* e = dlerror();
    if (e != NULL) {
        snprintf(s_err, sizeof(s_err), "dlsym: %s", e);
        return NULL;
    }
    return p;
}

void Mge_FreeLibrary(void* handle)
{
    if (handle != NULL)
        dlclose(handle);
}

#endif

const char* Mge_GetDylibError(void)
{
    return s_err;
}
