#include <stdio.h>

#define STRING_IMPLEMENTATION
#include "../string.h"

int main(void)
{
    // string_view is a non-owning { const char* data; size_t count; } window.
    const char* config = "  host = localhost \n port = 5432 \n mode = fast ";

    string_view rest = sv_from_cstr(config);
    while (rest.count > 0) {
        string_view line = sv_split_c(&rest, '\n'); // consumes one line + its '\n'

        string_view key = sv_trim(sv_split_c(&line, '='));
        string_view value = sv_trim(line);

        printf("key=[%.*s] value=[%.*s]\n", sv_fmt(key), sv_fmt(value));
    }

    string_view path = sv_from_cstr("src/lib/string.h");
    printf("\nis header:   %s\n", sv_end_with(path, ".h") ? "yes" : "no");
    printf("under src/:  %s\n", sv_start_with(path, "src/") ? "yes" : "no");
    printf("has \"lib\":   %s\n", sv_in_cstr(path, "lib") ? "yes" : "no");

    string_view first = sv_split_cstr(&path, "/"); // split on a substring
    printf("first segment: %.*s\n", sv_fmt(first));

    printf("\nparsed number: %zu\n", sv_to_digit(sv_from_cstr("42 items")));
    printf("rendered number: %.*s\n", sv_fmt(sv_from_digit(2026)));

    return 0;
}
