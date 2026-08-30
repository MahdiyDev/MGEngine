#include <string.h>

#define STRING_IMPLEMENTATION
#include "../string.h"
#include "test.h"

TEST(sv_from_cstr_and_equality)
{
    string_view a = sv_from_cstr("hello");
    CHECK(a.count == 5);
    CHECK(sv_equal_cstr(a, "hello"));
    CHECK(!sv_equal_cstr(a, "hell"));
    CHECK(sv_equal(a, sv_from_cstr("hello")));
    CHECK(sv_equal_c(a, 'h'));
}

TEST(sv_prefix_suffix_contains)
{
    string_view s = sv_from_cstr("prefix-middle-suffix");
    CHECK(sv_start_with(s, "prefix"));
    CHECK(!sv_start_with(s, "middle"));
    CHECK(sv_end_with(s, "suffix"));
    CHECK(!sv_end_with(s, "middle"));
    CHECK(sv_in_cstr(s, "middle"));
    CHECK(sv_in_c(s, 'x'));
    CHECK(!sv_in_c(s, 'z'));
}

TEST(sv_split_on_char)
{
    string_view rest = sv_from_cstr("a,b,c");
    string_view tok = sv_split_c(&rest, ',');
    CHECK(sv_equal_cstr(tok, "a"));
    CHECK(sv_equal_cstr(rest, "b,c"));

    tok = sv_split_c(&rest, ',');
    CHECK(sv_equal_cstr(tok, "b"));
    CHECK(sv_equal_cstr(rest, "c"));

    tok = sv_split_c(&rest, ','); // no separator left
    CHECK(sv_equal_cstr(tok, "c"));
    CHECK(rest.count == 0);
}

TEST(sv_split_on_substring)
{
    string_view rest = sv_from_cstr("a::bb::c");
    string_view tok = sv_split_cstr(&rest, "::");
    CHECK(sv_equal_cstr(tok, "a"));
    CHECK(sv_equal_cstr(rest, "bb::c"));

    tok = sv_split_cstr(&rest, "::");
    CHECK(sv_equal_cstr(tok, "bb"));
    CHECK(sv_equal_cstr(rest, "c"));

    tok = sv_split_cstr(&rest, "::"); // separator absent -> take the rest
    CHECK(sv_equal_cstr(tok, "c"));
    CHECK(rest.count == 0);

    // separator that overlaps but does not fully match must not be consumed
    string_view r2 = sv_from_cstr("x:y");
    string_view t2 = sv_split_cstr(&r2, "::");
    CHECK(sv_equal_cstr(t2, "x:y"));
}

TEST(sv_contains_respects_bounds)
{
    string_view s = sv_from_cstr("abcde");
    CHECK(sv_in_cstr(s, "cde"));   // suffix match, no read past end
    CHECK(sv_in_cstr(s, "abcde"));
    CHECK(!sv_in_cstr(s, "abcdef")); // needle longer than haystack
    CHECK(!sv_in_cstr(s, "cdef"));   // matches a prefix then runs off the end
    CHECK(sv_in_cstr(s, ""));        // empty needle is always contained
}

TEST(sv_trim)
{
    string_view s = sv_from_cstr("   \t padded  \n");
    string_view t = sv_trim(s);
    CHECK(sv_equal_cstr(t, "padded"));
    CHECK(sv_equal_cstr(sv_trim_left(sv_from_cstr("  x")), "x"));
    CHECK(sv_equal_cstr(sv_trim_right(sv_from_cstr("x  ")), "x"));
    CHECK(sv_trim(sv_from_cstr("    ")).count == 0); // all whitespace
    CHECK(sv_equal_cstr(sv_trim(sv_from_cstr("none")), "none"));
}

TEST(sv_end_with_cases)
{
    string_view s = sv_from_cstr("archive.tar.gz");
    CHECK(sv_end_with(s, ".gz"));
    CHECK(sv_end_with(s, "archive.tar.gz"));
    CHECK(!sv_end_with(s, "archive.tar.gz.sig")); // longer than the view
    CHECK(!sv_end_with(s, ".zip"));
}

TEST(sv_digits)
{
    CHECK(sv_to_digit(sv_from_cstr("123")) == 123);
    CHECK(sv_to_digit(sv_from_cstr("0")) == 0);
    CHECK(sv_to_digit(sv_from_cstr("42abc")) == 42);
    CHECK(sv_equal_cstr(sv_from_digit(123), "123"));
    CHECK(sv_equal_cstr(sv_from_digit(0), "0"));

    // two live results from the rotating pool must not clobber each other
    string_view a = sv_from_digit(11);
    string_view b = sv_from_digit(22);
    CHECK(sv_equal_cstr(a, "11"));
    CHECK(sv_equal_cstr(b, "22"));
}

TEST(sb_build_and_view)
{
    string_builder sb = sb_init("Hello");
    sb_add_c(&sb, ' ');
    sb_add_cstr(&sb, "World");
    CHECK_STR_EQ(sb.items, sb.count, "Hello World");

    string_view v = sb_to_sv(&sb);
    CHECK(sv_equal_cstr(v, "Hello World"));

    sb_add_first_c(&sb, '>');
    CHECK_STR_EQ(sb.items, sb.count, ">Hello World");

    sb_clear(&sb);
    CHECK(sb.count == 0);
    sb_add_cstr(&sb, "again");
    CHECK_STR_EQ(sb.items, sb.count, "again");

    sb_free(&sb);
}

TEST(sb_formatted_append)
{
    string_builder sb = sb_init(NULL);
    sb_add_f(&sb, "%d-%s", 7, "x");
    sb_add_f(&sb, "/%c", 'Z');
    CHECK_STR_EQ(sb.items, sb.count, "7-x/Z");
    sb_free(&sb);
}

int main(void)
{
    RUN(sv_from_cstr_and_equality);
    RUN(sv_prefix_suffix_contains);
    RUN(sv_split_on_char);
    RUN(sv_split_on_substring);
    RUN(sv_contains_respects_bounds);
    RUN(sv_trim);
    RUN(sv_end_with_cases);
    RUN(sv_digits);
    RUN(sb_build_and_view);
    RUN(sb_formatted_append);
    return test_summary();
}
