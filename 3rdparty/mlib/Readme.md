# Mahdiy's C library

Header-only C11 building blocks. Each module lives in its own folder with a
`USAGE.md`, runnable programs in `example/`, a `Makefile`, and a `test/` suite.
The shared test harness is [`test.h`](test.h) at the repository root.

```
make            # run every module's test suite
make examples   # build and run every example
make vec        # just one module   (also: string list stream hashmap async)
```

On Windows use `mingw32-make`.

## vec

Typed dynamic array, "C++ template" style. `DEFINE_VEC(T, Name)` stamps out a
`{ T* items; size_t count; size_t capacity; }` struct plus `static inline`
`Name_*` functions for that one element type — type-checked calls, no `typeof`,
no extensions.

```c
DEFINE_VEC(int, IntVec);
IntVec v = {0};
IntVec_append(&v, 42);
IntVec_free(&v);
```

Full API and tuning macros: [vec/USAGE.md](vec/USAGE.md).
Runnable example: [vec/example/](vec/example/) (`cd vec && make examples`).

## string

`string_builder` and `string_view` helpers. `string_builder` is
`DEFINE_VEC(char, string_builder)` plus the `sb_*` / `sv_*` API: building,
splitting, trimming, searching, file slurping. Depends on `vec` (compile with
`-I../vec`); needs `#define STRING_IMPLEMENTATION` in one translation unit.

```c
#define STRING_IMPLEMENTATION
#include "string.h"

string_builder sb = sb_init("Hello");
sb_add_f(&sb, ", %s!", "World");
printf("%.*s\n", sv_fmt(sb_to_sv(&sb)));
sb_free(&sb);

string_view rest = sv_from_cstr("a,b,c");
string_view first = sv_split_c(&rest, ',');   // "a"; rest becomes "b,c"
```

Full API: [string/USAGE.md](string/USAGE.md).
Runnable examples: [string/example/](string/example/) (`cd string && make examples`).

## list

Typed doubly-linked list. `DEFINE_LIST(T, Name)` stamps out a node type, a
`{ head, tail, count }` container, and `static inline` `Name_*` functions —
O(1) insert/remove at either end or at a known node, forward and backward
iteration, no `typeof`.

```c
DEFINE_LIST(int, IntList);
IntList l = {0};
IntList_push_back(&l, 1);
IntList_node* n = IntList_push_front(&l, 0);
IntList_insert_after(&l, n, 9);          // 0, 9, 1
list_foreach(IntList, it, &l) printf("%d ", it->value);
IntList_free(&l);
```

Full API: [list/USAGE.md](list/USAGE.md).
Runnable example: [list/example/](list/example/) (`cd list && make examples`).

## stream

Lazy, pull-based streams (Java Stream / iterator style). `DEFINE_STREAM(T, Name)`
generates a stream type plus `static inline` sources (`from_array`, `from_fn`),
lazy ops (`map`, `filter`, `take`, `skip`, `peek`) and terminals (`for_each`,
`reduce`, `count`, `any`/`all`, `find`, `collect`). Nothing runs until a terminal
op, so sources can be infinite.

```c
DEFINE_STREAM(int, IntStream);
int a[] = { 1, 2, 3, 4, 5, 6 };
IntStream_for_each(
    IntStream_map(IntStream_filter(IntStream_from_array(a, 6), is_even), square),
    print_i);                               // 4 16 36
```

Full API (plus opt-in `DEFINE_STREAM_RANGE` / cross-type `DEFINE_STREAM_MAP`):
[stream/USAGE.md](stream/USAGE.md).
Runnable example: [stream/example/](stream/example/) (`cd stream && make examples`).

## hashmap

Typed open-addressing hash map — linear probing with backward-shift deletion (no
tombstones), power-of-two capacity. `DEFINE_HASHMAP(K, V, Name, hash, eq)` plus
ready-made `DEFINE_HASHMAP_STR` (owned `const char*` keys) and
`DEFINE_HASHMAP_INT`. Generates `put` / `get` / `contains` / `remove` /
`get_or_put` / `reserve` / `clear` / `free` and callback-free iteration.

```c
DEFINE_HASHMAP_STR(int, Counts);
Counts m = {0};
(*Counts_get_or_put(&m, "hits", 0))++;
hashmap_foreach(Counts, it, &m) printf("%s=%d\n", it.key, *it.value);
Counts_free(&m);
```

Full API: [hashmap/USAGE.md](hashmap/USAGE.md).
Runnable example: [hashmap/example/](hashmap/example/) (`cd hashmap && make examples`).

## async

A single-threaded cooperative scheduler with stackless coroutines. A task is a
poll function; `co_begin` / `co_yield` / `co_await` / `co_return` let you write it
as linear code that suspends and resumes one scheduler tick at a time. Priorities,
`sched_block_on` for a blocking wait, and `async_fs.h` for chunked file I/O.

```c
static CoStatus greet(Task* t) {
    Greet* g = t->ctx;
    co_begin(&t->co);
    for (g->i = 0; g->i < 3; g->i++) co_yield(&t->co);
    co_return(&t->co, "hello");
    co_end(&t->co);
}

Sched s = {0};
Task* t = sched_spawn(&s, greet, &g);
char* msg = sched_block_on(&s, t);
sched_free(&s);
```

Full API: [async/USAGE.md](async/USAGE.md).
Runnable examples: [async/example/](async/example/) (`cd async && make examples`).
