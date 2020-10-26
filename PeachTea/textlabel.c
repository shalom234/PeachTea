#include <stdlib.h>
#include "textLabel.h"
#include "guiObj.h"
#include "guiUtil.h"
#include "glText.h"

#include "fontHandler.h"

Instance* PT_TEXTLABEL_new() {
	PT_TEXTLABEL* textlabel = (PT_TEXTLABEL*)calloc(1, sizeof(PT_TEXTLABEL));
	textlabel->visible = 1;

	Instance* inst = PT_GUI_OBJ_new();
	PT_GUI_OBJ* obj1 = (PT_GUI_OBJ*)inst->subInstance;

	textlabel->instance = inst;
	textlabel->guiObj = (PT_GUI_OBJ*)inst->subInstance;
	inst->subInstance = (void*)textlabel;
	inst->instanceType = IT_TEXTLABEL;

	return inst;
}

void PT_TEXTLABEL_destroy(void* obj) {
	PT_TEXTLABEL* textlabel = (PT_TEXTLABEL*)obj;

	PT_GUI_OBJ_destroy((void*)textlabel->guiObj);

	if (textlabel->text != NULL) {
		free(textlabel->text);
	}

	free(textlabel);
}

void getStrLines(const char* str, char*** linesOut, int* numLinesOut) {
	int len = strlen(str);
	
	int linesBufferSize = 1;
	int numLines = 0;
	char** lines = calloc(linesBufferSize, sizeof(char*));

	int bufferSize = 10;
	int numUsedBufferChars = 0;
	char* lineBuffer = calloc(bufferSize, sizeof(char));

	int lastLineIndex = -1;
	for (int i = 0; i < len; i++) {
		char c = *(str + i);

		if (numUsedBufferChars + 2 > bufferSize) {
			bufferSize *= 2; 
			lineBuffer = realloc(lineBuffer, bufferSize * sizeof(char));
		}

		*(lineBuffer + numUsedBufferChars) = c;
		numUsedBufferChars++;

		if (c == '\n' || i == len - 1) {
			if (numLines + 1 > linesBufferSize) {
				linesBufferSize *= 2;
				lines = realloc(lines, linesBufferSize * sizeof(char*));
			}

			lineBuffer = realloc(lineBuffer, (numUsedBufferChars + 1) * sizeof(char));
			*(lineBuffer + numUsedBufferChars) = 0;
			*(lines + numLines) = lineBuffer;
			numLines++;

			if (i != len - 1) {
				bufferSize = 10;
				numUsedBufferChars = 0;
				lineBuffer = calloc(bufferSize, sizeof(char));
			}
		}
	}

	lines = realloc(lines, numLines * sizeof(char*));

	*linesOut = lines;
	*numLinesOut = numLines;
}

void free_lines(char** lines, int numlines) {
	for (int i = 0; i < numlines; i++) {
		char* line = *(lines + i);
		free(line);
	}

	free(lines);
}

PT_GUI_DIMS PT_TEXTLABEL_render(PT_TEXTLABEL* textlabel, PT_GUI_DIMS parentDims) {
	textlabel->guiObj->visible = textlabel->visible;
	PT_GUI_DIMS childDims = PT_GUI_OBJ_render(textlabel->guiObj, parentDims);

	if (textlabel->visible) {
		char_set* cs = get_char_set(textlabel->font, textlabel->textSize);

		char** lines;
		int numLines;

		getStrLines(textlabel->text, &lines, &numLines);

		int linePadding = 3;
		int totalTextHeight = numLines * (textlabel->textSize + linePadding) - linePadding;
		int baselineY = childDims.absolutePos.y;
		switch (textlabel->verticalAlignment) {
		case PT_V_ALIGNMENT_CENTER:
			;
			int remainingHeight = childDims.absoluteSize.y - totalTextHeight;
			baselineY = childDims.absolutePos.y + remainingHeight / 2;
			break;
		case PT_V_ALIGNMENT_BOTTOM:
			baselineY = childDims.absolutePos.y + childDims.absoluteSize.y + -totalTextHeight;
			break;
		}

		for (int i = 0; i < numLines; i++) {
			char* line = *(lines + i);
			int len = strlen(line);

			int textWidth = get_text_width(cs, line);

			int baselineX = childDims.absolutePos.x;
			switch (textlabel->horizontalAlignment) {
			case PT_H_ALIGNMENT_CENTER:
				;
				int remainingWidth = childDims.absoluteSize.x - textWidth;
				baselineX = childDims.absolutePos.x + (remainingWidth / 2);
				break;
			case PT_H_ALIGNMENT_RIGHT:
				baselineX = childDims.absolutePos.x + childDims.absoluteSize.x + -textWidth;
				break;
			}


			render_text(cs, line, baselineX, baselineY + (i + 1) * (textlabel->textSize + linePadding) - linePadding);
		}

		free_lines(lines, numLines);
	}


	return childDims;
}