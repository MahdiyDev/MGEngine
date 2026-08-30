#include <string.h>

#include "../vec.h"
#include "test.h"

DEFINE_VEC(int, IntVec);

typedef struct {
    int id;
    double weight;
} Item;

DEFINE_VEC(Item, ItemVec);

static int int_sum(const IntVec* v)
{
    int s = 0;
    for (size_t i = 0; i < v->count; i++) {
        s += v->items[i];
    }
    return s;
}

TEST(zero_initialized_is_a_valid_empty_vec)
{
    IntVec v = {0};
    CHECK(v.items == NULL);
    CHECK(v.count == 0);
    CHECK(v.capacity == 0);

    IntVec_append(&v, 7); // must work without an explicit init
    CHECK(v.count == 1);
    CHECK(v.items[0] == 7);
    CHECK(v.capacity >= 1);

    IntVec_free(&v);
}

TEST(reserve_presizes_a_zeroed_vec)
{
    IntVec b = {0};
    IntVec_reserve(&b, 100);
    CHECK(b.count == 0);
    CHECK(b.capacity >= 100);
    CHECK(b.items != NULL);

    IntVec_append(&b, 1); // no reallocation until 100 elements
    CHECK(b.capacity >= 100);
    IntVec_free(&b);
}

TEST(append_grows_geometrically_and_keeps_order)
{
    IntVec v = {0};
    for (int i = 0; i < 1000; i++) {
        IntVec_append(&v, i);
    }
    CHECK(v.count == 1000);
    CHECK(v.capacity >= 1000);

    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        if (v.items[i] != i) {
            ok = 0;
        }
    }
    CHECK(ok);
    CHECK(int_sum(&v) == 999 * 1000 / 2);
    IntVec_free(&v);
}

TEST(reserve_never_shrinks)
{
    IntVec v = {0};
    IntVec_reserve(&v, 64);
    size_t cap = v.capacity;
    CHECK(cap >= 64);

    IntVec_reserve(&v, 10); // smaller request is a no-op
    CHECK(v.capacity == cap);

    IntVec_append(&v, 1);
    IntVec_reserve(&v, 1);
    CHECK(v.capacity == cap); // still no shrink
    IntVec_free(&v);
}

TEST(append_many)
{
    IntVec v = {0};
    int a[] = { 1, 2, 3 };
    int b[] = { 4, 5, 6, 7, 8 };

    IntVec_append_many(&v, a, 3);
    IntVec_append_many(&v, b, 5);
    IntVec_append_many(&v, a, 0); // zero count is a no-op

    CHECK(v.count == 8);
    for (int i = 0; i < 8; i++) {
        CHECK(v.items[i] == i + 1);
    }
    IntVec_free(&v);
}

TEST(prepend_and_prepend_many)
{
    IntVec v = {0};
    IntVec_append(&v, 100);
    IntVec_prepend(&v, 1);
    IntVec_prepend(&v, 0);
    // 0, 1, 100
    CHECK(v.count == 3);
    CHECK(v.items[0] == 0);
    CHECK(v.items[1] == 1);
    CHECK(v.items[2] == 100);

    int head[] = { -3, -2, -1 };
    IntVec_prepend_many(&v, head, 3);
    IntVec_prepend_many(&v, head, 0); // no-op
    // -3, -2, -1, 0, 1, 100
    CHECK(v.count == 6);
    CHECK(v.items[0] == -3);
    CHECK(v.items[3] == 0);
    CHECK(v.items[5] == 100);
    IntVec_free(&v);
}

TEST(get_returns_mutable_pointer)
{
    IntVec v = {0};
    IntVec_append(&v, 10);
    IntVec_append(&v, 20);
    IntVec_append(&v, 30);

    CHECK(*IntVec_get(&v, 1) == 20);
    *IntVec_get(&v, 1) = 99;
    CHECK(v.items[1] == 99);
    CHECK(IntVec_get(&v, 2) == &v.items[2]);
    IntVec_free(&v);
}

TEST(delete_first_middle_last)
{
    IntVec v = {0};
    int src[] = { 0, 1, 2, 3, 4 };
    IntVec_append_many(&v, src, 5);

    IntVec_delete(&v, 0); // -> 1 2 3 4
    CHECK(v.count == 4);
    CHECK(v.items[0] == 1);

    IntVec_delete(&v, 1); // remove '2' -> 1 3 4
    CHECK(v.count == 3);
    CHECK(v.items[0] == 1);
    CHECK(v.items[1] == 3);
    CHECK(v.items[2] == 4);

    IntVec_delete(&v, v.count - 1); // remove last -> 1 3
    CHECK(v.count == 2);
    CHECK(v.items[1] == 3);
    IntVec_free(&v);
}

TEST(delete_range_half_open)
{
    IntVec v = {0};
    int src[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    IntVec_append_many(&v, src, 8);

    IntVec_delete_range(&v, 2, 2); // empty range: no-op
    CHECK(v.count == 8);

    IntVec_delete_range(&v, 2, 5); // remove 2,3,4 -> 0 1 5 6 7
    CHECK(v.count == 5);
    CHECK(v.items[0] == 0);
    CHECK(v.items[1] == 1);
    CHECK(v.items[2] == 5);
    CHECK(v.items[3] == 6);
    CHECK(v.items[4] == 7);

    IntVec_delete_range(&v, 0, v.count); // remove everything
    CHECK(v.count == 0);
    IntVec_free(&v);
}

TEST(clear_keeps_capacity_and_stays_usable)
{
    IntVec v = {0};
    for (int i = 0; i < 50; i++) {
        IntVec_append(&v, i);
    }
    size_t cap = v.capacity;

    IntVec_clear(&v);
    CHECK(v.count == 0);
    CHECK(v.capacity == cap); // no reallocation on clear
    CHECK(v.items != NULL);

    IntVec_append(&v, 123);
    CHECK(v.count == 1);
    CHECK(v.items[0] == 123);
    IntVec_free(&v);
}

TEST(free_resets_and_tolerates_null)
{
    IntVec v = {0};
    IntVec_append(&v, 1);
    IntVec_free(&v);
    CHECK(v.items == NULL);
    CHECK(v.count == 0);
    CHECK(v.capacity == 0);

    IntVec_free(&v);   // second free is safe (items is NULL)
    IntVec_free(NULL); // NULL is safe
}

TEST(vector_of_structs)
{
    ItemVec v = {0};
    ItemVec_append(&v, (Item){ .id = 1, .weight = 1.5 });
    ItemVec_append(&v, (Item){ .id = 2, .weight = 2.5 });

    Item more[] = { { 3, 3.5 }, { 4, 4.5 } };
    ItemVec_append_many(&v, more, 2);

    CHECK(v.count == 4);
    CHECK(v.items[0].id == 1);
    CHECK(v.items[3].id == 4);
    CHECK(v.items[2].weight == 3.5);

    ItemVec_delete(&v, 0);
    CHECK(v.items[0].id == 2);
    CHECK(v.count == 3);

    ItemVec_free(&v);
}

TEST(interleaved_operations)
{
    IntVec v = {0};
    IntVec_append(&v, 5);
    IntVec_prepend(&v, 3);
    IntVec_append(&v, 8);
    int mid[] = { 6, 7 };
    IntVec_append_many(&v, mid, 2);   // 3 5 8 6 7
    IntVec_delete(&v, 2);             // 3 5 6 7
    IntVec_prepend(&v, 1);            // 1 3 5 6 7
    IntVec_delete_range(&v, 1, 3);    // 1 6 7

    CHECK(v.count == 3);
    CHECK(v.items[0] == 1);
    CHECK(v.items[1] == 6);
    CHECK(v.items[2] == 7);
    IntVec_free(&v);
}

int main(void)
{
    RUN(zero_initialized_is_a_valid_empty_vec);
    RUN(reserve_presizes_a_zeroed_vec);
    RUN(append_grows_geometrically_and_keeps_order);
    RUN(reserve_never_shrinks);
    RUN(append_many);
    RUN(prepend_and_prepend_many);
    RUN(get_returns_mutable_pointer);
    RUN(delete_first_middle_last);
    RUN(delete_range_half_open);
    RUN(clear_keeps_capacity_and_stays_usable);
    RUN(free_resets_and_tolerates_null);
    RUN(vector_of_structs);
    RUN(interleaved_operations);
    return test_summary();
}
