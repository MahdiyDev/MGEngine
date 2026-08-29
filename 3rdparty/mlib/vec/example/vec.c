#include <stdio.h>

#include "../vec.h"

// A vec of a primitive type...
DEFINE_VEC(int, IntVec);

// ...and a vec of a struct. Same generated API, still fully type checked.
typedef struct {
    const char* name;
    int score;
} Player;

DEFINE_VEC(Player, PlayerVec);

static void print_ints(const char* label, const IntVec* v)
{
    printf("%-16s [", label);
    for (size_t i = 0; i < v->count; i++) {
        printf("%s%d", i ? ", " : "", v->items[i]);
    }
    printf("]  (count=%zu capacity=%zu)\n", v->count, v->capacity);
}

int main(void)
{
    IntVec nums = {0}; // {0} is a ready-to-use empty vec; no init call needed

    for (int i = 1; i <= 5; i++) {
        IntVec_append(&nums, i * 10);
    }
    print_ints("appended:", &nums);

    int more[] = { 60, 70, 80 };
    IntVec_append_many(&nums, more, 3);
    IntVec_prepend(&nums, 0);
    print_ints("+ many / prepend:", &nums);

    *IntVec_get(&nums, 1) = 11; // get() hands back a mutable pointer
    IntVec_delete(&nums, 0);    // drop the leading 0
    IntVec_delete_range(&nums, 3, 6);
    print_ints("edited:", &nums);

    IntVec_clear(&nums); // keeps the allocation for reuse
    IntVec_append(&nums, 42);
    print_ints("cleared + 1:", &nums);

    IntVec_free(&nums);

    // struct vec
    PlayerVec players = {0};
    PlayerVec_append(&players, (Player){ .name = "Ada", .score = 12 });
    PlayerVec_append(&players, (Player){ .name = "Alan", .score = 9 });
    PlayerVec_append(&players, (Player){ .name = "Grace", .score = 15 });

    int total = 0;
    for (size_t i = 0; i < players.count; i++) {
        printf("%-6s %d\n", players.items[i].name, players.items[i].score);
        total += players.items[i].score;
    }
    printf("total score: %d\n", total);

    PlayerVec_free(&players);
    return 0;
}
