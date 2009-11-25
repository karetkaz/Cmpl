#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g2_argb.h"

#define __WIN_DRV
//~ #include "g3_draw.c"
//~ #include "drv_win.c"

#define SCRW 800
#define SCRH 600
struct gx_Surf offs;
long cBuff[SCRW * SCRH];
int px1 = 0 , py1 = 0, px2 = 800, py2 = 600;
int cx1 = 0 , cy1 = 600, cx2 = 800, cy2 = 600;
int cp = 0, state = 0, grad = 0;

void mixpixel(gx_Surf surf, int x, int y, argb c, long a) {
	argb *dst = (argb*)gx_getpaddr(surf, x, y);
	if (dst) {
		a &= 255;
		//~ dst->r += a * (c.r - dst->r) >> 8;
		//~ dst->g += a * (c.g - dst->g) >> 8;
		//~ dst->b += a * (c.b - dst->b) >> 8;
		dst->val = (a << 16) | (a << 8) | a;
		if (cp) dst->val = -1;
	}
}

int g2_clipcalc(gx_Clip roi, int x, int y) {
	int c = (y >= roi->ymax) << 0;
	c |= (y <  roi->ymin) << 1;
	c |= (x >= roi->xmax) << 2;
	c |= (x <  roi->xmin) << 3;
	return c;
}

int g2_clipline(gx_Clip roi, int *x1, int *y1, int *x2, int *y2) {
	//~ gx_Clip *roi = &rec;//(gx_Clip *)gx_getclip(surf);
	int c1 = g2_clipcalc(roi, *x1, *y1);
	int c2 = g2_clipcalc(roi, *x2, *y2);
	int x, dx, y, dy, e;

	while (c1 | c2) {
		if (c1 & c2) return 0;
		e = c1 ? c1 : c2;
		dx = *x2 - *x1;
		dy = *y2 - *y1;
		switch(e & -e) {		// get lowest bit set
			case 1 : {
				y = roi->ymax - 1;
				if (dy) dy = ((y - *y1) << 16) / dy;
				x = *x1 + ((dx * dy) >> 16);
			} break;
			case 2 : {
				y = roi->ymin;
				if (dy) dy = ((y - *y1) << 16) / dy;
				x = *x1 + ((dx * dy) >> 16);
			} break;
			case 4 : {
				x = roi->xmax - 1;
				if (dx) dx = ((x - *x1) << 16) / dx;
				y = *y1 + ((dx * dy) >> 16);
			} break;
			case 8 : {
				x = roi->xmin;
				if (dx) dx = ((x - *x1) << 16) / dx;
				y = *y1 + ((dx * dy) >> 16);
			} break;
		}
		if (c1) {
			*x1 = x; *y1 = y;
			c1 = g2_clipcalc(roi, x, y);
		}
		else {
			*x2 = x; *y2 = y;
			c2 = g2_clipcalc(roi, x, y);
		}
	}
	return 1;
}

void g2_drawlineA(gx_Surf surf, int x1, int y1, int x2, int y2, argb c) {		// under construction
	int x, y;

	if (!g2_clipline(gx_getclip(surf), &x1, &y1, &x2, &y2)) return;

	if (y1 > y2) {
		y = y1; y1 = y2; y2 = y;
		x = x1; x1 = x2; x2 = x;
	}

	if (y1 == y2 && x1 == x2) ;
	else if (y1 == y2) {			// vline
		if (x1 > x2) {x = x1; x1 = x2; x2 = x;}
		for (x = x1; x <= x2; x++)
			gx_setpixel(surf, x, y1, c.val);
	}
	else if (x1 == x2) {			// hline
		for (y = y1; y <= y2; y++)
			gx_setpixel(surf, x1, y, c.val);
	}
	else {
		long xs = (((x2 - x1) << 16) / (y2 - y1));
		if ((xs >> 16) == (xs >> 31)) {							// fixme
			x = (x1 << 16) - (x1 > x2);
			for (y = y1; y <= y2; y += 1, x += xs) {
				int X = x >> 16;
				mixpixel(surf, X + 0, y, c, (~x >> 8) & 0xff);
				mixpixel(surf, X + 1, y, c, (+x >> 8) & 0xff);
			}
		}
		else if (x1 < x2) {
			long ys = (((y2 - y1) << 16) / (x2 - x1));
			y = (y1 << 16);
			for (x = x1; x <= x2; x++, y += ys) {
				int Y = y >> 16;
				mixpixel(surf, x, Y + 0, c, (~y >> 8) & 0xff);
				mixpixel(surf, x, Y + 1, c, (+y >> 8) & 0xff);
			}
		}
		else {
			long ys = (((y2 - y1) << 16) / (x1 - x2));
			y = (y1 << 16);
			for (x = x1; x >= x2; x--, y += ys) {
				int Y = y >> 16;
				mixpixel(surf, x, Y + 0, c, (~y >> 8) & 0xff);
				mixpixel(surf, x, Y + 1, c, (+y >> 8) & 0xff);
			}
		}
	}
}

void g2_drawlineA2(gx_Surf s, int x1, int y1, int x2, int y2, argb c) {		//TODO
	int x, y, dx, dy;

	dx = x2 - x1; dy = y2 - y1;
	if (!g2_clipline(gx_getclip(s), &x1, &y1, &x2, &y2)) return;

	if (y1 > y2) {
		y = y1; y1 = y2; y2 = y;
		x = x1; x1 = x2; x2 = x;
	}

	if (y1 == y2 && x1 == x2) {		// pixel
		//~ g3_setpixel(x1, y1, z, c.val);
	}
	else if (y1 == y2) {			// vline
		if (x1 > x2) {x = x1; x1 = x2; x2 = x;}
		for (x = x1; x < x2; x++)
			gx_setpixel(s, x, y1, c.val);
	}
	else if (x1 == x2) {			// hline
		for (y = y1; y < y2; y++)
			gx_setpixel(s, x1, y, c.val);
	}
	else {							// fixme
		long xs;
		xs = (dx << 16) / (dy);
		x = (x1 << 16) - (x1 > x2);

		if ((xs >> 16) == (xs >> 31)) {
			for (y = y1; y < y2; y += 1) {
				int X = x >> 16;
				/*int a, offs = y * scrw + (x >> 16);
				argb *dst = (argb*)&cBuff[offs];
				a = ~x >> 8 & 0xff;
				dst->r += ((c.r - dst->r) * a) >> 8;
				dst->g += ((c.g - dst->g) * a) >> 8;
				dst->b += ((c.b - dst->b) * a) >> 8;
				dst += 1;
				a = +x >> 8 & 0xff;
				dst->r += ((c.r - dst->r) * a) >> 8;
				dst->g += ((c.g - dst->g) * a) >> 8;
				dst->b += ((c.b - dst->b) * a) >> 8;
				//~ */

				mixpixel(s, X + 0, y, c, ~x >> 8);
				mixpixel(s, X + 1, y, c, +x >> 8);
				x += xs;
			}
		}
		else {
			int as = ((dy << 16) / dx);
			//~ x1 = x1;
			for (y = y1; y < y2; y += 1, x += xs) {
				//~ int X, a = x1 > x2 ? 0xff00 : 0x0000;
				int X, a = -(x1 < x2);
				x1 = x >> 16;
				x2 = (x + xs) >> 16;
				if (x1 > x2) {X = x1; x1 = x2; x2 = X;}
				for (X = x1; X < x2; X++) {
					mixpixel(s, X, y + 0, c, ~a >> 8);
					mixpixel(s, X, y + 1, c, +a >> 8);
					//~ g3_mixpixel(X, y + 0, z, c, a);
					//~ g3_mixpixel(X, y + 1, z, c, ~a);
					a += as;
				}
			}
		}
	}
}

void g2_drawlineB(gx_Surf surf, int x1, int y1, int x2, int y2, long c) {		// BresenHam
	long sx = 1, dx;
	long sy = 1, dy;
	long e;

	if (!g2_clipline(gx_getclip(surf), &x1, &y1, &x2, &y2)) return;

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

void (*g2_drawlinen)(gx_Surf, int, int, int, int, long) = g2_drawlineB;

//~ void gx_drawbez3a(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, long col) {
void gx_drawbez3(gx_Surf dst, int px0, int py0, int cx0, int cy0, int cx1, int cy1, int px1, int py1, long col) {
	int px_0 = px0;
	int py_0 = py0;
	int px_1 = 3 * (cx0 - px0);
	int py_1 = 3 * (cy0 - py0);
	int px_2 = 3 * (cx1 - cx0) - px_1;
	int py_2 = 3 * (cy1 - cy0) - py_1;
	int px_3 = px1 - px_2 - px_1 - px_0;
	int py_3 = py1 - py_2 - py_1 - py_0;

	double t, dt = 1./128;
	for (t = dt; t < 1; t += dt) {
		int px = ((((px_3)*t + px_2)*t + px_1)*t + px_0);
		int py = ((((py_3)*t + py_2)*t + py_1)*t + py_0);
		g2_drawlinen(dst, px0, py0, px, py, col);
		px0 = px; py0 = py;
	}
	g2_drawlinen(dst, px0, py0, px1, py1, col);
}

void ratHND(int btn, int mx, int my) {
	//~ static int omx, omy;
	//~ int rx = mx - omx;
	//~ int ry = my - omy;
	if (btn & 1) {
		px1 = mx;
		py1 = my;
		/*if (cp) {
			cx1 += rx; cy1 += ry;
		} else {
			px1 += rx; py1 += ry;
		}// */
		state = 0;
	}
	if (btn & 2) {
		px2 = mx;
		py2 = my;
		/*if (cp) {
			cx2 += rx; cy2 += ry;
		} else {
			px2 += rx; py2 += ry;
		}*/
		state = 0;
	}
	//~ omx = mx;
	//~ omy = my;
}

int intmin(int a, int b) {return a < b ? a : b;}
int intmax(int a, int b) {return a > b ? a : b;}
//~ int abs(int a) {return max(a-b, b-a);}
int intlen(int a, int b) {return a -= b < 0 ? -a : a;}

static void nextval(int *val, int mask, int maxval) {
	int tmp = *val & mask;
	if (maxval == 0)
		maxval = mask;

	tmp += (mask & -mask);

	if (tmp > maxval)
		tmp = 0;

	*val &= ~mask;
	*val |= tmp;

	// *val += LOBIT(mask)
}

int kbdHND(int lkey, int key) {
	switch (key) {
		case  27 : return -1;
		case ' ' : g2_drawlinen = g2_drawlinen != g2_drawlineB ? g2_drawlineB : (void*)g2_drawlineA; state = 0; break;
		case '\t' : cp = !cp; state = 0; break;
		case 'w' : py1 -= 1; state = 0; break;
		case 's' : py1 += 1; state = 0; break;
		case 'a' : px1 -= 1; state = 0; break;
		case 'd' : px1 += 1; state = 0; break;
		case 'G' : nextval(&grad, gradient_TYP, gradient_max); break;
		case '/' : grad ^= gradient_mir; break;
		case '?' : grad ^= gradient_rep; break;
		case 'g' : {
			int retval;
			struct gx_Rect roi[1];
			struct gx_Clut lut[1];
			roi->x = px1;
			roi->y = py1;
			roi->w = px2 - px1;
			roi->h = py2 - py1;
			gx_clutblend(lut, argb_rgbc, rgbval(0x0000ff), rgbval(0x00ff00));
			retval = gx_gradsurf(&offs, roi, lut, grad);
			//~ debug("gx_gradsurf: %d", retval);
			state = -1;
		} break;
	}
	return 0;
}

#define BG_COL ~0xffffff
#define FG_COL (~BG_COL)

char *fnt = "media/font/modern-1.fnt";

typedef int (*flip_scr)(gx_Surf, int);
typedef int (*peek_msg)(int);
#include "drv_win.c"

int main(int argc, char* argv[]) {
	struct gx_Surf font;		// font

	struct {
		time_t	sec;
		time_t	cnt;
		int	col;
		int	fps;
	} fps;
	char str[256];
	flip_scr flip;
	int (*peek)(int);
	void (*done)(void);
	int e;

	offs.clipPtr = 0;
	offs.width = SCRW;
	offs.height = SCRH;
	offs.flags = 0;
	offs.depth = 32;
	offs.pixeLen = 4;
	offs.scanLen = SCRW*4;
	offs.tempPtr = 0;
	offs.basePtr = &cBuff;

	if ((e = gx_loadFNT(&font, fnt))) {
		printf("Cannot open font '%s': %d\n", fnt, e);
		return -2;
	}
	if (init_WIN(&offs, &flip, &peek, &done, ratHND, kbdHND)) {
		printf("Cannot init surface\n");
		gx_doneSurf(&font);
		return -1;
	}

	for(fps.cnt = 0, fps.sec = time(NULL); peek(1) != -1; fps.cnt++) {
		//~ vector p1, p2;
		#define nextln ((ln+=1)*16)
		int ln = -1;
		//~ gx_fillsurf(&offs, NULL, cblt_cpy, 0);
		if (state != -1) for (e = 0; e < SCRW * SCRH; e += 1) {		// clear
			cBuff[e] = BG_COL;
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

		//~ sprintf(str, "fps: %d", fps.fps);
		//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, fps.col);

		//~ sprintf(str, "laa: %d", g2_drawlinen == g2_drawlineA);
		//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, fps.col);
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

