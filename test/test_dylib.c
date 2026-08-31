// Mge_LoadLibrary / GetSymbol / FreeLibrary (source/mge_dylib.c). Compiles a
// trivial shared library with the C compiler, loads it, calls into it.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

void* Mge_LoadLibrary(const char* path);
void* Mge_GetSymbol(void* handle, const char* name);
void Mge_FreeLibrary(void* handle);
const char* Mge_GetDylibError(void);

#if defined(_WIN32)
    #define DL_EXT ".dll"
#else
    #define DL_EXT ".so"
#endif

typedef int (*add_fn)(int, int);

TEST(load_call_free)
{
    const char* src = "dylib_probe.c";
    const char* lib = "dylib_probe" DL_EXT;

    FILE* f = fopen(src, "wb");
    CHECK(f != NULL);
    fprintf(f, "int probe_add(int a, int b) { return a + b; }\n");
    fclose(f);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s -shared -fPIC -o %s %s 2>%s",
        getenv("CC") ? getenv("CC") : "gcc", lib, src, "dylib_probe.buildlog");
    int rc = system(cmd);
    CHECK(rc == 0);

    char libpath[64];
    snprintf(libpath, sizeof(libpath), "./%s", lib);
    void* h = Mge_LoadLibrary(libpath);
    CHECK(h != NULL);
    if (h == NULL)
        printf("  load error: %s\n", Mge_GetDylibError());

    add_fn add = (add_fn)Mge_GetSymbol(h, "probe_add");
    CHECK(add != NULL);
    if (add != NULL)
        CHECK(add(20, 22) == 42);

    CHECK(Mge_GetSymbol(h, "does_not_exist") == NULL);

    Mge_FreeLibrary(h);

    CHECK(Mge_LoadLibrary("no_such_library_here" DL_EXT) == NULL);
    CHECK(Mge_GetDylibError()[0] != '\0');

    remove(src);
    remove(lib);
    remove("dylib_probe.buildlog");
}

int main(void)
{
    RUN(load_call_free);
    return test_summary();
}
