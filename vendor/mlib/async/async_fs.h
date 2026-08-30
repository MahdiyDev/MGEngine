#pragma once

// File I/O as cooperative tasks: each poll moves one chunk, yielding between
// chunks so a large file does not stall the loop. Binary mode ("rb" / "wb").
// Built on async.h + string.h (compile with -I<repo> -I<repo>/vec).

#include <stdio.h>

#include "async.h"
#include "../string/string.h"

#ifndef ASYNC_FS_CHUNK
    #define ASYNC_FS_CHUNK 4096
#endif

typedef struct {
    FILE* file;
    string_builder data;          // read: the result; write: a copy of the source
    size_t pos;                   // write cursor
} AsyncFile;

static inline CoStatus async_fs_read_poll_(Task* t)
{
    AsyncFile* f = (AsyncFile*)t->ctx;
    char buf[ASYNC_FS_CHUNK];
    co_begin(&t->co);
    for (;;) {
        {
            size_t n = fread(buf, 1, sizeof(buf), f->file);
            if (n > 0) {
                string_builder_append_many(&f->data, buf, n);
            }
            if (n < sizeof(buf)) {
                break; // short read: EOF or error
            }
        }
        co_yield(&t->co);
    }
    if (ferror(f->file)) {
        co_fail(&t->co, "read error");
    }
    co_return(&t->co, &f->data);
    co_end(&t->co);
}

static inline CoStatus async_fs_write_poll_(Task* t)
{
    AsyncFile* f = (AsyncFile*)t->ctx;
    co_begin(&t->co);
    while (f->pos < f->data.count) {
        {
            size_t chunk = f->data.count - f->pos;
            if (chunk > ASYNC_FS_CHUNK) {
                chunk = ASYNC_FS_CHUNK;
            }
            size_t w = fwrite(f->data.items + f->pos, 1, chunk, f->file);
            f->pos += w;
            if (w < chunk) {
                co_fail(&t->co, "write error");
            }
        }
        co_yield(&t->co);
    }
    co_return(&t->co, NULL);
    co_end(&t->co);
}

static inline AsyncFile* async_file_new_(const char* path, const char* mode)
{
    FILE* fp = fopen(path, mode);
    if (fp == NULL) {
        return NULL;
    }
    AsyncFile* f = (AsyncFile*)ASYNC_MALLOC(sizeof(AsyncFile));
    ASYNC_ASSERT(f != NULL && "async_fs: allocation failed");
    f->file = fp;
    f->data = (string_builder){ 0 };
    f->pos = 0;
    return f;
}

// Read `path` into a string_builder. task_result() is a string_builder*.
// Returns NULL if the file could not be opened.
static inline Task* async_read_file_prio(Sched* s, const char* path, int prio)
{
    AsyncFile* f = async_file_new_(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    return sched_spawn_prio(s, async_fs_read_poll_, f, prio);
}

static inline Task* async_read_file(Sched* s, const char* path)
{
    return async_read_file_prio(s, path, 0);
}

// Write `len` bytes of `data` to `path`. `data` is copied, so it need not
// outlive the task. Returns NULL if the file could not be opened.
static inline Task* async_write_file_prio(Sched* s, const char* path, const void* data, size_t len, int prio)
{
    AsyncFile* f = async_file_new_(path, "wb");
    if (f == NULL) {
        return NULL;
    }
    string_builder_append_many(&f->data, data, len);
    return sched_spawn_prio(s, async_fs_write_poll_, f, prio);
}

static inline Task* async_write_file(Sched* s, const char* path, const void* data, size_t len)
{
    return async_write_file_prio(s, path, data, len, 0);
}

// Close the file and free the task's AsyncFile context. Call once the task is
// done and its result has been read. The Task struct is still freed by sched_free.
static inline void async_file_close(Task* t)
{
    AsyncFile* f = (AsyncFile*)t->ctx;
    if (f == NULL) {
        return;
    }
    if (f->file != NULL) {
        fclose(f->file);
    }
    string_builder_free(&f->data);
    ASYNC_FREE(f);
    t->ctx = NULL;
}
