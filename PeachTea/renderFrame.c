#include "renderFrame.h"
#include "vectorMath.h"
#include "PeachTeaShaders.h"
#include "glExtensions.h"

#include <stdlib.h>
#include <string.h>

Instance* PT_RENDERFRAME_new() {
	PT_RENDERFRAME* renderFrame = calloc(1, sizeof(PT_RENDERFRAME));
	renderFrame->visible = 1;

	Instance* instance = PT_GUI_OBJ_new();
	PT_GUI_OBJ* obj = (PT_GUI_OBJ*)instance->subInstance;

	renderFrame->guiObj = obj;
	renderFrame->instance = instance;
	instance->subInstance = (void*)renderFrame;
	instance->instanceType = IT_RENDERFRAME;

	instance->destroySubInstance = PT_RENDERFRAME_destroy;

	return instance;
}

PT_RENDERFRAME* PT_RENDERFRAME_clone(PT_RENDERFRAME* source, Instance* instance) {
	PT_RENDERFRAME* clone = calloc(1, sizeof(PT_RENDERFRAME));

	memcpy(clone, source, sizeof(PT_RENDERFRAME));
	clone->instance = instance;
	clone->guiObj = PT_GUI_OBJ_clone(source->guiObj, instance);

	return clone;
}

void PT_RENDERFRAME_destroy(void* obj) {
	PT_RENDERFRAME* renderFrame = (PT_RENDERFRAME*)obj;

	PT_GUI_OBJ_destroy((void*)renderFrame->guiObj);

	free(renderFrame);
}

PT_canvas PT_RENDERFRAME_update_size(PT_RENDERFRAME* renderFrame, PT_canvas parentCanvas) {
	PT_canvas lastCanvas = renderFrame->guiObj->lastCanvas;
	PT_canvas thisCanvas = PT_GUI_OBJ_update_size(renderFrame->guiObj, parentCanvas);

	vec2i lastCanvasSize = canvas_size(lastCanvas);
	vec2i thisCanvasSize = canvas_size(thisCanvas);

	if (thisCanvasSize.x > 0 && thisCanvasSize.y > 0) {
		if (!vector_equal_2i(lastCanvasSize, thisCanvasSize) && renderFrame->renderTexture.tex) {
			renderFrame->renderTexture = PT_FRAMETEXTURE_resize(renderFrame->renderTexture, thisCanvasSize.x, thisCanvasSize.y);
		}
		else if (!renderFrame->renderTexture.tex) {
			renderFrame->renderTexture = PT_FRAMETEXTURE_new(thisCanvasSize.x, thisCanvasSize.y, 0);
		}
	}

	return thisCanvas;
}
void PT_RENDERFRAME_render(PT_RENDERFRAME* renderFrame, PT_SCREEN_UI* ui) {
	renderFrame->guiObj->visible = renderFrame->visible;
	PT_canvas childCanvas = renderFrame->guiObj->lastCanvas;

	vec2i pos = canvas_pos(childCanvas);
	vec2i size = canvas_size(childCanvas);

	if (renderFrame->visible) {

		if (renderFrame->render) {
			if (renderFrame->renderTexture.tex) {
				PT_FRAMETEXTURE_bind(renderFrame->renderTexture);

				PT_FRAMETEXTURE_clear(renderFrame->renderTexture, renderFrame->clearColor);

				glDepthFunc(GL_LESS);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				// make sure we don't accidentally use the last rendered object's clipping uniforms
				PT_GUI_OBJ* obj = renderFrame->guiObj;
				GLuint ucbLoc = glGetUniformLocation(PTS_guiObj, "useClipBounds");
				GLuint clXLoc = glGetUniformLocation(PTS_guiObj, "clipX");
				GLuint clYLoc = glGetUniformLocation(PTS_guiObj, "clipY");
				glUniform1i(ucbLoc, obj->lastCanvas.clipDescendants);
				glUniform2i(clXLoc, obj->lastCanvas.cleft, obj->lastCanvas.cright);
				glUniform2i(clYLoc, obj->lastCanvas.ctop, obj->lastCanvas.cbottom);

				renderFrame->render(renderFrame); // call user-defined render function
			}
		}

		PT_FRAMETEXTURE_bind(ui->frameTexture);
		PT_GUI_OBJ_render(renderFrame->guiObj, ui);

		if (renderFrame->renderTexture.tex) {
			// copy user-rendered texture to window
			glUseProgram(PTS_tex);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, renderFrame->renderTexture.tex);

			set_quad_positions(pos, vector_add_2i(pos, size));
			set_quad_corners(
				(vec2f){0.0f, 1.0f},
				(vec2f) { 1.0f, 0.0f}
			);

			glDrawArrays(GL_QUADS, 0, 4);
		}
	}
}



