#include "mge.h"
#include "mge_gl.h"
#include "mge_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image Mge_LoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize)
{
	Image image = {0};

	if (
		(strcmp(fileType, ".png") == 0) || (strcmp(fileType, ".PNG") == 0)  ||
		(strcmp(fileType, ".bmp") == 0) || (strcmp(fileType, ".BMP") == 0)  ||
		(strcmp(fileType, ".tga") == 0) || (strcmp(fileType, ".TGA") == 0)  ||
		(strcmp(fileType, ".jpg") == 0) || (strcmp(fileType, ".jpeg") == 0) ||
		(strcmp(fileType, ".JPG") == 0) || (strcmp(fileType, ".JPEG") == 0) ||
		(strcmp(fileType, ".gif") == 0) || (strcmp(fileType, ".GIF") == 0)  ||
		(strcmp(fileType, ".pic") == 0) || (strcmp(fileType, ".PIC") == 0)  ||
		(strcmp(fileType, ".ppm") == 0) || (strcmp(fileType, ".pgm") == 0)  ||
		(strcmp(fileType, ".PPM") == 0) || (strcmp(fileType, ".PGM") == 0)  ||
		(strcmp(fileType, ".psd") == 0) || (strcmp(fileType, ".PSD") == 0)
	)
	{
		if (fileData != NULL)
		{
			int comp = 0;
			image.data = stbi_load_from_memory(fileData, dataSize, &image.width, &image.height, &comp, 0);

			if (image.data != NULL)
			{
				image.mipmaps = 1;

				if (comp == 1) image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
				else if (comp == 2) image.format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
				else if (comp == 3) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
				else if (comp == 4) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
			}
		}
	}

	return image;
}

Image Mge_LoadImage(const char* fileName)
{
	Image image = {0};

	size_t dataSize = 0;
	unsigned char *fileData = Mge_LoadFileData(fileName, &dataSize);

	if (fileData != NULL) image = Mge_LoadImageFromMemory(Mge_GetFileExtension(fileName), fileData, dataSize);

	Mge_UnloadFileData(fileData);

	return image;
}

void Mge_UnloadImage(Image image)
{
	free(image.data);
}

Texture2D Mge_LoadTextureFromImage(Image image)
{
	Texture2D texture = {0};

	if ((image.width != 0) && (image.height != 0))
	{
		texture.id = MgeGL_LoadTexture(image.data, image.width, image.height, image.format, image.mipmaps);
	} else TRACE_LOG(LOG_WARNING, "IMAGE: Data is not valid to load texture");

	texture.width = image.width;
	texture.height = image.height;
	texture.mipmaps = image.mipmaps;
	texture.format = image.format;

	return texture;
}

Texture2D Mge_LoadTexture(const char *fileName)
{
    Texture2D texture = { 0 };

    Image image = Mge_LoadImage(fileName);

    if (image.data != NULL)
    {
        texture = Mge_LoadTextureFromImage(image);
        Mge_UnloadImage(image);
    }

    return texture;
}