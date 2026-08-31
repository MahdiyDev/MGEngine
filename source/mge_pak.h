// .pak archives -- a directory packed into split <stem>.pak.NNN files. See
// mge_pak.c for the on-disk format. Kept in its own header (no <mge.h>) so the
// file-loader in mge_utils.c can call it without a cycle.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Pak Pak;

// Pack every file under `rootDir` into `<stem>.pak.001` (+ .002 ... when the
// running size passes `splitBytes`). Paths are stored `rootDir`-relative with
// '/' separators. Returns true on success.
bool Mge_PakWrite(const char* stem, const char* rootDir, uint32_t splitBytes);

Pak* Mge_PakOpen(const char* stem);
void Mge_PakClose(Pak* pak);
bool Mge_PakHas(const Pak* pak, const char* path);

// Read one packed file. Returns a malloc'd buffer (NUL-terminated past `*outSize`
// for convenience) the caller frees, or NULL if absent / corrupt (crc check).
void* Mge_PakRead(const Pak* pak, const char* path, size_t* outSize);

uint32_t Mge_Crc32(const void* data, size_t len);

// --- mount stack (consulted by Mge_LoadFileData / Mge_LoadImage etc.) ---
bool Mge_MountPak(const char* stem);   // last mounted wins; loose files still win over any pak
void Mge_UnmountPaks(void);
void* Mge_MountedRead(const char* path, size_t* outSize); // internal
