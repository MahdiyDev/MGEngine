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

// Drop the last extension from `name` in place ("scene.mge" -> "scene").
void Path_StripExt(char* name);

// true for "C:\x", "C:/x", "/x", "\\server\share".
bool Path_IsAbsolute(const char* path);

// Join `a` and `b` with a single '/' (or just `b` when it is absolute), into
// `out`. Safe when `out` aliases neither input.
void Path_Join(const char* a, const char* b, char* out, size_t outSize);

// Create `dir` and any missing parents. Returns true on success or if it already
// exists.
bool Path_MakeDirs(const char* dir);

// Copy the file at `src` to `dst` (binary). Returns true on success.
bool Path_CopyFile(const char* src, const char* dst);
