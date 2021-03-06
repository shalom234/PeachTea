#define _CRT_SECURE_NO_WARNINGS

#include "textEditorHandler.h"
#include "PeachTea.h";
#include "textCursor.h"
#include "textEditorRenderer.h"
#include "ui.h"
#include "actionHistory.h"

#include <stdlib.h>
#include <stdio.h>

#define LIST_ELEMENT_THICKNESS 50

PT_EXPANDABLE_ARRAY editors = { 0 };

TEXT_EDITOR* currentTextEditor = NULL;
PT_EXPANDABLE_ARRAY* lines = NULL;
int keyDownBound = 0;

PT_IMAGE xImage;

int charsTyped = 0;

#define REPLACE_STR(str, replacement) if (str) { free(str); } str = replacement;
#define IS_UPPER_CHAR(c) c >= 65 && c <= 90
#define IS_LOWER_CHAR(c) c >= 97 && c <= 122
#define IS_ALPHA_CHAR(c) IS_UPPER_CHAR(c) || IS_LOWER_CHAR(c);
#define IS_NUMBER_CHAR(c) c >= 48 && c <= 57

void on_undo();
void on_redo();

void update_list_element_order() {
	int bottomY = 0;
	for (int i = 0; i < editors.numElements; i++) {
		TEXT_EDITOR* editor = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);
		EDITOR_LIST_ELEMENT listElement = editor->listElement;
		
		int y = 2 + i * (LIST_ELEMENT_THICKNESS + 1);
		listElement.main->position = PT_REL_DIM_new(0, 2, 0, y);
		
		if (i == editors.numElements - 1) { // on the last element in the list, shrinkwrap the size of the container's canvas
			bottomY = y + LIST_ELEMENT_THICKNESS;
			PT_SCROLLFRAME* container = (PT_SCROLLFRAME*)(listElement.main->instance->parent->subInstance);
			container->canvasSize = PT_REL_DIM_new(0, 0, 0, bottomY);
		}
	}
}

void list_element_mouseEnter(void* args) {
	PT_GUI_OBJ* obj = (PT_GUI_OBJ*)args;
	TEXT_EDITOR* editor = NULL;

	for (int i = 0; i < editors.numElements; i++) {
		TEXT_EDITOR* e = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);
		
		if (e->listElement.main == obj) {
			editor = e;
			break;
		}
	}

	if (editor) { // editor shouldn't ever be null, but just in case
		if (editor != currentTextEditor) { // don't do hover effects if this element is already selected
			for (int i = 0; i < editor->listElement.onhoverTweens.numElements; i++) {
				PT_TWEEN* tween = *(PT_TWEEN**)PT_EXPANDABLE_ARRAY_get(&editor->listElement.onhoverTweens, i);
				PT_TWEEN_play(tween);
			}
		}
	}
}

void list_element_mouseLeave(void* args) {
	PT_GUI_OBJ* obj = (PT_GUI_OBJ*)args;
	TEXT_EDITOR* editor = NULL;

	for (int i = 0; i < editors.numElements; i++) {
		TEXT_EDITOR* e = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);

		if (e->listElement.main == obj) {
			editor = e;
			break;
		}
	}

	if (editor) { // editor shouldn't ever be null, but just in case
		if (editor != currentTextEditor) { // don't do hover effects if this element is already selected
			for (int i = 0; i < editor->listElement.deselectTweens.numElements; i++) {
				PT_TWEEN* tween = *(PT_TWEEN**)PT_EXPANDABLE_ARRAY_get(&editor->listElement.deselectTweens, i);
				PT_TWEEN_play(tween);
			}
		}
	}
}

void list_element_activated(void* args) {
	PT_GUI_OBJ* obj = (PT_GUI_OBJ*)args;
	TEXT_EDITOR* editor = NULL;

	for (int i = 0; i < editors.numElements; i++) {
		TEXT_EDITOR* e = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);

		if (e->listElement.main == obj) {
			editor = e;
			break;
		}
	}

	if (editor && editor != currentTextEditor) {
		TEXT_EDITOR_select(editor);
	}
}

void on_remove(void* args) {
	PT_GUI_OBJ* obj = (PT_GUI_OBJ*)args;
	for (int i = 0; i < editors.numElements; i++) {
		TEXT_EDITOR* editor = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);
		EDITOR_LIST_ELEMENT listElement = editor->listElement;

		if (listElement.removeButton->guiObj == obj) {
			editor->markedForDeletion = 1;

			return;
		}
	}
}

EDITOR_LIST_ELEMENT create_editor_list_element(TEXT_EDITOR* editor, PT_SCROLLFRAME* listContainer) {
	if (xImage.totalImageSize.x == 0) {
		xImage = PT_IMAGE_from_png("assets\\images\\x.png");
	}
	
	PT_COLOR SELECTED_COLOR = accentColor;
	PT_COLOR SELECTED_ACTIVE_COLOR = PT_COLOR_lerp(accentColor, colorTheme.borderColor, .3f);
	PT_REL_DIM SELECTED_SIZE = PT_REL_DIM_new(1, -(2 + listContainer->scrollBarThickness * 2), 0, LIST_ELEMENT_THICKNESS);
	PT_COLOR DESELECTED_COLOR = colorTheme.sidebarColor;
	PT_COLOR DESELECTED_ACTIVE_COLOR = PT_COLOR_lerp(colorTheme.sidebarColor, colorTheme.borderColor, .14f);
	PT_REL_DIM DESELECTED_SIZE = PT_REL_DIM_new(1, -(20 + listContainer->scrollBarThickness * 2), 0, LIST_ELEMENT_THICKNESS);
	PT_REL_DIM HOVER_SIZE = PT_REL_DIM_new(1, -(15 + listContainer->scrollBarThickness * 2), 0, LIST_ELEMENT_THICKNESS);

	EDITOR_LIST_ELEMENT listElement = { 0 };

	PT_GUI_OBJ* main = (PT_GUI_OBJ*)(PT_GUI_OBJ_new()->subInstance);
	main->instance->name = create_heap_str("editor list element");
	main->size = SELECTED_SIZE;
	main->reactive = 1;
	
	main->backgroundTransparency = .7f;
	main->activeBackgroundColor = DESELECTED_COLOR;
	main->activeBackgroundRange = (vec2f){ 50, 150 };
	
	main->borderWidth = 1;
	main->borderTransparancy = .7f;
	main->activeBorderColor = colorTheme.borderColor;
	main->activeBorderRange = (vec2f){ 10, 90 };

	//*
	PT_IMAGELABEL* removeButton = (PT_IMAGELABEL*)(PT_IMAGELABEL_new()->subInstance);
	removeButton->image = xImage;
	removeButton->imageTint = colorTheme.borderColor;
	removeButton->imageTintAlpha = 1;
	removeButton->imageScale = .7;
	PT_GUI_OBJ* removeButtonObj = removeButton->guiObj;
	removeButtonObj->size = PT_REL_DIM_new(0, 30, 0, 30);
	removeButtonObj->anchorPosition = (vec2f){ 0, .5f };
	removeButtonObj->position = PT_REL_DIM_new(0, 6, .5f, 0);
	removeButtonObj->backgroundColor = colorTheme.sidebarColor;
	removeButtonObj->borderWidth = 0;
	removeButtonObj->borderColor = colorTheme.borderColor;
	removeButtonObj->backgroundTransparency = .7;
	removeButtonObj->activeBackgroundColor = PT_COLOR_fromHSV(0, 0, 1);
	removeButtonObj->activeBackgroundRange = (vec2f){ 100, 0 };
	removeButtonObj->reactive = 1;

	PT_BINDABLE_EVENT_bind(&removeButtonObj->e_obj_activated, on_remove);

	set_instance_parent(removeButton->instance, main->instance);
	listElement.removeButton = removeButton;
	//*/

	PT_COLOR fringeColor = PT_TWEEN_PT_COLOR(colorTheme.sidebarColor, colorTheme.backgroundColor, sideFrameTransparency, PT_LINEAR, PT_IN);
	fringeColor = PT_TWEEN_PT_COLOR(main->backgroundColor, fringeColor, main->backgroundTransparency, PT_LINEAR, PT_IN);

	PT_TEXTLABEL* header = (PT_TEXTLABEL*)PT_TEXTLABEL_new()->subInstance;
	header->instance->name = create_heap_str("editor header");
	header->textSize = 11;
	header->font = PT_FONT_ARIAL;
	header->horizontalAlignment = PT_H_ALIGNMENT_LEFT;
	header->verticalAlignment = PT_V_ALIGNMENT_CENTER;
	header->textColor = colorTheme.borderColor;
	header->fringeColor = fringeColor;

	PT_GUI_OBJ* headerObj = header->guiObj;
	headerObj->size = PT_REL_DIM_new(1, -45, .65, 0);
	headerObj->anchorPosition = (vec2f){ 0, 0 };
	headerObj->position = PT_REL_DIM_new(0, 45, 0, 0);
	headerObj->backgroundTransparency = 1.0f;
	headerObj->clipDescendants = 1;
	headerObj->processEvents = 0;

	PT_TEXTLABEL* desc = (PT_TEXTLABEL*)PT_TEXTLABEL_new()->subInstance;
	desc->instance->name = create_heap_str("editor desc");
	desc->textSize = 9;
	desc->font = PT_FONT_ARIAL_I;
	desc->horizontalAlignment = PT_H_ALIGNMENT_LEFT;
	desc->verticalAlignment = PT_V_ALIGNMENT_TOP;
	desc->textColor = PT_COLOR_lerp(colorTheme.borderColor, colorTheme.accentColor, .2);
	desc->fringeColor = fringeColor;

	PT_GUI_OBJ* descObj = desc->guiObj;
	descObj->size = PT_REL_DIM_new(1, -headerObj->size.xOffset, 1 - headerObj->size.yFactor, 0);
	descObj->anchorPosition = (vec2f){ 0, 0 };
	descObj->position = PT_REL_DIM_new(0, 50, headerObj->size.yFactor, 0);
	descObj->backgroundTransparency = 1;
	descObj->clipDescendants = 1;
	descObj->processEvents = 0;

	listElement.main = main;
	listElement.header = header;
	listElement.desc = desc;
	listElement.editor = editor;
	listElement.selectTweens = PT_EXPANDABLE_ARRAY_new(4, sizeof(PT_TWEEN*));
	listElement.deselectTweens = PT_EXPANDABLE_ARRAY_new(4, sizeof(PT_TWEEN*));
	listElement.onhoverTweens = PT_EXPANDABLE_ARRAY_new(4, sizeof(PT_TWEEN*));

	set_instance_parent(header->instance, main->instance);
	set_instance_parent(desc->instance, main->instance);
	set_instance_parent(main->instance, listContainer->instance);

	TWEEN_CONFIG selectConfig = { 0 };
	selectConfig.direction = PT_OUT;
	selectConfig.type = PT_CUBIC;
	selectConfig.duration = .15f;

	TWEEN_CONFIG deselectConfig = { 0 };
	deselectConfig.direction = PT_IN;
	deselectConfig.type = PT_CUBIC;
	deselectConfig.duration = .15f;

	// on-select tweens
	PT_TWEEN* tween = PT_TWEEN_PT_COLOR_new(SELECTED_COLOR, &main->backgroundColor, selectConfig);
	PT_EXPANDABLE_ARRAY_add(&listElement.selectTweens, &tween);

	tween = PT_TWEEN_PT_COLOR_new(SELECTED_ACTIVE_COLOR, &main->activeBackgroundColor, selectConfig);
	PT_EXPANDABLE_ARRAY_add(&listElement.selectTweens, &tween);
	
	tween = PT_TWEEN_PT_REL_DIM_new(SELECTED_SIZE, &main->size, selectConfig);
	PT_EXPANDABLE_ARRAY_add(&listElement.selectTweens, &tween);

	// on-deselect tweens
	tween = PT_TWEEN_PT_COLOR_new(DESELECTED_COLOR, &main->backgroundColor, deselectConfig);
	PT_EXPANDABLE_ARRAY_add(&listElement.deselectTweens, &tween);

	tween = PT_TWEEN_PT_COLOR_new(DESELECTED_ACTIVE_COLOR, &main->activeBackgroundColor, deselectConfig);
	PT_EXPANDABLE_ARRAY_add(&listElement.deselectTweens, &tween);

	tween = PT_TWEEN_PT_REL_DIM_new(DESELECTED_SIZE, &main->size, deselectConfig);
	PT_EXPANDABLE_ARRAY_add(&listElement.deselectTweens, &tween);

	// on-hover tweens
	tween = PT_TWEEN_PT_REL_DIM_new(HOVER_SIZE, &main->size, selectConfig);
	PT_EXPANDABLE_ARRAY_add(&listElement.onhoverTweens, &tween);

	PT_BINDABLE_EVENT_bind(&main->e_obj_mouseEnter, list_element_mouseEnter);
	PT_BINDABLE_EVENT_bind(&main->e_obj_mouseLeave, list_element_mouseLeave);
	PT_BINDABLE_EVENT_bind(&main->e_obj_activated, list_element_activated);

	editor->listElement = listElement;

	TEXT_EDITOR_update_list_element(editor);

	PT_EXPANDABLE_ARRAY_insert(&editors, 0, &editor);

	update_list_element_order();
	update_rendertree();

	return listElement;
}

void TEXT_EDITOR_update_list_element(TEXT_EDITOR* editor) {
	EDITOR_LIST_ELEMENT listElement = editor->listElement;

	// free any old existing heap strings
	if (listElement.header->text) {
		free(listElement.header->text);
	}

	if (listElement.desc->text) {
		free(listElement.desc->text);
	}


	// update header string
	char* filename = calloc(1000, sizeof(char));
	if (editor->extension) {
		sprintf(filename, "%s.%s", editor->filename, editor->extension);
	}
	else {
		sprintf(filename, "%s", editor->filename);
	}
	if (!editor->saved) {
		*(filename + strlen(filename)) = '*';
	}
	listElement.header->text = filename;

	if (editor == currentTextEditor) {
		PT_set_window_title("PeachText - %s", filename);
	}

	// update desc string
	int truncateLen = 28;
	char* path = calloc(1000, sizeof(char));
	if (editor->path) {
		memcpy(path, editor->path, min(truncateLen, strlen(editor->path)));
	}
	if (editor->path && strlen(editor->path) > truncateLen) {
		for (int i = 0; i < 3; i++) {
			*(path + truncateLen + i) = '.';
		}
		*(path + truncateLen + 3) = '\0';
	}
	listElement.desc->text = path;
}

void TEXT_EDITOR_select(TEXT_EDITOR* textEditor) {
	if (currentTextEditor) { // deselect the old editor (if it exists)
		for (int i = 0; i < textEditor->listElement.deselectTweens.numElements; i++) {
			PT_TWEEN* tween = *(PT_TWEEN**)PT_EXPANDABLE_ARRAY_get(&currentTextEditor->listElement.deselectTweens, i);
			PT_TWEEN_play(tween);
		}

		currentTextEditor->scrollFrame->visible = 0;
	}

	if (textEditor) { 
		for (int i = 0; i < textEditor->listElement.selectTweens.numElements; i++) {
			PT_TWEEN* tween = *(PT_TWEEN**)PT_EXPANDABLE_ARRAY_get(&textEditor->listElement.selectTweens, i);
			PT_TWEEN_play(tween);
		}

		textEditor->scrollFrame->visible = 1;
	}

	currentTextEditor = textEditor;
	TEXT_EDITOR_update_list_element(textEditor);
}

void TEXT_EDITOR_save(TEXT_EDITOR* textEditor) {
	if (textEditor->path && textEditor->filename) { // this function requires the path and filename properties of the given struct to be non-null
		char* fullPath = calloc(1000, sizeof(char));
		if (textEditor->extension) {
			sprintf(fullPath, "%s%s.%s", textEditor->path, textEditor->filename, textEditor->extension);
		}
		else {
			sprintf(fullPath, "%s%s", textEditor->path, textEditor->filename);
		}

		FILE* file = fopen(fullPath, "w");
		if (!file) {
			wchar_t* wpath = calloc(1000, sizeof(wchar_t));
			mbstowcs(wpath, fullPath, 1000);
			fatal_error(L"failed to save file to %s", wpath);
			free(wpath);
			return;
		}

		for (int i = 0; i < textEditor->textLines->numElements; i++) {
			TEXT_LINE line = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(textEditor->textLines, i);
			fwrite(line.str, sizeof(char), line.numChars, file);
		}

		fclose(file);
		free(fullPath);

		textEditor->saved = 1;
		TEXT_EDITOR_update_list_element(textEditor);
	}
}

TEXT_LINE TEXT_LINE_new(const char* str, int len) {
	TEXT_LINE line = { 0 };

	line.str = calloc(len + 1, sizeof(char));
	line.numChars = 0;
	line.numCharSpace = len;
	line.flags = PT_EXPANDABLE_ARRAY_new(3, sizeof(TEXT_METADATA_FLAG));
	
	float time = PT_TIME_get();
	TEXT_METADATA_FLAG flag = create_text_metadata_flag(time);

	PT_EXPANDABLE_ARRAY_insert(&line.flags, 0, &flag);

	if (str && len > 0) {
		line.numChars = len;
		memcpy(line.str, str, len * sizeof(char));
	}

	return line;
}

TEXT_METADATA_FLAG create_text_metadata_flag(float t) {
	float* pTime = calloc(1, sizeof(float));
	*pTime = t;

	TEXT_METADATA_FLAG flag = { 0 };
	flag.index = 0;
	flag.color = colorTheme.textColor;
	flag.misc = pTime;

	return flag;
}

void TEXT_LINE_destroy(TEXT_LINE* line) {
	for (int i = 0; i < line->flags.numElements; i++) {
		TEXT_METADATA_FLAG flag = *(TEXT_METADATA_FLAG*)PT_EXPANDABLE_ARRAY_get(&line->flags, i);
		if (flag.misc) {
			free(flag.misc);
		}
	}
	PT_EXPANDABLE_ARRAY_destroy(&line->flags);

	free(line->str);
	line->numChars = 0;
	line->numCharSpace = 0;
}

TEXT_EDITOR* get_current_text_editor() {
	return currentTextEditor;
}

char get_last_char(TEXT_LINE line) {
	return *(line.str + (line.numChars - 1));
}

int* get_charsTyped() {
	return &charsTyped;
}

vec2i get_dir(int key) {
	vec2i dir = { 0 };

	switch (key) {
	case VK_UP:
		dir.y = 1;
		break;
	case VK_DOWN:
		dir.y = -1;
		break;
	case VK_LEFT:
		dir.x = -1;
		break;
	case VK_RIGHT:
		dir.x = 1;
		break;
	}

	return dir;
}

void insert_str_in_line(const char* str, int len, int index, TEXT_LINE* line) {
	if (line->numChars + len >= line->numCharSpace) {
		line->numCharSpace *= 2;
		line->str = realloc(line->str, sizeof(char) * line->numCharSpace);
	}

	int afterLen = line->numChars - index;
	char* afterStr = NULL;
	if (afterLen) {
		afterStr = calloc(afterLen, sizeof(char));
		memcpy(afterStr, line->str + index, afterLen * sizeof(char));
	}

	memcpy(line->str + index, str, len * sizeof(char)); // copy str into line

	if (afterLen) {
		memcpy(line->str + index + len, afterStr, afterLen * sizeof(char)); // append afterStr to line
		free(afterStr);
	}

	line->numChars += len;
}

// insert a tab at the beggining of every line in cursor selection
void tab_selection() {
	TEXT_CURSOR cursor = currentTextEditor->textCursor;

	vec2i sStart = { 0 };
	vec2i sEnd = { 0 };
	get_cursor_selection_bounds(cursor, &sStart, &sEnd);

	for (int i = sStart.y; i <= sEnd.y; i++) {
		TEXT_LINE* line = (TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(currentTextEditor->textLines, i);

		insert_str_in_line("\t", 1, 0, line);
	}
}

// remove a tab from the beginning of every line in cursor selection
void untab_selection() {
	TEXT_CURSOR cursor = currentTextEditor->textCursor;

	vec2i sStart = { 0 };
	vec2i sEnd = { 0 };
	get_cursor_selection_bounds(cursor, &sStart, &sEnd);

	for (int i = sStart.y; i <= sEnd.y; i++) {
		TEXT_LINE* line = (TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(currentTextEditor->textLines, i);

		if (*line->str == '\t') {
			TEXT_EDITOR_remove_str(currentTextEditor, (vec2i){ 0, i }, (vec2i){ 1, i });
		}
	}
}

void on_char_typed(void* args) {
	char c = *(char*)args;

	if (c < 32 && c != '\b' && c != '\n' && c != '\t') { // if its not a text-releated visible character (like ESC)
		return;
	}
	
	TEXT_CURSOR* cursor = &currentTextEditor->textCursor;
	int cursorY = cursor->position.y;
	int cloneLineY = cursorY + cursor->cloneLineOffset;

	int startY = min(cursorY, cloneLineY);
	int endY = max(cursorY, cloneLineY);
	for (int y = endY; y >= startY; y--) {
		vec2i cursorPos = cursor->position;
		int editsMade = 1;

		if (c == '\b') {
			if (vector_equal_2i(cursor->selectTo, cursor->position)) { // if nothing is selected
				vec2i end = cursorPos;
				vec3i startData = calculate_text_position(cursor->textArray, cursor->position, (vec2i) { -1, 0 }, cursor->targetX); 
				vec2i start = (vec2i){ startData.x, startData.y };

				if (!vector_equal_2i(end, start)) { // if there is actually something to be deleted
					TEXT_EDITOR_remove_str(currentTextEditor, start, end);
				}
			}
			else {
				TEXT_EDITOR_delete_selection(currentTextEditor);
			}
		}
		else if (c < 127) {
			if (c == '\t' && !vector_equal_2i(cursor->position, cursor->selectTo)) {
				if (is_key_down(VK_LSHIFT)) {
					untab_selection();
				}
				else {
					tab_selection();
				}
			}
			else {
				charsTyped++;
				vec2i p = (vec2i){ cursor->position.x, y };


				if (insertMode && c != '\n' && c != '\r') {
					int nextCharIndex = p.x + 1;

					TEXT_LINE thisLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, p.y);
					int lineLen = thisLine.numChars;

					if (p.y < cursor->textArray->numElements - 1) { // if this isn't the last line, sub 1 from lineLen (ignore '\n' char)
						lineLen--;
					}

					nextCharIndex = min(nextCharIndex, lineLen);

					if (nextCharIndex > p.x) {
						TEXT_EDITOR_remove_str(currentTextEditor, p, (vec2i) { nextCharIndex, p.y });
					}
				}

				TEXT_EDITOR_insert_str(currentTextEditor, &c, 1);
				
			}
		}
		else {
			editsMade = 0;
		}

		if (editsMade) {
			currentTextEditor->saved = 0;
			TEXT_EDITOR_update_list_element(currentTextEditor);
		}
	}	
}

void move_text_pos_in_view(vec2i textPosition) {
	if (currentTextEditor) {
		vec2i newCanvasPos = currentTextEditor->scrollFrame->canvasPosition;

		vec2i canvasSize = canvas_size(currentTextEditor->scrollFrame->guiObj->lastCanvas);
		canvasSize.x -= currentTextEditor->scrollFrame->scrollBarThickness;
		canvasSize.y -= currentTextEditor->scrollFrame->scrollBarThickness;

		int penY = editorLinePadding / 2;
		int penX = 0;
		int marginWidth = TEXT_EDITOR_get_margin(currentTextEditor);
		int margin = currentTextEditor->scrollFrame->guiObj->lastCanvas.left + marginWidth;
		int wrapX = TEXT_EDITOR_get_wrapX(currentTextEditor);
		if (wrapX) {
			wrapX -= margin;
		}
		int lineThickness = editorTextHeight + editorLinePadding;
		for (int i = 0; i <= textPosition.y; i++) {
			TEXT_LINE line = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(currentTextEditor->textLines, i);

			int numChars = line.numChars;
			if (i == textPosition.y)
			{
				numChars = textPosition.x;
			}
			else if (get_last_char(line) == '\n') {
				numChars--;
			}
			vec2i offset = get_text_offset(editorCharSet, line.str, numChars, wrapX);

			if (i == textPosition.y) {
				penX = offset.x;
			}
			else {
				penY += lineThickness * (1 + offset.y);
			}
		}

		// clamp to at least 0, 0
		vec2i canvasPos = currentTextEditor->scrollFrame->canvasPosition;
		if (penX < canvasPos.x) {
			newCanvasPos.x = penX;
		}
		else if (penX + marginWidth > canvasPos.x + canvasSize.x) {
			newCanvasPos.x = (penX + marginWidth) - canvasSize.x;
		}

		if (penY < canvasPos.y) {
			newCanvasPos.y = penY - editorLinePadding / 2;
		}
		else if (penY + lineThickness >= canvasPos.y + canvasSize.y) {
			newCanvasPos.y = (penY + lineThickness) - canvasSize.y ;
		}

		currentTextEditor->scrollFrame->targetCanvasPosition = newCanvasPos;
	}
}

void on_key_down(void* args) {
	if (!currentTextEditor) {
		return; // don't do anything if there is no current text editor
	}

	int key = *(int*)args;
	vec2i dir = get_dir(key);

	int shiftDown = is_key_down(VK_SHIFT);
	if (dir.x != 0 || dir.y != 0) { // if the cursor actually moved
		move_cursor(&currentTextEditor->textCursor, dir, shiftDown, 0);
	}
	else if (key == VK_DELETE) {
		TEXT_CURSOR* cursor = &currentTextEditor->textCursor;

		if (vector_equal_2i(cursor->position, cursor->selectTo)) { // nothing is selected
			vec3i nextPosData = calculate_text_position(cursor->textArray, cursor->position, (vec2i) { 1, 0 }, cursor->targetX);
			vec2i nextPos = (vec2i){ nextPosData.x, nextPosData.y };

			if (!vector_equal_2i(cursor->position, nextPos)) {
				TEXT_EDITOR_remove_str(currentTextEditor, cursor->position, nextPos);
			}
		}
		else {
			TEXT_EDITOR_delete_selection(currentTextEditor);
		}

		currentTextEditor->saved = 0;
		TEXT_EDITOR_update_list_element(currentTextEditor);
	}
	else if (key == VK_INSERT) {
		insertMode = insertMode ? 0 : 1;
	}
}

void copy(TEXT_CURSOR* cursor) {
	if (OpenClipboard(PT_GET_MAIN_HWND())) {
		EmptyClipboard();

		// get selection str data
		char* selection;
		int selectionLength;
		get_cursor_selection(cursor, &selection, &selectionLength);

		HGLOBAL hClipboardData;
		hClipboardData = GlobalAlloc(GMEM_DDESHARE, selectionLength);

		char* pchData;
		pchData = (char*)GlobalLock(hClipboardData);

		memcpy(pchData, selection, selectionLength * sizeof(char));
		free(selection); // we don't need our copy of selection any more, windows has it now

		GlobalUnlock(hClipboardData);

		SetClipboardData(CF_TEXT, hClipboardData);

		CloseClipboard();
	}
}

// https://www.codeproject.com/Articles/2242/Using-the-Clipboard-Part-I-Transferring-Simple-Tex
void on_command(void* args) {
	int command = *(int*)args;

	int editsMade = 0;

	if (currentTextEditor) {
		TEXT_CURSOR* cursor = &currentTextEditor->textCursor;

		switch (command) {
		case PT_COPY:
			;
			if (!vector_equal_2i(cursor->selectTo, cursor->position)) {
				copy(cursor);
			}
			break;
		case PT_CUT:
			if (!vector_equal_2i(cursor->selectTo, cursor->position)) {
				copy(cursor);

				vec2i start, end;
				get_cursor_selection_bounds(*cursor, &start, &end);
				TEXT_EDITOR_remove_str(currentTextEditor, start, end);

				editsMade = 1;
			}
			break;
		case PT_PASTE:
			if (currentTextEditor) {
				TEXT_EDITOR_delete_selection(currentTextEditor);
				
				if (OpenClipboard(PT_GET_MAIN_HWND()))
				{
					HANDLE hClipboardData = GetClipboardData(CF_TEXT);

					if (hClipboardData) {
						char* pchData = (char*)GlobalLock(hClipboardData);
						int clipboardLen = strlen(pchData);

						TEXT_EDITOR_insert_str(currentTextEditor, pchData, clipboardLen);
						editsMade = 1;

						GlobalUnlock(hClipboardData);
					}

					CloseClipboard();
				}
			}
			break;
		case PT_SELECTALL:
			;
			TEXT_LINE lastLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(cursor->textArray, cursor->textArray->numElements - 1);
			
			cursor->selectTo = (vec2i){ 0, 0 };
			cursor->position = (vec2i){ lastLine.numChars, cursor->textArray->numElements - 1 };

			break;
		case PT_ZOOM_IN:
			;
			int textHeight = editorTextHeight;
			textHeight++;

			textHeight = min(100, textHeight);
			editorTextHeight = textHeight;
			break;
		case PT_ZOOM_OUT:
			;
			textHeight = editorTextHeight;
			textHeight--;

			textHeight = max(4, textHeight);
			editorTextHeight = textHeight;
			break;
		case PT_UNDO:
			on_undo();
			break;
		case PT_REDO:
			on_redo();
			break;
		}
	}

	if (editsMade) {
		currentTextEditor->saved = 0;
		TEXT_EDITOR_update_list_element(currentTextEditor);
	}
}

void on_sys_key_down(void* args) {
	if (!currentTextEditor) {
		return;
	}

	int key = *(int*)args;
	vec2i dir = get_dir(key);

	int shiftDown = is_key_down(VK_SHIFT);
	if (dir.x != 0 || dir.y != 0) { // if the cursor actually moved
		move_cursor(&currentTextEditor->textCursor, dir, shiftDown, 1);
	}
}

void on_render_frame_render(PT_RENDERFRAME* renderFrame) {
	if (currentTextEditor && currentTextEditor->renderFrame == renderFrame) {
		render_text_editor(currentTextEditor);
	}
}

int TEXT_EDITOR_get_wrapX(TEXT_EDITOR* editor) {
	PT_GUI_OBJ* obj = editor->scrollFrame->guiObj;
	return wrapText ? (obj->lastCanvas.right - editor->scrollFrame->scrollBarThickness) : 0;
}

int TEXT_EDITOR_get_margin(TEXT_EDITOR* editor) {
	return editorCharWidth * 5;
}

PT_EXPANDABLE_ARRAY get_open_editors() {
	return editors;
}

vec2i TEXT_EDITOR_screenPos_to_cursorPos(vec2i screenPos) {
	vec2i cPos = (vec2i){ 0 };

	if (currentTextEditor) {
		vec2i canvasOffset = currentTextEditor->scrollFrame->canvasPosition;
		int xMargin = TEXT_EDITOR_get_margin(currentTextEditor);
		vec2i absPos = canvas_pos(currentTextEditor->scrollFrame->guiObj->lastCanvas);

		int xOffset = currentTextEditor->scrollFrame->guiObj->lastCanvas.left + TEXT_EDITOR_get_margin(currentTextEditor);
		int wrapX = TEXT_EDITOR_get_wrapX(currentTextEditor);

		if (wrapX) {
			wrapX = wrapX - xOffset;
		}

		vec2i relMousePos = vector_add_2i(screenPos, canvasOffset);
		relMousePos.x -= xMargin;
		relMousePos = vector_sub_2i(relMousePos, absPos);
		
		int lineThickness = editorTextHeight + editorLinePadding;

		int cx = 0;
		int cy = 0;
		int penX = 0;
		int penY = editorLinePadding / 2;
		for (int i = 0; i < currentTextEditor->textLines->numElements; i++) {
			TEXT_LINE* line = (TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(currentTextEditor->textLines, i);
			vec2i lineOrigin = (vec2i){ penX, penY};

			vec2i textBounds = get_text_rect(editorCharSet, line->str, line->numChars, wrapX);
			int boundsHeight = (textBounds.y + 1) * lineThickness;
			
			if (relMousePos.y >= penY && relMousePos.y < penY + boundsHeight) {
				cy = i;
				cx = get_text_position_from_rel_position(
					editorCharSet, 
					line->str, line->numChars, 
					vector_sub_2i(relMousePos, lineOrigin),
					lineThickness, wrapX
				);
			}

			penY += boundsHeight;
			
			if (i == currentTextEditor->textLines->numElements - 1) {
				if (relMousePos.y > penY) {
					cx = line->numChars;
					cy = i;
				}
			}

			if (relMousePos.y < penY) {
				break;
			}
		}

		cPos = (vec2i){ cx, cy };
	}

	return cPos;
}

void TEXT_EDITOR_on_click() {
	if (currentTextEditor) {
		collapse_sidebar();

		int altDown = is_key_down(VK_MENU);
		int shiftDown = is_key_down(VK_LSHIFT);

		vec2i cursorPos = TEXT_EDITOR_screenPos_to_cursorPos(mousePos);
		TEXT_CURSOR* cursor = &currentTextEditor->textCursor;
		cursor->targetX = mousePos.x;
		vec2i currentPos = cursor->position;
		vec2i selectPos = cursor->selectTo;

		if (!(altDown || shiftDown)) {
			if (vector_equal_2i(cursorPos, cursor->position) && vector_equal_2i(cursor->position, cursor->selectTo)) {
				TEXT_CURSOR_select_word(cursor);
			}
			else {
				cursor->position = cursorPos;
				cursor->selectTo = cursorPos;
				cursor->cloneLineOffset = 0;
			}
		}
		else if (shiftDown && !altDown) {
			currentTextEditor->textCursor.position = cursorPos;
			currentTextEditor->textCursor.cloneLineOffset = 0;
		}

		move_text_pos_in_view(currentTextEditor->textCursor.position);
	}
}

void TEXT_EDITOR_on_drag() {
	if (currentTextEditor) {
		int altDown = is_key_down(VK_MENU);
		int shiftDown = is_key_down(VK_LSHIFT);

		vec2i cursorPos = TEXT_EDITOR_screenPos_to_cursorPos(mousePos);

		if (!(altDown || shiftDown)) {
			currentTextEditor->textCursor.position = cursorPos;
			currentTextEditor->textCursor.cloneLineOffset = 0;
		}
		
	}
}

void TEXT_EDITOR_mouse_enter() {
	PT_set_window_cursor(IDC_IBEAM);
}

void TEXT_EDITOR_mouse_leave() {
	PT_set_window_cursor(IDC_ARROW);
}

void TEXT_EDITOR_mouse_move() {
	if (currentTextEditor) {
		int leftBound = currentTextEditor->scrollFrame->guiObj->lastCanvas.left + TEXT_EDITOR_get_margin(currentTextEditor);
		int rightBound = screenSize.x;

		int topBound = currentTextEditor->scrollFrame->guiObj->lastCanvas.top;
		int bottomBound = currentTextEditor->scrollFrame->guiObj->lastCanvas.bottom;

		if (mousePos.x < rightBound && mousePos.x > leftBound && mousePos.y < bottomBound && mousePos.y > topBound) {
			PT_set_window_cursor(IDC_IBEAM);
		}
		else {
			PT_set_window_cursor(IDC_ARROW);
		}
	}
}

void TEXT_EDITOR_destroy(TEXT_EDITOR* editor) {
	// retarget currentTextEditor if it's pointing to this editor
	if (currentTextEditor == editor) {
		currentTextEditor = NULL;
	}

	// remove editor from editors list
	for (int i = 0; i < editors.numElements; i++) {
		TEXT_EDITOR* e = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);
		if (e == editor) {
			PT_EXPANDABLE_ARRAY_remove(&editors, i);
			break;
		}
	}

	// destroy all editor instances
	destroy_instance(editor->scrollFrame->instance);
	destroy_instance(editor->listElement.main->instance);

	// free editor path strings
	if (editor->path) {
		free(editor->path);
	}
	if (editor->filename) {
		free(editor->filename);
	}
	if (editor->extension) {
		free(editor->extension);
	}

	// free editor text
	for (int i = 0; i < editor->textLines->numElements; i++) {
		TEXT_LINE* line = (TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(editor->textLines, i);
		TEXT_LINE_destroy(line);
	}
	PT_EXPANDABLE_ARRAY_destroy(editor->textLines);
}

TEXT_EDITOR* TEXT_EDITOR_new(Instance* backgroundInstance, PT_RENDERFRAME* renderFrame, PT_RENDERFRAME* sideRenderFrame, PT_SCROLLFRAME* listContainer) {
	if (!editors.data) { // init
		editors = PT_EXPANDABLE_ARRAY_new(5, sizeof(TEXT_EDITOR*));

		insertMode = 0;
		wrapText = 0;

		editorTextHeight = 13;
		editorLinePadding = editorTextHeight * .9;
		editorCharSet = get_char_set(PT_FONT_ARIAL, editorTextHeight);
		editorCharWidth = get_text_rect(editorCharSet, "M", 1, 0).x;
	}

	PT_GUI_OBJ* backgroundObj = (PT_GUI_OBJ*)backgroundInstance->subInstance;

	// main text scrollframe 
	PT_SCROLLFRAME* scrollFrame = create_editor_scrollframe();
	char* name = calloc(50, sizeof(char));
	sprintf(name, "editor %i", editors.numElements + 1);
	scrollFrame->instance->name = name;
	set_instance_parent(scrollFrame->instance, backgroundInstance);

	if (!keyDownBound) {
		keyDownBound = 1;

		PT_BINDABLE_EVENT_bind(&eOnCharTyped, on_char_typed);
		PT_BINDABLE_EVENT_bind(&eOnKeyPress, on_key_down);
		PT_BINDABLE_EVENT_bind(&eOnSysKeyPress, on_sys_key_down);
		PT_BINDABLE_EVENT_bind(&eOnCommand, on_command);
	}
	
	PT_BINDABLE_EVENT_bind(&scrollFrame->guiObj->e_obj_mouseEnter, TEXT_EDITOR_mouse_enter);
	PT_BINDABLE_EVENT_bind(&scrollFrame->guiObj->e_obj_mouseLeave, TEXT_EDITOR_mouse_leave);
	PT_BINDABLE_EVENT_bind(&scrollFrame->guiObj->e_obj_pressed, TEXT_EDITOR_on_click);
	PT_BINDABLE_EVENT_bind(&scrollFrame->guiObj->e_obj_dragged, TEXT_EDITOR_on_drag);

	TEXT_EDITOR* editor = calloc(1, sizeof(TEXT_EDITOR));

	editor->filename = create_heap_str("untitled");
	editor->extension = create_heap_str("txt");
	editor->textLines = calloc(1, sizeof(PT_EXPANDABLE_ARRAY));
	*editor->textLines = PT_EXPANDABLE_ARRAY_new(5, sizeof(TEXT_LINE));

	editor->renderFrame = renderFrame;
	editor->sideRenderFrame = sideRenderFrame;
	editor->scrollFrame = scrollFrame;

	editor->textColor = colorTheme.textColor;
	editor->editColor = colorTheme.editColor;
	editor->editFadeTime = .8f;

	renderFrame->render = on_render_frame_render;

	// init first text line
	TEXT_LINE firstLine = TEXT_LINE_new(NULL, 0);
	PT_EXPANDABLE_ARRAY_add(editor->textLines, (void*)&firstLine);

	TEXT_CURSOR mainCursor = TEXT_CURSOR_new(editor);
	editor->textCursor = mainCursor;

	create_editor_list_element(editor, listContainer);

	TEXT_EDITOR_select(editor);

	return editor;
}

TEXT_EDITOR* TEXT_EDITOR_from_file(Instance* backgroundInstance, PT_RENDERFRAME* renderFrame, PT_RENDERFRAME* sideRenderFrame, PT_SCROLLFRAME* listContainer, const char* filename) {
	// convert path to absolute full path and break path into components (parent path, filename, and file extension)
	char* path = calloc(1000, sizeof(char));
	if (*(filename + 1) != ':') { // if the second character isn't ':', then this is a relative directory. Convert to absolute (starting from a drive)
		sprintf(path, "%s\\%s", initWorkingDir, filename);
	}
	else {
		sprintf(path, "%s", filename);
	}
	char* name = calloc(1000, sizeof(char));
	char* extension = calloc(50, sizeof(char));
	int pathlen = strlen(path);
	int extensionIndex = pathlen + 1;
	int nameIndex = -1;
	for (int i = pathlen - 1; i >= 0; i--) {
		char c = *(path + i);

		if (c == '.') {
			int extensionLen = (pathlen - (i + 1));
			memcpy(extension, path + i + 1, extensionLen);
			extensionIndex = i + 1;
		}

		if (c == '\\' && nameIndex < 0) {
			int nameLen = (extensionIndex - 1) - (i + 1);
			memcpy(name, path + i + 1, nameLen);
			nameIndex = i + 1;
			break;
		}
	}

	if (extensionIndex == pathlen + 1) { // extensionIndex hasn't been changed: this file doesn't have an extension
		free(extension);
		extension = NULL;
	}

	if (nameIndex > 0) {
		*(path + nameIndex) = '\0';
	}
	else {
		fatal_error(L"nameIndex < 0 (%i)", nameIndex);
	}


	// make sure we don't already have the same file open
	for (int i = 0; i < editors.numElements; i++) {
		TEXT_EDITOR* e = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);

		int extensionsEqual = 0;
		if (e->extension && extension) {
			extensionsEqual = strcmp(e->extension, extension) == 0;
		}
		else { 
			if (!(e->extension || extension)) { // both are null
				extensionsEqual = 1;
			}
		}

		int namesEqual = strcmp(e->filename, name) == 0;
		int pathsEqual = 0;
		
		if (e->path && path) {
			pathsEqual = strcmp(e->path, path) == 0;
		}

		if (namesEqual || pathsEqual) {
			int x = 0;
		}

		if (extensionsEqual && namesEqual && pathsEqual) {
			return e;
		}
	}
	
	
	TEXT_EDITOR* editor = TEXT_EDITOR_new(backgroundInstance, renderFrame, sideRenderFrame, listContainer);
	
	FILE* file = fopen(filename, "rb+");
	if (!file) {
		int len = strlen(filename);
		wchar_t* wfilename = calloc(len + 1, sizeof(wchar_t));
		mbstowcs(wfilename, filename, len);

		error(L"Could not find file \"%s\"", wfilename);

		free(wfilename);

		return editor;
	}

	TEXT_CURSOR* cursor = &editor->textCursor;
	const int bufferSize = 1000;
	char* buffer = calloc(bufferSize + 1, sizeof(char)); // add one for null terminator
	int charsRead = 0;
	do {
		charsRead = fread(buffer, sizeof(char), bufferSize, file);
		if (charsRead > 0) {

			TEXT_EDITOR_insert_str(editor, buffer, charsRead);
		}
	} while (charsRead == bufferSize);
	free(buffer);
	fclose(file);

	// initial view should be at the top of the file
	editor->textCursor.position = (vec2i){ 0, 0 };
	editor->textCursor.selectTo = (vec2i){ 0, 0 };
	editor->textCursor.targetX = 0;
	editor->scrollFrame->canvasPosition = (vec2i){ 0, 0 };
	editor->scrollFrame->targetCanvasPosition = (vec2i){ 0, 0 };

	editor->saved = 1;
	REPLACE_STR(editor->path, path);
	REPLACE_STR(editor->filename, name);
	REPLACE_STR(editor->extension, extension);

	TEXT_EDITOR_update_list_element(editor);

	return editor;
}

void TEXT_EDITORs_update(float dt) {
	float t = PT_TIME_get();

	// destroy all editors marked for deletion
	int editorsRemoved = 0;
	for (int i = editors.numElements - 1; i >= 0; i--) {
		TEXT_EDITOR* e = *(TEXT_EDITOR**)PT_EXPANDABLE_ARRAY_get(&editors, i);
		if (e->markedForDeletion) {
			check_unsaved_work(e);
			TEXT_EDITOR_destroy(e);

			editorsRemoved++;
		}
	}
	if (editorsRemoved) {
		update_rendertree();
		update_list_element_order();
	}

	TEXT_EDITOR* editor = currentTextEditor;
	if (!editor) {
		return;
	}

	if (editorCharSet->textHeight != editorTextHeight) {
		editorCharSet = get_char_set(PT_FONT_ARIAL, editorTextHeight);
		editorCharWidth = get_text_rect(editorCharSet, "M", 1, 0).x;
		editorLinePadding = editorTextHeight * .9;
	}

	TEXT_CURSOR* textCursor = &editor->textCursor;
	int cursorY = textCursor->position.y;
	int cloneLineY = cursorY + textCursor->cloneLineOffset;

	int startY = min(cursorY, cloneLineY);
	int endY = max(cursorY, cloneLineY);
	for (int y = startY; y <= endY; y++) {
		vec2i cursorPos = textCursor->position;

		TEXT_LINE thisLine = *(TEXT_LINE*)PT_EXPANDABLE_ARRAY_get(editor->textLines, y);
		int x = min(thisLine.numChars, editor->textCursor.position.x);

		textCursor->color = colorTheme.cursorColor;

		if (insertMode) {
			int charWidth = 2;
			if (editor->textCursor.position.x < thisLine.numChars) {
				char c = *(thisLine.str + editor->textCursor.position.x);
				charWidth = max(charWidth, (editorCharSet->size + c)->x);
			}
		}
	}
}

void TEXT_EDITOR_insert_str(TEXT_EDITOR* editor, char* str, int len) {
	TEXT_CURSOR* cursor = &editor->textCursor;
	vec2i start = cursor->position;
	vec2i endPos = insert_str_in_text_array(cursor->textArray, start, str, len);
	cursor->position = endPos;
	cursor->selectTo = cursor->position;
	update_targetX(cursor);
	move_text_pos_in_view(cursor->position);

	ACTION_RANGE range = {
		start, endPos
	};
	TEXT_ACTION* textAction = TEXT_ACTION_new(str, len, 1, TA_INSERT, editor->lastAction, &editor->actionIndex);
	textAction->textLines = editor->textLines;
	PT_EXPANDABLE_ARRAY_add(&textAction->ranges, &range);
	TEXT_ACTION_append(&editor->firstAction, &editor->lastAction, textAction, &editor->actionIndex);
}

void TEXT_EDITOR_remove_str(TEXT_EDITOR* editor, vec2i start, vec2i end) {	TEXT_CURSOR* cursor = &editor->textCursor;
	cursor->lastTypedTime = PT_TIME_get();
	
	ACTION_RANGE range = {
		start, end
	};
	char* removedStr = NULL;
	int removedLen = 0;
	get_text_in_range(editor->textLines, start, end, &removedStr, &removedLen);

	remove_str_from_text_array(editor->textLines, start, end);

	TEXT_ACTION* textAction = TEXT_ACTION_new(removedStr, removedLen, 1, TA_DELETE, editor->lastAction, &editor->actionIndex);
	textAction->textLines = editor->textLines;
	PT_EXPANDABLE_ARRAY_add(&textAction->ranges, &range);
	TEXT_ACTION_append(&editor->firstAction, &editor->lastAction, textAction, &editor->actionIndex);

	// move cursor to start of deletion range
	cursor->position = start;
	cursor->selectTo = start;
	update_targetX(cursor);
	move_text_pos_in_view(cursor->position);
}

void TEXT_EDITOR_delete_selection(TEXT_EDITOR* editor) {
	TEXT_CURSOR* cursor = &editor->textCursor;
	if (!vector_equal_2i(cursor->selectTo, cursor->position)) { // if there is actually something selected...
		vec2i start, end;
		get_cursor_selection_bounds(*cursor, &start, &end);
		
		TEXT_EDITOR_remove_str(editor, start, end);
	}
}

void on_undo() {
	if (!currentTextEditor) {
		return;
	}

	TEXT_ACTION* lastAction = currentTextEditor->lastAction;
	int currentActionId = currentTextEditor->actionIndex;

	// move down the action chain until we hit null or the current action index
	while (lastAction != NULL && lastAction->actionId > currentActionId - 1) {
		lastAction = lastAction->lastAction;
	}

	if (lastAction) {
		vec2i newCursorPos = TEXT_ACTION_undo(lastAction);
		TEXT_CURSOR* cursor = &currentTextEditor->textCursor;
		cursor->position = newCursorPos;
		cursor->selectTo = newCursorPos;
		currentTextEditor->actionIndex--;
	}
}

void on_redo() {
	if (!currentTextEditor) {
		return;
	}

	TEXT_ACTION* lastUndoneAction = currentTextEditor->lastAction;
	int currentActionId = currentTextEditor->actionIndex;

	// move down the action chain until we hit null or the current action index
	while (lastUndoneAction != NULL && lastUndoneAction->actionId > currentActionId) {
		lastUndoneAction = lastUndoneAction->lastAction;
	}


	if (lastUndoneAction && currentActionId <= lastUndoneAction->actionId) {
		vec2i newCursorPos = TEXT_ACTION_do(lastUndoneAction);
		TEXT_CURSOR* cursor = &currentTextEditor->textCursor;
		cursor->position = newCursorPos;
		cursor->selectTo = newCursorPos;
		currentTextEditor->actionIndex++;
	}
}