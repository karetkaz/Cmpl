#include "../gx_gui.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <sched.h>

struct GxWindow {
	Display *display;
	Window window;
	XImage *image;

	Atom deleteWindow;
	XGCValues gr_values;
	GC gc;
};

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

int getWindowEvent(GxWindow window, int *button, int *x, int *y, int timeout) {
	static int btnState = 0;
	static int keyState = 0;

	if (timeout != 0 && !XPending(window->display)) {
		// park thread for a while
		sched_yield();
		return 0;
	}

	XEvent event;
	XNextEvent(window->display, &event);

	// use the last motion event
	if (event.type == MotionNotify) {
		XEvent nextEvent;
		while (XPending(window->display)) {
			XPeekEvent(window->display, &nextEvent);
			if (nextEvent.type != MotionNotify) {
				break;
			}
			// consume mouse motion events
			XNextEvent(window->display, &event);
		}
	}

	// do nopt not process key release while key is pressed
	if (event.type == KeyRelease && XPending(window->display)) {
		XEvent nextEvent;
		XPeekEvent(window->display, &nextEvent);
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
				default: {
					char buffer[64] = {0};
					XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, 0);
					*button = buffer[0];
					if (*button == 0) {
						*button = keysym;
					}
					break;
				}

				case XK_Tab:
					*button = '\t';
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
