# vec

Typed dynamic array, header-only, plain ISO C11 (no compiler extensions).

`DEFINE_VEC(T, Name)` stamps out a struct plus `static inline` functions for one
element type. Every call is type checked and steps through in a debugger.

```c
#include "vec.h"

DEFINE_VEC(int, IntVec);   // note the trailing ';'

int main(void)
{
    IntVec v = {0};        // a zeroed struct is a valid empty vec
    IntVec_append(&v, 42);

    int xs[] = { 1, 2, 3 };
    IntVec_append_many(&v, xs, 3);

    IntVec_prepend(&v, 0);
    *IntVec_get(&v, 1) = 99;

    for (size_t i = 0; i < v.count; i++)
        printf("%d\n", v.items[i]);

    IntVec_free(&v);
}
```

The generated struct is always `T* items; size_t count; size_t capacity;`, so a
zeroed struct (`= {0}`) is a valid empty vec — there is no `init`. To pre-size
one, `Name_reserve(&v, n)` straight into the zeroed struct.

## Generated API (for `DEFINE_VEC(T, Name)`)

| Function                                             | Notes                                         |
| --------------------------------------------------- | --------------------------------------------- |
| `void Name_reserve(Name* v, size_t want)`            | grows to `>= want` (geometric); never shrinks  |
| `void Name_free(Name* v)`                            | resets fields; `NULL` and double-free are safe |
| `T*   Name_get(Name* v, size_t i)`                   | bounds-checked with `VEC_ASSERT`               |
| `void Name_append(Name* v, T item)`                  |                                               |
| `void Name_append_many(Name* v, const T* p, size_t n)` | `n == 0` is a no-op                          |
| `void Name_prepend(Name* v, T item)`                 |                                               |
| `void Name_prepend_many(Name* v, const T* p, size_t n)` |                                            |
| `void Name_delete(Name* v, size_t i)`               | shifts the tail down                           |
| `void Name_delete_range(Name* v, size_t s, size_t e)` | removes the half-open range `[s, e)`         |
| `void Name_clear(Name* v)`                           | `count = 0`, keeps the allocation              |

## Tuning

Define before including (defaults shown):

```c
#define VEC_ASSERT   assert
#define VEC_REALLOC  realloc
#define VEC_FREE     free
#define VEC_INIT_CAP 4
#include "vec.h"
```

## Tests & examples

```
make            # build and run test/test_vec.c
make examples   # build and run example/vec.c
make clean
```

On Windows use `mingw32-make`. The test suite uses the repo-wide harness `test.h`
at the repository root (`-I..`).
