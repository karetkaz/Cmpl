#include "../gx_gui.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/poll.h>
#include <stdlib.h>

struct GxWindow {
	Display *display;
	Window window;
	XImage *image;

	Atom deleteWindow;
	XGCValues gr_values;
	GC gc;
};

void initWindowing() {
}

void quitWindowing() {
}

GxWindow createWindow(GxImage offs, const char *title) {
	if (offs == NULL) {
		return NULL;
	}

	GxWindow result = malloc(sizeof(struct GxWindow));
	if (result == NULL) {
		return NULL;
	}

	result->display = XOpenDisplay(NULL);
	int screen = DefaultScreen(result->display);
	int depth = DefaultDepth(result->display, screen);

	result->window = XCreateSimpleWindow(result->display, XDefaultRootWindow(result->display), 0, 0, offs->width, offs->height, 0, BlackPixel(result->display, 0), WhitePixel(result->display, 0));
	result->image = XCreateImage(result->display, CopyFromParent, depth, ZPixmap, 0, offs->basePtr, offs->width, offs->height, 32, offs->scanLen);
	result->gc = XCreateGC(result->display, result->window, GCForeground, &result->gr_values);
	XSetStandardProperties(result->display, result->window, title, "cmpl", None, NULL, 0, NULL);
	XMapWindow(result->display, result->window);

	XSelectInput(result->display, result->window, ExposureMask
		| ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask
		| EnterWindowMask | LeaveWindowMask   // mouse enter and leave the window
		| KeyPressMask | KeyReleaseMask
	);

	// handle window close event
	result->deleteWindow = XInternAtom(result->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(result->display, result->window, &result->deleteWindow, 1);

	// window is not resizeable
	XSizeHints *xsh = XAllocSizeHints();
	XWindowAttributes xwa;
	xsh->flags = PMinSize | PMaxSize;
	XGetWindowAttributes(result->display, result->window, &xwa);
	xsh->min_width = xwa.width;
	xsh->max_width = xwa.width;
	xsh->min_height = xwa.height;
	xsh->max_height = xwa.height;
	XSetWMNormalHints(result->display, result->window, xsh);
	XFree(xsh);

	return result;
}

void destroyWindow(GxWindow window) {
	if (window->image) {
		window->image->data = NULL;
		XDestroyImage(window->image);
	}

	if (window->window) {
		XDestroyWindow(window->display, window->window);
		XFreeGC(window->display, window->gc);
		XFlush(window->display);
		window->window = 0;
	}

	if (window->display) {
		XCloseDisplay(window->display);
	}

	free(window);
}

int getWindowEvent(GxWindow window, int *button, int *x, int *y, int timeout) {
	static int btnState = 0;
	static int keyState = 0;

	Display *display = window->display;

	if (!XPending(display)) {
		// based on: https://nrk.neocities.org/articles/x11-timeout-with-xsyncalarm
		struct pollfd pfd = {
			.fd = ConnectionNumber(display),
			.events = POLLIN,
		};

		if (poll(&pfd, 1, timeout) >= 0) {
			return 0;
		}
	}

	XEvent event;
	XNextEvent(display, &event);

	// use the last motion event
	if (event.type == MotionNotify) {
		XEvent nextEvent;
		while (XPending(display)) {
			XPeekEvent(display, &nextEvent);
			if (nextEvent.type != MotionNotify) {
				break;
			}
			// consume mouse motion events
			XNextEvent(display, &event);
		}
	}

	// do nopt not process key release while key is pressed
	if (event.type == KeyRelease && XPending(display)) {
		XEvent nextEvent;
		XPeekEvent(display, &nextEvent);
		if (nextEvent.type == KeyPress && nextEvent.xkey.time == event.xkey.time && nextEvent.xkey.keycode == event.xkey.keycode) {
			return 0;
		}
	}

	*button = *x = *y = 0;
	switch (event.type) {
		default:
			*button = event.type;
			break;

		case Expose:
			flushWindow(window);
			return 0;

		case ClientMessage:
			if ((Atom)event.xclient.data.l[0] == window->deleteWindow) {
				return WINDOW_CLOSE;
			}
			break;

		case DestroyNotify:
			return WINDOW_CLOSE;

		case EnterNotify:
			return WINDOW_ENTER;

		case LeaveNotify:
			return WINDOW_LEAVE;

		case ButtonPress:
			switch (event.xbutton.button) {
				case 1:
					btnState |= 1;
					break;
				case 2:
					btnState |= 4;
					break;
				case 3:
					btnState |= 2;
					break;
			}
			*button = btnState;
			*x = event.xmotion.x;
			*y = event.xmotion.y;
			return MOUSE_PRESS;

		case ButtonRelease:
			*button = btnState;
			*x = event.xmotion.x;
			*y = event.xmotion.y;
			switch (event.xbutton.button) {
				case 1:
					btnState &= ~1;
					break;
				case 2:
					btnState &= ~4;
					break;
				case 3:
					btnState &= ~2;
					break;
			}
			return MOUSE_RELEASE;

		case MotionNotify:
			*button = btnState;
			*x = event.xmotion.x;
			*y = event.xmotion.y;
			return MOUSE_MOTION;

		case KeyPress:
		case KeyRelease: {
			KeySym keysym = XLookupKeysym(&event.xkey, 0);
			switch (keysym) {
				default:
					*button = keysym;
					break;

				case XK_Escape:
				case XK_BackSpace:
				case XK_Tab:
				case XK_Return:
				case XK_Caps_Lock:
				case XK_Print:
				case XK_Scroll_Lock:
				case XK_Pause:
				case XK_Insert:
				case XK_Home:
				case XK_Page_Up:
				case XK_Delete:
				case XK_End:
				case XK_Page_Down:
				case XK_Right:
				case XK_Left:
				case XK_Down:
				case XK_Up:
				case XK_Super_L:
				case XK_Super_R:
					*button = keysym;
					break;

				case XK_Shift_L:
				case XK_Shift_R:
					*button = keysym;
					if (event.type == KeyRelease) {
						keyState &= ~KEY_MASK_SHIFT;
					} else {
						keyState |= KEY_MASK_SHIFT;
					}
					break;

				case XK_Control_L:
				case XK_Control_R:
					*button = keysym;
					if (event.type == KeyRelease) {
						keyState &= ~KEY_MASK_CTRL;
					} else {
						keyState |= KEY_MASK_CTRL;
					}
					break;

				case XK_Alt_L:
				case XK_Alt_R:
					*button = keysym;
					if (event.type == KeyRelease) {
						keyState &= ~KEY_MASK_ALT;
					} else {
						keyState |= KEY_MASK_ALT;
					}
					break;
			}

			*x = event.xkey.keycode;
			*y = keyState;
			if (event.type == KeyRelease) {
				return KEY_RELEASE;
			}
			return KEY_PRESS;
		}
	}
	return 0;
}

void setWindowTitle(GxWindow window, const char *title) {
	XSetStandardProperties(window->display, window->window, title, "cmpl", None, NULL, 0, NULL);
}

void flushWindow(GxWindow window) {
	XImage *screen = window->image;
	XPutImage(window->display, window->window, window->gc, screen, 0, 0, 0, 0, screen->width, screen->height);
	XFlush(window->display);
}

const int KEY_CODE_ESC = XK_Escape;
const int KEY_CODE_BACKSPACE = XK_BackSpace;
const int KEY_CODE_TAB = XK_Tab;
const int KEY_CODE_RETURN = XK_Return;
const int KEY_CODE_CAPSLOCK = XK_Caps_Lock;
const int KEY_CODE_PRINT_SCREEN = XK_Print;
const int KEY_CODE_SCROLL_LOCK = XK_Scroll_Lock;
const int KEY_CODE_PAUSE = XK_Pause;
const int KEY_CODE_INSERT = XK_Insert;
const int KEY_CODE_HOME = XK_Home;
const int KEY_CODE_PAGE_UP = XK_Page_Up;
const int KEY_CODE_DELETE = XK_Delete;
const int KEY_CODE_END = XK_End;
const int KEY_CODE_PAGE_DOWN = XK_Page_Down;
const int KEY_CODE_RIGHT = XK_Right;
const int KEY_CODE_LEFT = XK_Left;
const int KEY_CODE_DOWN = XK_Down;
const int KEY_CODE_UP = XK_Up;
const int KEY_CODE_L_SHIFT = XK_Shift_L;
const int KEY_CODE_R_SHIFT = XK_Shift_R;
const int KEY_CODE_L_CTRL = XK_Control_L;
const int KEY_CODE_R_CTRL = XK_Control_R;
const int KEY_CODE_L_ALT = XK_Alt_L;
const int KEY_CODE_R_ALT = XK_Alt_R;
const int KEY_CODE_L_GUI = XK_Super_L;
const int KEY_CODE_R_GUI = XK_Super_R;
