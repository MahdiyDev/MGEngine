// .pak archive: a directory tree packed into one logical byte stream (header +
// table of contents + concatenated file blobs), physically split into
// <stem>.pak.001, <stem>.pak.002, ... at a size limit. Its own translation unit
// so <dirent.h> / <windows.h> stay out of the renderer.
//
// On-disk layout of the logical stream (first bytes live in <stem>.pak.001):
//   char magic[8]  = "MGENGPAK"
//   u32  version   = 1
//   u32  splitBytes
//   u32  entryCount
//   u32  _reserved
//   PakEntry entries[entryCount] { char path[256]; u64 offset; u64 size; u32 crc32; u32 _pad; }
//   ... blob data, in entry order, `offset` measured from the first byte after the TOC ...
// Physical file boundaries sit every `splitBytes` from the start of .001.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mge_pak.h"

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

#define PAK_MAGIC "MGENGPAK"
#define PAK_VERSION 1u
#define PAK_PATH_LEN 256

#pragma pack(push, 1)
typedef struct PakEntry {
    char path[PAK_PATH_LEN];
    uint64_t offset;
    uint64_t size;
    uint32_t crc32;
    uint32_t _pad;
} PakEntry;

typedef struct PakHeader {
    char magic[8];
    uint32_t version;
    uint32_t splitBytes;
    uint32_t entryCount;
    uint32_t _reserved;
} PakHeader;
#pragma pack(pop)

// ------------------------------------------------------------------ crc32

static uint32_t s_crcTable[256];
static int s_crcReady = 0;

static void crc_init(void)
{
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        s_crcTable[n] = c;
    }
    s_crcReady = 1;
}

uint32_t Mge_Crc32(const void* data, size_t len)
{
    if (!s_crcReady)
        crc_init();
    const unsigned char* p = (const unsigned char*)data;
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++)
        c = s_crcTable[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// ------------------------------------------------------------------ split-file IO

typedef struct SplitWriter {
    char stem[512];
    uint32_t splitBytes;
    FILE* f;
    int fileIndex;      // 0 -> .001
    uint32_t posInFile; // bytes written to the current physical file
    int ok;
} SplitWriter;

static void sw_open_next(SplitWriter* w)
{
    if (w->f != NULL)
        fclose(w->f);
    w->fileIndex++;
    char path[560];
    snprintf(path, sizeof(path), "%s.pak.%03d", w->stem, w->fileIndex);
    w->f = fopen(path, "wb");
    w->posInFile = 0;
    if (w->f == NULL)
        w->ok = 0;
}

static void sw_write(SplitWriter* w, const void* data, size_t len)
{
    if (!w->ok)
        return;
    const unsigned char* p = (const unsigned char*)data;
    while (len > 0) {
        if (w->posInFile >= w->splitBytes)
            sw_open_next(w);
        if (!w->ok)
            return;
        uint32_t room = w->splitBytes - w->posInFile;
        size_t chunk = (len < room) ? len : room;
        if (fwrite(p, 1, chunk, w->f) != chunk) {
            w->ok = 0;
            return;
        }
        w->posInFile += (uint32_t)chunk;
        p += chunk;
        len -= chunk;
    }
}

// ------------------------------------------------------------------ directory walk

typedef struct FileList {
    char (*rel)[PAK_PATH_LEN];
    uint64_t* size;
    int count, cap;
} FileList;

// dirs never packed (generated build output); file extensions never packed
// (native code the OS loader must open directly)
static bool skip_dir(const char* name)
{
    return strcmp(name, "build") == 0 || strcmp(name, "dist") == 0 || strcmp(name, ".git") == 0;
}
static bool ext_is(const char* name, const char* ext)
{
    size_t n = strlen(name), e = strlen(ext);
    return n > e && strcmp(name + n - e, ext) == 0;
}
static bool skip_file(const char* name)
{
    // native code the OS loader opens directly, and build inputs -- not assets
    return ext_is(name, ".dll") || ext_is(name, ".exe") || ext_is(name, ".so") ||
        ext_is(name, ".c") || ext_is(name, ".h") || ext_is(name, ".o") ||
        ext_is(name, ".obj") || ext_is(name, ".d") ||
        strstr(name, ".pak.") != NULL; // don't nest paks
}

static void fl_add(FileList* fl, const char* rel, uint64_t size)
{
    if (fl->count == fl->cap) {
        fl->cap = fl->cap ? fl->cap * 2 : 64;
        fl->rel = realloc(fl->rel, sizeof(fl->rel[0]) * (size_t)fl->cap);
        fl->size = realloc(fl->size, sizeof(fl->size[0]) * (size_t)fl->cap);
    }
    snprintf(fl->rel[fl->count], PAK_PATH_LEN, "%s", rel);
    fl->size[fl->count] = size;
    fl->count++;
}

#if defined(_WIN32)
static void walk(const char* base, const char* rel, FileList* fl)
{
    char pat[1024];
    snprintf(pat, sizeof(pat), "%s\\%s\\*", base, rel[0] ? rel : ".");
    for (char* c = pat; *c; c++)
        if (*c == '/')
            *c = '\\';

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        char childRel[PAK_PATH_LEN];
        snprintf(childRel, sizeof(childRel), "%.180s%s%.60s", rel, rel[0] ? "/" : "", fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!skip_dir(fd.cFileName))
                walk(base, childRel, fl);
        } else if (!skip_file(fd.cFileName)) {
            uint64_t sz = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            fl_add(fl, childRel, sz);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
#else
static void walk(const char* base, const char* rel, FileList* fl)
{
    char dir[1024];
    snprintf(dir, sizeof(dir), "%s/%s", base, rel[0] ? rel : ".");
    DIR* d = opendir(dir);
    if (d == NULL)
        return;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
            continue;
        char childRel[PAK_PATH_LEN], full[1200];
        snprintf(childRel, sizeof(childRel), "%.180s%s%.60s", rel, rel[0] ? "/" : "", e->d_name);
        snprintf(full, sizeof(full), "%s/%s", base, childRel);
        struct stat st;
        if (stat(full, &st) != 0)
            continue;
        if (S_ISDIR(st.st_mode)) {
            if (!skip_dir(e->d_name))
                walk(base, childRel, fl);
        } else if (!skip_file(e->d_name)) {
            fl_add(fl, childRel, (uint64_t)st.st_size);
        }
    }
    closedir(d);
}
#endif

// ------------------------------------------------------------------ writer

bool Mge_PakWrite(const char* stem, const char* rootDir, uint32_t splitBytes)
{
    if (splitBytes < 4096)
        splitBytes = 4096;

    FileList fl = { 0 };
    walk(rootDir, "", &fl);

    PakHeader hdr = { 0 };
    memcpy(hdr.magic, PAK_MAGIC, 8);
    hdr.version = PAK_VERSION;
    hdr.splitBytes = splitBytes;
    hdr.entryCount = (uint32_t)fl.count;

    PakEntry* entries = calloc((size_t)(fl.count > 0 ? fl.count : 1), sizeof(PakEntry));
    uint64_t off = 0;
    for (int i = 0; i < fl.count; i++) {
        snprintf(entries[i].path, PAK_PATH_LEN, "%s", fl.rel[i]);
        entries[i].offset = off;
        entries[i].size = fl.size[i];
        off += fl.size[i];
    }

    SplitWriter w = { 0 };
    snprintf(w.stem, sizeof(w.stem), "%s", stem);
    w.splitBytes = splitBytes;
    w.fileIndex = 0;
    w.ok = 1;
    sw_open_next(&w); // opens .001

    sw_write(&w, &hdr, sizeof(hdr));
    sw_write(&w, entries, sizeof(PakEntry) * (size_t)fl.count);

    for (int i = 0; i < fl.count && w.ok; i++) {
        char full[1200];
        snprintf(full, sizeof(full), "%s/%s", rootDir, fl.rel[i]);
        FILE* in = fopen(full, "rb");
        if (in == NULL) {
            w.ok = 0;
            break;
        }
        unsigned char buf[65536];
        size_t n;
        uint32_t crc = 0xFFFFFFFFu;
        if (!s_crcReady)
            crc_init();
        while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
            for (size_t k = 0; k < n; k++)
                crc = s_crcTable[(crc ^ buf[k]) & 0xFF] ^ (crc >> 8);
            sw_write(&w, buf, n);
        }
        fclose(in);
        entries[i].crc32 = crc ^ 0xFFFFFFFFu;
    }

    // rewrite the TOC now that crcs are known: reopen .001 and patch it in place
    int ok = w.ok;
    if (w.f != NULL)
        fclose(w.f);
    if (ok) {
        char p1[560];
        snprintf(p1, sizeof(p1), "%s.pak.001", stem);
        FILE* patch = fopen(p1, "rb+");
        if (patch != NULL) {
            fseek(patch, (long)sizeof(hdr), SEEK_SET);
            fwrite(entries, sizeof(PakEntry), (size_t)fl.count, patch);
            fclose(patch);
        } else {
            ok = 0;
        }
    }

    free(entries);
    free(fl.rel);
    free(fl.size);
    return ok != 0;
}

// ------------------------------------------------------------------ reader

struct Pak {
    char stem[512];
    uint32_t splitBytes;
    uint32_t tocBytes; // sizeof(header) + entries
    PakEntry* entries;
    uint32_t entryCount;
};

Pak* Mge_PakOpen(const char* stem)
{
    char p1[560];
    snprintf(p1, sizeof(p1), "%s.pak.001", stem);
    FILE* f = fopen(p1, "rb");
    if (f == NULL)
        return NULL;

    PakHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 || memcmp(hdr.magic, PAK_MAGIC, 8) != 0 ||
        hdr.version != PAK_VERSION) {
        fclose(f);
        return NULL;
    }

    Pak* pak = calloc(1, sizeof(Pak));
    snprintf(pak->stem, sizeof(pak->stem), "%s", stem);
    pak->splitBytes = hdr.splitBytes;
    pak->entryCount = hdr.entryCount;
    pak->tocBytes = (uint32_t)sizeof(PakHeader) + hdr.entryCount * (uint32_t)sizeof(PakEntry);
    pak->entries = calloc((size_t)(hdr.entryCount ? hdr.entryCount : 1), sizeof(PakEntry));
    if (hdr.entryCount > 0 &&
        fread(pak->entries, sizeof(PakEntry), hdr.entryCount, f) != hdr.entryCount) {
        fclose(f);
        free(pak->entries);
        free(pak);
        return NULL;
    }
    fclose(f);
    return pak;
}

void Mge_PakClose(Pak* pak)
{
    if (pak == NULL)
        return;
    free(pak->entries);
    free(pak);
}

bool Mge_PakHas(const Pak* pak, const char* path)
{
    if (pak == NULL)
        return false;
    for (uint32_t i = 0; i < pak->entryCount; i++)
        if (strcmp(pak->entries[i].path, path) == 0)
            return true;
    return false;
}

// read `size` logical bytes starting at absolute byte `abs` (from start of .001),
// following physical splits. `out` must hold `size` bytes.
static bool read_spanning(const Pak* pak, uint64_t abs, uint64_t size, unsigned char* out)
{
    while (size > 0) {
        int fileIndex = (int)(abs / pak->splitBytes);
        uint32_t byteInFile = (uint32_t)(abs % pak->splitBytes);
        uint32_t room = pak->splitBytes - byteInFile;
        uint64_t chunk = (size < room) ? size : room;

        char path[560];
        snprintf(path, sizeof(path), "%s.pak.%03d", pak->stem, fileIndex + 1);
        FILE* f = fopen(path, "rb");
        if (f == NULL)
            return false;
        if (fseek(f, (long)byteInFile, SEEK_SET) != 0 ||
            fread(out, 1, (size_t)chunk, f) != (size_t)chunk) {
            fclose(f);
            return false;
        }
        fclose(f);

        out += chunk;
        abs += chunk;
        size -= chunk;
    }
    return true;
}

void* Mge_PakRead(const Pak* pak, const char* path, size_t* outSize)
{
    if (pak == NULL)
        return NULL;
    const PakEntry* e = NULL;
    for (uint32_t i = 0; i < pak->entryCount; i++)
        if (strcmp(pak->entries[i].path, path) == 0) {
            e = &pak->entries[i];
            break;
        }
    if (e == NULL)
        return NULL;

    unsigned char* buf = malloc((size_t)e->size + 1);
    if (buf == NULL)
        return NULL;
    if (!read_spanning(pak, pak->tocBytes + e->offset, e->size, buf)) {
        free(buf);
        return NULL;
    }
    buf[e->size] = '\0'; // convenient for text
    if (Mge_Crc32(buf, (size_t)e->size) != e->crc32) {
        free(buf);
        return NULL;
    }
    if (outSize != NULL)
        *outSize = (size_t)e->size;
    return buf;
}

// ------------------------------------------------------------------ mount stack

#define MGE_MAX_MOUNTS 8
static Pak* s_mounts[MGE_MAX_MOUNTS];
static int s_mountCount;

bool Mge_MountPak(const char* stem)
{
    if (s_mountCount >= MGE_MAX_MOUNTS)
        return false;
    Pak* p = Mge_PakOpen(stem);
    if (p == NULL)
        return false;
    s_mounts[s_mountCount++] = p; // last mounted wins on lookup
    return true;
}

void Mge_UnmountPaks(void)
{
    for (int i = 0; i < s_mountCount; i++)
        Mge_PakClose(s_mounts[i]);
    s_mountCount = 0;
}

// used by mge_utils.c's file loader: a loose file on disk always wins; otherwise
// the most recently mounted pak that has `path`.
void* Mge_MountedRead(const char* path, size_t* outSize)
{
    for (int i = s_mountCount - 1; i >= 0; i--) {
        void* d = Mge_PakRead(s_mounts[i], path, outSize);
        if (d != NULL)
            return d;
    }
    return NULL;
}
