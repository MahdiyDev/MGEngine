#include <stdio.h>

#include "../async.h"

// A task that "downloads" a resource over several ticks, then resolves.
typedef struct {
    const char* name;
    int steps;
    int i;
} Download;

static CoStatus download(Task* t)
{
    Download* d = (Download*)t->ctx;
    co_begin(&t->co);
    printf("  [%s] start\n", d->name);
    for (d->i = 0; d->i < d->steps; d->i++) {
        co_yield(&t->co);                    // one chunk per tick
    }
    printf("  [%s] done\n", d->name);
    co_return(&t->co, (void*)d->name);
    co_end(&t->co);
}

// A task that waits for two downloads, then combines their results.
typedef struct {
    Task* a;
    Task* b;
} Combine;

static CoStatus combine(Task* t)
{
    Combine* c = (Combine*)t->ctx;
    co_begin(&t->co);
    co_await(&t->co, c->a);
    co_await(&t->co, c->b);
    printf("  [combine] got \"%s\" + \"%s\"\n",
           (char*)task_result(c->a), (char*)task_result(c->b));
    co_return(&t->co, "combined");
    co_end(&t->co);
}

// A task that fails.
static CoStatus flaky(Task* t)
{
    co_begin(&t->co);
    co_yield(&t->co);
    co_fail(&t->co, "connection reset");
    co_end(&t->co);
}

int main(void)
{
    Sched s = {0};

    Download da = { .name = "config.json", .steps = 2 };
    Download db = { .name = "index.html", .steps = 5 };
    Task* ta = sched_spawn(&s, download, &da);
    Task* tb = sched_spawn_prio(&s, download, &db, 1);   // higher priority

    Combine cc = { .a = ta, .b = tb };
    Task* tc = sched_spawn(&s, combine, &cc);

    Task* tf = sched_spawn(&s, flaky, NULL);

    printf("running the loop...\n");
    sched_run(&s);

    printf("\ncombine result: %s\n", (char*)task_result(tc));
    printf("flaky task: %s\n",
           task_error(tf) ? (char*)task_error(tf) : "ok");

    // sched_block_on is the blocking form -- drives the loop until one task ends
    Download dq = { .name = "quick", .steps = 1 };
    Task* tq = sched_spawn(&s, download, &dq);
    printf("blocked result: %s\n", (char*)sched_block_on(&s, tq));

    sched_free(&s);
    return 0;
}
