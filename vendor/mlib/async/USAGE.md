# async

A single-threaded cooperative scheduler with **stackless coroutines**.
Header-only, plain ISO C11. A task is a poll function over a context struct; the
`co_*` macros let you write it as linear code that suspends and resumes.

```c
#include "async.h"

typedef struct { int i; } Greet;

static CoStatus greet(Task* t)
{
    Greet* g = (Greet*)t->ctx;
    co_begin(&t->co);
    for (g->i = 0; g->i < 3; g->i++)
        co_yield(&t->co);              // suspend; resume here next tick
    co_return(&t->co, "hello");
    co_end(&t->co);
}

int main(void)
{
    Sched s = {0};                     // {0} is a valid empty scheduler
    Greet g = {0};
    Task* t = sched_spawn(&s, greet, &g);
    char* msg = (char*)sched_block_on(&s, t);   // drive the loop until t ends
    printf("%s\n", msg);
    sched_free(&s);
}
```

## Coroutine macros (operate on a `Co*`, i.e. `&t->co`)

| | |
| --- | --- |
| `co_begin(co)` / `co_end(co)` | bracket the task body |
| `co_yield(co)` | suspend; the scheduler resumes here on the next tick |
| `co_await(co, task)` | suspend until another `Task*` is finished |
| `co_await_until(co, cond)` | suspend until `cond` is true (re-checked once per tick) |
| `co_return(co, value)` | finish with a result (`void*`) |
| `co_fail(co, err)` | finish with an error (`void*`) |

Stackless-coroutine rules (the usual protothread caveats):

- `co_yield` / `co_await` only in the task body, between `co_begin` and `co_end`
  — never in a helper function or a nested `switch`.
- at most one `co_yield` / `co_await` per source line.
- a local declared before a suspend is indeterminate after the resume — keep
  anything that must survive in the context struct.

## Scheduler API

| | |
| --- | --- |
| `Task* sched_spawn(Sched*, CoStatus (*poll)(Task*), void* ctx)` | register a task (priority 0) |
| `Task* sched_spawn_prio(..., int prio)` | higher priority is polled first; equal priority is FIFO |
| `bool sched_tick(Sched*)` | run every unfinished task once; `true` while work remains |
| `void sched_run(Sched*)` | tick until nothing is pending |
| `void* sched_block_on(Sched*, Task*)` | tick until that task finishes; returns its result |
| `bool task_done(Task*)` / `void* task_result(Task*)` / `void* task_error(Task*)` / `void* task_ctx(Task*)` | |
| `void sched_gc(Sched*)` | free finished `Task` structs — not while a `co_await` still targets one |
| `void sched_free(Sched*)` | free every `Task` struct (context structs are caller-owned) |

Tuning: `ASYNC_MALLOC` / `ASYNC_FREE` / `ASYNC_ASSERT`.

## `async_fs.h`

File I/O as tasks — each poll moves one `ASYNC_FS_CHUNK` (4 KB) and yields, so a
large file does not stall the loop. Needs `string.h` / `vec.h` on the include
path (`-I<repo>/vec`).

```c
#include "async_fs.h"

Sched s = {0};
Task* r = async_read_file(&s, "big.bin");           // NULL if it can't open
string_builder* data = (string_builder*)sched_block_on(&s, r);
// ... use data->items / data->count ...
async_file_close(r);                                 // fclose + free the buffer

Task* w = async_write_file(&s, "out.bin", buf, n);  // buf is copied
sched_run(&s);
async_file_close(w);
sched_free(&s);
```

`async_read_file_prio` / `async_write_file_prio` take a priority.

## Tests & examples

```
make            # build and run test/test_async.c and test/test_async_fs.c
make examples   # build and run example/async.c and example/async_fs.c
make clean
```

On Windows use `mingw32-make`. The test suites use the repo-wide harness `test.h`
at the repository root (`-I..`).
