#define _CRT_SECURE_NO_WARNINGS

#include "PeachTea.h"
#include "screenSize.h"
#include "renderer.h"
#include "PeachTeaShaders.h"
#include <dwmapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <direct.h>


#pragma comment(lib, "dwmapi")

HDC hMainDC = NULL;
HGLRC hMainRC = NULL;
HWND hMainWnd = NULL;

void (*on_contextmenu)(WPARAM, LPARAM) = NULL;
void(*renderCallback)(void) = NULL;

int mainProgramLoop(void(*updateCallback)(float), void(*renderCB)(void));

int PT_window_cursor = IDC_ARROW;
void PT_set_window_cursor(int idc) {
	PT_window_cursor = idc;
}

int mousewheelDelta = 0;
int get_mousewheel_delta() {
	return mousewheelDelta;
}

void PT_set_on_contextmenu(void (*f)(WPARAM, LPARAM)) {
	on_contextmenu = f;
}

void update_main_window_pos() {
	RECT rect = { 0 };

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
	initWorkingDir = calloc(MAX_PATH + 1, sizeof(char));
	GetModuleFileNameA(NULL, initWorkingDir, MAX_PATH);
	for (int i = strlen(_pgmptr) - 1; i >= 0; i--) {
		char c = *(initWorkingDir + i);

		if (c == '\\') {
			*(initWorkingDir + i + 1) = '\0';
			break;
		}
	}
	printf("%s\n", initWorkingDir);
	_chdir(initWorkingDir);

	screensize_init(screenSize);
	GLEInit();
	initFT();
	PT_SHADERS_init();
	PT_TIME_start();
	PT_TWEEN_init();

	renderer_init();

	PT_SCREEN_UI_init();

	update_main_window_pos();
}

float lastTime = 0;
float PT_UPDATE() {
	float time = PT_TIME_get();

	float diff = time - lastTime;

	lastTime = time;

	PT_TWEEN_update();

	return diff;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void update_accent_color() {
	DWORD c = 0u;
	BOOL bool = 0;
	HRESULT hr = DwmGetColorizationColor(&c, &bool);
	unsigned char a = c >> 24;
	unsigned char r = (c >> 16) & 255u;
	unsigned char g = (c >> 8) & 255u;
	unsigned char b = (c >> 0) & 255u;

	accentColor = PT_COLOR_fromRGB(r, g, b);
}

void PT_set_window_title(const char* format, ...) {
	va_list args = NULL;
	va_start(args, format);

	long length = (strlen(format) + 100) * 2;
	char* str = calloc(length, sizeof(char));

	vsprintf(str, format, args);
	va_end(args);

	SetWindowTextA(hMainWnd, str);

	free(str);
}

void PT_CREATE_MAIN_WND(vec2i size, const char* title, HICON icon) {	
	update_accent_color();

	int len = strlen(title) + 2;
	wchar_t* wtitle = calloc(len, sizeof(wchar_t));
	mbstowcs(wtitle, title, len);
	hMainWnd = createPeachWindow(NULL, size, (void*)WndProc, wtitle, icon);
	free(wtitle);

	RECT rect = { 0 };
	GetClientRect(hMainWnd, &rect);
	screenSize = (vec2i){ rect.right - rect.left, rect.bottom - rect.top };

	mouse_init(hMainWnd);

	hMainDC = GetDC(hMainWnd);
	hMainRC = wglCreateContext(hMainDC);
	wglMakeCurrent(hMainDC, hMainRC);

	PT_INIT(screenSize);
}

HWND PT_GET_MAIN_HWND() {
	return hMainWnd;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg) {
	case WM_COMMAND:
		;
		int commandId = LOWORD(wParam);
		PT_BINDABLE_EVENT_fire(&eOnCommand, &commandId);
		break;
	case WM_RBUTTONDOWN:
		if (on_contextmenu) {
			on_contextmenu(wParam, lParam);
		}
		break;
	case WM_SIZE:
		;
		vec2i size = (vec2i){ LOWORD(lParam), HIWORD(lParam) };
		PT_RESIZE(size); // update screenSize;
		update_main_window_pos(); // update window position
		renderer_resized(); // update glViewport
		render(renderCallback); 

		return 0;
	case WM_CLOSE:
		PostQuitMessage(69);
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
		PT_BINDABLE_EVENT_fire(&e_mouse1Down, NULL);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		PT_BINDABLE_EVENT_fire(&e_mouse1Up, NULL);
		break;
	case WM_SETCURSOR:
		;
		int ncHitVal = LOWORD(lParam);
		if (ncHitVal == HTCLIENT) {
			//printf("%i\n", PT_window_cursor);
			SetCursor(LoadCursor(NULL, PT_window_cursor));
		}
		else {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		break;
	case WM_MOUSEWHEEL:
		;
		int d = -GET_WHEEL_DELTA_WPARAM(wParam);
		mousewheelDelta = d;

		if (d > 0) {
			PT_BINDABLE_EVENT_fire(&e_wheelUp, &d);
		}
		else {
			PT_BINDABLE_EVENT_fire(&e_wheelDown, &d);
		}

		break;
	case WM_CHAR:
		;
		char c = wParam == '\r' ? '\n' : wParam; // convert '\r' to '\n'
		PT_BINDABLE_EVENT_fire(&eOnCharTyped, (void*)&c);
		break;
	case WM_KEYDOWN:
		if (is_key_down(VK_LCONTROL)) {
			int command = -1;
			switch(wParam) {
			case 'V':
				command = PT_PASTE;
				break;
			case 'X':
				command = PT_CUT;
				break;
			case 'C':
				command = PT_COPY;
				break;
			case 'A':
				command = PT_SELECTALL;
				break;
			case VK_OEM_PLUS:
				command = PT_ZOOM_IN;
				break;
			case VK_OEM_MINUS:
				command = PT_ZOOM_OUT;
				break;
			case 'S':
				command = is_key_down(VK_LSHIFT) ? PT_SAVE_AS : PT_SAVE;
				break;
			case 'Z':
				command = PT_UNDO;
				break;
			case 'Y':
				command = PT_REDO;
				break;
			}

			if (command >= 0) {
				PT_BINDABLE_EVENT_fire(&eOnCommand, &command);
			}
		}
		else {
			PT_BINDABLE_EVENT_fire(&eOnKeyPress, (void*)&wParam);
		}
		break;
	case WM_KEYUP:
		PT_BINDABLE_EVENT_fire(&eOnKeyRelease, (void*)&wParam);
		break;
	case WM_SYSKEYDOWN:
		PT_BINDABLE_EVENT_fire(&eOnSysKeyPress, (void*)&wParam);
		break;
	case WM_SYSKEYUP:
		PT_BINDABLE_EVENT_fire(&eOnSysKeyRelease, (void*)&wParam);
		break;
	case WM_GETMINMAXINFO: // set minimum size for window
		;
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 350;
		lpMMI->ptMinTrackSize.y = 350;
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

int mainProgramLoop(void(*updateCallback)(float), void(*renderCB)(void)) {
	static MSG msg;
	int exitCode = 0;

	renderCallback = renderCB;

	lastTime = PT_TIME_get();

	float lastRenderTime = 0;
	const float TARGET_FRAMERATE = 60;
	while (1) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				exitCode = msg.wParam;
				return;
			}
		}


		float t = PT_TIME_get();
		float timeSinceRender = t - lastRenderTime;
		if (timeSinceRender > 1 / TARGET_FRAMERATE) {
			float dt = PT_UPDATE(); 
			if (updateCallback) {
				updateCallback(dt);
			}
			
			lastRenderTime = t;
			render(renderCallback);
		}
	}


	wglMakeCurrent(NULL, NULL);
	ReleaseDC(hMainWnd, hMainDC);
	wglDeleteContext(hMainRC);

	return exitCode;
}

int PT_RUN(void(*updateCallback)(float), void(*renderCallback)(void)) {
	return mainProgramLoop(updateCallback, renderCallback);
}










