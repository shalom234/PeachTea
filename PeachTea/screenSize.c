#include "screenSize.h"

void PT_RESIZE(vec2i ss) {
	screenSize = ss;
	PT_BINDABLE_EVENT_fire(&eOnResize, (void*)0);
}

void screensize_init(vec2i ss) {
	screenSize = ss;
}