#include "../async.h"
#include "test.h"

// --- task bodies ----------------------------------------------------------

typedef struct {
    int target;
    int i;
} Counter;

static CoStatus counter_poll(Task* t)
{
    Counter* c = (Counter*)t->ctx;
    co_begin(&t->co);
    for (c->i = 0; c->i < c->target; c->i++) {
        co_yield(&t->co);
    }
    co_return(&t->co, "counted");
    co_end(&t->co);
}

static int g_order[16];
static int g_order_n;

typedef struct {
    int id;
} Marker;

static CoStatus mark_poll(Task* t)
{
    Marker* m = (Marker*)t->ctx;
    co_begin(&t->co);
    g_order[g_order_n++] = m->id;
    co_return(&t->co, NULL);
    co_end(&t->co);
}

typedef struct {
    Task* dep;
    void* got;
} Waiter;

static CoStatus waiter_poll(Task* t)
{
    Waiter* w = (Waiter*)t->ctx;
    co_begin(&t->co);
    co_await(&t->co, w->dep);
    w->got = task_result(w->dep);
    co_return(&t->co, w->got);
    co_end(&t->co);
}

typedef struct {
    bool flag;
    int flip_after;
    int i;
} Flipper;

static CoStatus flipper_poll(Task* t)
{
    Flipper* f = (Flipper*)t->ctx;
    co_begin(&t->co);
    for (f->i = 0; f->i < f->flip_after; f->i++) {
        co_yield(&t->co);
    }
    f->flag = true;
    co_return(&t->co, NULL);
    co_end(&t->co);
}

static CoStatus watch_flag_poll(Task* t)
{
    Flipper* f = (Flipper*)t->ctx;
    co_begin(&t->co);
    co_await_until(&t->co, f->flag);
    co_return(&t->co, "flag observed");
    co_end(&t->co);
}

static CoStatus failer_poll(Task* t)
{
    co_begin(&t->co);
    co_yield(&t->co);
    co_fail(&t->co, "boom");
    co_end(&t->co);
}

static CoStatus immediate_poll(Task* t)
{
    co_begin(&t->co);
    co_return(&t->co, "now");
    co_end(&t->co);
}

// --- tests --------------------------------------------------------------

TEST(zero_initialized_scheduler)
{
    Sched s = {0};
    CHECK(s.head == NULL && s.count == 0);
    sched_run(&s);          // nothing to do
    sched_gc(&s);
    sched_free(&s);
}

TEST(a_task_runs_to_completion)
{
    Sched s = {0};
    Counter c = { .target = 0 };
    Task* t = sched_spawn(&s, immediate_poll, &c);
    CHECK(!task_done(t));
    sched_run(&s);
    CHECK(task_done(t));
    CHECK(task_error(t) == NULL);
    CHECK(strcmp((char*)task_result(t), "now") == 0);
    sched_free(&s);
}

TEST(yield_suspends_one_tick_at_a_time)
{
    Sched s = {0};
    Counter c = { .target = 5 };
    Task* t = sched_spawn(&s, counter_poll, &c);

    int ticks = 0;
    while (!task_done(t)) {
        sched_tick(&s);
        ticks++;
    }
    CHECK(ticks == 6);          // 5 yields + the final step
    CHECK(c.i == 5);
    CHECK(strcmp((char*)task_result(t), "counted") == 0);
    sched_free(&s);
}

TEST(higher_priority_runs_first_equal_priority_is_fifo)
{
    Sched s = {0};
    g_order_n = 0;
    Marker a = { 1 }, b = { 2 }, c = { 3 }, d = { 4 };
    sched_spawn_prio(&s, mark_poll, &a, 0);
    sched_spawn_prio(&s, mark_poll, &b, 10);
    sched_spawn_prio(&s, mark_poll, &c, 5);
    sched_spawn_prio(&s, mark_poll, &d, 5);   // same prio as c, spawned after

    sched_tick(&s);
    CHECK(g_order_n == 4);
    CHECK(g_order[0] == 2);     // prio 10
    CHECK(g_order[1] == 3);     // prio 5, first
    CHECK(g_order[2] == 4);     // prio 5, second (FIFO)
    CHECK(g_order[3] == 1);     // prio 0
    sched_free(&s);
}

TEST(block_on_drives_the_whole_loop)
{
    Sched s = {0};
    Counter slow = { .target = 8 };
    Counter fast = { .target = 2 };
    Task* ts = sched_spawn(&s, counter_poll, &slow);
    Task* tf = sched_spawn(&s, counter_poll, &fast);

    void* r = sched_block_on(&s, ts);
    CHECK(strcmp((char*)r, "counted") == 0);
    CHECK(task_done(ts));
    CHECK(task_done(tf));       // the fast task also finished along the way
    sched_free(&s);
}

TEST(co_await_waits_for_another_task_and_sees_its_result)
{
    Sched s = {0};
    Counter dep_ctx = { .target = 4 };
    Task* dep = sched_spawn(&s, counter_poll, &dep_ctx);

    Waiter w = { .dep = dep };
    Task* waiter = sched_spawn(&s, waiter_poll, &w);

    sched_run(&s);
    CHECK(task_done(dep) && task_done(waiter));
    CHECK(w.got != NULL && strcmp((char*)w.got, "counted") == 0);
    CHECK(strcmp((char*)task_result(waiter), "counted") == 0);
    sched_free(&s);
}

TEST(co_await_until_watches_a_condition)
{
    Sched s = {0};
    Flipper f = { .flip_after = 3 };
    Task* watcher = sched_spawn(&s, watch_flag_poll, &f);
    Task* flipper = sched_spawn(&s, flipper_poll, &f);

    sched_run(&s);
    CHECK(f.flag == true);
    CHECK(task_done(watcher) && task_done(flipper));
    CHECK(strcmp((char*)task_result(watcher), "flag observed") == 0);
    sched_free(&s);
}

TEST(co_fail_sets_error_not_result)
{
    Sched s = {0};
    Task* t = sched_spawn(&s, failer_poll, NULL);
    sched_run(&s);
    CHECK(task_done(t));
    CHECK(task_result(t) == NULL);
    CHECK(task_error(t) != NULL && strcmp((char*)task_error(t), "boom") == 0);
    sched_free(&s);
}

TEST(gc_reaps_finished_tasks_only)
{
    Sched s = {0};
    Counter c = { .target = 4 };
    Task* quick = sched_spawn(&s, immediate_poll, NULL);
    Task* slow = sched_spawn(&s, counter_poll, &c);
    (void)quick;

    sched_tick(&s);             // quick finishes, slow still pending
    CHECK(s.count == 2);
    sched_gc(&s);
    CHECK(s.count == 1);
    CHECK(s.head == slow);
    CHECK(!task_done(slow));

    sched_run(&s);
    sched_gc(&s);
    CHECK(s.count == 0 && s.head == NULL);
    sched_free(&s);
}

TEST(many_tasks_all_complete)
{
    Sched s = {0};
    enum { N = 120 };
    Counter ctx[N];
    Task* tasks[N];
    for (int i = 0; i < N; i++) {
        ctx[i].target = i % 7;
        tasks[i] = sched_spawn_prio(&s, counter_poll, &ctx[i], i % 3);
    }
    sched_run(&s);
    int done = 0;
    for (int i = 0; i < N; i++) {
        if (task_done(tasks[i]) && strcmp((char*)task_result(tasks[i]), "counted") == 0) {
            done++;
        }
    }
    CHECK(done == N);
    CHECK(s.count == (size_t)N);
    sched_free(&s);
}

int main(void)
{
    RUN(zero_initialized_scheduler);
    RUN(a_task_runs_to_completion);
    RUN(yield_suspends_one_tick_at_a_time);
    RUN(higher_priority_runs_first_equal_priority_is_fifo);
    RUN(block_on_drives_the_whole_loop);
    RUN(co_await_waits_for_another_task_and_sees_its_result);
    RUN(co_await_until_watches_a_condition);
    RUN(co_fail_sets_error_not_result);
    RUN(gc_reaps_finished_tasks_only);
    RUN(many_tasks_all_complete);
    return test_summary();
}
