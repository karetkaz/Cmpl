#include "../gx_gui.h"
#include <windows.h>
#include <winuser.h>

struct GxWindow {
	GxImage image;

	HDC wDC;
	HWND hwnd;
	BITMAPINFO BMP;
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

GxWindow createWindow(GxImage image, const char *title) {
	if (image == NULL) {
		return NULL;
	}

	GxWindow result = malloc(sizeof(struct GxWindow));
	if (result == NULL) {
		return NULL;
	}

	HINSTANCE mainhins = 0;
	char *class_name = "GFX(2/3)D_Window";

	WNDCLASS windowClass;
	windowClass.style = CS_OWNDC; // The style of the window. CS_OWNDC means every window has it's own DC
	windowClass.lpfnWndProc = WindowProc;//bitmapWindowHandler; // The function to call when this window receives a message
	windowClass.cbClsExtra = 0; // Extra bytes to allocate for this class (none)
	windowClass.cbWndExtra = 0; // Extra bytes to allocate for each window (none)
	windowClass.hInstance = mainhins; // This application's instance
	windowClass.hIcon = LoadIcon(mainhins, IDI_APPLICATION); // A standard Icon
	windowClass.hCursor = LoadCursor(mainhins, IDC_ARROW); // A standard cursor
	windowClass.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE; // A standard background
	windowClass.lpszMenuName = NULL; // No menus in this window
	windowClass.lpszClassName = (void*)class_name; // A name for this class
	RegisterClass(&windowClass); // Register the class

	result->hwnd = CreateWindow(
		class_name,			// Name of the window class (registered above)
		title,				// Name of the window, appears in the window title bar
		WS_MINIMIZEBOX | WS_SYSMENU,	// Window style
		CW_USEDEFAULT,			// X coordinate of the window on-screen
		CW_USEDEFAULT,			// Y coordinate of the window on-screen
		image->width,			// width of the window
		image->height,			// height of the window
		NULL,				// Handle to a parent window (none)
		NULL,				// Handle to a child window (none)
		mainhins,			// Handle to the current Instance
		0					// Pointer to extra data I don't care about
	);
	result->BMP.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	result->BMP.bmiHeader.biWidth = image->width;
	result->BMP.bmiHeader.biHeight = -image->height;
	result->BMP.bmiHeader.biPlanes = 1;
	result->BMP.bmiHeader.biBitCount = 32;
	result->BMP.bmiHeader.biCompression = BI_RGB;
	result->BMP.bmiHeader.biSizeImage = 0;
	result->BMP.bmiHeader.biXPelsPerMeter = 0;
	result->BMP.bmiHeader.biYPelsPerMeter = 0;
	result->BMP.bmiHeader.biClrUsed = 0;
	result->BMP.bmiHeader.biClrImportant = 0;

	result->wDC = GetDC(result->hwnd);
	result->image = image;

	ShowWindow(result->hwnd, SW_SHOW);

	/*POINT ptDiff;
	RECT rcClient, rcWindow;
	GetClientRect(result->hwnd, &rcClient);
	GetWindowRect(result->hwnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(result->hwnd, rcWindow.left, rcWindow.top, ptDiff.x + image->width, ptDiff.y + image->height, TRUE);
	// */
	return result;
}

void flushWindow(GxWindow window) {
	GxImage src = window->image;
	SetDIBitsToDevice(window->wDC,
		0, 0, src->width, src->height,
		0, 0, 0, src->height,
		src->basePtr, &window->BMP, DIB_RGB_COLORS);
}

int getWindowEvent(GxWindow window, int *button, int *x, int *y) {
	static int btnMod = 0;
	static int keyMod = 0;

	MSG msg;
	msg.message = 0;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message != WM_MOUSEMOVE) {
			// consume mouse motion events
			break;
		}
	}
	*button = *x = *y = 0;
	switch (msg.message) {
		default:
			*button = msg.message;
			break;

		case WM_QUIT:
			return WINDOW_CLOSE;

		case WM_DESTROY:
			DispatchMessage(&msg);
			return WINDOW_CLOSE;

		case WM_ACTIVATE:
			if (msg.wParam == WA_INACTIVE) {
				DispatchMessage(&msg);
				return WINDOW_LEAVE;
			}
			// WA_ACTIVE or WA_CLICKACTIVE
			DispatchMessage(&msg);
			return WINDOW_ENTER;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			switch (msg.message) {
				case WM_LBUTTONDOWN:
					btnMod |= 1;
					break;

				case WM_RBUTTONDOWN:
					btnMod |= 2;
					break;

				case WM_MBUTTONDOWN:
					btnMod |= 4;
					break;
			}
			*button = btnMod;
			*x = LOWORD(msg.lParam);
			*y = HIWORD(msg.lParam);
			DispatchMessage(&msg);
			return MOUSE_PRESS;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			*button = btnMod;
			*x = LOWORD(msg.lParam);
			*y = HIWORD(msg.lParam);
			switch (msg.message) {
				case WM_LBUTTONUP:
					btnMod &= ~1;
					break;

				case WM_RBUTTONUP:
					btnMod &= ~2;
					break;

				case WM_MBUTTONUP:
					btnMod &= ~4;
					break;
			}
			DispatchMessage(&msg);
			return MOUSE_RELEASE;

		case WM_MOUSEMOVE:
			if (btnMod == 0) {
				break;
			}
			*button = btnMod;
			*x = LOWORD(msg.lParam);
			*y = HIWORD(msg.lParam);
			DispatchMessage(&msg);
			return MOUSE_MOTION;

		case WM_KEYDOWN:
			switch (msg.wParam) {
				case VK_SHIFT:
				case VK_LSHIFT:
				case VK_RSHIFT:
					keyMod |= KEY_MASK_SHIFT;
					break;

				case VK_CONTROL:
				case VK_LCONTROL:
				case VK_RCONTROL:
					keyMod |= KEY_MASK_CTRL;
					break;

				case VK_MENU:
				case VK_LMENU:
				case VK_RMENU:
					keyMod |= KEY_MASK_ALT;
					break;
			}

			*button = msg.wParam;
			*x = HIWORD(msg.lParam) & 127;
			*y = keyMod;
			DispatchMessage(&msg);
			return KEY_PRESS;

		case WM_KEYUP:
			switch (msg.wParam) {
				case VK_SHIFT:
				case VK_LSHIFT:
				case VK_RSHIFT:
					keyMod &= ~KEY_MASK_SHIFT;
					break;

				case VK_CONTROL:
				case VK_LCONTROL:
				case VK_RCONTROL:
					keyMod &= ~KEY_MASK_CTRL;
					break;

				case VK_MENU:
				case VK_LMENU:
				case VK_RMENU:
					keyMod &= ~KEY_MASK_ALT;
					break;
			}

			*button = msg.wParam;
			*x = HIWORD(msg.lParam) & 127;
			*y = keyMod;
			DispatchMessage(&msg);
			return KEY_RELEASE;
	}
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	(void) window;
	return 0;
}

void destroyWindow(GxWindow window) {
	free(window);
}

void setWindowTitle(GxWindow window, const char *title) {
	SetWindowTextA(window->hwnd, title);
}
