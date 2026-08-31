// Tiny path helpers for the editor's scene I/O. Windows + POSIX separators are
// both treated as dividers; output always uses '/'.
#pragma once

#include <stdbool.h>
#include <stddef.h>

// Directory part of `path` (everything before the last '/' or '\\'), into `out`.
// No trailing slash. "" when `path` has no directory part.
void Path_Dir(const char* path, char* out, size_t outSize);

// File-name part of `path` (everything after the last separator), into `out`.
void Path_Base(const char* path, char* out, size_t outSize);

// Drop the last extension from `name` in place ("scene.mgscene" -> "scene").
void Path_StripExt(char* name);

// true for "C:\x", "C:/x", "/x", "\\server\share".
bool Path_IsAbsolute(const char* path);

// Compare two paths for equality: '/' and '\\' are equivalent, a single
// trailing separator is ignored, and the comparison is case-insensitive on
// Windows.
bool Path_Equal(const char* a, const char* b);

// Join `a` and `b` with a single '/' (or just `b` when it is absolute), into
// `out`. Safe when `out` aliases neither input.
void Path_Join(const char* a, const char* b, char* out, size_t outSize);

// Create `dir` and any missing parents. Returns true on success or if it already
// exists.
bool Path_MakeDirs(const char* dir);

// Copy the file at `src` to `dst` (binary). Returns true on success.
bool Path_CopyFile(const char* src, const char* dst);

// List entries of `dir` into `out` (each <= 128 chars, name only). `ext` filters
// files by extension (e.g. ".c"); pass NULL for "all files". `wantDirs` lists
// subdirectories instead of files (`ext` ignored). "." / ".." are skipped.
// Returns the count written (capped at `maxOut`), or -1 if `dir` can't be read.
int Path_List(const char* dir, const char* ext, bool wantDirs, char (*out)[128], int maxOut);

// Last-modified time of `path` as a Unix timestamp, or 0 if it can't be stat'd.
long Path_MTime(const char* path);
