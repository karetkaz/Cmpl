#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "../gx_gui.h"

static int (*kbdH)(int, int) = 0;
static int (*ratH)(int, int, int) = 0;

static Display *display = NULL;
static Window window = 0;
static XImage *g_image = NULL;

static GC gc;

static int create_window(int width, int height) {
	display = XOpenDisplay(NULL);
	int screen = DefaultScreen(display);
	int depth = DefaultDepth(display, screen);
	Visual *visual = DefaultVisual(display, screen);

	XSetWindowAttributes    frame_attributes;
	XGCValues               gr_values;

	g_image = XCreateImage(display, CopyFromParent, depth, ZPixmap, 0, NULL, width, height, 32, 0);
	window = XCreateSimpleWindow(display, XDefaultRootWindow(display), 100, 100, width, height, 2, BlackPixel(display, 0), WhitePixel(display, 0));
	//frame_attributes.background_pixel = XWhitePixel(display, 0);

	window = XCreateWindow(display, XRootWindow(display, 0),
								 0, 0, width, height, 5, depth,
								 InputOutput, visual, CWBackPixel,
								 &frame_attributes);

	XSelectInput(display, window,  StructureNotifyMask | SubstructureNotifyMask | ExposureMask |
		ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask |
		VisibilityChangeMask | EnterWindowMask | LeaveWindowMask |
		PointerMotionMask | ButtonMotionMask
	);

	gc = XCreateGC(display, window, GCForeground, &gr_values);
	XMapWindow(display, window);
	XFlush(display);

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
	(void)unused;
	return 0;
}

static int winmsg(int wait) {
	XEvent event;
	int result = 0;
	static int btnstate = 0;

	if (wait)
		XPeekEvent(display, &event);

	while (XPending(display)) {

		XNextEvent(display, &event);

		switch (event.type) {

			case ClientMessage:
				return -1;
				//~ post = post_msg = -1;
				//~ printf("> Event:ClientMessage\n");
				//~ break;

			case Expose:
				//~ printf("> Event:Expose\n");
				break;

			case ButtonRelease:
			case ButtonPress:
			case MotionNotify: {
				switch (event.type) {
					case ButtonPress:
						switch (event.xbutton.button) {
							case 1: btnstate |= 1; break;
							case 2: btnstate |= 4; break;
							case 3: btnstate |= 2; break;
						}
						break;
					case ButtonRelease: 
						switch (event.xbutton.button) {
							case 1: btnstate &= ~1; break;
							case 2: btnstate &= ~4; break;
							case 3: btnstate &= ~2; break;
						}
						break;
						//~ return result;
				}
				if (ratH) result = ratH(btnstate, event.xmotion.x, event.xmotion.y);
			} break;

			//~ case KeyRelease:
			case KeyPress: {
				KeySym keysym = XLookupKeysym(&(event.xkey), 0);
				if (kbdH) {
					char buffer[1];
					//int nchars = 
					XLookupString(&(event.xkey), buffer, sizeof(buffer), &keysym, NULL);
					result = kbdH(*buffer, event.xkey.state);

					/*
					if (nchars == 1)
					{
						result = kbdH(*buffer, event.xkey.state);
					}
					else {
						result = kbdH(*buffer + (buffer[1] << 8), event.xkey.state);
					}// */
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

int initWin(gx_Surf offs, flip_scr *flip, peek_msg *msgp, int ratHND(int btn, int mx, int my), int kbdHND(int key, int state)) {
	/*if (XInitThreads() == 0) {
		printf("WARNING - cannot support multi threads\n");
		return -2;
	}// */

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
