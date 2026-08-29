# hashmap

Typed open-addressing hash map, header-only, plain ISO C11 — same generator
pattern as [`../vec`](../vec) and [`../list`](../list). Linear probing with
**backward-shift deletion** (no tombstones), power-of-two capacity, a murmur3
finalizer applied to every hash. No `prime.h`, no `<math.h>`.

```c
#include "hashmap.h"

DEFINE_HASHMAP_STR(int, Counts);   // const char* -> int, keys copied & owned

int main(void)
{
    Counts m = {0};                        // {0} is a valid empty map
    Counts_put(&m, "apples", 3);
    (*Counts_get(&m, "apples"))++;          // get() returns a mutable int*
    if (!Counts_contains(&m, "pears"))
        Counts_put(&m, "pears", 0);
    Counts_remove(&m, "apples");

    hashmap_foreach(Counts, it, &m)
        printf("%s = %d\n", it.key, *it.value);

    Counts_free(&m);
}
```

## Generators

| | |
| --- | --- |
| `DEFINE_HASHMAP(K, V, Name, HASH, EQ)` | `HASH`: `size_t` (fn or macro) of `K`; `EQ`: `bool` of `(K, K)`. Keys stored by value — not copied or freed. |
| `DEFINE_HASHMAP_FULL(K, V, Name, HASH, EQ, KEY_DUP, KEY_FREE)` | adds ownership hooks: `KEY_DUP(k) -> K` on insert, `KEY_FREE(k)` on remove / free / clear. |
| `DEFINE_HASHMAP_STR(V, Name)` | `const char*` keys, copied with `malloc` and freed automatically. |
| `DEFINE_HASHMAP_INT(K, V, Name)` | any scalar key type, compared with `==`. |

Keys and values are stored **by value**. If `V` owns resources, the caller frees
them (same as `vec` / `list`).

Custom key type — supply `hash` + `eq`:

```c
typedef struct { int x, y; } Point;
static size_t point_hash(Point p) { return (size_t)hashmap_fnv1a(&p, sizeof p); }
static bool   point_eq(Point a, Point b) { return a.x == b.x && a.y == b.y; }
DEFINE_HASHMAP(Point, int, PointMap, point_hash, point_eq);
```

## Generated API (for `Name`)

| Function | |
| --- | --- |
| `void Name##_free(Name*)` | release; `NULL`-safe, idempotent |
| `void Name##_clear(Name*)` | empty it, keep the allocation |
| `void Name##_reserve(Name*, size_t n)` | pre-size for `n` entries |
| `bool Name##_put(Name*, K, V)` | insert or overwrite; `true` if newly added |
| `V* Name##_get(Name*, K)` | mutable pointer, or `NULL` |
| `bool Name##_contains(Name*, K)` | |
| `bool Name##_remove(Name*, K)` | `true` if it was present |
| `V* Name##_get_or_put(Name*, K, V dflt)` | get, inserting `dflt` if absent |
| `Name##_iter Name##_iter_begin(Name*)` / `bool Name##_iter_next(Name##_iter*)` | fills `it.key`, `it.value` (a `V*`) |

`m.count` is the live entry count. Iterating a map while inserting or removing is
undefined.

Helpers (not generated): `hashmap_fnv1a(ptr, len)` for building key hashes.
Tuning: `HASHMAP_MALLOC` / `HASHMAP_FREE` / `HASHMAP_ASSERT` / `HASHMAP_MIN_CAP`.

## Tests & examples

```
make            # build and run test/test_hashmap.c
make examples   # build and run example/hashmap.c
make clean
```

On Windows use `mingw32-make`. The test suite uses the repo-wide harness `test.h`
at the repository root (`-I..`).
