#include "g2_surf.h"

typedef int (*peek_msg)(int);
typedef int (*flip_scr)(gx_Surf, int);

extern int initWin(gx_Surf offs, flip_scr *flip, peek_msg *msgp, int ratHND(int btn, int mx, int my), int kbdHND(/*int lkey, */int key));
extern void doneWin();

extern void setCaption(char* str);
