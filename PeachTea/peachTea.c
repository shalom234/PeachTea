#define _CRT_SECURE_NO_WARNINGS

#include "PeachTea.h"
#include "screenSize.h"
#include "renderer.h"
#include "PeachTeaShaders.h"

#include <stdlib.h>

HDC hMainDC;
HGLRC hMainRC;
HWND hMainWnd = NULL;

void(*renderCallback)(void);

int mainProgramLoop(void(*renderCallback)(void));

void update_main_window_pos() {
	static RECT rect;

	vec2i clientSize = { 0 };
	GetClientRect(hMainWnd, &rect);
	clientSize = (vec2i){ rect.right, rect.bottom };

	vec2i windowSize = { 0 };
	GetWindowRect(hMainWnd, &rect);
	windowSize = (vec2i){ rect.right - rect.left, rect.bottom - rect.top };

	int leftBorderSize = (windowSize.x - clientSize.x) / 2;
	int topBorderSize = windowSize.y + -clientSize.y + -leftBorderSize;

	mainWindowPosition = (vec2i){ rect.left + leftBorderSize, rect.top + topBorderSize };
}

void PT_INIT(vec2i screenSize) {
	screensize_init(screenSize);
	GLEInit();
	initFT();
	PT_SHADERS_init();

	renderer_init();

	PT_SCREEN_UI_init();

	update_main_window_pos();
}

void PT_UPDATE() {
	
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void PT_CREATE_MAIN_WND(vec2i size, const char* title) {	
	int len = strlen(title) + 2;
	wchar_t* wtitle = calloc(len, sizeof(wchar_t));
	mbstowcs(wtitle, title, len);
	hMainWnd = createPeachWindow(NULL, size, (void*)WndProc, wtitle);
	free(wtitle);

	RECT rect;
	GetClientRect(hMainWnd, &rect);
	screenSize = (vec2i){ rect.right - rect.left, rect.bottom - rect.top };

	mouse_init(hMainWnd);

	hMainDC = GetDC(hMainWnd);
	hMainRC = wglCreateContext(hMainDC);
	wglMakeCurrent(hMainDC, hMainRC);

	PT_INIT(screenSize);
}

void PT_GET_MAIN_HWND() {
	return hMainWnd;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_SIZE:
		;
		if (hWnd == hMainWnd) {
			vec2i size = (vec2i){ LOWORD(lParam), HIWORD(lParam) };
			PT_RESIZE(size);
			renderer_resized();
			update_main_window_pos();
			render(renderCallback);
		}

		return 0;
	case WM_CLOSE:
		if (hWnd == hMainWnd) {
			PostQuitMessage(69);
		}
		break;
	case WM_MOVING:
		if (hWnd == hMainWnd) {
			update_main_window_pos();
		}
		break;
	case WM_INPUT:
		;
		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)lpb;

		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			mouse_input(raw);
		}
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hMainWnd);
		PT_BINDABLE_EVENT_fire(&e_mouseDown, NULL);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		PT_BINDABLE_EVENT_fire(&e_mouseUp, NULL);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int mainProgramLoop(void(*renderCB)(void)) {
	static MSG msg;
	int exitCode = 0;

	renderCallback = renderCB;

	while (1) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				exitCode = msg.wParam;
				return;
			}
		}

		PT_UPDATE();
		render(renderCallback);
	}


	wglMakeCurrent(NULL, NULL);
	ReleaseDC(hMainWnd, hMainDC);
	wglDeleteContext(hMainRC);

	return exitCode;
}

int PT_RUN(void(*renderCallback)(void)) {
	return mainProgramLoop(renderCallback);
}










