#ifndef GL_TEXT_H
#define GL_TEXT_H

#include "glExtensions.h"
#include "vectorMath.h"
#include "Colors.h"

typedef struct {
	GLuint* textures;
	vec2i* size;
	vec2i* bearing;
	float* advance;
	int num_chars;
} char_set;

void initFT();

char_set create_char_set(const char* font, const int fontSize);
void free_char_set(char_set* cs);

void render_text(vec2i viewportSize, char_set* cs, PT_COLOR textColor, float textTransparency, const char* str, int baseline_x, int baseline_y);
int get_text_width(char_set* cs, const char* str);

#endif