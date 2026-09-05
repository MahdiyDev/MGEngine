// Text rendering -- LearnOpenGL In-Practice/Text-Rendering, with stb_truetype in
// place of FreeType.
//
// Mge_LoadFont rasterises ASCII 32..126 into one RG8 coverage atlas (R = 255,
// G = glyph coverage) via stb_truetype's rect-packer, keeping a stbtt_packedchar
// per glyph (atlas rect + placement offsets + advance). The atlas is swizzled
// {R,R,R,G} so a sample is (1,1,1,coverage): the engine's default batch shader
// (`texture(sampleTex, uv) * vertexColor`) then yields (tint.rgb, tint.a*cov)
// with no dedicated text shader -- text is just alpha-blended batch geometry.
//
// Mge_GetDefaultFont needs no file: it builds the same structures from a small
// built-in 8x8 bitmap font (public-domain font8x8_basic).

#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#include <glad/glad.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#define FIRST_CP 32
#define CP_COUNT 95 // 32..126

// --- built-in 8x8 bitmap font (public domain: dhepper/font8x8, printable ASCII).
// Each row byte is a scanline; bit N (LSB first) is the pixel in column N.
static const unsigned char FONT8X8[CP_COUNT][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ' '
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 }, // '!'
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '"'
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00 }, // '#'
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00 }, // '$'
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00 }, // '%'
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00 }, // '&'
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '\''
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00 }, // '('
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00 }, // ')'
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 }, // '*'
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00 }, // '+'
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // ','
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00 }, // '-'
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // '.'
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00 }, // '/'
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00 }, // '0'
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00 }, // '1'
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00 }, // '2'
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00 }, // '3'
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00 }, // '4'
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00 }, // '5'
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00 }, // '6'
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00 }, // '7'
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00 }, // '8'
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00 }, // '9'
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // ':'
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // ';'
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00 }, // '<'
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00 }, // '='
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00 }, // '>'
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00 }, // '?'
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00 }, // '@'
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00 }, // 'A'
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00 }, // 'B'
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00 }, // 'C'
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00 }, // 'D'
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00 }, // 'E'
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00 }, // 'F'
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00 }, // 'G'
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00 }, // 'H'
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // 'I'
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00 }, // 'J'
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00 }, // 'K'
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 }, // 'L'
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00 }, // 'M'
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00 }, // 'N'
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00 }, // 'O'
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00 }, // 'P'
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00 }, // 'Q'
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00 }, // 'R'
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00 }, // 'S'
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // 'T'
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00 }, // 'U'
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // 'V'
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00 }, // 'W'
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00 }, // 'X'
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00 }, // 'Y'
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00 }, // 'Z'
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00 }, // '['
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00 }, // '\\'
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00 }, // ']'
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00 }, // '^'
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF }, // '_'
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '`'
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00 }, // 'a'
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00 }, // 'b'
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00 }, // 'c'
    { 0x38, 0x30, 0x30, 0x3E, 0x33, 0x33, 0x6E, 0x00 }, // 'd'
    { 0x00, 0x00, 0x1E, 0x33, 0x3F, 0x03, 0x1E, 0x00 }, // 'e'
    { 0x1C, 0x36, 0x06, 0x0F, 0x06, 0x06, 0x0F, 0x00 }, // 'f'
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // 'g'
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00 }, // 'h'
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // 'i'
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E }, // 'j'
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00 }, // 'k'
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // 'l'
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00 }, // 'm'
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00 }, // 'n'
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00 }, // 'o'
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F }, // 'p'
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78 }, // 'q'
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00 }, // 'r'
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00 }, // 's'
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00 }, // 't'
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00 }, // 'u'
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // 'v'
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00 }, // 'w'
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00 }, // 'x'
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // 'y'
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00 }, // 'z'
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00 }, // '{'
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, // '|'
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00 }, // '}'
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '~'
};

// --- atlas upload -------------------------------------------------------------

// `cov` is a w*h single-channel coverage buffer. Uploads it as RG8 [255, cov],
// swizzled {R,R,R,G}, clamped, no mipmaps, with `filter` (GL_LINEAR / GL_NEAREST).
static Texture2D upload_atlas(const unsigned char* cov, int w, int h, GLint filter)
{
    unsigned char* rg = (unsigned char*)malloc((size_t)w * h * 2);
    if (rg == NULL)
        return (Texture2D){ 0 };
    for (int i = 0; i < w * h; i++) {
        rg[2 * i + 0] = 255;
        rg[2 * i + 1] = cov[i];
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, w, h, 0, GL_RG, GL_UNSIGNED_BYTE, rg);
    GLint swz[4] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(rg);

    return (Texture2D){
        .id = id, .width = w, .height = h, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,
    };
}

// --- TrueType path -----------------------------------------------------------

Font Mge_LoadFontFromMemory(const unsigned char* ttfData, int ttfSize, int pixelHeight)
{
    Font font = { 0 };
    if (ttfData == NULL || ttfSize <= 0 || pixelHeight <= 0)
        return font;

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttfData, stbtt_GetFontOffsetForIndex(ttfData, 0))) {
        TRACE_LOG(LOG_WARNING, "FONT: stb_truetype could not parse the font data");
        return font;
    }

    stbtt_packedchar* glyphs = (stbtt_packedchar*)calloc(CP_COUNT, sizeof(stbtt_packedchar));
    if (glyphs == NULL)
        return font;

    // grow the atlas until the range packs
    int dim = 512;
    unsigned char* cov = NULL;
    int packed = 0;
    for (; dim <= 2048; dim *= 2) {
        free(cov);
        cov = (unsigned char*)calloc((size_t)dim * dim, 1);
        if (cov == NULL) {
            free(glyphs);
            return font;
        }
        stbtt_pack_context pc;
        if (!stbtt_PackBegin(&pc, cov, dim, dim, 0, 1, NULL))
            continue;
        stbtt_PackSetOversampling(&pc, 2, 2); // crisper small text
        packed = stbtt_PackFontRange(&pc, ttfData, 0, (float)pixelHeight,
            FIRST_CP, CP_COUNT, glyphs);
        stbtt_PackEnd(&pc);
        if (packed)
            break;
    }
    if (!packed) {
        TRACE_LOG(LOG_WARNING, "FONT: glyph range did not fit a 2048x2048 atlas");
        free(cov);
        free(glyphs);
        return font;
    }

    font.atlas = upload_atlas(cov, dim, dim, GL_LINEAR);
    free(cov);
    if (font.atlas.id == 0) {
        free(glyphs);
        return font;
    }

    int a = 0, d = 0, gap = 0;
    stbtt_GetFontVMetrics(&info, &a, &d, &gap);
    float scale = stbtt_ScaleForPixelHeight(&info, (float)pixelHeight);
    font.glyphs = glyphs;
    font.size = (float)pixelHeight;
    font.ascent = (float)a * scale;
    font.lineAdvance = (float)(a - d + gap) * scale;
    font.first = FIRST_CP;
    font.count = CP_COUNT;
    return font;
}

Font Mge_LoadFont(const char* fileName, int pixelHeight)
{
    size_t size = 0;
    unsigned char* data = Mge_LoadFileData(fileName, &size);
    if (data == NULL) {
        TRACE_LOG(LOG_WARNING, "FONT: could not read %s", fileName ? fileName : "(null)");
        return (Font){ 0 };
    }
    Font font = Mge_LoadFontFromMemory(data, (int)size, pixelHeight);
    Mge_UnloadFileData(data);
    if (font.glyphs != NULL)
        TRACE_LOG(LOG_INFO, "FONT: %s baked at %dpx into a %dx%d atlas", fileName,
            pixelHeight, font.atlas.width, font.atlas.height);
    return font;
}

// --- built-in bitmap font --------------------------------------------------

static void* s_defaultGlyphs = NULL; // identity check for Mge_UnloadFont

Font Mge_GetDefaultFont(void)
{
    static Font s_font = { 0 };
    if (s_font.glyphs != NULL)
        return s_font;

    enum { CW = 8, CH = 8, COLS = 16, ROWS = 6, W = COLS * CW, H = ROWS * CH };
    unsigned char* cov = (unsigned char*)calloc(W * H, 1);
    stbtt_packedchar* glyphs = (stbtt_packedchar*)calloc(CP_COUNT, sizeof(stbtt_packedchar));
    if (cov == NULL || glyphs == NULL) {
        free(cov);
        free(glyphs);
        return (Font){ 0 };
    }

    for (int g = 0; g < CP_COUNT; g++) {
        int cellX = (g % COLS) * CW, cellY = (g / COLS) * CH;
        for (int row = 0; row < CH; row++)
            for (int col = 0; col < CW; col++)
                if ((FONT8X8[g][row] >> col) & 1)
                    cov[(cellY + row) * W + (cellX + col)] = 255;

        stbtt_packedchar* p = &glyphs[g];
        p->x0 = (unsigned short)cellX;
        p->y0 = (unsigned short)cellY;
        p->x1 = (unsigned short)(cellX + CW);
        p->y1 = (unsigned short)(cellY + CH);
        p->xoff = 0.0f;
        p->yoff = -7.0f; // ascent 7 -> cell spans [baseline-7, baseline+1]
        p->xoff2 = (float)CW;
        p->yoff2 = 1.0f;
        p->xadvance = (float)CW;
    }

    s_font.atlas = upload_atlas(cov, W, H, GL_NEAREST); // scale as crisp pixels
    free(cov);
    if (s_font.atlas.id == 0) {
        free(glyphs);
        return (Font){ 0 };
    }
    s_font.glyphs = glyphs;
    s_font.size = 8.0f;
    s_font.ascent = 7.0f;
    s_font.lineAdvance = 9.0f;
    s_font.first = FIRST_CP;
    s_font.count = CP_COUNT;
    s_defaultGlyphs = glyphs;
    return s_font;
}

// --- lifetime --------------------------------------------------------------

bool Mge_IsFontValid(Font font)
{
    return font.glyphs != NULL && font.atlas.id != 0 && font.count > 0;
}

void Mge_UnloadFont(Font font)
{
    // the default font is cached and shared -- leave it be
    if (font.glyphs == NULL || font.glyphs == s_defaultGlyphs)
        return;
    Mge_UnloadTexture(font.atlas);
    free(font.glyphs);
}

// --- drawing / measuring -------------------------------------------------

// Walk `text` glyph by glyph. `emit` (may be NULL) draws one quad. Returns the
// full extent {max line width, total height} in scaled pixels.
static Vector2 walk_text(Font font, const char* text, float fontSize,
    void (*emit)(const stbtt_packedchar* g, float x, float baseline, float s, float aw, float ah))
{
    if (!Mge_IsFontValid(font) || text == NULL || font.size <= 0.0f)
        return (Vector2){ 0 };

    const stbtt_packedchar* glyphs = (const stbtt_packedchar*)font.glyphs;
    float s = fontSize / font.size;
    float aw = (float)font.atlas.width, ah = (float)font.atlas.height;
    float x = 0.0f, maxW = 0.0f;
    int lines = 1;

    for (const unsigned char* c = (const unsigned char*)text; *c; c++) {
        if (*c == '\n') {
            if (x > maxW) maxW = x;
            x = 0.0f;
            lines++;
            continue;
        }
        int cp = *c;
        if (cp < font.first || cp >= font.first + font.count)
            cp = '?';
        const stbtt_packedchar* g = &glyphs[cp - font.first];
        if (emit != NULL)
            emit(g, x, (float)(lines - 1) * font.lineAdvance * s + font.ascent * s, s, aw, ah);
        x += g->xadvance * s;
    }
    if (x > maxW) maxW = x;
    return (Vector2){ maxW, (float)lines * font.lineAdvance * s };
}

static Vector2 s_drawOrigin;

static void emit_glyph(const stbtt_packedchar* g, float x, float baseline, float s, float aw, float ah)
{
    float x0 = s_drawOrigin.x + x + g->xoff * s;
    float y0 = s_drawOrigin.y + baseline + g->yoff * s;
    float x1 = s_drawOrigin.x + x + g->xoff2 * s;
    float y1 = s_drawOrigin.y + baseline + g->yoff2 * s;
    float u0 = g->x0 / aw, v0 = g->y0 / ah;
    float u1 = g->x1 / aw, v1 = g->y1 / ah;

    // two triangles, matching Draw_RectanglePro's winding: TL, BL, TR / TR, BL, BR
    MgeGL_TexCoord2f(u0, v0); MgeGL_Vertex2f(x0, y0);
    MgeGL_TexCoord2f(u0, v1); MgeGL_Vertex2f(x0, y1);
    MgeGL_TexCoord2f(u1, v0); MgeGL_Vertex2f(x1, y0);
    MgeGL_TexCoord2f(u1, v0); MgeGL_Vertex2f(x1, y0);
    MgeGL_TexCoord2f(u0, v1); MgeGL_Vertex2f(x0, y1);
    MgeGL_TexCoord2f(u1, v1); MgeGL_Vertex2f(x1, y1);
}

void Draw_Text(Font font, const char* text, Vector2 pos, float fontSize, Color tint)
{
    if (!Mge_IsFontValid(font) || text == NULL || text[0] == '\0')
        return;

    Mge_SetBlend(true);
    MgeGL_SetTexture(font.atlas.id);
    MgeGL_Begin(MGEGL_TRIANGLES);
    MgeGL_Color4ub(tint.r, tint.g, tint.b, tint.a);

    s_drawOrigin = pos;
    walk_text(font, text, fontSize, emit_glyph);

    MgeGL_TexCoord2f(0.0f, 0.0f); // don't leak the glyph uv into later draws
    MgeGL_End();
    MgeGL_Draw();                 // flush the text batch before the shader/blend changes
    MgeGL_SetTexture(MgeGL_GetWhiteTexture());
    Mge_SetBlend(false);
}

Vector2 Mge_MeasureText(Font font, const char* text, float fontSize)
{
    return walk_text(font, text, fontSize, NULL);
}
