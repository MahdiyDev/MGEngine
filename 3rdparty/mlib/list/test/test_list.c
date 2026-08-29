#include "../list.h"
#include "test.h"

DEFINE_LIST(int, IntList);

typedef struct {
    int id;
    int weight;
} Item;

DEFINE_LIST(Item, ItemList);

// Walk the list forwards and backwards and confirm prev/next stay consistent
// and that count matches the actual chain length.
static bool list_is_consistent(const IntList* l)
{
    size_t forward = 0;
    IntList_node* last = NULL;
    for (IntList_node* n = l->head; n != NULL; n = n->next) {
        if (n->prev != last) {
            return false;
        }
        last = n;
        forward++;
    }
    if (last != l->tail || forward != l->count) {
        return false;
    }

    size_t backward = 0;
    IntList_node* seen = NULL;
    for (IntList_node* n = l->tail; n != NULL; n = n->prev) {
        if (n->next != seen) {
            return false;
        }
        seen = n;
        backward++;
    }
    return seen == l->head && backward == l->count;
}

static bool equals_ctx(const int* v, void* ctx)
{
    return *v == *(int*)ctx;
}

TEST(zero_initialized_is_a_valid_empty_list)
{
    IntList l = {0};
    CHECK(l.head == NULL);
    CHECK(l.tail == NULL);
    CHECK(l.count == 0);
    CHECK(!IntList_pop_front(&l, NULL));
    CHECK(!IntList_pop_back(&l, NULL));
    CHECK(IntList_at(&l, 0) == NULL);
    CHECK(!IntList_remove_at(&l, 0));
    IntList_free(&l);
}

TEST(push_back_keeps_order_and_endpoints)
{
    IntList l = {0};
    for (int i = 0; i < 5; i++) {
        IntList_node* n = IntList_push_back(&l, i);
        CHECK(n->value == i);
        CHECK(l.tail == n);
    }
    CHECK(l.count == 5);
    CHECK(l.head->value == 0);
    CHECK(l.tail->value == 4);
    CHECK(list_is_consistent(&l));

    int expect = 0;
    list_foreach(IntList, it, &l) {
        CHECK(it->value == expect++);
    }
    IntList_free(&l);
}

TEST(push_front_reverses_order)
{
    IntList l = {0};
    for (int i = 0; i < 4; i++) {
        IntList_push_front(&l, i); // 3,2,1,0
    }
    CHECK(l.count == 4);
    CHECK(l.head->value == 3);
    CHECK(l.tail->value == 0);

    int expect = 3;
    list_foreach(IntList, it, &l) {
        CHECK(it->value == expect--);
    }
    int rexpect = 0;
    list_foreach_rev(IntList, it, &l) {
        CHECK(it->value == rexpect++);
    }
    CHECK(list_is_consistent(&l));
    IntList_free(&l);
}

TEST(mixed_front_back)
{
    IntList l = {0};
    IntList_push_back(&l, 2);
    IntList_push_front(&l, 1);
    IntList_push_back(&l, 3);
    IntList_push_front(&l, 0); // 0,1,2,3
    CHECK(l.count == 4);
    for (size_t i = 0; i < l.count; i++) {
        CHECK(IntList_at(&l, i)->value == (int)i);
    }
    CHECK(list_is_consistent(&l));
    IntList_free(&l);
}

TEST(pop_front_and_back)
{
    IntList l = {0};
    for (int i = 0; i < 3; i++) {
        IntList_push_back(&l, i); // 0,1,2
    }
    int v = -1;
    CHECK(IntList_pop_front(&l, &v) && v == 0);
    CHECK(IntList_pop_back(&l, &v) && v == 2);
    CHECK(l.count == 1);
    CHECK(l.head == l.tail);
    CHECK(l.head->value == 1);

    CHECK(IntList_pop_front(&l, &v) && v == 1);
    CHECK(l.count == 0 && l.head == NULL && l.tail == NULL);
    CHECK(!IntList_pop_back(&l, &v));
    IntList_free(&l);
}

TEST(insert_after_and_before)
{
    IntList l = {0};
    IntList_node* a = IntList_push_back(&l, 10);
    IntList_node* c = IntList_push_back(&l, 30);

    IntList_node* b = IntList_insert_after(&l, a, 20);  // 10,20,30
    IntList_node* d = IntList_insert_after(&l, c, 40);   // ...,30,40  (c is tail)
    IntList_node* z = IntList_insert_before(&l, a, 0);   // 0,10,...   (a is head)
    IntList_node* mid = IntList_insert_before(&l, c, 25); // ...,20,25,30,...

    CHECK(l.head == z && l.tail == d);
    CHECK(b->prev == a && b->next == mid);
    CHECK(mid->value == 25);
    CHECK(l.count == 6);

    int want[] = { 0, 10, 20, 25, 30, 40 };
    size_t i = 0;
    list_foreach(IntList, it, &l) {
        CHECK(it->value == want[i++]);
    }
    CHECK(list_is_consistent(&l));
    IntList_free(&l);
}

TEST(erase_head_tail_middle_and_only)
{
    IntList l = {0};
    IntList_node* nodes[5];
    for (int i = 0; i < 5; i++) {
        nodes[i] = IntList_push_back(&l, i); // 0,1,2,3,4
    }

    IntList_erase(&l, nodes[2]);          // 0,1,3,4
    CHECK(l.count == 4);
    CHECK(IntList_at(&l, 2)->value == 3);
    CHECK(list_is_consistent(&l));

    IntList_erase(&l, l.head);            // 1,3,4
    CHECK(l.head->value == 1 && l.head->prev == NULL);

    IntList_erase(&l, l.tail);            // 1,3
    CHECK(l.tail->value == 3 && l.tail->next == NULL);
    CHECK(list_is_consistent(&l));

    IntList_erase(&l, l.head);            // 3
    IntList_erase(&l, l.head);            // (empty)
    CHECK(l.count == 0 && l.head == NULL && l.tail == NULL);
    IntList_free(&l);
}

TEST(at_walks_from_nearer_end)
{
    IntList l = {0};
    for (int i = 0; i < 10; i++) {
        IntList_push_back(&l, i * i);
    }
    CHECK(IntList_at(&l, 0)->value == 0);
    CHECK(IntList_at(&l, 5)->value == 25);
    CHECK(IntList_at(&l, 9)->value == 81);  // last -> tail-side walk
    CHECK(IntList_at(&l, 10) == NULL);
    CHECK(IntList_at(&l, 999) == NULL);
    IntList_free(&l);
}

TEST(remove_at_bounds)
{
    IntList l = {0};
    for (int i = 0; i < 5; i++) {
        IntList_push_back(&l, i); // 0,1,2,3,4
    }
    CHECK(IntList_remove_at(&l, 0));      // 1,2,3,4
    CHECK(IntList_remove_at(&l, 3));      // 1,2,3
    CHECK(!IntList_remove_at(&l, 3));     // out of range
    CHECK(l.count == 3);
    CHECK(l.head->value == 1 && l.tail->value == 3);
    CHECK(list_is_consistent(&l));
    IntList_free(&l);
}

TEST(find_if_with_context)
{
    IntList l = {0};
    for (int i = 0; i < 6; i++) {
        IntList_push_back(&l, i * 10);
    }
    int key = 30;
    IntList_node* hit = IntList_find_if(&l, equals_ctx, &key);
    CHECK(hit != NULL && hit->value == 30);

    key = 31;
    CHECK(IntList_find_if(&l, equals_ctx, &key) == NULL);
    IntList_free(&l);
}

TEST(clear_is_reusable_free_is_null_safe)
{
    IntList l = {0};
    for (int i = 0; i < 8; i++) {
        IntList_push_back(&l, i);
    }
    IntList_clear(&l);
    CHECK(l.count == 0 && l.head == NULL && l.tail == NULL);

    IntList_push_back(&l, 99);
    CHECK(l.count == 1 && l.head->value == 99 && l.head == l.tail);
    IntList_free(&l);

    IntList_free(&l);   // second free is safe
    IntList_free(NULL); // NULL is safe
}

TEST(list_of_structs)
{
    ItemList l = {0};
    ItemList_push_back(&l, (Item){ .id = 1, .weight = 5 });
    ItemList_push_back(&l, (Item){ .id = 2, .weight = 7 });
    ItemList_push_front(&l, (Item){ .id = 0, .weight = 3 });

    int total = 0;
    list_foreach(ItemList, it, &l) {
        total += it->value.weight;
    }
    CHECK(total == 15);
    CHECK(l.head->value.id == 0);
    CHECK(l.tail->value.id == 2);

    ItemList_free(&l);
}

int main(void)
{
    RUN(zero_initialized_is_a_valid_empty_list);
    RUN(push_back_keeps_order_and_endpoints);
    RUN(push_front_reverses_order);
    RUN(mixed_front_back);
    RUN(pop_front_and_back);
    RUN(insert_after_and_before);
    RUN(erase_head_tail_middle_and_only);
    RUN(at_walks_from_nearer_end);
    RUN(remove_at_bounds);
    RUN(find_if_with_context);
    RUN(clear_is_reusable_free_is_null_safe);
    RUN(list_of_structs);
    return test_summary();
}
