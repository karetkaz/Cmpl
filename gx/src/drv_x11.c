#include "g2_surf.h"

// TODO
static int (*kbdH)(int,int) = 0;
static void (*ratH)(int,int,int) = 0;
static int do_quit = 0;

static void *event_thread(void *arg) {
	static int is_first = 1;
	arg = arg;

	while (1) {

		XEvent x_event;

		XNextEvent(g_p_display, &x_event);

		switch (x_event.type) {
			case Expose:
				if (!is_first) {
					cwn_driver_flush_fb(g_p_fb, x_event.xexpose.x, x_event.xexpose.y, x_event.xexpose.width, x_event.xexpose.height);
				}
				else {
					is_first = 0;
				}
				break;

			case ButtonPress:
				cwn_driver_usr_touch_proc(x_event.xbutton.x, x_event.xbutton.y, CWN_TOUCH_DOWN);
				break;

			case ButtonRelease:
				cwn_driver_usr_touch_proc(x_event.xbutton.x, x_event.xbutton.y, CWN_TOUCH_UP);
				break;

			case MotionNotify:
				cwn_driver_usr_touch_proc(x_event.xbutton.x, x_event.xbutton.y, CWN_TOUCH_DOWN_LONG);
				break;

			default:
				break;
		}

		//~ cwn_driver_touch_proc();

		if (do_quit) break;
	}

	pthread_exit(NULL);
	return NULL;
}

static int create_window(int width, int height) {
	if (g_i_is_win_closed) {

		g_p_display = XOpenDisplay(NULL);

		g_root_window = XDefaultRootWindow(g_p_display);

		g_window = XCreateSimpleWindow(g_p_display, g_root_window, 100, 100, width, height, 2, BlackPixel(g_p_display, 0), WhitePixel(g_p_display, 0));

		// register events
		XSelectInput(g_p_display, g_window, ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

		XMapWindow(g_p_display, g_window);

		XFlush(g_p_display);

		g_image_fb = XCreateImage(g_p_display, DefaultVisual(g_p_display, DefaultScreen(g_p_display)), DefaultDepth(g_p_display, DefaultScreen(g_p_display)), ZPixmap, 0, (char*)g_p_fb32, CWN_FB_WIDTH, CWN_FB_HEIGHT, 32, CWN_FB_WIDTH*4);

		g_i_is_win_closed = 0;
	}

	return 1;
}

static int close_window(void) {

	if (!g_i_is_win_closed) {

		//~ g_image_fb->data = NULL; // static data must not be freed.
		//~ XDestroyImage(g_image_fb);

		XDestroyWindow(g_p_display, g_window);

		XFlush(g_p_display);

		XCloseDisplay(g_p_display);

		g_i_is_win_closed = 1;
	}

	return 1;
}

int gx_wincopy(gx_Surf src, int unused) {
	//~ if(hWnd != 0) {
		//~ SetDIBitsToDevice( wDC,
			//~ 0, 0, src->width, src->height,
			//~ 0, 0, 0, src->height,
			//~ src->basePtr, &BMP, DIB_RGB_COLORS);
		XPutImage(g_p_display, g_window, g_gc, g_image_fb, x, y, x, y, w, h);
		XFlush(g_p_display);
	//~ }
	return 0;
}





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

void ClientResize(HWND hWnd, int nWidth, int nHeight) {
	POINT ptDiff;
	RECT rcClient, rcWindow;
	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd,rcWindow.left, rcWindow.top, nWidth + ptDiff.x, nHeight + ptDiff.y, TRUE);
}

int initWIN(int w, int h, int (*kbd)(int,int), void (*rat)(int,int,int)) {
	POINT ptDiff;
	RECT rcClient, rcWindow;
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
		class_name,			// Name of the window class (registered above)
		"Window",			// Name of the window, appears in the window title bar
		WS_MINIMIZEBOX | WS_SYSMENU,	// Window style
		CW_USEDEFAULT,			// X coordinate of the window on-screen
		CW_USEDEFAULT,			// Y coordinate of the window on-screen
		w,					// width of the window
		h,					// height of the window
		NULL,				// Handle to a parent window (none)
		NULL,				// Handle to a child window (none)
		mainhins,			// Handle to the current Instance
		0					// Pointer to extra data I don't care about
	);
	BMP.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BMP.bmiHeader.biWidth = w;
	BMP.bmiHeader.biHeight = -h;
	BMP.bmiHeader.biPlanes = 1;
	BMP.bmiHeader.biBitCount = 32;
	BMP.bmiHeader.biCompression = BI_RGB;
	BMP.bmiHeader.biSizeImage = 0;
	BMP.bmiHeader.biXPelsPerMeter = 0;
	BMP.bmiHeader.biYPelsPerMeter = 0;
	BMP.bmiHeader.biClrUsed = 0;
	BMP.bmiHeader.biClrImportant = 0;

	wDC = GetDC(hWnd);
	kbdH = kbd;
	ratH = rat;
	ShowWindow(hWnd, SW_SHOW);
	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd, rcWindow.left, rcWindow.top, ptDiff.x + w, ptDiff.y + h, TRUE);
	return 0;
}

void setCaption(char* str) {
	SetWindowTextA(hWnd, str);
}

void doneWin(void) {
	printf("done\n");
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

int init_WIN(gx_Surf offs, flip_scr *flip, peek_msg *msgp, void (**done)(void), void ratHND(int btn, int mx, int my), int kbdHND(int lkey, int key)) {
	initWIN(offs->width, offs->height, kbdHND, ratHND);
	*flip = gx_wincopy;
	*done = doneWin;
	*msgp = winmsg;
	return 0;
}

