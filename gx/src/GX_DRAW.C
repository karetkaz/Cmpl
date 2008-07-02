#include "gx_surf.h"

//~ gx_draw____
//~ gx_fill____
//~ gx_clip____

#define bezp

void gx_drawrect(gx_Surf dst, int x0, int y0, int x1, int y1, long col) {
	//~ TODO
}

void gx_drawoval(gx_Surf dst, int x0, int y0, int x1, int y1, long col) {
	/*
	int dx,dy,sx,sy,r;
	if(x0 > x1)xchg(x0,x1);
	if(y0 > y1)xchg(y0,y1);
	dx = x1 - x0; dy = y1 - y0;
	x1 = x0 += dx >> 1; x0 +=dx & 1;
	dx += dx & 1; dy += dy & 1;
	sx = dx * dx; sy = dy * dy;
	r = sx * dy >> 2;
	dx = 0; dy = r << 1;
	putpixel(s,x0,y0,c);putpixel(s,x0,y1,c);
	putpixel(s,x1,y0,c);putpixel(s,x1,y1,c);
	while(y0 < y1){
		if(r >= 0){
			x0--; x1++;
			r -= dx += sy;
		}
		if(r < 0){
			y0++; y1--;
			r += dy -= sx;
		}
		putpixel(s,x0,y0,c);putpixel(s,x0,y1,c);
		putpixel(s,x1,y0,c);putpixel(s,x1,y1,c);
	}
	*/
}

void gx_drawline(gx_Surf dst, int x0, int y0, int x1, int y1, long col) {
	//~ TODO : replace Bresenham with DDA, resolve clipping 
	int dx, dy, sx, sy, r;
	dx = x1 - x0;
	dy = y1 - y0;
	if (dx < 0) { dx = -dx;  sx = -1; } else sx = 1;
	if (dy < 0) { dy = -dy;  sy = -1; } else sy = 1;
	if (dx > dy) {
		r = dx >> 1;
		while (x0 != x1) {
			gx_setpixel(dst, x0, y0, col);
			if ((r += dy) > dx) {
				y0 += sy;
				r -= dx;
			}
			x0 += sx;
		}
	} else {
		r = dy >> 1;
		while (y0 != y1) {
			gx_setpixel(dst, x0, y0, col);
			if ((r+=dx) > dy) {
				x0 += sx;
				r -= dy;
			}
			y0 += sy;
		}
	}
	gx_setpixel(dst, x0, y0, col);
}

void gx_drawbez2(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, long col) {
	int px_2, px_1, px_0 = x0;
	int py_2, py_1, py_0 = y0;
	double t, dt = .01;
	#ifdef bezp
	long olc = gx_mapcolor(dst, 0x8f8f8f);
	gx_drawline(dst, x0, y0, x1, y1, olc);
	gx_drawline(dst, x1, y1, x2, y2, olc);
	gx_drawline(dst, (x0+x1)/2, (y0+y1)/2, (x1+x2)/2, (y1+y2)/2, olc);
	#endif
	px_1 = 2 * (x1 - x0);
	py_1 = 2 * (y1 - y0);
	px_2 = x2 - 2*x1 + x0;
	py_2 = y2 - 2*y1 + y0;
	for (t = dt; t < 1; t += dt) {
		x1 = (((px_2)*t + px_1)*t + px_0);
		y1 = (((py_2)*t + py_1)*t + py_0);
		gx_drawline(dst, x0, y0, x1, y1, col);
		x0 = x1; y0 = y1;
	}
	gx_drawline(dst, x0, y0, x2, y2, col);
}

void gx_drawbez3(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, long col) {
	int px_3, px_2, px_1, px_0 = x0;
	int py_3, py_2, py_1, py_0 = y0;
	double t, dt = 1./128;
	#ifdef bezp
	long olc = gx_mapcolor(dst, 0x8f8f8f);
	gx_drawline(dst, x0, y0, x1, y1, olc);
	gx_drawline(dst, x1, y1, x2, y2, olc);
	gx_drawline(dst, x2, y2, x3, y3, olc);
	gx_drawline(dst, (x0+x1)/2, (y0+y1)/2, (x1+x2)/2, (y1+y2)/2, olc);
	gx_drawline(dst, (x1+x2)/2, (y1+y2)/2, (x2+x3)/2, (y2+y3)/2, olc);
	gx_drawline(dst, ((x0+x1)/2 + (x1+x2)/2)/2, ((y0+y1)/2 + (y1+y2)/2)/2, ((x1+x2)/2 + (x2+x3)/2)/2, ((y1+y2)/2 + (y2+y3)/2)/2, olc);
	#endif
	px_1 = 3 * (x1 - x0);
	py_1 = 3 * (y1 - y0);
	px_2 = 3 * (x2 - x1) - px_1;
	py_2 = 3 * (y2 - y1) - py_1;
	px_3 = x3 - px_2 - px_1 - px_0;
	py_3 = y3 - py_2 - py_1 - py_0;
	for (t = dt; t < 1; t += dt) {
		x1 = ((((px_3)*t + px_2)*t + px_1)*t + px_0);
		y1 = ((((py_3)*t + py_2)*t + py_1)*t + py_0);
		gx_drawline(dst, x0, y0, x1, y1, col);
		x0 = x1; y0 = y1;
	}
	gx_drawline(dst, x0, y0, x3, y3, col);
}//*/
