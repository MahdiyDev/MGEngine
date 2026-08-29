#pragma once

// Repo-wide minimal test harness. No dependencies; one translation unit per
// test executable. Each module builds its own test binaries and includes this
// header (add the repo root to the include path, e.g. -I<root>).
//
//     #include "test.h"
//     TEST(does_a_thing) { CHECK(1 + 1 == 2); }
//     int main(void) { RUN(does_a_thing); return test_summary(); }

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int test_checks = 0;
static int test_failures = 0;
static const char* test_current = "";

#define CHECK(cond)                                                              \
    do {                                                                        \
        test_checks++;                                                           \
        if (!(cond)) {                                                           \
            test_failures++;                                                     \
            printf("  FAIL %s:%d  (%s)  in %s\n", __FILE__, __LINE__, #cond,     \
                   test_current);                                               \
        }                                                                       \
    } while (0)

#define CHECK_STR_EQ(got, got_len, expect)                                       \
    do {                                                                        \
        test_checks++;                                                           \
        const char* e_ = (expect);                                               \
        size_t el_ = strlen(e_);                                                 \
        if ((size_t)(got_len) != el_ || memcmp((got), e_, el_) != 0) {           \
            test_failures++;                                                     \
            printf("  FAIL %s:%d  expected \"%s\", got \"%.*s\"  in %s\n",       \
                   __FILE__, __LINE__, e_, (int)(got_len), (got), test_current); \
        }                                                                       \
    } while (0)

#define TEST(name) static void test_##name(void)

#define RUN(name)                             \
    do {                                     \
        test_current = #name;                 \
        printf("- %s\n", #name);              \
        test_##name();                        \
    } while (0)

static inline int test_summary(void)
{
    printf("\n%s: %d checks, %d failed\n",
           test_failures ? "FAILED" : "ok", test_checks, test_failures);
    return test_failures ? 1 : 0;
}
