#include "gx_surf.h"

#define KEY_PRESS 1
#define KEY_RELEASE 2
#define MOUSE_WHEEL 3
#define MOUSE_PRESS 4
#define MOUSE_MOTION 5
#define MOUSE_RELEASE 6
#define FINGER_PRESS 7
#define FINGER_MOTION 8
#define FINGER_RELEASE 9
#define EVENT_TIMEOUT 10
#define WINDOW_INIT 100
#define WINDOW_CLOSE 102
#define WINDOW_ENTER 103
#define WINDOW_LEAVE 104

extern const int KEY_CODE_ESC;
extern const int KEY_CODE_BACKSPACE;
extern const int KEY_CODE_TAB;
extern const int KEY_CODE_RETURN;
extern const int KEY_CODE_CAPSLOCK;
//todo const int KEY_CODE_F1 ... F12;
extern const int KEY_CODE_PRINT_SCREEN;
extern const int KEY_CODE_SCROLL_LOCK;
extern const int KEY_CODE_PAUSE;
extern const int KEY_CODE_INSERT;
extern const int KEY_CODE_HOME;
extern const int KEY_CODE_PAGE_UP;
extern const int KEY_CODE_DELETE;
extern const int KEY_CODE_END;
extern const int KEY_CODE_PAGE_DOWN;
extern const int KEY_CODE_RIGHT;
extern const int KEY_CODE_LEFT;
extern const int KEY_CODE_DOWN;
extern const int KEY_CODE_UP;
//todo const int KEY_CODE_NUMPAD_...
extern const int KEY_CODE_L_SHIFT;
extern const int KEY_CODE_R_SHIFT;
extern const int KEY_CODE_L_CTRL;
extern const int KEY_CODE_R_CTRL;
extern const int KEY_CODE_L_ALT;
extern const int KEY_CODE_R_ALT;
extern const int KEY_CODE_L_GUI;
extern const int KEY_CODE_R_GUI;

#define KEY_MASK_SHIFT 1
#define KEY_MASK_CTRL 2
#define KEY_MASK_ALT 4

typedef struct GxWindow *GxWindow;

extern GxWindow createWindow(GxImage image, const char *title);

extern int getWindowEvent(GxWindow window, int *button, int *x, int *y, int timeout);

extern void setWindowTitle(GxWindow window, const char *title);

extern void flushWindow(GxWindow window);

extern void destroyWindow(GxWindow window);
