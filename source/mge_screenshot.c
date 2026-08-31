// Framebuffer readback -> PNG. Its own translation unit so the stb_image_write
// implementation (and the GL read path) stay out of the renderer core and out of
// the stubbed-GL unit tests, which compile mge_gl.c directly.

#include "mge_gl.h"

#include <glad/glad.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

bool MgeGL_SaveScreenshot(const char* fileName, int x, int y, int width, int height)
{
    if (fileName == NULL || width <= 0 || height <= 0)
        return false;

    const size_t stride = (size_t)width * 4;
    unsigned char* px = malloc(stride * (size_t)height);
    if (px == NULL)
        return false;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, px);

    // GL's origin is bottom-left; PNG wants the top row first -- flip in place.
    unsigned char* tmp = malloc(stride);
    if (tmp != NULL) {
        for (int row = 0; row < height / 2; row++) {
            unsigned char* a = px + (size_t)row * stride;
            unsigned char* b = px + (size_t)(height - 1 - row) * stride;
            memcpy(tmp, a, stride);
            memcpy(a, b, stride);
            memcpy(b, tmp, stride);
        }
        free(tmp);
    }

    int ok = stbi_write_png(fileName, width, height, 4, px, (int)stride);
    free(px);
    return ok != 0;
}
