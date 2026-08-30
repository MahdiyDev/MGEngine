#include <stdio.h>

#include "../stream.h"

DEFINE_STREAM(int, IntStream);
DEFINE_STREAM_RANGE(int, IntStream);

DEFINE_STREAM(double, DblStream);
DEFINE_STREAM_MAP(int, IntStream, double, DblStream);

// --- pipeline callbacks --------------------------------------------------

static bool   is_even(const int* x)        { return (*x % 2) == 0; }
static void   cube(int* x)                 { *x = *x * *x * *x; }
static void   show_int(const int* x)       { printf("%d ", *x); }
static int    add(int acc, const int* x)   { return acc + *x; }
static double c_to_f(const int* c)         { return *c * 9.0 / 5.0 + 32.0; }
static void   show_f(const double* f)      { printf("%.1f ", *f); }

// an infinite source: the Fibonacci sequence
typedef struct {
    long a;
    long b;
} Fib;

static bool fib_next(void* ctx, int* out)
{
    Fib* f = (Fib*)ctx;
    *out = (int)f->a;
    long next = f->a + f->b;
    f->a = f->b;
    f->b = next;
    return true;
}

int main(void)
{
    int data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    // nested style: reads inside-out
    printf("even^3:      ");
    IntStream_for_each(
        IntStream_map(IntStream_filter(IntStream_from_array(data, 8), is_even), cube),
        show_int);                                  // 8 64 216 512
    printf("\n");

    // same pipeline, reassignment style: reads top-to-bottom. Each step consumes
    // the old `s` and rebinds it; the terminal frees the whole chain.
    printf("even^3 again: ");
    IntStream s2 = IntStream_from_array(data, 8);
    s2 = IntStream_filter(s2, is_even);
    s2 = IntStream_map(s2, cube);
    IntStream_for_each(s2, show_int);                // 8 64 216 512
    printf("\n");

    int sum = IntStream_reduce(IntStream_from_range(1, 101, 1), 0, add);
    printf("sum 1..100:  %d\n", sum);               // 5050

    // a lazy infinite source, made finite by take()
    Fib fib = { 0, 1 };
    printf("fib x10:     ");
    IntStream_for_each(
        IntStream_take(IntStream_from_fn(fib_next, &fib, NULL), 10),
        show_int);                                  // 0 1 1 2 3 5 8 13 21 34
    printf("\n");

    // map to a different element type
    int celsius[] = { 0, 20, 37, 100 };
    printf("fahrenheit:  ");
    DblStream_for_each(
        IntStream_map_to_DblStream(IntStream_from_array(celsius, 4), c_to_f),
        show_f);                                    // 32.0 68.0 98.6 212.0
    printf("\n");

    // manual pull loop
    IntStream s = IntStream_skip(IntStream_from_range(0, 100, 10), 3);
    int v;
    printf("skip 3 of range: ");
    while (IntStream_next(&s, &v)) {
        printf("%d ", v);                           // 30 40 50 60 70 80 90
    }
    IntStream_free(&s);
    printf("\n");

    return 0;
}
