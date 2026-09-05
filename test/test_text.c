// Unit tests for text rendering (source/mge_text.c) with NO GL context:
// mge_text.c + mge_gl.c are compiled against test/glstub (a fake <glad/glad.h>).
// Covers the built-in font, glyph metrics, Mge_MeasureText's cursor walk, and
// that Draw_Text emits batch geometry (6 verts/glyph) only for valid input.

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "mge.h"
#include "mge_gl.h"
#include "glstub.h"
#include "test.h"

static bool feq(float a, float b) { return fabsf(a - b) < 1e-3f; }
#define CHECK_F(a, b) CHECK(feq((a), (b)))

// mge_text.c references these; test_text doesn't exercise the file/TTF paths
unsigned char* Mge_LoadFileData(const char* f, size_t* s) { (void)f; if (s) *s = 0; return NULL; }
void Mge_UnloadFileData(unsigned char* d) { (void)d; }
void Mge_UnloadTexture(Texture2D t) { (void)t; }

// total vertices across every glDrawArrays recorded since the last reset
static int drawn_verts(void)
{
    int n = 0;
    for (int i = 0; i < glstub.drawArraysCount; i++)
        n += glstub.drawArrays[i].count;
    return n;
}

TEST(default_font_is_valid_with_sane_metrics)
{
    Font f = Mge_GetDefaultFont();
    CHECK(Mge_IsFontValid(f));
    CHECK(f.atlas.id != 0);
    CHECK_F(f.size, 8.0f);
    CHECK_F(f.ascent, 7.0f);
    CHECK(f.first == 32);
    CHECK(f.count == 95); // ASCII 32..126
    CHECK(f.lineAdvance > f.size);

    // cached: same handle every call
    Font g = Mge_GetDefaultFont();
    CHECK(g.glyphs == f.glyphs && g.atlas.id == f.atlas.id);
}

TEST(invalid_font_reports_and_no_ops)
{
    Font zero = { 0 };
    CHECK(!Mge_IsFontValid(zero));

    Vector2 m = Mge_MeasureText(zero, "hello", 16.0f);
    CHECK_F(m.x, 0.0f);
    CHECK_F(m.y, 0.0f);

    glstub_reset();
    Draw_Text(zero, "hello", (Vector2){ 0, 0 }, 16.0f, WHITE);
    CHECK(drawn_verts() == 0); // nothing drawn
}

TEST(measure_text_advances_and_scales)
{
    Font f = Mge_GetDefaultFont();

    // built-in font: fixed 8px advance. fontSize 16 -> scale 2 -> 16px/glyph
    CHECK_F(Mge_MeasureText(f, "", 16.0f).x, 0.0f);
    CHECK_F(Mge_MeasureText(f, "A", 16.0f).x, 16.0f);
    CHECK_F(Mge_MeasureText(f, "AAAA", 16.0f).x, 64.0f);
    CHECK_F(Mge_MeasureText(f, "A", 8.0f).x, 8.0f); // 1:1 at the natural size

    // width grows monotonically with length
    float w1 = Mge_MeasureText(f, "M", 20.0f).x;
    float w3 = Mge_MeasureText(f, "MMM", 20.0f).x;
    CHECK(w3 > w1 && w3 - 3.0f * w1 < 0.01f);

    // height = lines * lineAdvance * scale; longest line sets the width
    Vector2 ml = Mge_MeasureText(f, "ab\nabcd\nx", 16.0f);
    CHECK_F(ml.x, 64.0f);                  // "abcd" -> 4 * 16
    CHECK_F(ml.y, 3.0f * f.lineAdvance * 2.0f);
}

TEST(draw_text_emits_six_verts_per_glyph)
{
    Font f = Mge_GetDefaultFont();

    glstub_reset();
    Draw_Text(f, "Hi!", (Vector2){ 10, 10 }, 16.0f, (Color){ 200, 100, 50, 255 });
    CHECK(drawn_verts() == 3 * 6);

    glstub_reset();
    Draw_Text(f, "a\nbc", (Vector2){ 0, 0 }, 16.0f, WHITE); // '\n' isn't a glyph
    CHECK(drawn_verts() == 3 * 6);

    // an out-of-range byte falls back to '?' -- still one glyph
    glstub_reset();
    Draw_Text(f, "\x80", (Vector2){ 0, 0 }, 16.0f, WHITE);
    CHECK(drawn_verts() == 6);

    // empty string draws nothing
    glstub_reset();
    Draw_Text(f, "", (Vector2){ 0, 0 }, 16.0f, WHITE);
    CHECK(drawn_verts() == 0);
}

TEST(unload_default_font_is_a_no_op)
{
    Font f = Mge_GetDefaultFont();
    glstub_reset();
    Mge_UnloadFont(f);             // must NOT free the shared cache
    CHECK(Mge_IsFontValid(Mge_GetDefaultFont()));
    Draw_Text(Mge_GetDefaultFont(), "ok", (Vector2){ 0, 0 }, 16.0f, WHITE);
    CHECK(drawn_verts() == 2 * 6); // still usable
}

int main(void)
{
    MgeGL_Init(800, 600); // the batcher Draw_Text feeds

    RUN(default_font_is_valid_with_sane_metrics);
    RUN(invalid_font_reports_and_no_ops);
    RUN(measure_text_advances_and_scales);
    RUN(draw_text_emits_six_verts_per_glyph);
    RUN(unload_default_font_is_a_no_op);
    return test_summary();
}
