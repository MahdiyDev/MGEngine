#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mge.h"
#include "mge_utils.h"
#include "test.h" // mlib repo-wide harness

#define TMP "_mge_utils.tmp"

static void write_raw(const char* path, const void* data, size_t n)
{
    FILE* f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        CHECK(fwrite(data, 1, n, f) == n);
        fclose(f);
    }
}

TEST(get_file_extension)
{
    CHECK(strcmp(Mge_GetFileExtension("shader.frag"), ".frag") == 0);
    CHECK(strcmp(Mge_GetFileExtension("a/b/c.tar.gz"), ".gz") == 0);
    CHECK(Mge_GetFileExtension("no_extension") == NULL);
    CHECK(Mge_GetFileExtension(".hidden") == NULL); // dot at start is not an extension
}

TEST(load_file_text_round_trips_and_nul_terminates)
{
    const char* body = "#version 460 core\nvoid main(){}\n";
    write_raw(TMP, body, strlen(body));

    char* text = Mge_LoadFileText(TMP);
    CHECK(text != NULL);
    if (text != NULL) {
        CHECK(strcmp(text, body) == 0); // NUL-terminated, exact content
        CHECK(strlen(text) == strlen(body));
        Mge_UnLoadFileText(text);
    }

    remove(TMP);
}

TEST(load_file_text_missing_file_returns_null)
{
    CHECK(Mge_LoadFileText("definitely_absent.tmp") == NULL);
    CHECK(Mge_LoadFileText(NULL) == NULL);
}

TEST(load_file_data_reports_size)
{
    unsigned char blob[777];
    for (size_t i = 0; i < sizeof(blob); i++)
        blob[i] = (unsigned char)(i * 13 + 1);
    write_raw(TMP, blob, sizeof(blob));

    size_t size = 0;
    unsigned char* data = Mge_LoadFileData(TMP, &size);
    CHECK(data != NULL);
    if (data != NULL) {
        CHECK(size == sizeof(blob));
        CHECK(memcmp(data, blob, sizeof(blob)) == 0);
        Mge_UnloadFileData(data);
    }

    size = 123;
    CHECK(Mge_LoadFileData("definitely_absent.tmp", &size) == NULL);
    CHECK(size == 0); // reset on failure

    remove(TMP);
}

TEST(trace_log_does_not_crash)
{
    Trace_Log(LOG_INFO, "unit test log: %d %s", 42, "ok");
    Trace_Log(LOG_WARNING, "%s", "warning path");
    CHECK(true);
}

int main(void)
{
    RUN(get_file_extension);
    RUN(load_file_text_round_trips_and_nul_terminates);
    RUN(load_file_text_missing_file_returns_null);
    RUN(load_file_data_reports_size);
    RUN(trace_log_does_not_crash);
    return test_summary();
}
