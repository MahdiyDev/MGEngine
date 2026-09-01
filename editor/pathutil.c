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

static char norm(char c)
{
    if (c == '\\')
        c = '/';
#if defined(_WIN32)
    if (c >= 'A' && c <= 'Z')
        c += 32;
#endif
    return c;
}

bool Path_Equal(const char* a, const char* b)
{
    size_t la = strlen(a), lb = strlen(b);
    if (la > 0 && (a[la - 1] == '/' || a[la - 1] == '\\'))
        la--;
    if (lb > 0 && (b[lb - 1] == '/' || b[lb - 1] == '\\'))
        lb--;
    if (la != lb)
        return false;
    for (size_t i = 0; i < la; i++)
        if (norm(a[i]) != norm(b[i]))
            return false;
    return true;
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
    if (Path_Equal(src, dst))
        return true; // same file -> opening dst "wb" would truncate src to 0 bytes

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

static bool has_ext(const char* name, const char* ext)
{
    size_t n = strlen(name), e = strlen(ext);
    return n > e && strcmp(name + n - e, ext) == 0;
}

#if defined(_WIN32)
    #include <windows.h>
int Path_List(const char* dir, const char* ext, bool wantDirs, char (*out)[128], int maxOut)
{
    char pat[1024];
    snprintf(pat, sizeof(pat), "%s\\*", dir);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    int n = 0;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (wantDirs != isDir)
            continue;
        if (!wantDirs && ext != NULL && !has_ext(fd.cFileName, ext))
            continue;
        if (n < maxOut)
            snprintf(out[n], 128, "%.127s", fd.cFileName);
        n++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return n < maxOut ? n : maxOut;
}
#else
    #include <dirent.h>
int Path_List(const char* dir, const char* ext, bool wantDirs, char (*out)[128], int maxOut)
{
    DIR* d = opendir(dir);
    if (d == NULL)
        return -1;

    int n = 0;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
            continue;
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(full, &st) != 0)
            continue;
        bool isDir = S_ISDIR(st.st_mode);
        if (wantDirs != isDir)
            continue;
        if (!wantDirs && ext != NULL && !has_ext(e->d_name, ext))
            continue;
        if (n < maxOut)
            snprintf(out[n], 128, "%.127s", e->d_name);
        n++;
    }
    closedir(d);
    return n < maxOut ? n : maxOut;
}
#endif

long Path_MTime(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0) ? (long)st.st_mtime : 0;
}

#if !defined(_WIN32)
    #include <unistd.h>
#endif

void Path_ExeDir(char* out, size_t outSize)
{
    char buf[1024] = { 0 };
#if defined(_WIN32)
    DWORD n = GetModuleFileNameA(NULL, buf, (DWORD)sizeof(buf)); // windows.h: incl above
    if (n == 0 || n >= sizeof(buf)) {
        out[0] = '\0';
        return;
    }
#else
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) {
        out[0] = '\0';
        return;
    }
    buf[n] = '\0';
#endif
    Path_Dir(buf, out, outSize); // also normalises '\\' -> '/'
}

bool Path_NextLine(char** cur, char* out, int outSize)
{
    char* p = *cur;
    if (*p == '\0')
        return false;
    int i = 0;
    while (*p != '\0' && *p != '\n') {
        if (i < outSize - 1)
            out[i++] = *p;
        p++;
    }
    out[i] = '\0';
    if (*p == '\n')
        p++;
    *cur = p;
    return true;
}

bool Path_IsDir(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0 && (st.st_mode & S_IFDIR);
}

bool Path_Rename(const char* from, const char* to)
{
    return rename(from, to) == 0;
}

bool Path_Remove(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return true; // already gone

    if (!(st.st_mode & S_IFDIR))
        return remove(path) == 0;

    char kids[256][128];
    int n = Path_List(path, NULL, false, kids, 256);
    for (int i = 0; i < n; i++) {
        char child[1024];
        Path_Join(path, kids[i], child, sizeof(child));
        Path_Remove(child);
    }
    n = Path_List(path, NULL, true, kids, 256);
    for (int i = 0; i < n; i++) {
        char child[1024];
        Path_Join(path, kids[i], child, sizeof(child));
        Path_Remove(child);
    }
#if defined(_WIN32)
    return _rmdir(path) == 0;
#else
    return rmdir(path) == 0;
#endif
}

bool Path_CopyTree(const char* src, const char* dst)
{
    struct stat st;
    if (stat(src, &st) != 0)
        return false;

    if (!(st.st_mode & S_IFDIR))
        return Path_CopyFile(src, dst);

    if (!Path_MakeDirs(dst))
        return false;

    bool ok = true;
    char names[256][128];

    int n = Path_List(src, NULL, false, names, 256);
    for (int i = 0; i < n; i++) {
        char s[1024], d[1024];
        Path_Join(src, names[i], s, sizeof(s));
        Path_Join(dst, names[i], d, sizeof(d));
        ok = Path_CopyFile(s, d) && ok;
    }
    n = Path_List(src, NULL, true, names, 256);
    for (int i = 0; i < n; i++) {
        char s[1024], d[1024];
        Path_Join(src, names[i], s, sizeof(s));
        Path_Join(dst, names[i], d, sizeof(d));
        ok = Path_CopyTree(s, d) && ok;
    }
    return ok;
}
