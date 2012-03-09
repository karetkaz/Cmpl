#include "drv_gui.h"

static int (*kbdH)(int/* , int */) = 0;
static int (*ratH)(int, int, int) = 0;

#if defined(WIN32)
#include <windows.h>
#include <winuser.h>

static HDC wDC = 0;
static HWND hWnd = 0;
static HINSTANCE mainhins = 0;
static char *class_name = "GFX(2/3)D_GDI_Window";

static BITMAPINFO BMP;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {

		case WM_MOUSEMOVE :
			//~ if (!wParam) goto defcase;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP: {
			static int btnstate = 0;
			switch (message) {
				case WM_LBUTTONDOWN: btnstate |= 1; break;
				case WM_MBUTTONDOWN: btnstate |= 4; break;
				case WM_RBUTTONDOWN: btnstate |= 2; break;
				case WM_LBUTTONUP: btnstate &= ~1; break;
				case WM_MBUTTONUP: btnstate &= ~4; break;
				case WM_RBUTTONUP: btnstate &= ~2; break; 
			}
			return ratH ? ratH(btnstate, LOWORD(lParam), HIWORD(lParam)) : 0;
		}

		case WM_MOUSEWHEEL : {
			POINT P;
			P.x = LOWORD(lParam);
			P.y = HIWORD(lParam);
			if (ScreenToClient(hWnd, &P)) {
				//~ printf("%d, %d")
				return ratH ? ratH(wParam, P.x, P.y) : 0;
			}
			return 0;
			//~ return ratH ? ratH(wParam, LowWord(lParam), HighWord(lParam)) : 0;
			//~ return ratH ? ratH(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) : 0;
		}
		case WM_CHAR :
			return kbdH ? kbdH(wParam/*, lParam*/) : 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		//~ defcase:
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void resize_window(int nWidth, int nHeight) {
	POINT ptDiff;
	RECT rcClient, rcWindow;
	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd,rcWindow.left, rcWindow.top, nWidth + ptDiff.x, nHeight + ptDiff.y, TRUE);
}// */

static int create_window(int w, int h) {
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
	ShowWindow(hWnd, SW_SHOW);
	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd, rcWindow.left, rcWindow.top, ptDiff.x + w, ptDiff.y + h, TRUE);
	return 1;
}

static int copy2_window(gx_Surf src, int unused) {
	if(hWnd != 0) {
		SetDIBitsToDevice( wDC,
			0, 0, src->width, src->height,
			0, 0, 0, src->height,
			src->basePtr, &BMP, DIB_RGB_COLORS);
	}
	return 0;
}

static int winmsg(int wait) {
	MSG msg;
	msg.wParam = 0;

	if (wait)
		WaitMessage();

	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return -1;
		}
		TranslateMessage(&msg);
		return DispatchMessage(&msg);
	}
	return 0;
}

int initWin(gx_Surf offs, flip_scr *flip, peek_msg *msgp, int ratHND(int btn, int mx, int my), int kbdHND(int key/* , int lkey */)) {
	if (offs && create_window(offs->width, offs->height)) {
		*flip = copy2_window;
		*msgp = winmsg;
		kbdH = kbdHND;
		ratH = ratHND;
	}
	return 0;
}

void doneWin(void) {
	//~ debug("done");
}

void setCaption(char* str) {
	SetWindowTextA(hWnd, str);
}

#elif defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Display *display = NULL;
static Window window = 0;
static XImage *g_image = NULL;

static GC gc;
static XGCValues gcvalues;

static int create_window(int width, int height) {

	static Atom wmDelete;
	void *fb32 = NULL;

	if (!window) {

		window = XCreateSimpleWindow(display, XDefaultRootWindow(display), 100, 100, width, height, 2, BlackPixel(display, 0), WhitePixel(display, 0));

		wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(display, window, &wmDelete, 1);

		// register events
		XSelectInput(display, window, KeyPressMask | ExposureMask
			| ButtonPressMask | ButtonReleaseMask
			| PointerMotionMask
			//~ | ButtonMotionMask
			);

		XMapWindow(display, window);

		XFlush(display);

		g_image = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)), DefaultDepth(display, DefaultScreen(display)), ZPixmap, 0, (char *)fb32, width, height, 32, width * 4);

		gc = XCreateGC(display, window, GCForeground, &gcvalues);

		XSizeHints *size_hints = XAllocSizeHints();
		if (size_hints != NULL) {
			size_hints->flags=USPosition | PAspect | PMinSize | PMaxSize;
			size_hints->min_width=0;
			size_hints->min_height=0;
			size_hints->max_width=65535;
			size_hints->max_height=65535;
			//~ size_hints->min_aspect.x=19;
			//~ size_hints->max_aspect.x=size_hints->min_aspect.x;
			//~ size_hints->min_aspect.y=8;
			//~ size_hints->max_aspect.y=size_hints->min_aspect.y;

			XSetWMNormalHints(display, window, size_hints);
		}
	}

	return 1;
}

static int close_window(void) {

	if (window) {
		XDestroyWindow(display, window);
		XFlush(display);
		window = 0;
	}

	return 1;
}

static int copy_window(gx_Surf src, int unused) {
	int x = 0, y = 0;
	int w = src->width;
	int h = src->height;

	XImage image = *g_image;
	image.data = src->basePtr;
	image.width = src->width;
	image.height = src->height;
	image.bytes_per_line = src->scanLen;
	image.bits_per_pixel = src->depth;

	XPutImage(display, window, gc, &image, x, y, x, y, w, h);
	XFlush(display);
	return 0;
}

static int winmsg(int wait) {
	XEvent event;
	int result = 0;

	if (wait)
		XPeekEvent(display, &event);

	while (XPending(display)) {

		XNextEvent(display, &event);

		switch (event.type) {

			case ClientMessage: return -1;
				//~ post = post_msg = -1;
				//~ printf("> Event:ClientMessage\n");
				//~ break;

			case Expose:
				//~ printf("> Event:Expose\n");
				break;

			case ButtonPress:
			case ButtonRelease:
			case MotionNotify: {
				static int btnstate = 0;
				switch (event.type) {
					case ButtonPress: switch (event.xbutton.button) {
						case 1: btnstate |= 1; break;
						case 2: btnstate |= 4; break;
						case 3: btnstate |= 2; break;
					} break;
					case ButtonRelease: switch (event.xbutton.button) {
						case 1: btnstate &= ~1; break;
						case 2: btnstate &= ~4; break;
						case 3: btnstate &= ~2; break;
					} break;
				}
				if (ratH) result = ratH(btnstate, event.xmotion.x, event.xmotion.y);
			} break;

			case KeyPress: {
				KeySym keysym = XLookupKeysym(&(event.xkey), 0);
				if (kbdH) {
					char buffer[2];
					int nchars = XLookupString(&(event.xkey), buffer, sizeof(buffer), &keysym, NULL);
					if (nchars == 1) {
						//~ printf("> Key[%d] '%c' pressed\n", *buffer, *buffer);
						result = kbdH(*buffer/* , event.xkey.state */);
					}
				}
			} break;

			default:
				break;
		}
	}
	return result;
}

void setCaption(char *str) {
	XSetStandardProperties(display, window, str, "3d", None, NULL, 0, NULL);
}

void doneWin(void) {

	if (window)
		close_window();

	if (display)
		XCloseDisplay(display);

	if (g_image) {
		g_image->data = NULL; // static data must not be freed.
		XDestroyImage(g_image);
	}

	//~ pthread_join(thread_id, NULL);
}

int initWin(gx_Surf offs, flip_scr *flip, peek_msg *msgp, int ratHND(int btn, int mx, int my), int kbdHND(int key/* , int lkey */)) {
	/*if (XInitThreads() == 0) {
		printf("WARNING - cannot support multi threads\n");
		return -2;
	}// */

	display = XOpenDisplay(NULL);

	if (offs && create_window(offs->width, offs->height)) {
		*flip = copy_window;
		*msgp = winmsg;
		kbdH = kbdHND;
		ratH = ratHND;
		/*if (pthread_create(&thread_id, NULL, event_thread, NULL) != 0) {
			printf("ERROR - cannot create event thread for X11\n");
			return -3;
		}// */
		return 0;
	}
	return offs ? -1 : 0;
}

#else
#error UNSUPPORTED PLATFORM
#endif
