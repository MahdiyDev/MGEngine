#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../async_fs.h"
#include "test.h"

#define TMP "test/async_fs_test.tmp"

static void write_raw(const char* path, const void* data, size_t n)
{
    FILE* f = fopen(path, "wb");
    CHECK(f != NULL);
    CHECK(fwrite(data, 1, n, f) == n);
    fclose(f);
}

TEST(read_a_file_in_one_run)
{
    const char* payload = "the quick brown fox";
    write_raw(TMP, payload, strlen(payload));

    Sched s = {0};
    Task* t = async_read_file(&s, TMP);
    CHECK(t != NULL);

    string_builder* sb = (string_builder*)sched_block_on(&s, t);
    CHECK(task_error(t) == NULL);
    CHECK(sb->count == strlen(payload));
    CHECK(memcmp(sb->items, payload, sb->count) == 0);

    async_file_close(t);
    sched_free(&s);
    remove(TMP);
}

TEST(write_then_read_round_trips)
{
    unsigned char payload[10000];
    for (size_t i = 0; i < sizeof(payload); i++) {
        payload[i] = (unsigned char)(i * 37 + 11);
    }

    Sched s = {0};
    Task* w = async_write_file(&s, TMP, payload, sizeof(payload));
    CHECK(w != NULL);
    sched_run(&s);
    CHECK(task_error(w) == NULL);
    async_file_close(w);

    Task* r = async_read_file(&s, TMP);
    CHECK(r != NULL);
    string_builder* sb = (string_builder*)sched_block_on(&s, r);
    CHECK(sb->count == sizeof(payload));                 // spans several 4 KB chunks
    CHECK(memcmp(sb->items, payload, sizeof(payload)) == 0);

    async_file_close(r);
    sched_free(&s);
    remove(TMP);
}

TEST(source_buffer_need_not_outlive_the_write)
{
    Sched s = {0};
    Task* w;
    {
        char scratch[64];
        strcpy(scratch, "copied, not referenced");
        w = async_write_file(&s, TMP, scratch, strlen(scratch));
        memset(scratch, 'X', sizeof(scratch));          // clobber the caller's buffer
    }
    sched_run(&s);
    CHECK(task_error(w) == NULL);
    async_file_close(w);

    FILE* f = fopen(TMP, "rb");
    char got[64] = {0};
    size_t n = fread(got, 1, sizeof(got) - 1, f);
    fclose(f);
    CHECK(n == strlen("copied, not referenced"));
    CHECK(strcmp(got, "copied, not referenced") == 0);

    sched_free(&s);
    remove(TMP);
}

TEST(missing_file_yields_a_null_task)
{
    Sched s = {0};
    Task* t = async_read_file(&s, "test/definitely_not_here.tmp");
    CHECK(t == NULL);
    CHECK(s.count == 0);
    sched_free(&s);
}

TEST(read_and_write_interleave_in_one_loop)
{
    const char* payload = "interleaved io payload";
    write_raw(TMP ".in", payload, strlen(payload));

    Sched s = {0};
    Task* r = async_read_file_prio(&s, TMP ".in", 1);
    Task* w = async_write_file_prio(&s, TMP ".out", "second stream", 13, 0);
    CHECK(r != NULL && w != NULL);

    sched_run(&s);
    CHECK(task_done(r) && task_done(w));
    CHECK(task_error(r) == NULL && task_error(w) == NULL);

    string_builder* sb = (string_builder*)task_result(r);
    CHECK(sb->count == strlen(payload) && memcmp(sb->items, payload, sb->count) == 0);

    async_file_close(r);
    async_file_close(w);
    sched_free(&s);
    remove(TMP ".in");
    remove(TMP ".out");
}

int main(void)
{
    RUN(read_a_file_in_one_run);
    RUN(write_then_read_round_trips);
    RUN(source_buffer_need_not_outlive_the_write);
    RUN(missing_file_yields_a_null_task);
    RUN(read_and_write_interleave_in_one_loop);
    return test_summary();
}
