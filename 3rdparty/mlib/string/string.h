#pragma once

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vec.h"

// string_builder is a vec of char: { char* items; size_t count; size_t capacity; }
// plus the generated string_builder_* helpers. The sb_* API below wraps it.
DEFINE_VEC(char, string_builder);

typedef struct {
    const char* data;
    size_t count;
} string_view;

#define sv_fmt(sv) (int)(sv).count, (sv).data

#ifndef arr_count
    #define arr_count(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

string_view sv_from_cstr(const char* cstr);
string_view sv_from_parts(const char* data, size_t count);

string_view sb_to_sv(string_builder* sb);

string_view sv_split_cstr(string_view* sv, const char* separator);
string_view sv_split_c(string_view* sv, char separator);
string_view sv_split_mul_cstr(string_view* sv, const char* separator, int n);
string_view sv_split_mul_c(string_view* sv, char separator, int n);

bool sv_equal(string_view a, string_view b);
bool sv_equal_c(string_view a, const char b);
bool sv_equal_cstr(string_view a, const char* b);

bool sv_start_with(string_view sv, const char* cstr);
bool sv_end_with(string_view sv, const char* cstr);

bool sv_isdigit(const char c);
size_t sv_to_digit(string_view sv);
string_view sv_from_digit(size_t n);

string_view sv_trim_left(string_view sv);
string_view sv_trim_right(string_view sv);
string_view sv_trim(string_view sv);

bool sv_in(string_view sv, const char** arr, int count);
bool sv_in_sv(string_view a, string_view b);
bool sv_in_cstr(string_view a, const char* cstr);
bool sv_in_c(string_view a, const char c);
#define sv_in_carr(sv, arr) sv_in(sv, arr, arr_count(arr))

string_builder sb_init(const char* cstr);
void sb_free(string_builder* sb);

void sb_add(string_builder* sb, string_view sv);

void sb_add_cstr(string_builder* sb, const char* cstr);
void sb_add_first_cstr(string_builder* sb, const char* cstr);
void sb_delete_range_cstr(string_builder* sb, int start, int end);

void sb_add_c(string_builder* sb, const char c);
void sb_add_first_c(string_builder* sb, const char c);
void sb_delete_c(string_builder* sb, int index);

void sb_add_f(string_builder* sb, const char* format, ...);

void sb_clear(string_builder* sb);

// UTILS
bool sb_read_file(string_builder* sb, const char* file_path);
bool sb_read_file_from_fp(string_builder* sb, FILE* fp);

#ifdef STRING_IMPLEMENTATION
#include <stdarg.h>

string_view sv_from_parts(const char* data, size_t count)
{
    string_view sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

string_view sv_from_cstr(const char* cstr)
{
    return sv_from_parts(cstr, strlen(cstr));
}

string_view sv_split_c(string_view* sv, char separator)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != separator) {
        i += 1;
    }

    string_view head = sv_from_parts(sv->data, i);

    size_t advance = (i < sv->count) ? i + 1 : i; // step over the separator if we hit one
    sv->data += advance;
    sv->count -= advance;

    return head;
}

string_view sv_split_cstr(string_view* sv, const char* separator)
{
    size_t sep_len = strlen(separator);

    if (sep_len != 0) {
        for (size_t i = 0; i + sep_len <= sv->count; i++) {
            if (memcmp(sv->data + i, separator, sep_len) == 0) {
                string_view head = sv_from_parts(sv->data, i);
                sv->data += i + sep_len;
                sv->count -= i + sep_len;
                return head;
            }
        }
    }

    string_view rest = *sv; // separator not found (or empty): consume the rest
    sv->data += sv->count;
    sv->count = 0;
    return rest;
}

string_view sv_split_mul_cstr(string_view* sv, const char* separator, int n)
{
    string_view result = { 0 };
    for (int i = 0; i < n; i++) {
        result = sv_split_cstr(sv, separator);
    }
    return result;
}

string_view sv_split_mul_c(string_view* sv, char separator, int n)
{
    string_view result = { 0 };
    for (int i = 0; i < n; i++) {
        result = sv_split_c(sv, separator);
    }
    return result;
}

bool sv_equal(string_view a, string_view b)
{
    if (a.count != b.count) {
        return false;
    }
    return memcmp(a.data, b.data, a.count) == 0;
}

bool sv_equal_c(string_view a, const char b)
{
    return a.count > 0 && a.data[0] == b;
}

bool sv_equal_cstr(string_view a, const char* b)
{
    return sv_equal(a, sv_from_cstr(b));
}

bool sv_start_with(string_view sv, const char* cstr)
{
    size_t cstr_count = strlen(cstr);
    if (sv.count < cstr_count) {
        return false;
    }
    return memcmp(sv.data, cstr, cstr_count) == 0;
}

bool sv_end_with(string_view sv, const char* cstr)
{
    size_t cstr_count = strlen(cstr);
    if (sv.count < cstr_count) {
        return false;
    }
    return memcmp(sv.data + (sv.count - cstr_count), cstr, cstr_count) == 0;
}

bool sv_isdigit(const char c)
{
    return c >= '0' && c <= '9';
}

size_t sv_to_digit(string_view sv)
{
    size_t value = 0;
    for (size_t i = 0; i < sv.count; i++) {
        unsigned char c = (unsigned char)sv.data[i];
        if (!isdigit(c)) {
            break;
        }
        value = value * 10 + (size_t)(c - '0');
    }
    return value;
}

string_view sv_from_digit(size_t n)
{
    // Result points into a small rotating static pool: copy it out if it must
    // survive several more sv_from_digit() calls. Not thread-safe.
    enum { POOL = 4, WIDTH = 20 };
    static char pool[POOL][WIDTH];
    static unsigned next = 0;

    char* buffer = pool[next++ % POOL];
    size_t i = WIDTH;
    do {
        buffer[--i] = (char)('0' + (n % 10));
        n /= 10;
    } while (n > 0 && i > 0);

    return sv_from_parts(&buffer[i], WIDTH - i);
}

string_view sv_trim_left(string_view sv)
{
    size_t i = 0;
    while (i < sv.count && isspace((unsigned char)sv.data[i])) {
        i += 1;
    }
    return sv_from_parts(sv.data + i, sv.count - i);
}

string_view sv_trim_right(string_view sv)
{
    size_t count = sv.count;
    while (count > 0 && isspace((unsigned char)sv.data[count - 1])) {
        count -= 1;
    }
    return sv_from_parts(sv.data, count);
}

string_view sv_trim(string_view sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

bool sv_in(string_view sv, const char** arr, int count)
{
    for (int i = 0; i < count; i++) {
        if (sv_equal_cstr(sv, arr[i])) {
            return true;
        }
    }
    return false;
}

bool sv_in_sv(string_view a, string_view b)
{
    if (b.count == 0) {
        return true;
    }
    if (a.count < b.count) {
        return false;
    }
    for (size_t i = 0; i + b.count <= a.count; i++) {
        if (memcmp(a.data + i, b.data, b.count) == 0) {
            return true;
        }
    }
    return false;
}

bool sv_in_cstr(string_view a, const char* cstr)
{
    return sv_in_sv(a, sv_from_cstr(cstr));
}

bool sv_in_c(string_view a, const char c)
{
    for (size_t i = 0; i < a.count; i++) {
        if (a.data[i] == c) {
            return true;
        }
    }
    return false;
}

string_view sb_to_sv(string_builder* sb)
{
    return sv_from_parts(sb->items, sb->count);
}

string_builder sb_init(const char* cstr)
{
    string_builder sb = { 0 };
    if (cstr != NULL) {
        sb_add_cstr(&sb, cstr);
    }
    return sb;
}

void sb_free(string_builder* sb)
{
    string_builder_free(sb);
}

void sb_add(string_builder* sb, string_view sv)
{
    string_builder_append_many(sb, sv.data, sv.count);
}

void sb_add_cstr(string_builder* sb, const char* cstr)
{
    string_builder_append_many(sb, cstr, strlen(cstr));
}

void sb_add_first_cstr(string_builder* sb, const char* cstr)
{
    string_builder_prepend_many(sb, cstr, strlen(cstr));
}

void sb_delete_range_cstr(string_builder* sb, int start, int end)
{
    string_builder_delete_range(sb, (size_t)start, (size_t)end);
}

void sb_clear(string_builder* sb)
{
    string_builder_clear(sb);
}

void sb_add_c(string_builder* sb, const char c)
{
    string_builder_append(sb, c);
}

void sb_add_first_c(string_builder* sb, const char c)
{
    string_builder_prepend(sb, c);
}

void sb_delete_c(string_builder* sb, int index)
{
    string_builder_delete(sb, (size_t)index);
}

void sb_add_f(string_builder* sb, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);
    VEC_ASSERT(n >= 0 && "vsnprintf failed");

    size_t start = sb->count;
    string_builder_reserve(sb, sb->count + (size_t)n + 1);

    va_start(args, format);
    vsnprintf(sb->items + start, (size_t)n + 1, format, args);
    va_end(args);

    sb->count = start + (size_t)n; // drop the '\0' vsnprintf wrote
}

// UTILS
bool sb_read_file(string_builder* sb, const char* file_path)
{
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        return false;
    }

    bool result = sb_read_file_from_fp(sb, file);
    fclose(file);
    return result;
}

bool sb_read_file_from_fp(string_builder* sb, FILE* fp)
{
    char chunk[4096];
    size_t got;
    while ((got = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        string_builder_append_many(sb, chunk, got);
    }
    return ferror(fp) == 0;
}

#endif // STRING_IMPLEMENTATION
