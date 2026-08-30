#include <stdio.h>

#define STRING_IMPLEMENTATION
#include "../string.h"

int main(void)
{
    // sb_init(NULL) starts empty; sb_init("...") seeds it with a C string.
    string_builder sb = sb_init("shopping list:\n");

    const char* items[] = { "eggs", "flour", "butter", "sugar" };
    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        sb_add_f(&sb, "  %zu. %s\n", i + 1, items[i]); // printf-style append
    }

    sb_add_cstr(&sb, "----\n");
    sb_add_first_cstr(&sb, "== ");        // prepend
    sb_add_c(&sb, '\n');

    // A string_builder is not NUL-terminated: print it as a counted slice.
    printf("%.*s", sv_fmt(sb_to_sv(&sb)));
    printf("length: %zu, capacity: %zu\n", sb.count, sb.capacity);

    sb_clear(&sb); // reuse the same buffer
    sb_add_cstr(&sb, "done");
    printf("after clear: %.*s\n", sv_fmt(sb_to_sv(&sb)));

    sb_free(&sb);
    return 0;
}
