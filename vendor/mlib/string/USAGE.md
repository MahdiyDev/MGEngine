# string

`string_builder` / `string_view` helpers, header-only, plain ISO C11.

`string_builder` is `DEFINE_VEC(char, string_builder)` (from [`../vec`](../vec))
plus the `sb_*` / `sv_*` API. Add `../vec` to the include path so `string.h` can
find `vec.h`. Put `#define STRING_IMPLEMENTATION` in exactly one translation unit
before the include; other units just include the header.

```c
#define STRING_IMPLEMENTATION
#include "string.h"

string_builder sb = sb_init("Hello");   // sb_init(NULL) starts empty
sb_add_c(&sb, ' ');
sb_add_cstr(&sb, "World");
sb_add_f(&sb, " (%d)", 42);
printf("%.*s\n", sv_fmt(sb_to_sv(&sb)));
sb_free(&sb);

string_view v = sv_trim(sv_from_cstr("  a,b,c  "));
string_view first = sv_split_c(&v, ',');     // "a"; v becomes "b,c"
string_view field = sv_split_cstr(&v, "::"); // splits on the whole "::"
```

`string_view` is a non-owning `{ const char* data; size_t count; }`. Note
`sv_from_digit()` returns a view into a small rotating static pool — copy it out
if it must outlive the next few calls, and it is not thread-safe.

See `string.h` for the full `sv_*` / `sb_*` surface.

## Tests & examples

```
make            # build and run test/test_string.c
make examples   # build and run example/string_builder.c and example/string_view.c
make clean
```

On Windows use `mingw32-make`. The test suite uses the repo-wide harness `test.h`
at the repository root (`-I..`); everything here also needs `vec.h` from `../vec`
(`-I../vec`).
