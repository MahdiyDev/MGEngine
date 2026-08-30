#include <stdio.h>

#include "../async_fs.h"

int main(void)
{
    Sched s = {0};

    // Kick off a write and a read in the same loop; they interleave chunk by
    // chunk instead of blocking one another.
    const char* greeting = "hello from async_fs\n";
    Task* w = async_write_file(&s, "example/greeting.tmp", greeting, strlen(greeting));
    if (w == NULL) {
        fprintf(stderr, "could not open file for writing\n");
        return 1;
    }
    sched_run(&s);
    if (task_error(w)) {
        fprintf(stderr, "write failed: %s\n", (char*)task_error(w));
        return 1;
    }
    async_file_close(w);

    Task* r = async_read_file(&s, "example/greeting.tmp");
    if (r == NULL) {
        fprintf(stderr, "could not open file for reading\n");
        return 1;
    }

    string_builder* sb = (string_builder*)sched_block_on(&s, r);
    if (task_error(r)) {
        fprintf(stderr, "read failed: %s\n", (char*)task_error(r));
        return 1;
    }
    printf("read %zu bytes: %.*s", sb->count, (int)sb->count, sb->items);

    async_file_close(r);
    sched_free(&s);
    remove("example/greeting.tmp");
    return 0;
}
