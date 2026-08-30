#include <stdio.h>

#include "../list.h"

DEFINE_LIST(int, IntList);

static void print_list(const char* label, const IntList* l)
{
    printf("%-14s", label);
    list_foreach(IntList, it, l) {
        printf("%d%s", it->value, it->next ? " <-> " : "");
    }
    printf("   (count=%zu)\n", l->count);
}

static bool is_odd(const int* v, void* ctx)
{
    (void)ctx;
    return (*v % 2) != 0;
}

int main(void)
{
    IntList l = {0}; // {0} is a ready-to-use empty list

    for (int i = 1; i <= 4; i++) {
        IntList_push_back(&l, i);  // 1 2 3 4  -- O(1)
    }
    IntList_push_front(&l, 0);     // 0 1 2 3 4
    print_list("built:", &l);

    // Inserts return the new node, so you can keep editing around it.
    IntList_node* two = IntList_at(&l, 2);
    IntList_insert_after(&l, two, 99);
    IntList_insert_before(&l, two, 42);
    print_list("+ around 2:", &l);      // 0 1 42 2 99 3 4

    IntList_erase(&l, two);             // O(1), no index walk
    print_list("- the 2:", &l);         // 0 1 42 99 3 4

    IntList_node* odd = IntList_find_if(&l, is_odd, NULL);
    printf("first odd:     %d\n", odd->value);

    int front = 0, back = 0;
    IntList_pop_front(&l, &front);
    IntList_pop_back(&l, &back);
    printf("popped:        %d and %d\n", front, back);
    print_list("remaining:", &l);       // 1 42 99 3

    // reverse iteration comes for free with a doubly-linked list
    printf("reversed:     ");
    list_foreach_rev(IntList, it, &l) {
        printf("%d ", it->value);
    }
    printf("\n");

    IntList_free(&l);                   // frees every node
    return 0;
}
