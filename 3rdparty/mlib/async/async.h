#pragma once

// Cooperative scheduler + stackless coroutines. Single-threaded, header-only,
// plain C11. A "task" is a poll function over a context struct; the coroutine
// macros let you write it as linear code that suspends and resumes.
//
//     typedef struct { int i; } Greet;
//
//     static CoStatus greet(Task* t)
//     {
//         Greet* g = (Greet*)t->ctx;
//         co_begin(&t->co);
//         for (g->i = 0; g->i < 3; g->i++)
//             co_yield(&t->co);              // one step per scheduler tick
//         co_return(&t->co, "hello");
//         co_end(&t->co);
//     }
//
//     Sched s = {0};                          // {0} is a valid empty scheduler
//     Greet g = {0};
//     Task* t = sched_spawn(&s, greet, &g);
//     char* msg = (char*)sched_block_on(&s, t);   // runs the loop until t finishes
//     sched_free(&s);
//
// Coroutine rules (stackless -- the usual protothread caveats):
//   * co_yield / co_await may only appear in the task body, between co_begin and
//     co_end -- never inside a helper function or a nested `switch`.
//   * at most one co_yield/co_await per source line.
//   * a local declared before a yield is indeterminate after a resume -- keep
//     anything that must survive a suspend in the context struct.
//
// Task lifetime: the scheduler owns Task structs until sched_free() (or
// sched_gc(), which reaps finished tasks -- don't call it while a co_await still
// targets a finished task). The context struct is always caller-owned.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef ASYNC_MALLOC
    #define ASYNC_MALLOC malloc
#endif
#ifndef ASYNC_FREE
    #define ASYNC_FREE free
#endif
#ifndef ASYNC_ASSERT
    #define ASYNC_ASSERT assert
#endif

typedef enum {
    CO_PENDING = 0,
    CO_DONE
} CoStatus;

typedef struct {
    int pc;         // resume point (0 = start, -1 = finished)
    void* result;   // set by co_return
    void* error;    // set by co_fail
} Co;

typedef struct Task {
    struct Task* next;
    CoStatus (*poll)(struct Task* self);
    void* ctx;
    Co co;
    int prio;
    bool done;
} Task;

typedef struct {
    Task* head;     // kept sorted: higher prio first, then spawn order
    size_t count;
} Sched;

// --- coroutine control flow (operate on a Co*) --------------------------------

#define co_begin(co) \
    switch ((co)->pc) { \
    case 0:

#define co_yield(co) \
    do { \
        (co)->pc = __LINE__; \
        return CO_PENDING; \
        case __LINE__:; \
    } while (0)

// suspend until another task is finished
#define co_await(co, task) \
    while (!(task)->done) { \
        co_yield(co); \
    }

// suspend until `cond` becomes true (re-checked once per tick)
#define co_await_until(co, cond) \
    while (!(cond)) { \
        co_yield(co); \
    }

#define co_return(co, value) \
    do { \
        (co)->result = (void*)(value); \
        (co)->pc = -1; \
        return CO_DONE; \
    } while (0)

#define co_fail(co, err) \
    do { \
        (co)->error = (void*)(err); \
        (co)->pc = -1; \
        return CO_DONE; \
    } while (0)

#define co_end(co) \
    } \
    (co)->pc = -1; \
    return CO_DONE

// --- scheduler ---------------------------------------------------------------

static inline Task* sched_spawn_prio(Sched* s, CoStatus (*poll)(Task*), void* ctx, int prio)
{
    Task* t = (Task*)ASYNC_MALLOC(sizeof(Task));
    ASYNC_ASSERT(t != NULL && "sched: task allocation failed");
    t->next = NULL;
    t->poll = poll;
    t->ctx = ctx;
    t->co = (Co){ 0 };
    t->prio = prio;
    t->done = false;

    Task** link = &s->head;
    while (*link != NULL && (*link)->prio >= prio) {
        link = &(*link)->next;
    }
    t->next = *link;
    *link = t;
    s->count++;
    return t;
}

static inline Task* sched_spawn(Sched* s, CoStatus (*poll)(Task*), void* ctx)
{
    return sched_spawn_prio(s, poll, ctx, 0);
}

// Run every unfinished task once, highest priority first.
// Returns true while at least one task is still pending.
static inline bool sched_tick(Sched* s)
{
    bool pending = false;
    for (Task* t = s->head; t != NULL; t = t->next) {
        if (t->done) {
            continue;
        }
        if (t->poll(t) == CO_DONE) {
            t->done = true;
        } else {
            pending = true;
        }
    }
    return pending;
}

// Tick until no task is pending.
static inline void sched_run(Sched* s)
{
    while (sched_tick(s)) {
    }
}

// Tick until `t` is finished; returns its result.
static inline void* sched_block_on(Sched* s, Task* t)
{
    while (!t->done) {
        sched_tick(s);
    }
    return t->co.result;
}

static inline bool task_done(const Task* t) { return t->done; }
static inline void* task_result(const Task* t) { return t->co.result; }
static inline void* task_error(const Task* t) { return t->co.error; }
static inline void* task_ctx(const Task* t) { return t->ctx; }

// Free finished Task structs. Unsafe while a co_await still refers to one.
static inline void sched_gc(Sched* s)
{
    Task** link = &s->head;
    while (*link != NULL) {
        Task* t = *link;
        if (t->done) {
            *link = t->next;
            ASYNC_FREE(t);
            s->count--;
        } else {
            link = &t->next;
        }
    }
}

// Free every Task struct. Context structs are not touched.
static inline void sched_free(Sched* s)
{
    Task* t = s->head;
    while (t != NULL) {
        Task* next = t->next;
        ASYNC_FREE(t);
        t = next;
    }
    s->head = NULL;
    s->count = 0;
}
