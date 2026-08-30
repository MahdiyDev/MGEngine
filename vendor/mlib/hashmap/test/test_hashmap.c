#include <string.h>

#include "../hashmap.h"
#include "test.h"

DEFINE_HASHMAP_STR(int, StrIntMap);
DEFINE_HASHMAP_INT(int, int, IntMap);

typedef struct {
    int x;
    int y;
} Point;

static size_t point_hash(Point p) { return (size_t)hashmap_fnv1a(&p, sizeof(p)); }
static bool point_eq(Point a, Point b) { return a.x == b.x && a.y == b.y; }
DEFINE_HASHMAP(Point, int, PointMap, point_hash, point_eq);

// every key collides -> one long probe chain, to exercise backshift deletion
static size_t always_zero(int k) { (void)k; return 0; }
DEFINE_HASHMAP_FULL(int, int, ClashMap, always_zero, HASHMAP_EQ_SCALAR_,
                    HASHMAP_KEY_ID_, HASHMAP_KEY_NOFREE_);

TEST(zero_initialized_is_a_valid_empty_map)
{
    StrIntMap m = {0};
    CHECK(m.slots == NULL && m.cap == 0 && m.count == 0);
    CHECK(StrIntMap_get(&m, "x") == NULL);
    CHECK(!StrIntMap_contains(&m, "x"));
    CHECK(!StrIntMap_remove(&m, "x"));
    StrIntMap_free(&m);
    StrIntMap_free(&m);   // idempotent
    StrIntMap_free(NULL);
}

TEST(put_get_overwrite)
{
    StrIntMap m = {0};
    CHECK(StrIntMap_put(&m, "a", 1) == true);   // newly added
    CHECK(StrIntMap_put(&m, "b", 2) == true);
    CHECK(m.count == 2);

    CHECK(*StrIntMap_get(&m, "a") == 1);
    CHECK(*StrIntMap_get(&m, "b") == 2);

    CHECK(StrIntMap_put(&m, "a", 99) == false);  // overwrite
    CHECK(m.count == 2);
    CHECK(*StrIntMap_get(&m, "a") == 99);

    *StrIntMap_get(&m, "b") += 100;              // get() is a mutable V*
    CHECK(*StrIntMap_get(&m, "b") == 102);

    StrIntMap_free(&m);
}

TEST(keys_are_copied)
{
    StrIntMap m = {0};
    char buf[16];
    strcpy(buf, "hello");
    StrIntMap_put(&m, buf, 42);
    strcpy(buf, "XXXXX");                        // clobber the caller's buffer
    int* v = StrIntMap_get(&m, "hello");
    CHECK(v != NULL && *v == 42);
    StrIntMap_free(&m);
}

TEST(grows_and_keeps_everything)
{
    StrIntMap m = {0};
    char key[16];
    for (int i = 0; i < 1000; i++) {
        sprintf(key, "k%d", i);
        CHECK(StrIntMap_put(&m, key, i * 3));
    }
    CHECK(m.count == 1000);
    CHECK(m.cap >= 1000);

    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        sprintf(key, "k%d", i);
        int* v = StrIntMap_get(&m, key);
        if (v == NULL || *v != i * 3) {
            ok = 0;
        }
    }
    CHECK(ok);
    CHECK(StrIntMap_get(&m, "k1000") == NULL);
    StrIntMap_free(&m);
}

TEST(reserve_avoids_rehash)
{
    StrIntMap m = {0};
    StrIntMap_reserve(&m, 500);
    size_t cap = m.cap;
    CHECK(cap >= 500);

    char key[16];
    for (int i = 0; i < 400; i++) {
        sprintf(key, "k%d", i);
        StrIntMap_put(&m, key, i);
    }
    CHECK(m.cap == cap);   // no rehash below the load threshold
    StrIntMap_free(&m);
}

TEST(remove_and_reinsert)
{
    IntMap m = {0};
    for (int i = 0; i < 50; i++) {
        IntMap_put(&m, i, i * i);
    }
    CHECK(m.count == 50);

    for (int i = 0; i < 50; i += 2) {
        CHECK(IntMap_remove(&m, i));
    }
    CHECK(m.count == 25);
    CHECK(!IntMap_remove(&m, 0));        // already gone
    CHECK(IntMap_get(&m, 4) == NULL);
    CHECK(*IntMap_get(&m, 5) == 25);     // odd keys survive

    for (int i = 0; i < 50; i += 2) {
        IntMap_put(&m, i, -i);
    }
    CHECK(m.count == 50);
    CHECK(*IntMap_get(&m, 4) == -4);
    CHECK(*IntMap_get(&m, 5) == 25);
    IntMap_free(&m);
}

TEST(backshift_keeps_a_collision_chain_searchable)
{
    ClashMap m = {0};
    for (int k = 10; k <= 50; k += 10) {
        ClashMap_put(&m, k, k / 10);      // 10..50 all share one probe chain
    }
    CHECK(m.count == 5);

    CHECK(ClashMap_remove(&m, 30));       // middle of the chain
    CHECK(!ClashMap_contains(&m, 30));
    CHECK(*ClashMap_get(&m, 10) == 1);
    CHECK(*ClashMap_get(&m, 20) == 2);
    CHECK(*ClashMap_get(&m, 40) == 4);
    CHECK(*ClashMap_get(&m, 50) == 5);

    CHECK(ClashMap_remove(&m, 10));       // head of the chain (home slot)
    CHECK(*ClashMap_get(&m, 20) == 2);
    CHECK(*ClashMap_get(&m, 40) == 4);
    CHECK(*ClashMap_get(&m, 50) == 5);
    CHECK(m.count == 3);
    ClashMap_free(&m);
}

TEST(backshift_survives_many_random_ish_removals)
{
    IntMap m = {0};
    for (int i = 0; i < 300; i++) {
        IntMap_put(&m, i, i + 1);
    }
    int removed[300] = {0};
    for (int i = 7; i < 300; i += 7) {   // remove a scattered subset
        CHECK(IntMap_remove(&m, i));
        removed[i] = 1;
    }
    int ok = 1;
    for (int i = 0; i < 300; i++) {
        int* v = IntMap_get(&m, i);
        if (removed[i]) {
            if (v != NULL) ok = 0;
        } else {
            if (v == NULL || *v != i + 1) ok = 0;
        }
    }
    CHECK(ok);
    IntMap_free(&m);
}

TEST(clear_keeps_capacity_and_is_reusable)
{
    StrIntMap m = {0};
    char key[16];
    for (int i = 0; i < 100; i++) {
        sprintf(key, "k%d", i);
        StrIntMap_put(&m, key, i);
    }
    size_t cap = m.cap;
    StrIntMap_clear(&m);
    CHECK(m.count == 0);
    CHECK(m.cap == cap);
    CHECK(StrIntMap_get(&m, "k1") == NULL);

    StrIntMap_put(&m, "fresh", 7);
    CHECK(m.count == 1 && *StrIntMap_get(&m, "fresh") == 7);
    StrIntMap_free(&m);
}

TEST(get_or_put)
{
    IntMap m = {0};
    CHECK(*IntMap_get_or_put(&m, 1, 10) == 10);   // inserted
    CHECK(*IntMap_get_or_put(&m, 1, 999) == 10);  // already there
    CHECK(m.count == 1);

    (*IntMap_get_or_put(&m, 2, 0))++;
    (*IntMap_get_or_put(&m, 2, 0))++;
    CHECK(*IntMap_get(&m, 2) == 2);
    IntMap_free(&m);
}

TEST(iteration_visits_every_entry_once)
{
    IntMap m = {0};
    for (int i = 0; i < 64; i++) {
        IntMap_put(&m, i, i * 10);
    }
    int seen[64] = {0};
    size_t visited = 0;
    hashmap_foreach(IntMap, it, &m) {
        CHECK(it.key >= 0 && it.key < 64);
        CHECK(*it.value == it.key * 10);
        seen[it.key]++;
        *it.value += 1;          // mutate through the iterator
        visited++;
    }
    CHECK(visited == 64 && visited == m.count);
    int all_once = 1;
    for (int i = 0; i < 64; i++) {
        if (seen[i] != 1 || *IntMap_get(&m, i) != i * 10 + 1) {
            all_once = 0;
        }
    }
    CHECK(all_once);
    IntMap_free(&m);
}

TEST(struct_keys_and_int_key_edge_values)
{
    PointMap pm = {0};
    PointMap_put(&pm, (Point){ 1, 2 }, 100);
    PointMap_put(&pm, (Point){ 2, 1 }, 200);   // distinct from {1,2}
    PointMap_put(&pm, (Point){ 1, 2 }, 111);   // overwrite
    CHECK(pm.count == 2);
    CHECK(*PointMap_get(&pm, (Point){ 1, 2 }) == 111);
    CHECK(*PointMap_get(&pm, (Point){ 2, 1 }) == 200);
    CHECK(PointMap_get(&pm, (Point){ 9, 9 }) == NULL);
    PointMap_free(&pm);

    IntMap im = {0};
    IntMap_put(&im, 0, 1);
    IntMap_put(&im, -1, 2);
    IntMap_put(&im, -2147483647, 3);
    CHECK(*IntMap_get(&im, 0) == 1);
    CHECK(*IntMap_get(&im, -1) == 2);
    CHECK(*IntMap_get(&im, -2147483647) == 3);
    CHECK(im.count == 3);
    IntMap_free(&im);
}

int main(void)
{
    RUN(zero_initialized_is_a_valid_empty_map);
    RUN(put_get_overwrite);
    RUN(keys_are_copied);
    RUN(grows_and_keeps_everything);
    RUN(reserve_avoids_rehash);
    RUN(remove_and_reinsert);
    RUN(backshift_keeps_a_collision_chain_searchable);
    RUN(backshift_survives_many_random_ish_removals);
    RUN(clear_keeps_capacity_and_is_reusable);
    RUN(get_or_put);
    RUN(iteration_visits_every_entry_once);
    RUN(struct_keys_and_int_key_edge_values);
    return test_summary();
}
