#include <windows.h>
#include <winuser.h>
#include "g2_surf.h"

static HWND hWnd = 0;
static HINSTANCE  mainhins = 0;
static char *class_name = "Library_BMP_Window";

#ifdef __OLD
static HDC mDC;
static void* PIX;
static HBITMAP MEM;
#endif

static BITMAPINFO BMP;
static int (*kbdH)(int,int) = 0;
static void (*ratH)(int,int,int) = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_MOUSEMOVE : {
			if (ratH) ratH(wParam, LOWORD(lParam), HIWORD(lParam));
		} return 0;
		case WM_CHAR : 
		//~ case WM_KEYDOWN : {
			return kbdH(lParam, wParam);
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int initWIN(int w, int h, int (*kbd)(int,int), void (*rat)(int,int,int)) {
	WNDCLASS windowClass;
	windowClass.style = CS_OWNDC; // The style of the window. CS_OWNDC means every window has it's own DC
	windowClass.lpfnWndProc = DefWindowProc;//bitmapWindowHandler; // The function to call when this window receives a message
	windowClass.cbClsExtra = 0; // Extra bytes to allocate for this class (none)
	windowClass.cbWndExtra = 0; // Extra bytes to allocate for each window (none)
	windowClass.hInstance = mainhins; // This application's instance
	windowClass.hIcon = LoadIcon(mainhins, IDI_APPLICATION); // A standard Icon
	windowClass.hCursor = LoadCursor(mainhins, IDC_ARROW); // A standard cursor
	windowClass.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE; // A standard background
	windowClass.lpszMenuName = NULL; // No menus in this window
	windowClass.lpszClassName = class_name; // A name for this class
	windowClass.lpfnWndProc	= (WNDPROC)WndProc;

	RegisterClass(&windowClass); // Register the class
	hWnd = CreateWindow(
		"Library_BMP_Window",		// Name of the window class (registered above)
		"Window",			// Name of the window, appears in the window title bar
		WS_BORDER | WS_MINIMIZEBOX | \
		WS_CAPTION | WS_SYSMENU,	// Window style
		CW_USEDEFAULT,			// X coordinate of the window on-screen
		CW_USEDEFAULT,			// Y coordinate of the window on-screen
		w,				// width of the window
		h,				// height of the window
		NULL,				// Handle to a parent window (none)
		NULL,				// Handle to a child window (none)
		mainhins,			// Handle to the current Instance
		0					// Pointer to extra data I don't care about
	);
	BMP.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BMP.bmiHeader.biWidth = w;
	BMP.bmiHeader.biHeight = h;
	BMP.bmiHeader.biPlanes = 1;
	BMP.bmiHeader.biBitCount = 32;
	BMP.bmiHeader.biCompression = BI_RGB;
	BMP.bmiHeader.biSizeImage = 0;
	BMP.bmiHeader.biXPelsPerMeter = 0;
	BMP.bmiHeader.biYPelsPerMeter = 0;
	BMP.bmiHeader.biClrUsed = 0;
	BMP.bmiHeader.biClrImportant = 0;
	#ifdef __OLD
	mDC = CreateCompatibleDC(0);
	MEM = CreateDIBSection(mDC, &BMP, DIB_RGB_COLORS, &PIX, NULL, 0);
	#else
	BMP.bmiHeader.biHeight = -h;
	#endif
	//~ wDC = GetDC(hWnd);
	kbdH = kbd;
	ratH = rat;
	ShowWindow(hWnd, SW_SHOW);
	
	return 0;
}

void setCaption(char* str) {
	SetWindowTextA(hWnd, str);
}

void doneWin(void) {
#ifdef __OLD
	DeleteObject(MEM);
	DeleteDC(mDC);
#endif
	printf("done\n");
}

int gx_wincopy(gx_Surf *src, int unused) {
	if(hWnd != 0) {
#ifdef __OLD
		int y, w, h;
		HBITMAP hbmOld = SelectObject(mDC, MEM);
		cblt_proc cvt = gx_getcbltf(cblt_conv, 32, src->depth);
		DWORD *dpix = (DWORD *)PIX + (BMP.bmiHeader.biHeight * BMP.bmiHeader.biWidth);
		char *spix = src->basePtr;

		w = src->width < BMP.bmiHeader.biWidth ? src->width : BMP.bmiHeader.biWidth;
		h = src->height < BMP.bmiHeader.biHeight ? src->height : BMP.bmiHeader.biHeight;
		if (!PIX || !spix || !cvt || w <= 0 || h <= 0) return -1;
		for(y = 0; y < h; ++y) {
			dpix -= BMP.bmiHeader.biWidth;
			gx_callcbltf(cvt, dpix, spix, w, 0);
			spix += src->scanLen;
		}
		BitBlt(GetDC(hWnd), 0, 0, BMP.bmiHeader.biWidth, BMP.bmiHeader.biHeight, mDC, 0, 0, SRCCOPY);
		SelectObject(mDC, hbmOld);
#else
		SetDIBitsToDevice( GetDC(hWnd), 
			0, 0, src->width, src->height,
			0, 0, 0, src->height,
			src->basePtr, &BMP, DIB_RGB_COLORS);

#endif
	}
	return 0;
}

int winmsg(int a) {
	MSG msg;
	msg.wParam = 0;
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return -1;
		}
		else if (a) {
				TranslateMessage(&msg);
		}
		return DispatchMessage(&msg);
	}
	return 0;
	//~ return act;
}

int init_WIN(gx_Surf *offs, flip_scr *flip, peek_msg *msgp, void (**done)(void), void ratHND(int btn, int mx, int my), int kbdHND(int lkey, int key)) {
	initWIN(SCRW, SCRH, kbdHND, ratHND);
	*msgp = winmsg;
	*done = doneWin;
	*flip = gx_wincopy;
	return 0;
}

