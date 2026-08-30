# stream

Lazy, pull-based streams — Java Stream / iterator style. Header-only, plain
ISO C11, same generator pattern as [`../vec`](../vec) and [`../list`](../list).
**Nothing runs until a terminal operation**, so a source can be infinite.

```c
#include "stream.h"

DEFINE_STREAM(int, IntStream);   // note the trailing ';'

static bool is_even(const int* x) { return *x % 2 == 0; }
static void square(int* x)        { *x *= *x; }
static void print_i(const int* x) { printf("%d ", *x); }

int main(void)
{
    int a[] = { 1, 2, 3, 4, 5, 6 };
    IntStream_for_each(
        IntStream_map(
            IntStream_filter(IntStream_from_array(a, 6), is_even),
            square),
        print_i);                // 4 16 36
}
```

The nested form reads inside-out. The same pipeline in **reassignment style**
reads top-to-bottom — each step consumes the old `s` and rebinds it, and the
terminal frees the whole chain:

```c
IntStream s = IntStream_from_array(a, 6);
s = IntStream_filter(s, is_even);
s = IntStream_map(s, square);
IntStream_for_each(s, print_i);   // 4 16 36
```

## "Move" semantics

Every combinator and terminal **consumes** the stream handed to it. Once you
pass a stream on, that variable is spent — don't read it, pass it again, or free
it. A terminal frees the whole pipeline. Call `Name_free(&s)` only to drop a
pipeline that never reached a terminal (it is idempotent and `NULL`-safe). A
zeroed stream (`{0}`) is a valid empty stream.

## Generated API (for `DEFINE_STREAM(T, Name)`)

`T` is any assignable complete type.

| | |
| --- | --- |
| **sources** | |
| `Name Name##_from_array(const T* p, size_t n)` | borrows `p`; does **not** copy — the array must outlive the stream |
| `Name Name##_from_fn(bool (*gen)(void* ctx, T* out), void* ctx, void (*free_ctx)(void*))` | custom / infinite source; `free_ctx` (may be `NULL`) runs when the stream is freed |
| **lazy ops** (return a new stream) | |
| `Name Name##_map(Name, void (*)(T*))` | in place, `T -> T` |
| `Name Name##_filter(Name, bool (*)(const T*))` | keep where the predicate is true |
| `Name Name##_take(Name, size_t n)` | first `n` items |
| `Name Name##_skip(Name, size_t n)` | drop the first `n` |
| `Name Name##_peek(Name, void (*)(const T*))` | side effect, pass through |
| **terminals** (consume + free the pipeline) | |
| `void Name##_for_each(Name, void (*)(const T*))` | |
| `void Name##_for_each_ctx(Name, void (*)(const T*, void*), void* ctx)` | collect into anything |
| `T Name##_reduce(Name, T init, T (*)(T acc, const T*))` | |
| `size_t Name##_count(Name)` | |
| `bool Name##_any(Name, bool (*)(const T*))` / `Name##_all(...)` | short-circuit |
| `bool Name##_find(Name, bool (*)(const T*), T* out)` | first match into `*out` |
| `size_t Name##_collect(Name, T* out, size_t max)` | up to `max`; returns count written |
| **primitive / cleanup** | |
| `bool Name##_next(Name* s, T* out)` | pull one; `false` when exhausted |
| `void Name##_free(Name* s)` | |

## Opt-in extras

```c
DEFINE_STREAM_RANGE(int, IntStream);
// -> IntStream IntStream_from_range(int start, int stop, int step)   half-open, numeric T

DEFINE_STREAM(double, DblStream);
DEFINE_STREAM_MAP(int, IntStream, double, DblStream);
// -> DblStream IntStream_map_to_DblStream(IntStream, double (*)(const int*))
//    the cross-type map; both streams must already be DEFINE_STREAM'd
```

## Tuning

```c
#define STREAM_MALLOC malloc
#define STREAM_FREE   free
#define STREAM_ASSERT assert
#include "stream.h"
```

## Tests & examples

```
make            # build and run test/test_stream.c
make examples   # build and run example/stream.c
make clean
```

On Windows use `mingw32-make`. The test suite uses the repo-wide harness `test.h`
at the repository root (`-I..`).
