#include "../gx_gui.h"
#include <SDL2/SDL.h>
#include <stdlib.h>

#define SDL_Log(...) do { /*fprintf(stdout, ##__VA_ARGS__); fprintf(stdout, "\n");*/ } while(0)

struct GxWindow {
	SDL_Window *window;
	SDL_Surface *image;
	SDL_Surface *screen;
};

void initWindowing() {
	SDL_Init(SDL_INIT_VIDEO);
}

void quitWindowing() {
	SDL_Quit();
}

GxWindow createWindow(GxImage offs, const char *title) {
	if (offs == NULL) {
		return NULL;
	}

	GxWindow result = malloc(sizeof(struct GxWindow));
	if (result == NULL) {
		return NULL;
	}

	result->image = SDL_CreateRGBSurfaceFrom(offs->basePtr, offs->width, offs->height, offs->depth, offs->scanLen, rch(255), gch(255), bch(255), 0);
	result->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, offs->width, offs->height, SDL_WINDOW_HIDDEN);
	result->screen = SDL_GetWindowSurface(result->window);
	SDL_ShowWindow(result->window);
	return result;
}

void destroyWindow(GxWindow window) {
	SDL_FreeSurface(window->image);
	SDL_FreeSurface(window->screen);
	SDL_DestroyWindow(window->window);
	free(window);
}

int getWindowEvent(GxWindow window, int *button, int *x, int *y, int timeout) {
	SDL_Event event;
	if (!SDL_WaitEventTimeout(&event, timeout)) {
		return 0;
	}

	// use the last motion event
	if (event.type == SDL_MOUSEMOTION) {
		while (SDL_HasEvent(SDL_MOUSEMOTION)) {
			SDL_PollEvent(&event);
		}
	}

	// use the last motion event
	if (event.type == SDL_FINGERMOTION) {
		while (SDL_HasEvent(SDL_FINGERMOTION)) {
			SDL_PollEvent(&event);
		}
	}

	*button = *x = *y = 0;
	switch (event.type) {
		default:
			*button = event.type;
			break;

		case SDL_QUIT:
			return WINDOW_CLOSE;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {

				case SDL_WINDOWEVENT_CLOSE:
					//SDL_Log("Window %d closed", event.window.windowID);
					return WINDOW_CLOSE;

				case SDL_WINDOWEVENT_ENTER:
					//SDL_Log("Mouse entered window %d", event.window.windowID);
					return WINDOW_ENTER;

				case SDL_WINDOWEVENT_LEAVE:
					//SDL_Log("Mouse left window %d", event.window.windowID);
					return WINDOW_LEAVE;

				case SDL_WINDOWEVENT_SHOWN:
					SDL_Log("Window %d shown", event.window.windowID);
					break;

				case SDL_WINDOWEVENT_HIDDEN:
					SDL_Log("Window %d hidden", event.window.windowID);
					break;

				case SDL_WINDOWEVENT_EXPOSED:
					// SDL_Log("Window %d exposed", event.window.windowID);
					flushWindow(window);
					break;

				case SDL_WINDOWEVENT_MOVED:
					SDL_Log("Window %d moved to %d,%d", event.window.windowID, event.window.data1, event.window.data2);
					break;

				case SDL_WINDOWEVENT_RESIZED:
					SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window
						.data2
					);
					break;

				case SDL_WINDOWEVENT_SIZE_CHANGED:
					SDL_Log("Window %d size changed to %dx%d", event.window.windowID, event.window.data1, event.window
						.data2
					);
					break;

				case SDL_WINDOWEVENT_MINIMIZED:
					SDL_Log("Window %d minimized", event.window.windowID);
					break;

				case SDL_WINDOWEVENT_MAXIMIZED:
					SDL_Log("Window %d maximized", event.window.windowID);
					break;

				case SDL_WINDOWEVENT_RESTORED:
					SDL_Log("Window %d restored", event.window.windowID);
					break;

				case SDL_WINDOWEVENT_FOCUS_GAINED:
					SDL_Log("Window %d gained keyboard focus", event.window.windowID);
					break;

				case SDL_WINDOWEVENT_FOCUS_LOST:
					SDL_Log("Window %d lost keyboard focus", event.window.windowID);
					break;

				default:
					SDL_Log("Window %d got unknown event %d", event.window.windowID, event.window.event);
					break;
			}
			break;

		case SDL_FINGERDOWN:
			*button = event.tfinger.fingerId;
			*x = event.tfinger.x * window->image->w;
			*y = event.tfinger.y * window->image->h;
			return FINGER_PRESS;

		case SDL_FINGERUP:
			*button = event.tfinger.fingerId;
			*x = event.tfinger.x * window->image->w;
			*y = event.tfinger.y * window->image->h;
			return FINGER_RELEASE;

		case SDL_FINGERMOTION:
			*button = event.tfinger.fingerId;
			*x = event.tfinger.x * window->image->w;
			*y = event.tfinger.y * window->image->h;
			return FINGER_MOTION;

		case SDL_MOUSEBUTTONDOWN:
			if (event.button.which == SDL_TOUCH_MOUSEID) {
				return 0;
			}
			*button = event.button.button;
			*x = event.button.x;
			*y = event.button.y;
			return MOUSE_PRESS;

		case SDL_MOUSEBUTTONUP:
			if (event.button.which == SDL_TOUCH_MOUSEID) {
				return 0;
			}
			*button = event.button.button;
			*x = event.button.x;
			*y = event.button.y;
			return MOUSE_RELEASE;

		case SDL_MOUSEMOTION:
			if (event.motion.which == SDL_TOUCH_MOUSEID) {
				return 0;
			}
			*button = event.motion.state;
			*x = event.motion.x;
			*y = event.motion.y;
			return MOUSE_MOTION;

		case SDL_MOUSEWHEEL:
			*button = event.wheel.direction;
			*x = event.wheel.x;
			*y = event.wheel.y;
			return GESTURE_SCROLL;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			*button = event.key.keysym.sym;
			*x = event.key.keysym.scancode;
			*y = 0;
			if (event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
				*y |= KEY_MASK_SHIFT;
			}
			if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
				*y |= KEY_MASK_CTRL;
			}
			if (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT)) {
				*y |= KEY_MASK_ALT;
			}
			switch (event.type) {
				case SDL_KEYDOWN:
					return KEY_PRESS;

				case SDL_KEYUP:
					return KEY_RELEASE;
			}
			break;
	}
	return 0;
}

void setWindowTitle(GxWindow window, const char *caption) {
	SDL_SetWindowTitle(window->window, caption);
}

void flushWindow(GxWindow window) {
	SDL_BlitSurface(window->image, NULL, window->screen, NULL);
	SDL_UpdateWindowSurface(window->window);
}

const int KEY_CODE_ESC = SDLK_ESCAPE;
const int KEY_CODE_BACKSPACE = SDLK_BACKSPACE;
const int KEY_CODE_TAB = SDLK_TAB;
const int KEY_CODE_RETURN = SDLK_RETURN;
const int KEY_CODE_CAPSLOCK = SDLK_CAPSLOCK;
const int KEY_CODE_PRINT_SCREEN = SDLK_PRINTSCREEN;
const int KEY_CODE_SCROLL_LOCK = SDLK_SCROLLLOCK;
const int KEY_CODE_PAUSE = SDLK_PAUSE;
const int KEY_CODE_INSERT = SDLK_INSERT;
const int KEY_CODE_HOME = SDLK_HOME;
const int KEY_CODE_PAGE_UP = SDLK_PAGEUP;
const int KEY_CODE_DELETE = SDLK_DELETE;
const int KEY_CODE_END = SDLK_END;
const int KEY_CODE_PAGE_DOWN = SDLK_PAGEDOWN;
const int KEY_CODE_RIGHT = SDLK_RIGHT;
const int KEY_CODE_LEFT = SDLK_LEFT;
const int KEY_CODE_DOWN = SDLK_DOWN;
const int KEY_CODE_UP = SDLK_UP;
const int KEY_CODE_L_SHIFT = SDLK_LSHIFT;
const int KEY_CODE_R_SHIFT = SDLK_RSHIFT;
const int KEY_CODE_L_CTRL = SDLK_LCTRL;
const int KEY_CODE_R_CTRL = SDLK_RCTRL;
const int KEY_CODE_L_ALT = SDLK_LALT;
const int KEY_CODE_R_ALT = SDLK_RALT;
const int KEY_CODE_L_GUI = SDLK_LGUI;
const int KEY_CODE_R_GUI = SDLK_RGUI;
