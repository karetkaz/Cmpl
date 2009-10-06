#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g2_argb.h"

//~ #define __WIN_DRV
#include "g3_draw.c"

int px1 = 0 , py1 = 0, px2 = 800, py2 = 600;
int cx1 = 0 , cy1 = 600, cx2 = 800, cy2 = 600;
int cp = 0;

void g2_mixpixel(gx_Surf *surf, int x, int y, argb c, long a) {
	argb *dst = (argb*)gx_getpaddr(surf, x, y);
	if (dst) {
		//~ dst->r = a;
		//~ dst->g = a;
		//~ dst->b = a;
		a += a >> 8;
		dst->r += a * (c.r - dst->r) >> 8;
		dst->g += a * (c.g - dst->g) >> 8;
		dst->b += a * (c.b - dst->b) >> 8;
	}
}

void g2_drawlineA(gx_Surf *surf, int x1, int y1, int x2, int y2, argb c) {		// under construction
	int x, y;

	if (!g2_clipline((gx_Clip *)gx_getclip(surf), &x1, &y1, &x2, &y2)) return;

	if (y1 > y2) {
		y = y1; y1 = y2; y2 = y;
		x = x1; x1 = x2; x2 = x;
	}

	if (y1 == y2 && x1 == x2) {		// pixel
		gx_setpixel(surf, x1, y1, c.val);
	}
	else if (y1 == y2) {			// vline
		if (x1 > x2) {x = x1; x1 = x2; x2 = x;}
		for (x = x1; x <= x2; x++)
			gx_setpixel(surf, x, y1, c.val);
	}
	else if (x1 == x2) {			// hline
		for (y = y1; y <= y2; y++)
			gx_setpixel(surf, x1, y, c.val);
	}
	else {					// fixme
		long xs, ys, i, a = 0x1000;// - (x1 > x2);
		x = (x1 << 16) - (x1 > x2);
		//~ x = (x1 << 16) | 0x7fff;
		xs = (((x2 - x1) << 16) / (y2 - y1));
		ys = (((y2 - y1) << 16) / (x2 - x1));
		for (y = y1; y < y2; y += 1, x += xs) {
			x1 = x >> 16;
			x2 = (x + xs) >> 16;
			if (((xs >> 16) != (xs >> 31))) {	// abs(xs) < 1
				/*if (x1 < x2) for (i = x1; i < x2; i++, ya += ys) {
					//~ gx_setpixel(surf, i, y + 0, c.val);
					//~ gx_setpixel(surf, i, y + 1, c.val);
					g2_mixpixel(surf, i, y + 0, c, (~ya >> 8) & 0xff);
					g2_mixpixel(surf, i, y + 1, c, (+ya >> 8) & 0xff);
				}
				else // */
				a = x & 0xff00;
				//~ for (i = x1; i < x2; i++, a += ys) {
					gx_setpixel(surf, x1, y + 0, c.val);
					gx_setpixel(surf, x1, y + 1, c.val);
					//~ g2_mixpixel(surf, i, y + 0, c, (~a >> 8) & 0xff);
					//~ g2_mixpixel(surf, i, y + 1, c, (+a >> 8) & 0xff);
				//~ }// */
				//~ g2_mixpixel(surf, x1, y + 0, c, (~a >> 8) & 0xff);
				//~ g2_mixpixel(surf, x1, y + 1, c, (+a >> 8) & 0xff);
				//~ g2_mixpixel(surf, x2, y + 0, c, (~a >> 8) & 0xff);
				//~ g2_mixpixel(surf, x2, y + 1, c, (+a >> 8) & 0xff);
			}
			else {
				//~ gx_setpixel(surf, x1 + 0, y, c.val);
				//~ gx_setpixel(surf, x1 + 1, y, c.val);
				g2_mixpixel(surf, x1 + 0, y, c, (~x >> 8) & 0xff);
				g2_mixpixel(surf, x1 + 1, y, c, (+x >> 8) & 0xff);
			}
		}
	}
}

void g2_drawlineB(gx_Surf *surf, int x1, int y1, int x2, int y2, long c) {		// BresenHam
	long sx = 1, dx;
	long sy = 1, dy;
	long e;

	if (!g2_clipline((gx_Clip *)gx_getclip(surf), &x1, &y1, &x2, &y2)) return;

	if ((dy = y2 - y1) < 0) {
		dy = -dy;
		sy = -1;
	}
	if ((dx = x2 - x1) < 0) {
		dx = -dx;
		sx = -1;
	}
	if (dx > dy) {
		e = dx >> 1;
		while (x1 != x2) {
			gx_setpixel(surf, x1, y1, c);
			if ((e += dy) > dx) {
				y1 += sy;
				e -= dx;
			}
			x1 += sx;
		}
	}
	else if (dy) {
		e = dy >> 1;
		while (y1 != y2) {
			gx_setpixel(surf, x1, y1, c);
			if ((e += dx) > dy) {
				x1 += sx;
				e -= dy;
			}
			y1 += sy;
		}
	}
}

void (*g2_drawlinen)(gx_Surf*, int, int, int, int, long) = g2_drawlineB;

void gx_drawbez3(gx_Surf *dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, long col) {
	int px_3, px_2, px_1, px_0 = x0;
	int py_3, py_2, py_1, py_0 = y0;
	double t, dt = 1./128;
	px_1 = 3 * (x1 - x0);
	py_1 = 3 * (y1 - y0);
	px_2 = 3 * (x2 - x1) - px_1;
	py_2 = 3 * (y2 - y1) - py_1;
	px_3 = x3 - px_2 - px_1 - px_0;
	py_3 = y3 - py_2 - py_1 - py_0;
	for (t = dt; t < 1; t += dt) {
		x1 = ((((px_3)*t + px_2)*t + px_1)*t + px_0);
		y1 = ((((py_3)*t + py_2)*t + py_1)*t + py_0);
		g2_drawlinen(dst, x0, y0, x1, y1, col);
		x0 = x1; y0 = y1;
	}
	g2_drawlinen(dst, x0, y0, x3, y3, col);
}

void ratHND(int btn, int mx, int my) {
	static int omx, omy;
	int rx = mx - omx;
	int ry = my - omy;
	omx = mx; omy = my;
	if (btn & 1) {
		if (cp) {
			cx1 += rx; cy1 += ry;
		} else {
			px1 += rx; py1 += ry;
		}
	}
	if (btn & 2) {
		if (cp) {
			cx2 += rx; cy2 += ry;
		} else {
			px2 += rx; py2 += ry;
		}
	}
}

void kbdHND(int lkey, int key) {
	switch (key) {
		case  27 : act = -1; break;
		case ' ' : g2_drawlinen = g2_drawlinen != g2_drawlineB ? g2_drawlineB : g2_drawlineA; break;
		case '\t' : cp = !cp; break;
		case 'q' : px1 -= 1; break;
		case 'w' : px1 += 1; break;
	}
}

#define BG_COL ~0xffffff
#define FG_COL (~BG_COL)

int main(int argc, char* argv[]) {
	struct {
		time_t	sec;
		time_t	cnt;
		int	col;
		int	fps;
	} fps;
	char str[256];
	flip_scr flip;
	void (*peek)(void);
	void (*done)(void);
	int e;

	offs.clipPtr = 0;
	offs.width = SCRW;
	offs.height = SCRH;
	offs.flags = 0;
	offs.depth = 32;
	offs.pixeLen = 4;
	offs.scanLen = SCRW*4;
	offs.clutPtr = 0;
	offs.basePtr = &cBuff;

	if (init_WIN(&offs, &flip, &peek, &done, ratHND, kbdHND)) {
		printf("Cannot init surface\n");
		return -1;
	}

	if ((e = gx_loadFNT(&font, "MODERN-1.FNT"))) {
		//~ done_win();
		gx_doneSurf(&font);
		printf("Cannot open font %d : antique.fnt\n", e);
		getch();
		done();
		return -2;
	}

	for(fps.cnt = 0, fps.sec = time(NULL); peek(), act != -1; fps.cnt++) {
		//~ vector p1, p2;
		#define nextln ((ln+=1)*16)
		int ln = -1;
		//~ gx_fillsurf(&offs, NULL, cblt_cpy, 0);
		for (e = 0; e < SCRW * SCRH; e += 1) {		// clear
			cBuff[e] = BG_COL;
			zBuff[e] = 0x00ffffff;
		}

		//~ vecldf(&p1, mx1 / (SCRW/2.), my1 / (-SCRH/2.), 0, 1);
		//~ vecldf(&p2, mx2 / (SCRW/2.), my2 / (-SCRH/2.), 0, 1);
		//~ vecldf(&p1, (mx1 - (SCRW/2.)) / (SCRW/2.), -(my1 - (SCRH/2.)) / (SCRH/2.), 0, 1);
		//~ vecldf(&p2, (mx2 - (SCRW/2.)) / (SCRW/2.), -(my2 - (SCRH/2.)) / (SCRH/2.), 0, 1);
		
		g2_drawlinen(&offs, px1, py1, px2, py2, FG_COL);
		//~ g2_drawlinen(&offs, px1, py1, cx1, cy1, 0xffffffff);
		//~ g2_drawlinen(&offs, px2, py2, cx2, cy2, 0xffffffff);
		//~ gx_drawbez3(&offs, px1, py1, cx1, cy1, cx2, cy2, px2, py2, 0xffffffff);

		//~ g3_drawline(&p1, &p2, 0xffffffff);
		if (fps.sec != time(NULL)) {			// fps
			fps.col = fps.cnt < 30 ? 0xff0000 : fps.cnt < 50 ? 0xffff00 : 0x00ff00;
			fps.sec = time(NULL);
			fps.fps = fps.cnt;
			fps.cnt = 0;
		}

		sprintf(str, "fps: %d", fps.fps);
		gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, fps.col);

		sprintf(str, "laa: %d", g2_drawlinen == g2_drawlineA);
		gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, fps.col);
		//~ sprintf(str, "pos: (%d, %d), (%d, %d)", mx1, my1, mx2, my2);
		//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffff);
		//~ sprintf(str, "p1(%g, %g, %g)", p1.x, p1.y, p1.z);
		//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);
		//~ sprintf(str, "p2(%g, %g, %g)", p2.x, p2.y, p2.z);
		//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);

		flip(&offs, 0);

		#undef nextln
	}

	//freeMesh(&msh);
	gx_doneSurf(&font);
	done();
	return 0;
}
