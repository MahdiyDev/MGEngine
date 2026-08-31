#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image Mge_LoadImageFromMemory(const char* fileType, const unsigned char* fileData, int dataSize)
{
    Image image = { 0 };

    if (fileType == NULL || fileData == NULL)
        return image;

    if ((strcmp(fileType, ".png") == 0) || (strcmp(fileType, ".PNG") == 0) ||
        (strcmp(fileType, ".bmp") == 0) || (strcmp(fileType, ".BMP") == 0) ||
        (strcmp(fileType, ".tga") == 0) || (strcmp(fileType, ".TGA") == 0) ||
        (strcmp(fileType, ".jpg") == 0) || (strcmp(fileType, ".jpeg") == 0) ||
        (strcmp(fileType, ".JPG") == 0) || (strcmp(fileType, ".JPEG") == 0) ||
        (strcmp(fileType, ".gif") == 0) || (strcmp(fileType, ".GIF") == 0) ||
        (strcmp(fileType, ".pic") == 0) || (strcmp(fileType, ".PIC") == 0) ||
        (strcmp(fileType, ".ppm") == 0) || (strcmp(fileType, ".pgm") == 0) ||
        (strcmp(fileType, ".PPM") == 0) || (strcmp(fileType, ".PGM") == 0) ||
        (strcmp(fileType, ".psd") == 0) || (strcmp(fileType, ".PSD") == 0)) {
        int comp = 0;
        image.data = stbi_load_from_memory(fileData, dataSize, &image.width, &image.height, &comp, 0);

        if (image.data != NULL) {
            image.mipmaps = 1;

            if (comp == 1)
                image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
            else if (comp == 2)
                image.format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
            else if (comp == 3)
                image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
            else if (comp == 4)
                image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        }
    }

    return image;
}

Image Mge_LoadImage(const char* fileName)
{
    Image image = { 0 };

    size_t dataSize = 0;
    unsigned char* fileData = Mge_LoadFileData(fileName, &dataSize);

    if (fileData != NULL) {
        image = Mge_LoadImageFromMemory(Mge_GetFileExtension(fileName), fileData, (int)dataSize);
        Mge_UnloadFileData(fileData);
    }

    return image;
}

void Mge_UnloadImage(Image image)
{
    free(image.data);
}

Texture2D Mge_LoadTextureFromImageEx(Image image, bool sRGB)
{
    Texture2D texture = { 0 };

    if ((image.width != 0) && (image.height != 0)) {
        texture.id = MgeGL_LoadTexture(image.data, image.width, image.height, image.format, image.mipmaps, sRGB ? 1 : 0);
    } else {
        TRACE_LOG(LOG_WARNING, "IMAGE: Data is not valid to load texture");
    }

    texture.width = image.width;
    texture.height = image.height;
    texture.mipmaps = image.mipmaps;
    texture.format = image.format;

    return texture;
}

Texture2D Mge_LoadTextureFromImage(Image image)
{
    return Mge_LoadTextureFromImageEx(image, false); // linear: caller didn't say the source is sRGB
}

Texture2D Mge_LoadTextureEx(const char* fileName, bool sRGB)
{
    Texture2D texture = { 0 };

    Image image = Mge_LoadImage(fileName);

    if (image.data != NULL) {
        texture = Mge_LoadTextureFromImageEx(image, sRGB);
        Mge_UnloadImage(image);
    }

    return texture;
}

Texture2D Mge_LoadTexture(const char* fileName)
{
    return Mge_LoadTextureEx(fileName, false);
}

void Mge_UnloadTexture(Texture2D texture)
{
    MgeGL_UnloadTexture(texture.id);
}

Texture2D Mge_LoadTextureHDR(const char* fileName)
{
    Texture2D texture = { 0 };

    size_t dataSize = 0;
    unsigned char* fileData = Mge_LoadFileData(fileName, &dataSize);
    if (fileData == NULL)
        return texture;

    int w = 0, h = 0, comp = 0;
    float* px = stbi_loadf_from_memory(fileData, (int)dataSize, &w, &h, &comp, 0);
    Mge_UnloadFileData(fileData);

    if (px != NULL) {
        int channels = (comp == 4) ? 4 : 3;
        texture.id = MgeGL_LoadTextureHDR(px, w, h, channels);
        texture.width = w;
        texture.height = h;
        texture.mipmaps = 1;
        texture.format = (channels == 4) ? PIXELFORMAT_UNCOMPRESSED_R32G32B32A32
                                         : PIXELFORMAT_UNCOMPRESSED_R32G32B32;
        stbi_image_free(px);
    } else {
        TRACE_LOG(LOG_WARNING, "IMAGE: [%s] failed to load HDR: %s", fileName, stbi_failure_reason());
    }

    return texture;
}

void Mge_SetTextureWrap(Texture2D texture, int wrap)
{
    MgeGL_SetTextureWrap(texture.id, wrap, wrap);
}

void Mge_SetTextureWrapEx(Texture2D texture, int wrapU, int wrapV)
{
    MgeGL_SetTextureWrap(texture.id, wrapU, wrapV);
}
