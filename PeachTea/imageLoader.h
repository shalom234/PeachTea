#ifndef PT_IMAGELOADER_H
#define PT_IMAGELOADER_H

#include "lightWindows.h"
#include "vectorMath.h"

HBITMAP load_bitmap(const char* filename);

typedef struct {
	unsigned int width;
	unsigned int height;
	char* lpBits;
} image_bit_data;

typedef struct {
	unsigned int texId;
	vec2i totalImageSize;
	vec2i topLeft, bottomRight;
} PT_IMAGE;

image_bit_data read_png_file(const char* filename);
image_bit_data read_png_file_simple(const char* filename);
void free_image_bit_data(image_bit_data* ibd);

PT_IMAGE PT_IMAGE_from_image_bit_data(image_bit_data* ibd);
PT_IMAGE PT_IMAGE_from_png(const char* filename);

#endif