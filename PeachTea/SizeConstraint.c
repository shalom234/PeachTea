#include "SizeConstraint.h"
#include <stdlib.h>
#include "ScreenDimension.h"
#include "screenSize.h"
#include "guiObj.h"

#include <string.h>
#include <stdio.h>
#include "clamp.h"

vec2i onResize_none(PT_GUI_OBJ* obj, PT_ABS_DIM parentDims) {
	vec2i dims = calculate_screen_dimension(obj->size, parentDims.size);

	return dims;
}

vec2i onResize_aspect(PT_GUI_OBJ* obj, PT_ABS_DIM parentDims) {
	vec2i absSize = calculate_screen_dimension(obj->size, parentDims.size);

	PTSC_AXIS lockedAxis = obj->sizeConstraint->lockedAxis;
	if (lockedAxis > 1) {
		PTSC_AXIS biggest = screenSize.x > screenSize.y ? PTSC_LOCK_X : PTSC_LOCK_Y;
		PTSC_AXIS smallest = biggest == PTSC_LOCK_X ? PTSC_LOCK_Y : PTSC_LOCK_X;

		lockedAxis = lockedAxis == PTSC_LOCK_BIGGEST ? biggest : smallest;
	}

	if (lockedAxis == PTSC_LOCK_X) {
		absSize.y = absSize.x / obj->sizeConstraint->aspectRatio;
	}
	else { // PTSC_LOCK_Y
		absSize.x = absSize.y * obj->sizeConstraint->aspectRatio;
	}

	return absSize;
}

vec2i onResize_bounds(PT_GUI_OBJ* obj, PT_ABS_DIM parentDims) {
	vec2i size = onResize_none(obj, parentDims);

	size = (vec2i){
		clamp(size.x, obj->sizeConstraint->minSize.x, obj->sizeConstraint->maxSize.x),
		clamp(size.y, obj->sizeConstraint->minSize.y, obj->sizeConstraint->maxSize.y),
	};

	return size;
}

PT_SIZE_CONSTRAINT* PT_SIZE_CONSTRAINT_new() {
	PT_SIZE_CONSTRAINT* constraint = calloc(1, sizeof(PT_SIZE_CONSTRAINT));

	return constraint;
}

PT_SIZE_CONSTRAINT* PT_SIZE_CONSTRAINT_clone(PT_SIZE_CONSTRAINT* source) {
	PT_SIZE_CONSTRAINT* clone = PT_SIZE_CONSTRAINT_new();

	memcpy(clone, source, sizeof(PT_SIZE_CONSTRAINT));

	return clone;
}

PT_SIZE_CONSTRAINT* PT_SIZE_CONSTRAINT_none() {
	PT_SIZE_CONSTRAINT* constraint = PT_SIZE_CONSTRAINT_new();
	
	constraint->calculateSize = onResize_none;

	return constraint;
}

PT_SIZE_CONSTRAINT* PT_SIZE_CONSTRAINT_aspect(float aspectRatio, PTSC_AXIS lockedAxis) {
	PT_SIZE_CONSTRAINT* constraint = PT_SIZE_CONSTRAINT_new();

	constraint->aspectRatio = aspectRatio;
	constraint->lockedAxis = lockedAxis;

	constraint->calculateSize = onResize_aspect;

	return constraint;
}

PT_SIZE_CONSTRAINT* PT_SIZE_CONSTRAINT_bounds(vec2i min, vec2i max) {
	PT_SIZE_CONSTRAINT* constraint = PT_SIZE_CONSTRAINT_new();

	constraint->calculateSize = onResize_bounds;

	return constraint;
}

void PT_SIZE_CONSTRAINT_destroy(PT_SIZE_CONSTRAINT* ptsc) {
	free(ptsc);
}