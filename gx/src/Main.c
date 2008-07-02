#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "mcga.h"
#include "gx_vbe.h"
#include "gx_surf.h"
#include "gx_color.h"

//~ #include "libs\\libfixed\\include\\fixed.h"
//~ #include "libs\\libfixed\\include\\fxmat.h"
//~ /*

//~ #include "gx_draw.c"

//~ #include "test\\ft2.c"
//~ #include "test\\zoom.c"

// */

//~ #include "test\\vbe_rat.c"
//~ #include "test\\vbe_fnt.c"
//~ #include "test\\vbe_zoom.c"

#include "g3d.c"
//~ #include "g3dmcga.c"
/*

#include "drv_win.c"

int main() {
	struct gx_Surf_t img1;//, *res = &img1;
	MSG msg;
	msg.wParam = 0;
	//~ printf("%d\n", intitWIN(640,480));
	if (gx_loadBMP(&img1, "Base.bmp", 24)) {
		MessageBox(NULL, "Cannot open bitmap : Base.bmp", "???", MB_OK);
		exit(1);
	}
	MessageBox(NULL, "bitmap : Base.bmp", "???", MB_OK);
	gx_wincopy(&img1);
	gx_doneSurf(&img1);
	if (gx_loadBMP(&img1, "blend.bmp", 24)) {
		MessageBox(NULL, "Cannot open bitmap : Base.bmp", "???", MB_OK);
		exit(1);
	}
	MessageBox(NULL, "bitmap : Blend.bmp", "???", MB_OK);
	gx_wincopy(&img1);
	gx_doneSurf(&img1);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	doneWin();
	return msg.wParam;
}//*/

/*
#define FXPOINT 16
#define fxpldf(__a) ((__a) * (float)(1 << FXPOINT))
#define fxptof(__a) ((__a) / (float)(1 << FXPOINT))

long fxpmul1(long a, long b);
long fxpmul2(long a, long b) {
	register long long c = (long long)a * b;
	return (c + (c >> (FXPOINT+1))) >> FXPOINT;
	//~ return (c + (FXP_ONE / 2)) >> FXPOINT;
	return 0;
}

long fxpdiv2(long a, long b) {
	return (((long long)a << FXPOINT) / b);
}

#if (1 && (defined __WATCOMC__) && (FXPOINT == 16))

#pragma aux fxpmul1 parm [eax] [edx] modify [edx eax] value [eax] = \
	"imul	edx"\
	"shrd	eax, edx, 16";

#pragma aux fxpmul2 parm [eax] [edx] modify [eax ebx edx ecx] value [eax] = \
	"imul	edx"\
	"mov	ecx, eax"\
	"mov	ebx, edx"\
	"shrd	ecx, edx, 18"\
	"sar	ebx, 18"\
	"add	eax, ecx"\
	"adc	edx, ebx"\
	"shrd	eax, edx, 16";

#endif

#include <stdio.h>
void multest(float a, float b) {
	printf ("%f * %f = (flt)%f (fxp)%f\n", a, b, a*b, fxptof(fxpmul1(fxpldf(a), fxpldf(b))));
	printf ("%f * %f = (flt)%f (fxp)%f\n", a, b, a*b, fxptof(fxpmul2(fxpldf(a), fxpldf(b))));
}

int main() {
	float a = 3.201576;
	float b = 3.123466;
	multest(a,b);
	//~ getch();
	return 0;
}
*/
