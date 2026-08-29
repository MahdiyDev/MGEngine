#include "../stream.h"
#include "test.h"

DEFINE_STREAM(int, IntStream);
DEFINE_STREAM_RANGE(int, IntStream);

DEFINE_STREAM(long, LongStream);
DEFINE_STREAM_MAP(int, IntStream, long, LongStream);

typedef struct {
    int id;
    int score;
} Row;
DEFINE_STREAM(Row, RowStream);

// --- callbacks -------------------------------------------------------------

static int  g_map_calls = 0;
static int  g_peek_calls = 0;

static void square(int* x)          { *x = *x * *x; }
static void inc10(int* x)           { *x += 10; }
static void square_counted(int* x)  { g_map_calls++; *x = *x * *x; }
static bool is_even(const int* x)   { return (*x % 2) == 0; }
static bool is_negative(const int* x) { return *x < 0; }
static bool gt_100(const int* x)    { return *x > 100; }
static void peek_count(const int* x) { (void)x; g_peek_calls++; }
static int  add(int acc, const int* x) { return acc + *x; }
static long widen(const int* x)     { return (long)*x * 1000000L; }

static void push_to_buf(const int* x, void* ctx)
{
    int** p = (int**)ctx;
    *(*p)++ = *x;
}

// infinite source: 0, 1, 2, ...  counting how many times it is pulled
typedef struct {
    int next_val;
    int calls;
} Counter;

static bool counter_gen(void* ctx, int* out)
{
    Counter* c = (Counter*)ctx;
    c->calls++;
    *out = c->next_val++;
    return true;
}

// -------------------------------------------------------------------------

TEST(zero_stream_and_empty_array_are_exhausted)
{
    IntStream z = {0};
    int v;
    CHECK(!IntStream_next(&z, &v));
    IntStream_free(&z);

    CHECK(IntStream_count(IntStream_from_array(NULL, 0)) == 0);
}

TEST(from_array_yields_every_element_without_touching_source)
{
    int src[] = { 3, 1, 4, 1, 5, 9 };
    int out[6] = {0};
    size_t k = IntStream_collect(IntStream_from_array(src, 6), out, 6);
    CHECK(k == 6);
    for (int i = 0; i < 6; i++) {
        CHECK(out[i] == src[i]);
    }
    // map mutates pulled copies, never the backing array
    IntStream_count(IntStream_map(IntStream_from_array(src, 6), square));
    CHECK(src[2] == 4 && src[5] == 9);
}

TEST(map_and_filter)
{
    int src[] = { 1, 2, 3, 4, 5, 6 };
    int out[8] = {0};

    size_t k = IntStream_collect(
        IntStream_map(IntStream_filter(IntStream_from_array(src, 6), is_even), square),
        out, 8);
    CHECK(k == 3);
    CHECK(out[0] == 4 && out[1] == 16 && out[2] == 36);
}

TEST(chained_maps_apply_in_order)
{
    int src[] = { 1, 2, 3 };
    int out[3] = {0};
    IntStream_collect(
        IntStream_map(IntStream_map(IntStream_from_array(src, 3), square), inc10),
        out, 3);
    CHECK(out[0] == 11 && out[1] == 14 && out[2] == 19);
}

TEST(take_and_skip)
{
    int src[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    int out[8] = {0};

    size_t k = IntStream_collect(IntStream_take(IntStream_from_array(src, 8), 3), out, 8);
    CHECK(k == 3 && out[0] == 0 && out[2] == 2);

    k = IntStream_collect(IntStream_skip(IntStream_from_array(src, 8), 5), out, 8);
    CHECK(k == 3 && out[0] == 5 && out[2] == 7);

    // over-take / over-skip are harmless
    CHECK(IntStream_count(IntStream_take(IntStream_from_array(src, 8), 100)) == 8);
    CHECK(IntStream_count(IntStream_skip(IntStream_from_array(src, 8), 100)) == 0);

    // skip then take
    k = IntStream_collect(
        IntStream_take(IntStream_skip(IntStream_from_array(src, 8), 2), 3), out, 8);
    CHECK(k == 3 && out[0] == 2 && out[2] == 4);
}

TEST(take_makes_an_infinite_source_finite)
{
    Counter c = { 0, 0 };
    int out[5] = {0};
    size_t k = IntStream_collect(
        IntStream_from_fn(counter_gen, &c, NULL) /* infinite */, out, 0); // collect 0
    CHECK(k == 0);

    c = (Counter){ 0, 0 };
    k = IntStream_collect(IntStream_take(IntStream_from_fn(counter_gen, &c, NULL), 5), out, 5);
    CHECK(k == 5);
    CHECK(out[0] == 0 && out[4] == 4);
    CHECK(c.calls == 5); // pulled exactly what `take` asked for -- lazy
}

TEST(map_runs_only_for_elements_that_pass_the_filter)
{
    int src[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    g_map_calls = 0;
    size_t n = IntStream_count(
        IntStream_map(IntStream_filter(IntStream_from_array(src, 10), is_even), square_counted));
    CHECK(n == 5);
    CHECK(g_map_calls == 5); // not 10 -- operations are fused, not staged
}

TEST(peek_passes_through)
{
    int src[] = { 1, 2, 3, 4 };
    g_peek_calls = 0;
    int total = IntStream_reduce(
        IntStream_peek(IntStream_from_array(src, 4), peek_count), 0, add);
    CHECK(total == 10);
    CHECK(g_peek_calls == 4);
}

TEST(reduce_count_for_each_ctx)
{
    int src[] = { 10, 20, 30, 40 };
    CHECK(IntStream_reduce(IntStream_from_array(src, 4), 100, add) == 200);
    CHECK(IntStream_count(IntStream_from_array(src, 4)) == 4);

    int buf[4] = {0};
    int* cur = buf;
    IntStream_for_each_ctx(IntStream_from_array(src, 4), push_to_buf, &cur);
    CHECK(cur == buf + 4);
    CHECK(buf[0] == 10 && buf[3] == 40);
}

TEST(any_all_short_circuit)
{
    int src[] = { 2, 4, 6, 7, 8 };
    CHECK(IntStream_any(IntStream_from_array(src, 5), is_negative) == false);
    CHECK(IntStream_all(IntStream_from_array(src, 5), is_even) == false);
    CHECK(IntStream_all(IntStream_from_array(src, 3), is_even) == true);

    // any() must stop pulling at the first match
    Counter c = { 0, 0 }; // yields 0,1,2,3,... ; gt_100 never true early
    bool hit = IntStream_any(
        IntStream_map(IntStream_from_fn(counter_gen, &c, NULL), inc10), // 10,11,12,...
        gt_100);
    CHECK(hit == true);
    // source yields 0..91 before inc10(91)==101 first exceeds 100
    CHECK(c.calls == 92);
}

TEST(find)
{
    int src[] = { 5, 8, 11, 14, 17 };
    int got = -1;
    CHECK(IntStream_find(IntStream_from_array(src, 5), is_even, &got));
    CHECK(got == 8);
    CHECK(!IntStream_find(IntStream_from_array(src, 5), is_negative, &got));
}

TEST(from_range)
{
    int out[16] = {0};
    size_t k = IntStream_collect(IntStream_from_range(0, 10, 2), out, 16);
    CHECK(k == 5 && out[0] == 0 && out[4] == 8);

    k = IntStream_collect(IntStream_from_range(5, 0, -2), out, 16);
    CHECK(k == 3 && out[0] == 5 && out[1] == 3 && out[2] == 1);

    CHECK(IntStream_count(IntStream_from_range(0, 0, 1)) == 0);

    int sum = IntStream_reduce(IntStream_from_range(1, 101, 1), 0, add);
    CHECK(sum == 5050);
}

TEST(map_to_a_different_type)
{
    int src[] = { 1, 2, 3 };
    long out[3] = {0};
    size_t k = LongStream_collect(
        IntStream_map_to_LongStream(
            IntStream_filter(IntStream_from_array(src, 3), is_even),
            widen),
        out, 3);
    CHECK(k == 1);
    CHECK(out[0] == 2000000L);
}

TEST(abandoned_pipeline_frees_cleanly)
{
    int src[] = { 1, 2, 3, 4 };
    IntStream s = IntStream_map(
        IntStream_filter(IntStream_take(IntStream_from_array(src, 4), 3), is_even),
        square);
    int v;
    CHECK(IntStream_next(&s, &v) && v == 4); // pull one, then bail
    IntStream_free(&s);
    IntStream_free(&s); // idempotent
}

static bool row_passing(const Row* r) { return r->score >= 50; }

static void row_sum_scores(const Row* r, void* ctx)
{
    *(int*)ctx += r->score;
}

TEST(stream_of_structs)
{
    Row rows[] = { { 1, 30 }, { 2, 55 }, { 3, 40 }, { 4, 90 } };

    CHECK(RowStream_count(RowStream_filter(RowStream_from_array(rows, 4), row_passing)) == 2);

    int total = 0;
    RowStream_for_each_ctx(
        RowStream_filter(RowStream_from_array(rows, 4), row_passing),
        row_sum_scores, &total);
    CHECK(total == 145);

    Row first = {0};
    CHECK(RowStream_find(RowStream_from_array(rows, 4), row_passing, &first));
    CHECK(first.id == 2 && first.score == 55);
}

int main(void)
{
    RUN(zero_stream_and_empty_array_are_exhausted);
    RUN(from_array_yields_every_element_without_touching_source);
    RUN(map_and_filter);
    RUN(chained_maps_apply_in_order);
    RUN(take_and_skip);
    RUN(take_makes_an_infinite_source_finite);
    RUN(map_runs_only_for_elements_that_pass_the_filter);
    RUN(peek_passes_through);
    RUN(reduce_count_for_each_ctx);
    RUN(any_all_short_circuit);
    RUN(find);
    RUN(from_range);
    RUN(map_to_a_different_type);
    RUN(abandoned_pipeline_frees_cleanly);
    RUN(stream_of_structs);
    return test_summary();
}
