#ifndef __MCGA_H
#define __MCGA_H

/*typedef struct {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
} argb;//*/

typedef struct {
	unsigned short	posx;
	unsigned short	posy;
	signed short	relx;
	signed short	rely;
	unsigned char	button;
	signed char	whell;
} mouseinf;

typedef struct {
	unsigned short	xmin;
	unsigned short	ymin;
	unsigned short	xmax;
	unsigned short	ymax;
} rect;
#pragma pack(1)

#if __cplusplus
extern "C" {
#endif

void MCGA_open();
void MCGA_exit();
void setpixel(void*, int x, int y, int c);
char getpixel(void*, int x, int y);
void setpalette(void* pal);
void waitretrace();
void blurscr(void*, int val);
void blurcpy(void*, void*, int val);
void copyscr(void*, rect*);

typedef void (*mousefun)(int, mouseinf*);

int initMouse(mousefun, int);
void exitMouse();
void resetMouse();
void showMouse();
void hideMouse();
void setMouse(int x, int y);
void getMouse(mouseinf*);
void bndMouse(void*);
void hndMouse(mousefun);

//void hndMouse(void interrupt(*v)());

#if __cplusplus
}
#endif

#endif
