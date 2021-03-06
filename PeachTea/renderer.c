#define _CRT_SECURE_NO_WARNINGS

#include "renderer.h"
#include "PeachTea.h"
#include <stdlib.h>
#include "screenSize.h"

vec2i vpSize = { 0 };
int initialized = 0;

char_set cs;

void renderer_resized() {
	glViewport(0, 0, screenSize.x, screenSize.y);
	vpSize = screenSize;
}

void renderer_init() {
	renderer_resized();

	initialized = 1;
}

void render(void(*renderCallback)(void)) {
	if (initialized) {
		glDepthFunc(GL_ALWAYS);
		glDisable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT);

		glViewport(0, 0, screenSize.x, screenSize.y);

		renderCallback();

		HDC hdc = GetDC(PT_GET_MAIN_HWND());
		SwapBuffers(hdc);
		ReleaseDC(PT_GET_MAIN_HWND(), hdc);
	}
}

