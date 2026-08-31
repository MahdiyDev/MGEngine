#include "pathutil.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #define MKDIR(p) mkdir(p, 0755)
#endif

static int last_sep(const char* path)
{
    int at = -1;
    for (int i = 0; path[i] != '\0'; i++)
        if (path[i] == '/' || path[i] == '\\')
            at = i;
    return at;
}

void Path_Dir(const char* path, char* out, size_t outSize)
{
    int at = last_sep(path);
    if (at <= 0) {
        out[0] = '\0';
        return;
    }
    size_t n = (size_t)at < outSize - 1 ? (size_t)at : outSize - 1;
    memcpy(out, path, n);
    out[n] = '\0';
    for (size_t i = 0; i < n; i++)
        if (out[i] == '\\')
            out[i] = '/';
}

void Path_Base(const char* path, char* out, size_t outSize)
{
    int at = last_sep(path);
    snprintf(out, outSize, "%s", path + at + 1);
}

void Path_StripExt(char* name)
{
    char* dot = strrchr(name, '.');
    if (dot != NULL && dot != name)
        *dot = '\0';
}

bool Path_IsAbsolute(const char* path)
{
    if (path[0] == '/' || path[0] == '\\')
        return true;
    return path[0] != '\0' && path[1] == ':'; // drive letter
}

void Path_Join(const char* a, const char* b, char* out, size_t outSize)
{
    if (Path_IsAbsolute(b) || a[0] == '\0') {
        snprintf(out, outSize, "%s", b);
    } else {
        size_t la = strlen(a);
        bool slash = la > 0 && (a[la - 1] == '/' || a[la - 1] == '\\');
        snprintf(out, outSize, "%s%s%s", a, slash ? "" : "/", b);
    }
    for (size_t i = 0; out[i] != '\0'; i++)
        if (out[i] == '\\')
            out[i] = '/';
}

bool Path_MakeDirs(const char* dir)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s", dir);
    for (size_t i = 0; buf[i] != '\0'; i++)
        if (buf[i] == '\\')
            buf[i] = '/';

    for (char* p = buf + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            if (buf[1] != '\0' && !(buf[1] == ':' && buf[2] == '\0'))
                MKDIR(buf); // ignore "exists"
            *p = '/';
        }
    }
    MKDIR(buf);

    struct stat st;
    return stat(buf, &st) == 0 && (st.st_mode & S_IFDIR);
}

bool Path_CopyFile(const char* src, const char* dst)
{
    FILE* in = fopen(src, "rb");
    if (in == NULL)
        return false;
    FILE* out = fopen(dst, "wb");
    if (out == NULL) {
        fclose(in);
        return false;
    }

    char buf[8192];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) {
            ok = false;
            break;
        }

    fclose(in);
    fclose(out);
    return ok;
}
