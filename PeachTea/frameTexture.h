#ifndef PT_TEXTURE_H
#define PT_TEXTURE_H

#include "glExtensions.h"
#include "vectorMath.h"
#include "Colors.h"

typedef struct {
	int width, height;
	int multisampled;

	GLuint fbo, tex, rbo;
} PT_FRAMETEXTURE;

PT_FRAMETEXTURE PT_FRAMETEXTURE_new(int w, int h, int multisampled);
PT_FRAMETEXTURE PT_FRAMETEXTURE_resize(PT_FRAMETEXTURE tex, int w, int h);
void PT_FRAMETEXTURE_destroy(PT_FRAMETEXTURE tex);

void PT_FRAMETEXTURE_bind(PT_FRAMETEXTURE tex);
void PT_FRAMETEXTURE_unbind();
void PT_FRAMETEXTURE_clear(PT_FRAMETEXTURE tex, PT_COLOR clearColor);

void PT_FRAMETEXTURE_blur(GLuint src, PT_FRAMETEXTURE dest, vec2f dir, int blurRadius, int textureFlipped);
void PT_FRAMETEXTURE_copy_to_framebuffer(PT_FRAMETEXTURE frameTexture, GLuint framebuffer);

void PT_FRAMETEXTURE_bind_to_screensize(PT_FRAMETEXTURE* frameTexture);

#endif