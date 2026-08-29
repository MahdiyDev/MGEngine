#pragma once

// Lazy, pull-based streams -- Java Stream / iterator style, same generator
// pattern as vec.h / list.h. Nothing runs until a terminal operation, so a
// source may be infinite. Plain C11, no typeof, no extensions.
//
//     DEFINE_STREAM(int, IntStream);
//
//     static void square(int* x)        { *x *= *x; }
//     static bool is_even(const int* x) { return *x % 2 == 0; }
//     static void print_i(const int* x) { printf("%d ", *x); }
//
//     int a[] = { 1, 2, 3, 4, 5, 6 };
//     IntStream_for_each(
//         IntStream_map(
//             IntStream_filter(IntStream_from_array(a, 6), is_even),
//             square),
//         print_i);                    // 4 16 36
//
// "Move" semantics: every combinator and terminal CONSUMES the stream handed to
// it. Once you pass a stream on, that variable is spent -- don't read it, pass
// it again, or free it. A terminal frees the whole pipeline; call Name##_free()
// only to drop a pipeline that never reached a terminal. A zeroed stream ({0})
// is a valid empty stream.
//
// DEFINE_STREAM(T, Name) generates (T = any assignable complete type):
//   sources    Name##_from_array(const T*, size_t)   -- borrows, does not copy
//              Name##_from_fn(gen, ctx, free_ctx)     -- custom / infinite source
//   lazy ops   Name##_map(Name, void (*)(T*))        -- in place, T -> T
//              Name##_filter(Name, bool (*)(const T*))
//              Name##_take(Name, size_t)             -- first n
//              Name##_skip(Name, size_t)             -- drop first n
//              Name##_peek(Name, void (*)(const T*)) -- side effect, pass through
//   terminals  Name##_for_each / _for_each_ctx, Name##_reduce, Name##_count,
//              Name##_any, Name##_all, Name##_find, Name##_collect
//   primitive  Name##_next(Name*, T*)   -- pull one; false when exhausted
//   cleanup    Name##_free(Name*)
//
// Opt-in extras:
//   DEFINE_STREAM_RANGE(T, Name)       -> Name##_from_range(start, stop, step)   (numeric T)
//   DEFINE_STREAM_MAP(T, Src, U, Dst)  -> Src##_map_to_##Dst(Src, U (*)(const T*))

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef STREAM_MALLOC
    #define STREAM_MALLOC malloc
#endif
#ifndef STREAM_FREE
    #define STREAM_FREE free
#endif
#ifndef STREAM_ASSERT
    #define STREAM_ASSERT assert
#endif

enum {
    STREAM_KIND_EMPTY = 0,
    STREAM_KIND_ARRAY,
    STREAM_KIND_FN,
    STREAM_KIND_MAP,
    STREAM_KIND_FILTER,
    STREAM_KIND_TAKE,
    STREAM_KIND_SKIP,
    STREAM_KIND_PEEK
};

#define DEFINE_STREAM(T, Name) \
    typedef struct Name { \
        int kind; \
        struct Name* up;                       /* upstream node, heap, owned */ \
        const T* arr;                           /* ARRAY source */ \
        size_t arr_i; \
        size_t arr_n; \
        bool (*gen)(void* ctx, T* out);         /* FN source */ \
        void* gen_ctx; \
        void (*gen_free)(void* ctx); \
        void (*map_fn)(T* item);                /* MAP / PEEK */ \
        void (*peek_fn)(const T* item); \
        bool (*pred)(const T* item);            /* FILTER */ \
        size_t n;                               /* TAKE / SKIP counter */ \
    } Name; \
 \
    static inline Name* Name##_heapdup_(Name src) \
    { \
        Name* p = (Name*)STREAM_MALLOC(sizeof(Name)); \
        STREAM_ASSERT(p != NULL && "stream node allocation failed"); \
        *p = src; \
        return p; \
    } \
 \
    /* Pull the next item into *out; false once the stream is exhausted. */ \
    static inline bool Name##_next(Name* s, T* out) \
    { \
        switch (s->kind) { \
        case STREAM_KIND_ARRAY: \
            if (s->arr_i >= s->arr_n) { \
                return false; \
            } \
            *out = s->arr[s->arr_i++]; \
            return true; \
        case STREAM_KIND_FN: \
            return s->gen(s->gen_ctx, out); \
        case STREAM_KIND_MAP: \
            if (!Name##_next(s->up, out)) { \
                return false; \
            } \
            s->map_fn(out); \
            return true; \
        case STREAM_KIND_FILTER: \
            while (Name##_next(s->up, out)) { \
                if (s->pred(out)) { \
                    return true; \
                } \
            } \
            return false; \
        case STREAM_KIND_TAKE: \
            if (s->n == 0 || !Name##_next(s->up, out)) { \
                s->n = 0; \
                return false; \
            } \
            s->n--; \
            return true; \
        case STREAM_KIND_SKIP: \
            while (s->n > 0) { \
                T discard; \
                if (!Name##_next(s->up, &discard)) { \
                    s->n = 0; \
                    return false; \
                } \
                s->n--; \
            } \
            return Name##_next(s->up, out); \
        case STREAM_KIND_PEEK: \
            if (!Name##_next(s->up, out)) { \
                return false; \
            } \
            s->peek_fn(out); \
            return true; \
        default: /* STREAM_KIND_EMPTY */ \
            return false; \
        } \
    } \
 \
    /* Release the pipeline. Idempotent; NULL-safe. */ \
    static inline void Name##_free(Name* s) \
    { \
        if (s == NULL) { \
            return; \
        } \
        for (Name* n = s->up; n != NULL;) { \
            Name* up = n->up; \
            if (n->kind == STREAM_KIND_FN && n->gen_free != NULL) { \
                n->gen_free(n->gen_ctx); \
            } \
            STREAM_FREE(n); \
            n = up; \
        } \
        if (s->kind == STREAM_KIND_FN && s->gen_free != NULL) { \
            s->gen_free(s->gen_ctx); \
        } \
        s->kind = STREAM_KIND_EMPTY; \
        s->up = NULL; \
        s->gen_free = NULL; \
    } \
 \
    static inline Name Name##_from_array(const T* items, size_t count) \
    { \
        return (Name){ .kind = STREAM_KIND_ARRAY, .arr = items, .arr_n = count }; \
    } \
 \
    static inline Name Name##_from_fn(bool (*gen)(void* ctx, T* out), void* ctx, void (*free_ctx)(void* ctx)) \
    { \
        return (Name){ .kind = STREAM_KIND_FN, .gen = gen, .gen_ctx = ctx, .gen_free = free_ctx }; \
    } \
 \
    static inline Name Name##_map(Name up, void (*fn)(T* item)) \
    { \
        return (Name){ .kind = STREAM_KIND_MAP, .up = Name##_heapdup_(up), .map_fn = fn }; \
    } \
 \
    static inline Name Name##_filter(Name up, bool (*pred)(const T* item)) \
    { \
        return (Name){ .kind = STREAM_KIND_FILTER, .up = Name##_heapdup_(up), .pred = pred }; \
    } \
 \
    static inline Name Name##_take(Name up, size_t count) \
    { \
        return (Name){ .kind = STREAM_KIND_TAKE, .up = Name##_heapdup_(up), .n = count }; \
    } \
 \
    static inline Name Name##_skip(Name up, size_t count) \
    { \
        return (Name){ .kind = STREAM_KIND_SKIP, .up = Name##_heapdup_(up), .n = count }; \
    } \
 \
    static inline Name Name##_peek(Name up, void (*fn)(const T* item)) \
    { \
        return (Name){ .kind = STREAM_KIND_PEEK, .up = Name##_heapdup_(up), .peek_fn = fn }; \
    } \
 \
    static inline void Name##_for_each(Name s, void (*fn)(const T* item)) \
    { \
        T item; \
        while (Name##_next(&s, &item)) { \
            fn(&item); \
        } \
        Name##_free(&s); \
    } \
 \
    static inline void Name##_for_each_ctx(Name s, void (*fn)(const T* item, void* ctx), void* ctx) \
    { \
        T item; \
        while (Name##_next(&s, &item)) { \
            fn(&item, ctx); \
        } \
        Name##_free(&s); \
    } \
 \
    static inline T Name##_reduce(Name s, T init, T (*fn)(T acc, const T* item)) \
    { \
        T item; \
        T acc = init; \
        while (Name##_next(&s, &item)) { \
            acc = fn(acc, &item); \
        } \
        Name##_free(&s); \
        return acc; \
    } \
 \
    static inline size_t Name##_count(Name s) \
    { \
        T item; \
        size_t k = 0; \
        while (Name##_next(&s, &item)) { \
            k++; \
        } \
        Name##_free(&s); \
        return k; \
    } \
 \
    static inline bool Name##_any(Name s, bool (*pred)(const T* item)) \
    { \
        T item; \
        bool hit = false; \
        while (!hit && Name##_next(&s, &item)) { \
            hit = pred(&item); \
        } \
        Name##_free(&s); \
        return hit; \
    } \
 \
    static inline bool Name##_all(Name s, bool (*pred)(const T* item)) \
    { \
        T item; \
        bool ok = true; \
        while (ok && Name##_next(&s, &item)) { \
            ok = pred(&item); \
        } \
        Name##_free(&s); \
        return ok; \
    } \
 \
    static inline bool Name##_find(Name s, bool (*pred)(const T* item), T* out) \
    { \
        T item; \
        bool hit = false; \
        while (Name##_next(&s, &item)) { \
            if (pred(&item)) { \
                if (out != NULL) { \
                    *out = item; \
                } \
                hit = true; \
                break; \
            } \
        } \
        Name##_free(&s); \
        return hit; \
    } \
 \
    /* Copy up to `max` items into `out`; returns how many were written. */ \
    static inline size_t Name##_collect(Name s, T* out, size_t max) \
    { \
        T item; \
        size_t k = 0; \
        while (k < max && Name##_next(&s, &item)) { \
            out[k++] = item; \
        } \
        Name##_free(&s); \
        return k; \
    } \
 \
    struct Name##_semicolon_eater_

#define DEFINE_STREAM_RANGE(T, Name) \
    typedef struct { \
        T cur; \
        T stop; \
        T step; \
    } Name##_range_ctx_; \
 \
    static inline bool Name##_range_gen_(void* ctx, T* out) \
    { \
        Name##_range_ctx_* r = (Name##_range_ctx_*)ctx; \
        if (r->step > 0 ? r->cur >= r->stop : r->cur <= r->stop) { \
            return false; \
        } \
        *out = r->cur; \
        r->cur = (T)(r->cur + r->step); \
        return true; \
    } \
 \
    static inline void Name##_range_free_(void* ctx) \
    { \
        STREAM_FREE(ctx); \
    } \
 \
    /* Half-open [start, stop) stepping by `step` (non-zero). */ \
    static inline Name Name##_from_range(T start, T stop, T step) \
    { \
        STREAM_ASSERT(step != 0 && "from_range: step must be non-zero"); \
        Name##_range_ctx_* r = (Name##_range_ctx_*)STREAM_MALLOC(sizeof(*r)); \
        STREAM_ASSERT(r != NULL && "from_range allocation failed"); \
        r->cur = start; \
        r->stop = stop; \
        r->step = step; \
        return Name##_from_fn(Name##_range_gen_, r, Name##_range_free_); \
    } \
 \
    struct Name##_range_semicolon_eater_

#define DEFINE_STREAM_MAP(T, Src, U, Dst) \
    typedef struct { \
        Src source; \
        U (*fn)(const T* item); \
    } Src##_to_##Dst##_ctx_; \
 \
    static inline bool Src##_to_##Dst##_gen_(void* ctx, U* out) \
    { \
        Src##_to_##Dst##_ctx_* x = (Src##_to_##Dst##_ctx_*)ctx; \
        T in; \
        if (!Src##_next(&x->source, &in)) { \
            return false; \
        } \
        *out = x->fn(&in); \
        return true; \
    } \
 \
    static inline void Src##_to_##Dst##_free_(void* ctx) \
    { \
        Src##_to_##Dst##_ctx_* x = (Src##_to_##Dst##_ctx_*)ctx; \
        Src##_free(&x->source); \
        STREAM_FREE(x); \
    } \
 \
    /* Bridge a Src stream to a Dst stream, applying fn: T -> U lazily. */ \
    static inline Dst Src##_map_to_##Dst(Src source, U (*fn)(const T* item)) \
    { \
        Src##_to_##Dst##_ctx_* x = (Src##_to_##Dst##_ctx_*)STREAM_MALLOC(sizeof(*x)); \
        STREAM_ASSERT(x != NULL && "map_to allocation failed"); \
        x->source = source; \
        x->fn = fn; \
        return Dst##_from_fn(Src##_to_##Dst##_gen_, x, Src##_to_##Dst##_free_); \
    } \
 \
    struct Src##_to_##Dst##_semicolon_eater_
