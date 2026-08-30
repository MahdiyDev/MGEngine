#pragma once

// Typed open-addressing hash map -- same generator pattern as vec.h / list.h.
// Linear probing with backward-shift deletion (no tombstones), power-of-two
// capacity, murmur3 finalizer on every hash. Plain C11, no typeof, no extensions.
//
//     DEFINE_HASHMAP_STR(int, Counts);          // const char* -> int, owned keys
//
//     Counts m = {0};                           // {0} is a valid empty map
//     Counts_put(&m, "apples", 3);
//     (*Counts_get(&m, "apples"))++;             // get() returns a mutable V*
//     if (!Counts_contains(&m, "pears")) Counts_put(&m, "pears", 0);
//     Counts_remove(&m, "apples");
//
//     hashmap_foreach(Counts, it, &m)
//         printf("%s = %d\n", it.key, *it.value);
//
//     Counts_free(&m);
//
// Generators:
//   DEFINE_HASHMAP(K, V, Name, HASH, EQ)
//       HASH: size_t (or macro)  taking K       EQ: bool  taking (K, K)
//       Keys and values are stored by value; K is not copied or freed.
//   DEFINE_HASHMAP_FULL(K, V, Name, HASH, EQ, KEY_DUP, KEY_FREE)
//       adds ownership hooks: KEY_DUP(k)->K on insert, KEY_FREE(k) on remove/free.
//   DEFINE_HASHMAP_STR(V, Name)      const char* keys, copied with malloc/free
//   DEFINE_HASHMAP_INT(K, V, Name)   integer keys (any scalar), compared with ==
//
// Generated API (for Name):
//   void   Name##_free(Name*)                 release; NULL-safe, idempotent
//   void   Name##_clear(Name*)                empty it, keep the allocation
//   void   Name##_reserve(Name*, size_t n)    pre-size for n entries
//   bool   Name##_put(Name*, K, V)            insert/overwrite; true if newly added
//   V*     Name##_get(Name*, K)               mutable pointer, or NULL
//   bool   Name##_contains(Name*, K)
//   bool   Name##_remove(Name*, K)            true if it was present
//   V*     Name##_get_or_put(Name*, K, V dflt)
//   Name##_iter Name##_iter_begin(Name*)
//   bool   Name##_iter_next(Name##_iter*)     fills it.key / it.value (V*)
//
// Iterating a map while inserting or removing is undefined.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef HASHMAP_MALLOC
    #define HASHMAP_MALLOC malloc
#endif
#ifndef HASHMAP_FREE
    #define HASHMAP_FREE free
#endif
#ifndef HASHMAP_ASSERT
    #include <assert.h>
    #define HASHMAP_ASSERT assert
#endif
#ifndef HASHMAP_MIN_CAP
    #define HASHMAP_MIN_CAP 8
#endif

// murmur3 64-bit finalizer -- spreads low-entropy user hashes for pow2 masking
static inline uint64_t hashmap_mix_(uint64_t h)
{
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return h;
}

static inline uint64_t hashmap_fnv1a(const void* data, size_t len)
{
    const unsigned char* p = (const unsigned char*)data;
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static inline size_t hashmap_hash_str_(const char* s) { return (size_t)hashmap_fnv1a(s, strlen(s)); }
static inline bool hashmap_eq_str_(const char* a, const char* b) { return strcmp(a, b) == 0; }

static inline const char* hashmap_dup_str_(const char* s)
{
    size_t n = strlen(s) + 1;
    char* p = (char*)HASHMAP_MALLOC(n);
    HASHMAP_ASSERT(p != NULL && "hashmap: key copy failed");
    memcpy(p, s, n);
    return p;
}
static inline void hashmap_free_str_(const char* s) { HASHMAP_FREE((void*)s); }

#define HASHMAP_HASH_SCALAR_(k) ((size_t)(k))
#define HASHMAP_EQ_SCALAR_(a, b) ((a) == (b))
#define HASHMAP_KEY_ID_(k) (k)
#define HASHMAP_KEY_NOFREE_(k) ((void)(k))

static inline size_t hashmap_next_pow2_(size_t n)
{
    size_t c = HASHMAP_MIN_CAP;
    while (c < n) {
        c <<= 1;
    }
    return c;
}

#define DEFINE_HASHMAP(K, V, Name, HASH, EQ) \
    DEFINE_HASHMAP_FULL(K, V, Name, HASH, EQ, HASHMAP_KEY_ID_, HASHMAP_KEY_NOFREE_)

#define DEFINE_HASHMAP_STR(V, Name) \
    DEFINE_HASHMAP_FULL(const char*, V, Name, hashmap_hash_str_, hashmap_eq_str_, \
                        hashmap_dup_str_, hashmap_free_str_)

#define DEFINE_HASHMAP_INT(K, V, Name) \
    DEFINE_HASHMAP(K, V, Name, HASHMAP_HASH_SCALAR_, HASHMAP_EQ_SCALAR_)

#define DEFINE_HASHMAP_FULL(K, V, Name, HASH, EQ, KEY_DUP, KEY_FREE) \
    typedef struct { \
        K key; \
        V value; \
        size_t hash; \
        bool used; \
    } Name##_slot; \
 \
    typedef struct { \
        Name##_slot* slots; \
        size_t cap;   /* power of two, or 0 */ \
        size_t count; \
    } Name; \
 \
    typedef struct { \
        Name* map_; \
        size_t i_; \
        K key; \
        V* value; \
    } Name##_iter; \
 \
    static inline void Name##_place_(Name* m, K key, V value, size_t h) \
    { \
        size_t mask = m->cap - 1; \
        size_t i = (size_t)h & mask; \
        while (m->slots[i].used) { \
            i = (i + 1) & mask; \
        } \
        m->slots[i].key = key; \
        m->slots[i].value = value; \
        m->slots[i].hash = h; \
        m->slots[i].used = true; \
        m->count++; \
    } \
 \
    static inline void Name##_rehash_(Name* m, size_t new_cap) \
    { \
        Name##_slot* old = m->slots; \
        size_t old_cap = m->cap; \
        m->slots = (Name##_slot*)HASHMAP_MALLOC(new_cap * sizeof(Name##_slot)); \
        HASHMAP_ASSERT(m->slots != NULL && "hashmap: allocation failed"); \
        memset(m->slots, 0, new_cap * sizeof(Name##_slot)); \
        m->cap = new_cap; \
        m->count = 0; \
        for (size_t x = 0; x < old_cap; x++) { \
            if (old[x].used) { \
                Name##_place_(m, old[x].key, old[x].value, old[x].hash); \
            } \
        } \
        HASHMAP_FREE(old); \
    } \
 \
    static inline void Name##_reserve(Name* m, size_t n) \
    { \
        size_t want = hashmap_next_pow2_(n + n / 7 + 1); /* keep load < 7/8 */ \
        if (m->cap < want) { \
            Name##_rehash_(m, want); \
        } \
    } \
 \
    static inline size_t Name##_find_(Name* m, K key, size_t h) \
    { \
        size_t mask = m->cap - 1; \
        size_t i = (size_t)h & mask; \
        while (m->slots[i].used) { \
            if (m->slots[i].hash == h && (EQ(m->slots[i].key, key))) { \
                return i; \
            } \
            i = (i + 1) & mask; \
        } \
        return m->cap; /* not found */ \
    } \
 \
    static inline V* Name##_get(Name* m, K key) \
    { \
        if (m->cap == 0) { \
            return NULL; \
        } \
        size_t h = (size_t)hashmap_mix_((uint64_t)(HASH(key))); \
        size_t i = Name##_find_(m, key, h); \
        return i == m->cap ? NULL : &m->slots[i].value; \
    } \
 \
    static inline bool Name##_contains(Name* m, K key) \
    { \
        return Name##_get(m, key) != NULL; \
    } \
 \
    static inline bool Name##_put(Name* m, K key, V value) \
    { \
        size_t h = (size_t)hashmap_mix_((uint64_t)(HASH(key))); \
        if (m->cap != 0) { \
            size_t i = Name##_find_(m, key, h); \
            if (i != m->cap) { \
                m->slots[i].value = value; \
                return false; \
            } \
        } \
        if (m->cap == 0) { \
            Name##_rehash_(m, HASHMAP_MIN_CAP); \
        } else if ((m->count + 1) * 8 > m->cap * 7) { \
            Name##_rehash_(m, m->cap * 2); \
        } \
        Name##_place_(m, KEY_DUP(key), value, h); \
        return true; \
    } \
 \
    static inline V* Name##_get_or_put(Name* m, K key, V dflt) \
    { \
        V* p = Name##_get(m, key); \
        if (p != NULL) { \
            return p; \
        } \
        Name##_put(m, key, dflt); \
        return Name##_get(m, key); \
    } \
 \
    static inline void Name##_backshift_(Name* m, size_t i) \
    { \
        size_t mask = m->cap - 1; \
        for (;;) { \
            m->slots[i].used = false; \
            size_t j = i; \
            for (;;) { \
                j = (j + 1) & mask; \
                if (!m->slots[j].used) { \
                    return; \
                } \
                size_t home = m->slots[j].hash & mask; \
                if (((i - home) & mask) <= ((j - home) & mask)) { \
                    m->slots[i] = m->slots[j]; \
                    i = j; \
                    break; \
                } \
            } \
        } \
    } \
 \
    static inline bool Name##_remove(Name* m, K key) \
    { \
        if (m->cap == 0) { \
            return false; \
        } \
        size_t h = (size_t)hashmap_mix_((uint64_t)(HASH(key))); \
        size_t i = Name##_find_(m, key, h); \
        if (i == m->cap) { \
            return false; \
        } \
        KEY_FREE(m->slots[i].key); \
        m->count--; \
        Name##_backshift_(m, i); \
        return true; \
    } \
 \
    static inline void Name##_clear(Name* m) \
    { \
        for (size_t i = 0; i < m->cap; i++) { \
            if (m->slots[i].used) { \
                KEY_FREE(m->slots[i].key); \
                m->slots[i].used = false; \
            } \
        } \
        m->count = 0; \
    } \
 \
    static inline void Name##_free(Name* m) \
    { \
        if (m == NULL || m->slots == NULL) { \
            return; \
        } \
        for (size_t i = 0; i < m->cap; i++) { \
            if (m->slots[i].used) { \
                KEY_FREE(m->slots[i].key); \
            } \
        } \
        HASHMAP_FREE(m->slots); \
        m->slots = NULL; \
        m->cap = 0; \
        m->count = 0; \
    } \
 \
    static inline Name##_iter Name##_iter_begin(Name* m) \
    { \
        return (Name##_iter){ .map_ = m, .i_ = 0 }; \
    } \
 \
    static inline bool Name##_iter_next(Name##_iter* it) \
    { \
        while (it->i_ < it->map_->cap) { \
            Name##_slot* s = &it->map_->slots[it->i_++]; \
            if (s->used) { \
                it->key = s->key; \
                it->value = &s->value; \
                return true; \
            } \
        } \
        return false; \
    } \
 \
    struct Name##_semicolon_eater_

// Iterate a generated map; `Name` is the map type.
//     hashmap_foreach(Counts, it, &m) { use(it.key, *it.value); }
#define hashmap_foreach(Name, it, mp) \
    for (Name##_iter it = Name##_iter_begin(mp); Name##_iter_next(&it);)
