#include "textCursor.h"
#include "textEditorHandler.h"
#include <stdio.h>

TEXT_CURSOR TEXT_CURSOR_new(TEXT_EDITOR* editor) {
	Instance* cursorFrame = PT_GUI_OBJ_new();
	PT_GUI_OBJ* cursorObj = (PT_GUI_OBJ*)cursorFrame->subInstance;
	cursorObj->size = PT_REL_DIM_new(0, 2, 0, editor->textHeight + editor->linePadding);
	cursorObj->zIndex = 3;
	cursorObj->backgroundColor = PT_COLOR_fromHSV(0, 0, .9f);

	set_instance_parent(cursorFrame, editor->scrollFrame->instance->parent);

	TEXT_CURSOR cursor = { 0 };
	cursor.position = (vec2i){ 0, 0 };
	cursor.textArray = editor->textLines;
	cursor.cursorFrame = cursorFrame;
	cursor.flashInterval = .5f;

	return cursor;
}

void update_targetX(TEXT_CURSOR* cursor) {
	TEXT_EDITOR* textEditor = get_current_text_editor();

	TEXT_LINE currentLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, cursor->position.y);

	int margin = TEXT_EDITOR_get_margin(textEditor) + textEditor->scrollFrame->guiObj->lastCanvas.left;
	int wrapX = TEXT_EDITOR_get_wrapX(textEditor);
	if (wrapX) {
		wrapX -= margin;
	}

	vec2i offset = get_text_offset(textEditor->charSet, currentLine.str, cursor->position.x, wrapX);

	cursor->targetX = margin + offset.x;
}

int is_text_pos_in_range(vec2i p, vec2i start, vec2i end) {
	int afterRightBound = (p.x >= start.x && p.y == start.y) || p.y > start.y;
	int beforeLeftBound = (p.x <= end.x && p.y == end.y) || p.y < end.y;
	
	return afterRightBound && beforeLeftBound;
}

// splits str by '\n' and adds each string and its length to linesOut and lengthsOut
void format_insert_str(const char* str, int strLen, char*** linesOut, int** lengthsOut, int* numLinesOut) {
	// count lines
	int numLines = 1;
	for (int i = 0; i < strLen; i++) {
		char c = *(str + i);
		if (c == '\n') {
			numLines++;
		}
	}

	char** lines = calloc(numLines, sizeof(char*));
	int* lengths = calloc(numLines, sizeof(int));

	char* tempStr = calloc(strLen, sizeof(char));
	int tempLen = 0;

	int lineIndex = 0;
	for (int i = 0; i < strLen; i++) {
		char c = *(str + i);
		if (c == '\r') {
			continue;
		}

		*(tempStr + tempLen) = c;
		tempLen++;

		// clone current line str and add line to lines array
		if (c == '\n') {
			char* line = calloc(tempLen + 1, sizeof(char));

			if (tempLen > 0) {
				memcpy(line, tempStr, tempLen * sizeof(char));
			}

			*(lines + lineIndex) = line;
			*(lengths + lineIndex) = tempLen;

			tempLen = 0;
			lineIndex++;
		}

		if (i == strLen - 1) {// at the last character
			char* line = calloc(tempLen + 1, sizeof(char));

			if (tempLen > 0) {
				memcpy(line, tempStr, tempLen * sizeof(char));
			}

			*(lines + lineIndex) = line;
			*(lengths + lineIndex) = tempLen;
		}
	}

	free(tempStr);

	*numLinesOut = numLines;
	*linesOut = lines;
	*lengthsOut = lengths;
}

void is_position_in_range(vec2i p, vec2i start, vec2i end) {
	//              check y range                if on start line, check start x         if on last line, check end x         
	return p.y >= start.y && p.x <= end.y && (p.y == start.y ? p.x >= start.x : 1) && (p.y == end.y ? p.x <= end.x : 1);
}

void remove_str_at_cursor(TEXT_CURSOR* cursor, vec2i start, vec2i end) {
	float time = PT_TIME_get();
	cursor->lastTypedTime = time;

	TEXT_LINE startLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, start.y);
	TEXT_LINE endLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, end.y);

	// get the strings of the lines before and after the deletion range
	int beforeLen = start.x;
	char* beforeStr = calloc(beforeLen + 1, sizeof(char));
	if (beforeLen > 0) {
		memcpy(beforeStr, startLine.str, beforeLen * sizeof(char));
	}

	int afterLen = endLine.numChars - end.x;
	char* afterStr = calloc(afterLen + 1, sizeof(char));
	if (afterLen > 0) {
		memcpy(afterStr, endLine.str + end.x, afterLen * sizeof(char));
	}

	TEXT_LINE newLine = TEXT_LINE_new(NULL, beforeLen + afterLen);
	for (int i = 0; i < startLine.flags.numElements; i++) {
		TEXT_METADATA_FLAG* pFlag = (TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&startLine.flags, i);
		TEXT_METADATA_FLAG flag = *pFlag;

		if (pFlag->index < start.x) {
			if (flag.index == 0) {
				TEXT_METADATA_FLAG* oldFlag = (TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&newLine.flags, 0);
				free(oldFlag->misc);
				oldFlag->misc = flag.misc;
			}
			else {
				TEXT_METADATA_FLAG_insert(&newLine.flags, flag);
			}
			pFlag->misc = NULL; // before startLine is destroyed, set the flags we reuse misc's to NULL (they won't get freed if null)
		}

	}

	for (int i = 0; i < endLine.flags.numElements; i++) {
		TEXT_METADATA_FLAG* pFlag = (TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&endLine.flags, i);
		TEXT_METADATA_FLAG flag = *pFlag;

		if (pFlag->index >= end.x) {
			flag.index -= end.x;
			flag.index += start.x;

			if (flag.index == 0) {
				TEXT_METADATA_FLAG* oldFlag = (TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&newLine.flags, 0);
				free(oldFlag->misc);
				oldFlag->misc = flag.misc;
			}
			else {
				TEXT_METADATA_FLAG_insert(&newLine.flags, flag);
			}
			
			pFlag->misc = NULL; // before startLine is destroyed, set the flags we reuse misc's to NULL (they won't get freed if null)
		}

	}

	// 'startLine' str will be replaced, everything after 'startLine' through 'endLine' get deleted
	TEXT_LINE_destroy(&startLine);

	if (beforeLen > 0) {
		memcpy(newLine.str + newLine.numChars, beforeStr, beforeLen * sizeof(char));
		newLine.numChars += beforeLen;
	}
	if (afterLen > 0) {
		memcpy(newLine.str + newLine.numChars, afterStr, afterLen * sizeof(char));
		newLine.numChars += afterLen;
	}
	free(beforeStr);
	free(afterStr);

	PT_EXPANDABLE_ARRAY_set(cursor->textArray, start.y, (void*)&newLine);

	// delete lines in between start and end
	for (int i = end.y; i >= start.y + 1; i--) {
		TEXT_LINE* textLine = (TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, i);
		TEXT_LINE_destroy(textLine);
		PT_EXPANDABLE_ARRAY_remove(cursor->textArray, i);
	}

	// move cursor to start of deletion range
	cursor->position = start;
	cursor->selectTo = start;
	update_targetX(cursor);
	move_text_pos_in_view(cursor->position);
}

void get_cursor_selection_bounds(TEXT_CURSOR cursor, vec2i* startOut, vec2i* endOut) {
	vec2i cPos = cursor.position;
	vec2i sPos = cursor.selectTo;
	if (cPos.y < sPos.y || (cPos.y == sPos.y && cPos.x < sPos.x)) { // selecTo is after cursor position
		*startOut = cPos;
		*endOut = sPos;
	}
	else { // selectTo is before cursor position
		*startOut = sPos;
		*endOut = cPos;
	}
}

void delete_cursor_selection(TEXT_CURSOR* cursor) {
	if (!vector_equal_2i(cursor->selectTo, cursor->position)) { // if there is actually something selected...
		vec2i start, end;
		get_cursor_selection_bounds(*cursor, &start, &end);
		cursor->position = start;
		remove_str_at_cursor(cursor, start, end);
		cursor->selectTo = start;
		cursor->cloneLineOffset = 0;
	}
}

void get_cursor_selection(TEXT_CURSOR* cursor, char** selectionOut, int* selectionLengthOut) {
	vec2i start, end;
	char* selection;
	int selectionLength = 0;

	get_cursor_selection_bounds(*cursor, &start, &end);

	int numLines = (end.y - start.y) + 1; // add 1, if start/end are on the same line, there is still 1 line selected

	char** lines = calloc(numLines, sizeof(char*));
	int* lengths = calloc(numLines, sizeof(char*));

	// copy all selected lines
	for (int y = start.y; y <= end.y; y++) {
		TEXT_LINE* line = PT_EXPANDABLE_ARRAY_get(cursor->textArray, y);
		
		int xi = y == start.y ? start.x : 0;
		int xf = y == end.y ? end.x : line->numChars;

		int numChars = xf - xi;

		char* str = calloc(numChars + 1, sizeof(char)); // add 1 for null terminator
		memcpy(str, line->str + xi, numChars * sizeof(char));

		selectionLength += numChars;

		*(lengths + (y - start.y)) = numChars;
		*(lines + (y - start.y)) = str;
	}

	
	// concatenate all lines into one single big line
	int charIndex = 0;
	selectionLength += 5; // add some extra chars for null terminator
	selection = calloc(selectionLength, sizeof(char)); 
	for (int i = 0; i < numLines; i++) {
		char* line = *(lines + i);
		int length = *(lengths + i);

		memcpy(selection + charIndex, line, length * sizeof(char));
		charIndex += length;

		free(line);
	}

	*selectionOut = selection;
	*selectionLengthOut = selectionLength;

	free(lines);
	free(lengths);
}

void insert_str_at_cursor(TEXT_CURSOR* cursor, char* str, int len) {
	float time = PT_TIME_get();
	cursor->lastTypedTime = time;

	delete_cursor_selection(cursor);
	vec2i pos = cursor->position;

	TEXT_LINE* currentLine = (TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, pos.y);
	int lineJustTextLen = currentLine->numChars;
	if (get_last_char(*currentLine)) {
		lineJustTextLen--;
	}

	int beforeLen = pos.x;
	char* beforeStr = calloc(beforeLen + 1, sizeof(char));
	if (beforeLen > 0) {
		memcpy(beforeStr, currentLine->str, beforeLen * sizeof(char)); // copy everything before cursor
	}

	int afterLen = (currentLine->numChars - pos.x);
	char* afterStr = calloc(afterLen + 1, sizeof(char));
	
	if (afterLen > 0) {
		memcpy(afterStr, currentLine->str + beforeLen, afterLen * sizeof(char)); // copy everything after cursor
	}

	PT_EXPANDABLE_ARRAY beforeFlags = PT_EXPANDABLE_ARRAY_new(currentLine->flags.numElements, sizeof(TEXT_METADATA_FLAG));
	PT_EXPANDABLE_ARRAY afterFlags = PT_EXPANDABLE_ARRAY_new(currentLine->flags.numElements, sizeof(TEXT_METADATA_FLAG));
	for (int i = 0; i < currentLine->flags.numElements; i++) {
		TEXT_METADATA_FLAG flag = *(TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&currentLine->flags, i);
		
		if (flag.index < pos.x) {
			PT_EXPANDABLE_ARRAY_add(&beforeFlags, &flag);
		}
		else if (flag.index >= pos.x && flag.index != 0 && pos.x != lineJustTextLen) {
			PT_EXPANDABLE_ARRAY_add(&afterFlags, &flag);
		}
		else {
			if (flag.misc) {
				free(flag.misc);
			}
		}
	}

	TEXT_METADATA_FLAG insertFlag = create_text_metadata_flag(time);
	insertFlag.index = pos.x;

	// break up the insertion string into its component lines
	char** strLines = NULL;
	int* lengths = NULL;
	int numLines = 0;
	format_insert_str(str, len, &strLines, &lengths, &numLines);

	printf("\n%i\n", numLines);

	for (int i = 0; i < numLines; i++) {
		int l = *(lengths + i);
		char* line = *(strLines + i);
		int justTextLen = l;
		if (*(line + l - 1) == '\n') {
			justTextLen = l - 1;
		}

		TEXT_LINE insertLine = TEXT_LINE_new(NULL, beforeLen + l + afterLen);
		insertLine.numChars = 0;

		if (i == 0) { // first line - prepend beforeStr to this text line
			if (beforeLen > 0) {
				memcpy(insertLine.str + insertLine.numChars, beforeStr, beforeLen * sizeof(char));
				insertLine.numChars += beforeLen;
			}
		}

		// add str text between beforeStr and afterStr
		if (l > 0) {
			memcpy(insertLine.str + insertLine.numChars, line, l * sizeof(char));
			insertLine.numChars += l;
		}

		if (i == numLines - 1) { // last line - append afterStr to this text line
			if (afterLen > 0) {
				memcpy(insertLine.str + insertLine.numChars, afterStr, afterLen * sizeof(char));
				insertLine.numChars += afterLen;
			}
		}

		if (i == 0) {
			// prepend old line's flags to the beginning of the first insertion line 
			for (int j = 0; j < beforeFlags.numElements; j++) {
				TEXT_METADATA_FLAG* pFlag = (TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&beforeFlags, j);

				if (pFlag->index == 0) { // first flag is created w/ line by default, overwrite it
					TEXT_METADATA_FLAG insertFirstFlag = *(TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&insertLine.flags, 0);
					memcpy(insertFirstFlag.misc, pFlag->misc, sizeof(float));
					free(pFlag->misc);
					pFlag->misc = insertFirstFlag.misc;
					printf("overwriting firstflag @ 0 %.2f\n", *(float*)insertFirstFlag.misc);
				}
				else {
					printf("inserting beforeflag @ %i %.2f\n", pFlag->index, *(float*)pFlag->misc);
					TEXT_METADATA_FLAG_insert(&insertLine.flags, *pFlag);
				}
			}

			if (insertFlag.index != 0) {
				printf("inserting insert flag @ %i %.2f\n", insertFlag.index, *(float*)insertFlag.misc);
				TEXT_METADATA_FLAG_insert(&insertLine.flags, insertFlag);
			}
		}


		if (i == numLines - 1) { // last line
			// insert any pre-existing interrupted flag
			if (beforeFlags.numElements > 0 && afterLen > 0) {
				TEXT_METADATA_FLAG firstAfterFlag = *(TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&afterFlags, 0);

				if (firstAfterFlag.index != pos.x && pos.x != lineJustTextLen) {
					TEXT_METADATA_FLAG lastFlag = *(TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&beforeFlags, beforeFlags.numElements - 1);
					lastFlag = create_text_metadata_flag(*(float*)lastFlag.misc);

					if (i == 0) {
						lastFlag.index = beforeLen;
					}
					else {
						lastFlag.index = 0;
					}
					lastFlag.index += justTextLen;

					printf("inserting interrupted flag @ %i %.2f\n", lastFlag.index, *(float*)lastFlag.misc);
					TEXT_METADATA_FLAG_insert(&insertLine.flags, lastFlag);
				}
			}
								 
			// append afterFlags to the last line
			for (int j = 0; j < afterFlags.numElements; j++) {
				TEXT_METADATA_FLAG flag = *(TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&afterFlags, j);

				printf("%i - ", flag.index);

				if (i > 0) {
					flag.index = flag.index - beforeLen;
					printf("%i - ", flag.index);
				} 

				flag.index += justTextLen;

				if (flag.index != 0) {
					printf("inserting afterflag @ %i %.2f\n", flag.index, *(float*)flag.misc);
					TEXT_METADATA_FLAG_insert(&insertLine.flags, flag);
				}
			}			
		}
		
		for (int i = 0; i < insertLine.flags.numElements; i++) {
			TEXT_METADATA_FLAG flag = *(TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&insertLine.flags, i);
			printf("%i -> ", flag.index);
		}
		printf("\n");


		if (i == 0) { // if this is the first line, free the existing line and replace array's index with new line
			//don't do: TEXT_LINE_destroy(currentLine);...
			free(currentLine->str); // this might look like a leak-- don't worry, i'm reusing the flag misc's. They get freed in TEXT_EDITOR_render
			
			PT_EXPANDABLE_ARRAY_set(cursor->textArray, pos.y, (void*)&insertLine);	
		} else { // if this isn't the first line, insert this text line into the array
			if (pos.y + i >= cursor->textArray->numElements) {
				PT_EXPANDABLE_ARRAY_add(cursor->textArray, (void*)&insertLine);
			}
			else {
				PT_EXPANDABLE_ARRAY_insert(cursor->textArray, pos.y + i, (void*)&insertLine);
			}
		}


		free(line);
	}


	PT_EXPANDABLE_ARRAY_destroy(&beforeFlags);
	PT_EXPANDABLE_ARRAY_destroy(&afterFlags);

	TEXT_LINE* lastLine = (TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, pos.y + numLines - 1);
	cursor->position = (vec2i){
		lastLine->numChars - afterLen,
		pos.y + numLines - 1
	};
	cursor->selectTo = cursor->position;
	update_targetX(cursor);
	move_text_pos_in_view(cursor->position);

	free(beforeStr);
	free(afterStr);

	free(strLines);
	free(lengths);
}

// z is new targetX;
vec3i calculate_text_position(PT_EXPANDABLE_ARRAY* textArray, vec2i pos, vec2i dir, int targetX) {
	vec3i newPos = (vec3i){ pos.x, pos.y, targetX };

	int thisY = pos.y;
	int lastY = max(thisY - 1, 0);
	int nextY = min(thisY + 1, textArray->numElements - 1);

	TEXT_LINE thisLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(textArray, thisY);
	TEXT_LINE lastLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(textArray, lastY);
	TEXT_LINE nextLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(textArray, nextY);

	if (dir.x == 1) { // move right
		newPos.x++;

		int numChars = thisLine.numChars;
		if (get_last_char(thisLine) == '\n') {
			numChars--;
		}

		if (newPos.x > numChars) {
			if (nextY > thisY) {
				newPos.x = 0;
				newPos.y = nextY;
			}
			else {
				newPos.x = numChars;
			}
		}

		newPos.z = newPos.x;
	}
	else if (dir.x == -1) { // move left
		newPos.x--;

		if (newPos.x < 0) {
			if (lastY < thisY) {
				newPos.x = lastLine.numChars - 1; // move cursor to before the last line's '\n' character
				newPos.y = lastY;
			}
			else {
				newPos.x = 0;
			}
		}

		newPos.z = newPos.x;
	}
	else if (dir.y == 1) { // move up
		if (thisY > lastY) { // if pos isn't on the first line (the cursor has a line to move up to)
			newPos.y = lastY;

			int lastLineNumChars = lastLine.numChars;
			if (get_last_char(lastLine) == '\n') {
				lastLineNumChars--;
			}
			newPos.x = min(targetX, lastLineNumChars);
		}
		else { // pos is on the first line
			newPos.x = 0; // move to the beginning of first line
			newPos.z = newPos.x;
		}
	}
	else if (dir.y == -1) { // move down
		if (thisY < nextY) { // if pos isn't on the last line (the cursor has a line to move down to)
			newPos.y = nextY;

			int nextLineNumChars = nextLine.numChars;
			if (get_last_char(nextLine) == '\n') {
				nextLineNumChars--;
			}
			newPos.x = min(targetX, nextLineNumChars);
		}
		else { // pos is on the last line
			newPos.x = nextLine.numChars; // move to end of last line line
			newPos.z = newPos.x;
		}
	}

	return newPos;
}

/*
u/d/l/r -> move cursor position; bring cursor selection position to cursor position

alt + u/d -> drag selected lines up/down

shift + u/d/l/r -> move cursor selection position

alt + shift + u/d -> move cursor clone line offset
*/
void move_cursor(TEXT_CURSOR* cursor, vec2i dir, int shiftDown, int altDown) {
	float time = PT_TIME_get();
	cursor->lastTypedTime = time;
	
	vec3i newPosData = calculate_text_position(cursor->textArray, cursor->position, dir, cursor->targetX);

	PT_GUI_OBJ* cursorObj = (PT_GUI_OBJ*)cursor->cursorFrame->subInstance;
	PT_canvas cursorCanvas = cursorObj->lastCanvas;
	vec2i midPos = (vec2i){ (cursorCanvas.left + cursorCanvas.right) / 2, (cursorCanvas.top + cursorCanvas.bottom) / 2 };

	vec2i newPos = (vec2i){ newPosData.x, newPosData.y };
	TEXT_EDITOR* currentTextEditor = get_current_text_editor();
	if (currentTextEditor && dir.y != 0) {
		int margin = currentTextEditor->scrollFrame->guiObj->lastCanvas.left + TEXT_EDITOR_get_margin(currentTextEditor);
		int lineThickness = currentTextEditor->textHeight + currentTextEditor->linePadding;
		midPos.y += -dir.y * lineThickness;
		midPos.x = cursor->targetX;
		newPos = TEXT_EDITOR_screenPos_to_cursorPos(midPos);
	}

	if (dir.x != 0) {
		int margin = currentTextEditor->scrollFrame->guiObj->lastCanvas.left + TEXT_EDITOR_get_margin(currentTextEditor);
		int wrapX = TEXT_EDITOR_get_wrapX(currentTextEditor) - margin;
		TEXT_LINE line = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, newPos.y);
		vec2i offset = get_text_offset(currentTextEditor->charSet, line.str, newPos.x, wrapX);
		cursor->targetX = offset.x + margin;
	}

	// there is at least one line clone and at least alt or shift is being held down
	if (abs(cursor->cloneLineOffset) > 0 && (altDown || shiftDown)) {
		int dy = newPos.y - cursor->position.y;
		cursor->cloneLineOffset -= dy;
	}
	else if (altDown && !shiftDown) { // drag selection with cursor

	}
	else if (!(shiftDown || altDown)) {
		cursor->selectTo = newPos;
		cursor->cloneLineOffset = 0;
	}

	cursor->position = newPos;
	move_text_pos_in_view(cursor->position);
}

int is_word_char(char c) {
	int isNumber = c >= 48 && c <= 57;
	int isLowerAlpha = c >= 65 && c <= 90;
	int isUpperAlpha = c >= 97 && c <= 122;

	return isNumber || isLowerAlpha || isUpperAlpha;
}

void TEXT_CURSOR_select_word(TEXT_CURSOR* cursor) {
	vec2i cPos = cursor->position;
	TEXT_LINE line = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, cPos.y);
	
	int leftBound = cPos.x;
	int rightBound = leftBound;

	while (is_word_char(*(line.str + leftBound)) && leftBound > 0 && is_word_char(*(line.str + leftBound - 1))) {
		leftBound--;
	}

	while (is_word_char(*(line.str + rightBound)) && rightBound < line.numChars) {
		rightBound++;
	}

	cursor->selectTo = (vec2i){ leftBound, cPos.y };
	cursor->position = (vec2i){ rightBound, cPos.y };
}