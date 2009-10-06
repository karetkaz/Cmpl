.386p
.model flat, c
; option casemap : none

; public gx_locksurf		; int	gx_locksurf(gx_Surf, lock_type);
; public gx_initdraw		; void	gx_initdraw(gx_Surf, cmix_type);
public gx_mapcolor		; long	gx_mapcolor(gx_Surf, argb);					Get Color in internal format
public gx_cliprect		; void* gx_cliprect(gx_Surf, gx_Rect);
; public gx_clipclip		; void* gx_clipclip(gx_Surf, gx_Clip);
public gx_getpaddr		; void* gx_getpaddr(gx_Surf, int, int);					Get Pixel Address
public gx_getpixel		; long	gx_getpixel(gx_Surf, int, int);					Get Pixel
public gx_getpnear		; long	gx_getpnear(gx_Surf, fixed16, fixed16);				Get Pixel
public gx_getpblin		; long	gx_getpnear(gx_Surf, fixed16, fixed16);				Get Pixel
public gx_setpixel		; void	gx_setpixel(gx_Surf, int, int, long);				Set Pixel
public gx_sethline		; void	gx_sethline(gx_Surf, int x1, int y, int x2, long);		Horizontal Line
public gx_setvline		; void	gx_setvline(gx_Surf, int x, int y1, int x2, long);		Verical Line
public gx_setpline		; void	gx_setpline(gx_Surf, int x1, int y, int x2, long, Pattern);	Pattern Line
public gx_setblock		; void	gx_setblock(gx_Surf, int x1, int y1, int x2, int y2, long);	Block (fill rect)
public gx_getcbltf		; gx_cbltf gx_getcbltf(cbltf_type, int depth, int opbpp);
public gx_callcbltf		; void gx_callcbltf(gx_cbltf, void*, void*, unsigned, void*);


; public mixcol_32
; public mixcol_24
; public mixcol_16
; public mixcol_08

; public setpix_32
; public setpix_24
; public setpix_16
; public setpix_08

; color transfer function	: copy/ fill/ conv : set/ and/ or/ xor/ mix
; clt(void *dst:edi, void *src:esi, unsigned cnt:ecx, void* pal||col||alpha:edx)

; conv
; edi : dst
; esi : src
; ecx : cnt
; edx : unused / palette

; copy
; edi : dst
; esi : src
; ecx : cnt
; edx : unused / alpha

; fill
; edi : dst
; esi : unused / alpha map
; ecx : cnt
; edx : col

public colcpy_32_24
public colcpy_32_16
; public colcpy_32_15
public colcpy_32_08
public colcpy_32pal	;rgbcpy_32_08 where: dst[i] = pal[src[i]].rgb
public colcpy_32lum	;rgbcpy_32_08 where: dst[i].r = dst[i].g = dst[i].b = src[i]

public colcpy_24_32
public colcpy_24_16
; public colcpy_24_15
public colcpy_24_08
; public colcpy_24pal		;colcpy_24__08 with edx != 0
; public colcpy_24lum		;colcpy_24__08 with edx == 0

public colcpy_16_32
public colcpy_16_24
; public colcpy_16_15
; public colcpy_16_08
; public colcpy_16pal		;colcpy_16__08 with edx != 0
; public colcpy_16lum		;colcpy_16__08 with edx == 0

public colcpy_15_32
public colcpy_15_24
public colcpy_15_16
; public colcpy_15_08
; public colcpy_15pal		;colcpy_16__08 with edx != 0
; public colcpy_15lum		;colcpy_16__08 with edx == 0

; public colcpy_08_32
; public colcpy_08_24
; public colcpy_08_16
public colcpy_lum32
public colcpy_xtr32

public colcpy_32cpy	;[edi] := [esi]
public colcpy_32mix	;[edi] += (edx * ([esi] - [edi])) >> 8
public colcpy_32and	;[edi] &= [esi]
public colcpy_32ior	;[edi] |= [esi]
public colcpy_32xor	;[edi] ^= [esi]

public colcpy_24cpy
public colcpy_24and
public colcpy_24ior
public colcpy_24xor
public colcpy_16cpy
public colcpy_16and
public colcpy_16ior
public colcpy_16xor
; public colcpy_15cpy
; public colcpy_15and
; public colcpy_15ior
; public colcpy_15xor
public colcpy_08cpy
public colcpy_08and
public colcpy_08ior
public colcpy_08xor

public colset_32cpy	;[edi] := edx
public colset_32mix	;[edi] += ([esi] * (edx - [edi])) >> 8
; public colset_32and	;[edi] &= edx
; public colset_32ior	;[edi] |= edx
; public colset_32xor	;[edi] ^= edx

; public colset_24cpy
; public colset_24and
; public colset_24ior
; public colset_24xor
; public colset_16cpy
; public colset_16and
; public colset_16ior
; public colset_16xor
; public colset_15cpy
; public colset_15and
; public colset_15ior
; public colset_15xor
; public colset_08cpy
; public colset_08and
; public colset_08ior
; public colset_08xor

;misc

;#public colcpy_32msk	; [edi] := ([edi] & ~edx) | ([esi] | edx)
;#public colcpy_32lut	; [edi] := [edx[esi]]
;#public colcpy_32key	; if ([esi] != edx) {[edi] := [esi]}

gx_Rect struc				; Rectangle
	rectX			dd ?		; X
	rectY			dd ?		; Y
	rectW			dd ?		; width
	rectH			dd ?		; Height
gx_Rect ends

gx_Clip struc				; Clip Region
	clipL			dw ?		; Horizontal Start	(minX)
	clipT			dw ?		; Vertical   Start	(minY)
	clipR			dw ?		; Horizontal End	(maxX)
	clipB			dw ?		; Vertical   End	(maxY)
gx_Clip ends

; gx_Clut struc				; Color Lukup Table
	; P_ColCnt		dw ?		; Palette color count
	; P_1stCol		db ?		; first entry
	; P_trtCol		db ?		; transparent color
	; Pal_Buff		dd 256 dup (?)	; Palette
; gx_Clut ends

gx_Surf struc				; Surface
	clipPtr			dd ?		; + 04
	horzRes			dw ?		; + 06
	vertRes			dw ?		; + 08
	flags			dw ?		; + 10
	depth			db ?		; + 11
	pixeLen			db ?		; + 12
	scanLen			dd ?		; + 16
	clutPtr			dd ?		; + 20
	basePtr			dd ?		; + 24
	; offsPtr			dd ?		; + 28
	; movePtr			dd ?		; + 32
gx_Surf ends
;/--------------------------------------------------------------\  0
;|			     ClipPtr				|
;|------------------------------+-------------------------------|  4
;|	      Width		|	     Height		|
;|--------------+---------------+-------------------------------|  8
;|	     Flags		|     Depth	|   PixelLen	|
;|--------------+---------------+-------------------------------| 12
;|			  ScanLineLength			|
;|--------------+---------------+---------------+---------------|.16
;|			     clutPtr				|
;|--------------+---------------+---------------+---------------| 20
;|			     basePtr				|
;|--------------------------------------------------------------| 24
;|			     offsPtr				|X
;|--------------------------------------------------------------| 28
;|			     movePtr				|X
;|--------------+---------------+---------------+---------------|.32
;|			     fillPtr				|X
;|--------------+---------------+---------------+---------------| 36
;|								|
;|--------------------------------------------------------------| 40
;|								|
;|--------------------------------------------------------------| 44
;|								|
;\--------------------------------------------------------------/.48
;SurfFlags: ----------- Mask ---------- Bit --- Meaning(0/1) -------------------
; #define SURF_RDONLY	0X1000
SF_RDONLY	equ	00100h		;0	read only flag
IS_RDONLY	equ	0FF00h		;0	read only mask
; SF_MEMNIL	equ	00200h		;0	no memory flag : gx_Surf.basePtr = 0
; F_cardmem	equ	00001h		;0	memory location : Image/Screen
; F_memaloc	equ	00002h		;1	memory type : linked/allocd
; F_palaloc	equ	00004h		;2	palette type : linked/allocd
; F_resvd01	equ	00008h		;3	?linear / packed(x,y) offset
; F_resvd02	equ	00020h		;5	?
; F_resvd03	equ	00040h		;6	palette type : allocd/linked
; F_resvd04	equ	00080h		;7	Clip is a scanlist
;-------------------------------------------------------------------------------

.data

	; col_32_put	dd	setpix_32_cpy,	setpix_32_and,	setpix_32_ior,	setpix_32_xor
	; col_24_put	dd	setpix_24_cpy,	setpix_24_and,	setpix_24_ior,	setpix_24_xor
	; col_16_put	dd	setpix_16_cpy,	setpix_16_and,	setpix_16_ior,	setpix_16_xor
	; col_08_put	dd	setpix_08_cpy,	setpix_08_and,	setpix_08_ior,	setpix_08_xor
	; col_24_put	dd	setpix_24_cpy,	andpix_24,	iorpix_24,	xorpix_24
	; col_16_put	dd	setpix_16_cpy,	andpix_16,	iorpix_16,	xorpix_16
	; col_08_put	dd	setpix_08_cpy,	andpix_08,	iorpix_08,	xorpix_08

	col_32_cpy	dd	colcpy_32cpy,	colcpy_32and,	colcpy_32ior,	colcpy_32xor
	col_24_cpy	dd	colcpy_24cpy,	colcpy_24and,	colcpy_24ior,	colcpy_24xor
	col_16_cpy	dd	colcpy_16cpy,	colcpy_16and,	colcpy_16ior,	colcpy_16xor
	col_08_cpy	dd	colcpy_08cpy,	colcpy_08and,	colcpy_08ior,	colcpy_08xor

	; col_32_cvt	dd	colcpy_32_cpy,	colcpy_32__24,	colcpy_32__16,	colcpy_32__08
	; col_24_cvt	dd	colcpy_24__32,	colcpy_24_cpy,	colcpy_24__16,	colcpy_24__08
	; col_16_cvt	dd	colcpy_16__32,	colcpy_16__24,	colcpy_16_cpy,		0
	; col_08_cvt	dd		0,		0,		0,	colcpy_08_cpy

	; col_32_set	dd	colset_32_cpy,	colset_32_and,	colset_32_ior,	colset_32_xor
	; col_24_set	dd	colset_24_cpy,	colset_24_and,	colset_24_ior,	colset_24_xor
	; col_16_set	dd	colset_16_cpy,	colset_16_and,	colset_16_ior,	colset_16_xor
	; col_08_set	dd	colset_08_cpy,	colset_08_and,	colset_08_ior,	colset_08_xor

.code

proc_dummi:ret
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Lock Surface
; int gx_locksurf(Surface, enum lock_type);
; in  :	Surface to lock
;	unlock/lock/lockwait
; out :	status : 0 on success
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_locksurf proc near
	mov	edx, [esp+4+4]
	mov	eax, [esp+4+8]
	test	eax, eax
	jz	locksurf_unlock
	mov	ax, [edx].gx_Surf.flags
	or	ax, SF_RDONLY
	xchg	[edx].gx_Surf.flags, ax
	and	eax, SF_RDONLY
	ret
	locksurf_unlock:
	and	[edx].gx_Surf.flags, not SF_RDONLY
	xor	eax, eax
	ret

gx_locksurf endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; adjust Rect
; void* gx_cliprect(gx_Surf, gx_Rect);
; in  :	Surface
;	Rectangle
; out : Address of first point (NULL if fail)
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_cliprect:
	; in srf : gx_Surf
	; in roi : gx_Rect
		; gx_Clip clp = srf->clipPtr ? srf->clipPtr : (gx_Clip)srf;
		; roi->w += roi->x; roi->h += roi->y;
		; if (clp->l > roi->x) roi->x = clp->l;
		; if (clp->t > roi->y) roi->y = clp->t;
		; if (clp->r < roi->w) roi->w = clp->r;
		; if (clp->b < roi->h) roi->h = clp->b;
		; if ((roi->w -= roi->x) < 0) return 0;
		; if ((roi->h -= roi->y) < 0) return 0;
		; return gx_getpaddr(srf, roi->x, roi->y);
	push	ebx
	xor	eax, eax
	mov	ebx, [esp + 4 + 4*1]		; surf
	mov	ecx, [esp + 4 + 4*2]		; rect
	mov	edx, ebx
	cmp	[edx], eax
	jz	gx_cliprect_clip
	mov	edx, [edx]
	gx_cliprect_clip:

	mov	eax, [ecx].gx_Rect.rectX
	add	[ecx].gx_Rect.rectW, eax
	mov	eax, [ecx].gx_Rect.rectY
	add	[ecx].gx_Rect.rectH, eax
	xor	eax, eax

	mov	ax, [edx].gx_Clip.clipL
	cmp	eax, [ecx].gx_Rect.rectX
	jle	gx_cliprect_Lok
	mov	[ecx].gx_Rect.rectX, eax
	gx_cliprect_Lok:
	mov	ax, [edx].gx_Clip.clipT
	cmp	eax, [ecx].gx_Rect.rectY
	jle	gx_cliprect_Tok
	mov	[ecx].gx_Rect.rectY, eax
	gx_cliprect_Tok:
	mov	ax, [edx].gx_Clip.clipR
	cmp	eax, [ecx].gx_Rect.rectW
	jge	gx_cliprect_Rok
	mov	[ecx].gx_Rect.rectW, eax
	gx_cliprect_Rok:
	mov	ax, [edx].gx_Clip.clipB
	cmp	eax, [ecx].gx_Rect.rectH
	jge	gx_cliprect_Bok
	mov	[ecx].gx_Rect.rectH, eax
	gx_cliprect_Bok:
	mov	eax, [ecx].gx_Rect.rectX
	sub	[ecx].gx_Rect.rectW, eax
	jbe	gx_cliprect_fail		; if (CF OR ZF is set) return 0;
	mov	eax, [ecx].gx_Rect.rectY
	sub	[ecx].gx_Rect.rectH, eax
	jbe	gx_cliprect_fail		; if (CF OR ZF is set) return 0;

	mov	edx, [ecx].gx_Rect.rectY
	mov	ecx, [ecx].gx_Rect.rectX

	movzx	eax, [ebx].gx_Surf.pixeLen
	call	addr_dd[eax * 4]

	pop	ebx
	ret

	gx_cliprect_fail:
	xor	eax, eax
	pop	ebx
	ret

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Map argb color to surface internal color type
; long mapcolor(gx_Surf, long);
; in  :	gx_Surf
;	argb color format
; out : Surface internal color format (alpha removed) -1 if fail
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_mapcolor proc
	mov	edx, [esp + 0 + 4*1]
	mov	eax, [esp + 0 + 4*2]
	mov	ah, [edx].gx_Surf.depth
	cmp	ah, 24
	je	gx_mapcolor_24
	jb	gx_mapcolor_splt
	cmp	ah, 32
	je	gx_mapcolor_32
	jmp	gx_mapcolor_exit
	gx_mapcolor_splt:
	cmp	ah, 8
	je	gx_mapcolor_08
	jb	gx_mapcolor_exit
	cmp	ah, 15
	je	gx_mapcolor_15
	jb	gx_mapcolor_exit
	cmp	ah, 16
	je	gx_mapcolor_16
	gx_mapcolor_exit:
	mov	eax, -1
	ret

	gx_mapcolor_24:
	gx_mapcolor_32:
	mov	eax, [esp+4*(0+2)]
	and	eax, 00FFFFFFH
	ret

	gx_mapcolor_16:
	mov	eax, [esp+4*(0+2)]
	push	dx
	mov	dx, ax
	shr	dh, 2
	shr	eax, 8
	shr	dx, 3
	and	eax, 0000F800H
	or	ax, dx
	pop	dx
	ret

	gx_mapcolor_15:
	mov	eax, [esp+4*(0+2)]
	push	dx
	mov	dx, ax
	shr	dh, 3
	shr	dx, 3
	shr	eax, 9
	and	eax, 00007C00H
	or	ax, dx
	pop	dx
	ret

	; gx_mapcolor_cpal:
	; mov	eax, [esp+4*(0+1)]
	; mov	eax, [eax].gx_Surf.PalPtr
	; test	eax, eax
	; jz	gx_mapcolor_gray		; there is no palette -> lum or 3:3:2
	; pushad
	; mov	dword ptr [esp+28], -1
	; mov	esi, eax
	; mov	ecx, -1
	; lodsd				; eax : inf
	; xor	ebx, ebx


	; gx_mapcolor_lpal:			; loop trough the palette
	; xor	eax, eax		; RED
	; xor	edx, edx
	; mov	al, [esi].ARGBQ.r
	; mov	dl, [esp+4*(0+2)].ARGBQ.r
	; sub	ax, dx
	; movsx	eax, ax
	; mul	eax
	; mov	edi, eax
	; xor	eax, eax			; GREEN
	; xor	edx, edx
	; mov	al, [esi].ARGBQ.g
	; mov	dl, [esp+4*(0+2)].ARGBQ.g
	; sub	ax, dx
	; movsx	eax, ax
	; mul	eax
	; add	edi, eax
	; xor	eax, eax			; BLUE
	; xor	edx, edx
	; mov	al, [esi].ARGBQ.b
	; mov	dl, [esp+4*(0+2)].ARGBQ.b
	; sub	ax, dx
	; movsx	eax, ax
	; mul	eax
	; add	edi, eax
	; cmp	edi, ecx
	; jnb	gx_mapcolor_ncol		; next palette color
		; mov	ecx, edi
		; xor	eax, eax
		; mov	al, bh
		; mov	[esp+28], eax	; ax
	; gx_mapcolor_ncol:
	; add	esi, 4
	; inc	bh
	; dec	bl
	; jnz	gx_mapcolor_lpal
	; popad
	; ret

	gx_mapcolor_08:			; (r * 76 + g * 150 + b * 29) >> 8;
	push	ebx
	push	edx
	xor	eax, eax
	mov	ebx, [esp+8+8]
	mov	al, 29
	mul	bl
	mov	dx, ax
	shr	ebx, 8
	mov	al, 150
	mul	bl
	add	dx, ax
	shr	ebx, 8
	mov	al, 76
	mul	bl
	add	ax, dx
	shr	eax, 8
	pop	edx
	pop	ebx
	ret

gx_mapcolor endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Get Pixel Address
; void* gx_getpaddr(gx_Surf, int, int);
; in  :	Surface
;	X
;	Y
; out : Address (NULL if fail)
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_getpaddr proc
	push	ebx
	mov	ebx, [esp + 4 + 4*1]
	mov	ecx, [esp + 4 + 4*2]
	mov	edx, [esp + 4 + 4*3]

	movzx	eax, [ebx].gx_Surf.vertRes
	cmp	edx, eax
	jae	gx_getpaddr_fail
	movzx	eax, [ebx].gx_Surf.horzRes
	cmp	ecx, eax
	jae	gx_getpaddr_fail

	movzx	eax, [ebx].gx_Surf.pixeLen
	call	addr_dd[eax * 4]

	pop	ebx
	ret

	gx_getpaddr_fail:
	xor	eax, eax
	pop	ebx
	ret

gx_getpaddr endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Get Pixel
; long getpixel(gx_Surf, int, int);
; in  :	Surface
;	X
;	Y
; out :	Color, 0 if fail
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_getpixel proc
	push	ebx
	xor	eax, eax
	mov	ebx, [esp + 4 + 4*1]
	mov	ecx, [esp + 4 + 4*2]
	mov	edx, [esp + 4 + 4*3]

	mov	ax, [ebx].gx_Surf.vertRes
	cmp	edx, eax
	jae	gx_getpixel_fail
	mov	ax, [ebx].gx_Surf.horzRes
	cmp	ecx, eax
	jae	gx_getpixel_fail

	movzx	eax, [ebx].gx_Surf.pixeLen
	call	addr_dd[eax * 4]

	mov	eax, [eax]
	pop	ebx
	ret

	gx_getpixel_fail:
	xor	eax, eax
	pop	ebx
	ret

gx_getpixel endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Get Pixel using nearest filter
; long getpnear(gx_Surf, fixed16, fixed16);
; in  :	Surface
;	X
;	Y
; out :	Color, 0 if fail
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_getpnear proc
	push	ebx
	xor	eax, eax
	mov	ebx, [esp + 4 + 4*1]
	mov	ecx, [esp + 4 + 4*2]
	mov	edx, [esp + 4 + 4*3]

	cmp	[ebx].gx_Surf.depth, 32
	jnz	gx_getpnear_fail

	sar	edx, 16
	movzx	eax, [ebx].gx_Surf.vertRes
	cmp	edx, eax
	jae	gx_getpnear_fail

	sar	ecx, 16
	movzx	eax, [ebx].gx_Surf.horzRes
	cmp	ecx, eax
	jae	gx_getpnear_fail

	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	eax, [eax + ecx * 4]
	add	eax, [ebx].gx_Surf.basePtr
	mov	eax, [eax]
	pop	ebx
	ret

	gx_getpnear_fail:
	xor	eax, eax
	pop	ebx
	ret

gx_getpnear endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Get Pixel using bilinear filter
; long getpblin(gx_Surf, fixed16, fixed16);
; in  :	Surface
;	X : -> ecx
;	Y : -> edx
; out :	Color, 0 if fail
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_getpblin proc
	push	esi
	push	ebx
	mov	ebx, [esp + 8 + 4*1]
	mov	ecx, [esp + 8 + 4*2]
	mov	edx, [esp + 8 + 4*3]

	cmp	[ebx].gx_Surf.depth, 32
	jnz	gx_getpblin_fail

	movzx	eax, [ebx].gx_Surf.horzRes
	sar	ecx, 16
	dec	eax
	; TODO ;inc	ecx
	cmp	ecx, eax
	ja	gx_getpblin_fail
	je	gx_getpblin_lrpY

	movzx	eax, [ebx].gx_Surf.vertRes
	sar	edx, 16
	dec	eax
	cmp	edx, eax
	ja	gx_getpblin_fail
	je	gx_getpblin_lrpX

	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx].gx_Surf.basePtr

	mov	eax, [esi]
	mov	edx, [esi + 4]
	movzx	ecx, byte ptr [esp + 8 + 4*2+1]

	add	esi, [ebx].gx_Surf.scanLen
	mov	ebx, eax
	mov	ax, dx
	and	edx, 00FF00FFH
	mov	al, bh
	and	ebx, 00FF00FFH
	sub	edx, ebx
	imul	edx, ecx
	shr	edx, 8
	add	edx, ebx
	mov	bl, al
	shr	ax, 8
	sub	ax, bx
	imul	ax, cx
	add	ah, bl
	mov	dh, ah

	push	edx
	mov	eax, [esi + 0]
	mov	edx, [esi + 4]
	mov	ebx, eax
	mov	ax, dx
	and	edx, 00FF00FFH
	mov	al, bh
	and	ebx, 00FF00FFH
	sub	edx, ebx
	imul	edx, ecx
	shr	edx, 8
	add	edx, ebx
	mov	bl, al
	shr	ax, 8
	sub	ax, bx
	imul	ax, cx
	add	ah, bl
	mov	dh, ah
	pop	eax

	movzx	ecx, byte ptr [esp + 8 + 4*3+1]			; alpha y

	gx_getpblin_lrp2:
	mov	ebx, eax
	mov	ax, dx
	and	edx, 00FF00FFH
	mov	al, bh			; dst
	and	ebx, 00FF00FFH
	sub	edx, ebx
	imul	edx, ecx
	shr	edx, 8
	add	edx, ebx		; R & B done
	mov	bl, al			; bh = 0
	shr	ax, 8			; ah = 0
	sub	ax, bx
	imul	ax, cx
	add	ah, bl
	mov	dh, ah
	mov	eax, edx

	pop	ebx
	pop	esi
	ret

	gx_getpblin_lrpY:
	movzx	eax, [ebx].gx_Surf.vertRes
	sar	edx, 16
	dec	eax
	cmp	edx, eax
	ja	gx_getpblin_fail
	je	gx_getpblin_lrp0

	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx].gx_Surf.basePtr

	mov	eax, [esi]					; first pixel
	add	esi, [ebx].gx_Surf.scanLen
	movzx	ecx, byte ptr [esp + 8 + 4*2+1]			; alpha x
	mov	edx, [esi]
	jmp	gx_getpblin_lrp2

	gx_getpblin_lrpX:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx].gx_Surf.basePtr

	mov	eax, [esi]					; first pixel
	movzx	ecx, byte ptr [esp + 8 + 4*3+1]			; alpha y
	mov	edx, [esi + 4]
	jmp	gx_getpblin_lrp2

	gx_getpblin_lrp0:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx].gx_Surf.basePtr
	mov	eax, [esi]					; first pixel
	pop	ebx
	pop	esi
	ret

	gx_getpblin_fail:
	xor	eax, eax
	pop	ebx
	pop	esi
	ret

gx_getpblin endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Set Pixel
; void setpixel(Surface, int, int, long);
; in  :	Surface
;	X
;	Y
;	Color
; out :
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_setpixel proc
; ret
	push	ebx
	push	edi
	mov	ebx, [esp + 8 + 4*1]			; surf
	mov	ecx, [esp + 8 + 4*2]			; x
	mov	edx, [esp + 8 + 4*3]			; y

	mov	edi, ebx						; clip
	cmp	dword ptr [edi], 0
	je	gx_setpixel_clip
	mov	edi, [edi]
	gx_setpixel_clip:

	movzx	eax, [edi].gx_Clip.clipL
	cmp	ecx, eax
	jl	gx_setpixel_fail

	movzx	eax, [edi].gx_Clip.clipT
	cmp	edx, eax
	jl	gx_setpixel_fail

	movzx	eax, [edi].gx_Clip.clipR
	cmp	ecx, eax
	jae	gx_setpixel_fail

	movzx	eax, [edi].gx_Clip.clipB
	cmp	edx, eax
	jae	gx_setpixel_fail

	movzx	eax, [ebx].gx_Surf.pixeLen
	call	addr_dd[eax * 4]
	mov	edi, eax

	mov	eax, [esp + 8 + 4*4]			; col

	movzx	ebx, [ebx].gx_Surf.pixeLen
	call	spix_dd[ebx * 4]

	gx_setpixel_fail:
	pop	edi
	pop	ebx
	ret

gx_setpixel endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Horizontal Line
; void gx_hline(gx_Surf, int, int, int, long);
; in  :	gx_Surf
;	X1
;	Y
;	X2
;	Color
; out :
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_sethline proc near
	push	ebx
	push	edi
	mov	ebx, [esp + 8 + 4*1]			; surf
	mov	ecx, [esp + 8 + 4*2]			; x1
	mov	edx, [esp + 8 + 4*3]			; y

	cmp	ecx, [esp + 8 + 4*4]			; x1 < x2
	jl	gx_sethline_x1x2
	xchg	ecx, [esp + 8 + 4*4]

	gx_sethline_x1x2:
	mov	edi, ebx

	cmp	dword ptr [edi], 0
	je	gx_sethline_clip
	mov	edi, [edi]

	gx_sethline_clip:
	; clip y first
	movzx	eax, [edi].gx_Clip.clipT		; if (y  < clip.T) return;
	cmp	edx, eax
	jl	gx_sethline_fail

	movzx	eax, [edi].gx_Clip.clipB		; if (y >= clip.B) return;
	cmp	edx, eax
	jae	gx_sethline_fail

	movzx	eax, [edi].gx_Clip.clipL		; if (x1 < clip.L) x1 = clip.L
	cmp	ecx, eax
	jge	gx_sethline_x1ok
	mov	ecx, eax

	gx_sethline_x1ok:
	movzx	eax, [edi].gx_Clip.clipR		; if (x2 > clip.R) x2 = clip.R
	cmp	[esp + 8 + 4*4], eax
	jl	gx_sethline_x2ok
	mov	[esp + 8 + 4*4], eax

	gx_sethline_x2ok:
	sub	[esp + 8 + 4*4], ecx				; if ((x2 -= x1) <= 0) return;
	jbe	gx_sethline_fail

	movzx	eax, [ebx].gx_Surf.pixeLen
	cmp	eax, 1
	je	gx_sethline_1bpp
	cmp	eax, 2
	je	gx_sethline_2bpp
	cmp	eax, 4
	je	gx_sethline_4bpp
	jmp	gx_sethline_fail

	gx_sethline_1bpp:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	edi, [eax + ecx*1]
	add	edi, [ebx].gx_Surf.basePtr		; buffp
	mov	ecx, [esp + 8 + 4*4]			; count
	mov	eax, [esp + 8 + 4*5]			; color
	rep stosb
	pop	edi
	pop	ebx
	ret

	gx_sethline_2bpp:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	edi, [eax + ecx*2]
	add	edi, [ebx].gx_Surf.basePtr
	mov	ecx, [esp + 8 + 4*4]
	mov	eax, [esp + 8 + 4*5]
	rep stosw
	pop	edi
	pop	ebx
	ret

	gx_sethline_4bpp:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	edi, [eax + ecx*4]
	add	edi, [ebx].gx_Surf.basePtr
	mov	ecx, [esp + 8 + 4*4]
	mov	eax, [esp + 8 + 4*5]
	rep stosd
	pop	edi
	pop	ebx
	ret

	gx_sethline_fail:
	pop	edi
	pop	ebx
	ret

gx_sethline endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Vertical Line
; void gx_vline(gx_Surf, int, int, int, long);
; in  :	gx_Surf
;	X1
;	Y1
;	Y2
;	Color
; out :
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_setvline proc near
	push	ebx
	push	edi
	mov	ebx, [esp + 8 + 4*1]			; surf
	mov	ecx, [esp + 8 + 4*2]			; x
	mov	edx, [esp + 8 + 4*3]			; y1
	cmp	edx, [esp + 8 + 4*4]			; y2
	jl	gx_setvline_nosy			; y1 < y2
	xchg	edx, [esp + 8 + 4*4]
	gx_setvline_nosy:
	mov	edi, ebx
	cmp	dword ptr [edi], 0
	je	gx_setvline_clip
	mov	edi, [edi]
	gx_setvline_clip:
	xor	eax, eax
	; clip x first
	mov	ax, [edi].gx_Clip.clipL		; if (x  < clip.L) return
	cmp	ecx, eax
	jl	gx_setvline_fail
	mov	ax, [edi].gx_Clip.clipR		; if (x >= clip.R) return
	cmp	ecx, eax
	jge	gx_setvline_fail
	mov	ax, [edi].gx_Clip.clipT		; if (y1 < clip.T) y1 = clip.T
	cmp	edx, eax
	jge	gx_setvline_y1ok
	mov	edx, eax
	gx_setvline_y1ok:
	mov	ax, [edi].gx_Clip.clipB		; if (y2 > clip.B) y2 = clip.Y
	cmp	[esp + 8 + 4*4], eax
	jl	gx_setvline_y2ok
	mov	[esp + 8 + 4*4], eax
	gx_setvline_y2ok:
	sub	[esp + 8 + 4*4], edx			; if ((y2 -= y1) <= 0) return;
	jbe	gx_setvline_fail			; if CF OR ZF is set 

	; TODO
	gx_setvline_fail:
	pop	edi
	pop	ebx
	ret

gx_setvline endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; Pattern Line
; void gx_pline(gx_Surf, int, int, int, long, Pattern);
; in  :	gx_Surf
;	X1
;	Y1
;	X2
;	Color
;	Pattern
; out :
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_setpline proc near
	push	ebx
	push	edi
	mov	ebx, [esp + 8 + 4*1]			; surf
	mov	ecx, [esp + 8 + 4*2]			; x1
	mov	edx, [esp + 8 + 4*3]			; y
	cmp	ecx, [esp + 8 + 4*4]			; x2
	jl	gx_setpline_nosx			; x1 < x2
	xchg	ecx, [esp + 8 + 4*4]
	gx_setpline_nosx:
	mov	edi, ebx
	cmp	dword ptr [edi], 0
	je	gx_setpline_clip
	mov	edi, [edi]
	gx_setpline_clip:
	xor	eax, eax
	; clip y first
	mov	ax, [edi].gx_Clip.clipT		; if (y  < clip.T) return;
	cmp	edx, eax
	jl	gx_setpline_fail
	mov	ax, [edi].gx_Clip.clipB		; if (y >= clip.B) return;
	cmp	edx, eax
	jge	gx_setpline_fail
	mov	ax, [edi].gx_Clip.clipL		; if (x1 < clip.L) x1 = clip.L
	cmp	ecx, eax
	jge	gx_setpline_x1ok
	mov	ecx, eax
	gx_setpline_x1ok:
	mov	ax, [edi].gx_Clip.clipR		; if (x2 > clip.R) x2 = clip.R
	cmp	[esp + 8 + 4*4], eax
	jl	gx_setpline_x2ok
	mov	[esp + 8 + 4*4], eax
	gx_setpline_x2ok:
	sub	[esp + 8 + 4*4], ecx			; if ((x2 -= x1) <= 0) return;
	jbe	gx_setpline_fail			; if CF OR ZF is set 

	mov	eax, [esp + 8 + 4*6]			; prepare pattern
	push	ecx
	push	edx
	and	edx, 7
	mov	al, [eax + edx]
	and	cl, 7
	rol	al, cl
	pop	edx
	pop	ecx
	mov	[esp + 8 + 4*6], al			; save it

	; TODO
	gx_setpline_fail:
	pop	edi
	pop	ebx
	ret

gx_setpline endp

;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
; block
; void gx_block(gx_Surf, int, int, int, int, long);
; in  :	gx_Surf
;	X1
;	Y1
;	X2
;	Y2
;	Color
; out :
; use :
; call:
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
gx_setblock proc near
	push	ebx
	push	edi
	mov	ebx, [esp + 8 + 4*1]			; surf
	mov	ecx, [esp + 8 + 4*2]			; x1
	mov	edx, [esp + 8 + 4*3]			; y1

	cmp	ecx, [esp + 8 + 4*4]			; x2
	jl	gx_setblock_nosx			; x1 < x2
	xchg	ecx, [esp + 8 + 4*4]
	gx_setblock_nosx:

	cmp	edx, [esp + 8 + 4*5]		; y2
	jl	gx_setblock_nosy		; y1 < y2
	xchg	edx, [esp + 8 + 4*5]
	gx_setblock_nosy:

	mov	edi, ebx
	cmp	dword ptr [edi], 0
	je	gx_setblock_clip
	mov	edi, [edi]
	gx_setblock_clip:

	movzx	eax, [edi].gx_Clip.clipL	; if (x1 < clip.L) x1 = clip.L
	cmp	ecx, eax
	jge	gx_setblock_x1ok
	mov	ecx, eax
	gx_setblock_x1ok:

	movzx	eax, [edi].gx_Clip.clipT	; if (y1 < clip.T) y1 = clip.T
	cmp	edx, eax
	jge	gx_setblock_y1ok
	mov	edx, eax
	gx_setblock_y1ok:

	movzx	eax, [edi].gx_Clip.clipR	; if (x2 > clip.R) x2 = clip.R
	cmp	[esp + 8 + 4*4], eax
	jl	gx_setblock_x2ok
	mov	[esp + 8 + 4*4], eax
	gx_setblock_x2ok:

	movzx	eax, [edi].gx_Clip.clipB	; if (y2 > clip.B) y2 = clip.B
	cmp	[esp + 8 + 4*5], eax
	jl	gx_setblock_y2ok
	mov	[esp + 8 + 4*5], eax
	gx_setblock_y2ok:

	sub	[esp + 8 + 4*4], ecx		; if ((x2 -= x1) <= 0) return;
	jbe	gx_setblock_fail		; if CF OR ZF is set
	sub	[esp + 8 + 4*5], edx		; if ((y2 -= y1) <= 0) return;
	jbe	gx_setblock_fail		; if CF OR ZF is set

	movzx	eax, [ebx].gx_Surf.pixeLen
	cmp	eax, 1
	je	gx_setblock_1bpp
	cmp	eax, 2
	je	gx_setblock_2bpp
	cmp	eax, 4
	je	gx_setblock_4bpp
	jmp	gx_setblock_fail

	gx_setblock_1bpp:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	edi, [eax + ecx*1]
	add	edi, [ebx].gx_Surf.basePtr		; buffp

	mov	[esp + 8 + 4*1], edi

	mov	ebx, [ebx].gx_Surf.scanLen		; scanlen
	mov	eax, [esp + 8 + 4*6]			; color

	gx_setblock_1bLP:
	mov	edi, [esp + 8 + 4*1]			; buffp
	add	[esp + 8 + 4*1], ebx
	mov	ecx, [esp + 8 + 4*4]			; count
	rep stosb
	dec	dword ptr[esp + 8 + 4*5]
	; sub	[esp + 8 + 4*5], 1
	jnz	gx_setblock_1bLP

	gx_setblock_2bpp:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	edi, [eax + ecx*2]
	add	edi, [ebx].gx_Surf.basePtr

	mov	[esp + 8 + 4*1], edi

	mov	ebx, [ebx].gx_Surf.scanLen
	mov	eax, [esp + 8 + 4*6]

	gx_setblock_2bLP:
	mov	edi, [esp + 8 + 4*1]
	add	[esp + 8 + 4*1], ebx
	mov	ecx, [esp + 8 + 4*4]
	rep stosw
	dec	dword ptr[esp + 8 + 4*5]
	jnz	gx_setblock_2bLP

	gx_setblock_4bpp:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	edx
	lea	edi, [eax + ecx*4]
	add	edi, [ebx].gx_Surf.basePtr

	mov	[esp + 8 + 4*1], edi

	mov	ebx, [ebx].gx_Surf.scanLen
	mov	eax, [esp + 8 + 4*6]

	gx_setblock_4bLP:
	mov	edi, [esp + 8 + 4*1]
	add	[esp + 8 + 4*1], ebx
	mov	ecx, [esp + 8 + 4*4]
	rep stosd
	dec	dword ptr[esp + 8 + 4*5]
	jnz	gx_setblock_4bLP

	pop	edi
	pop	ebx
	ret



	; push	esi

	; movzx	esi, [ebx].gx_Surf.pixeLen

	; call	addr_dd[esi * 4]

	; mov	[esp + 12 + 4*1], eax		; save addres of first pixel

	; mov	eax, [ebx].gx_Surf.scanLen
	; mov	[esp + 12 + 4*2], eax		; scanLen

	; mov	ebx, spix_dd[esi * 4]

	; mov	eax, [esp + 12 + 4*6]		; col

	; gx_setblock_vert:				; vertical loop
	; mov	edi, [esp + 12 + 4*1]			; base
	; mov	ecx, [esp + 12 + 4*2]			; scanLen
	; add	[esp + 12 + 4*1], ecx			; base += scanLen

	; mov	ecx, [esp + 12 + 4*4]			; get width
	; gx_setblock_horz:
	; call	ebx
	; mov	[edi], eax
	; add	edi, esi
	; dec	ecx
	; jnz	gx_setblock_horz

	; dec	dword ptr[esp + 12 + 4*5]
	; jnz	gx_setblock_vert
	; pop	esi

	gx_setblock_fail:
	pop	edi
	pop	ebx
	ret

gx_setblock endp


spix_dd	dd	proc_dummi,	setpix_08,	setpix_16,	setpix_24,	setpix_32
addr_dd	dd	proc_dummi,	addr_get08,	addr_get16,	addr_get24,	addr_get32
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
;	Pixel Address
; in  :	ebx : Surface
;		ecx : X
;		edx : Y
; ret :	eax : addr of pixel
; mod :	edx
;°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

addr_get08:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*1]
	add	eax, [ebx].gx_Surf.basePtr
	ret

addr_get15:
addr_get16:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*2]
	add	eax, [ebx].gx_Surf.basePtr
	ret

addr_get24:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	dx
	shl	edx, 16
	mov	dx, ax
	add	edx, ecx
	lea	eax, [edx + ecx*2]
	add	eax, [ebx].gx_Surf.basePtr
	ret

addr_get32:
	mov	eax, [ebx].gx_Surf.scanLen
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*4]
	add	eax, [ebx].gx_Surf.basePtr
	ret

setpix_32:
	mov	[edi], eax
	ret

setpix_24:
	mov	[edi+0], al
	mov	edx, eax
	mov	[edi+1], ah
	shr	edx, 16
	mov	[edi+2], dl
	ret

setpix_16:
setpix_15:
	mov	[edi], ax
	ret

setpix_08:
	mov	[edi], al
	ret

;###############################################################################
; mixcol_32:			; in eax color, edx color, ecx value
; mixcol_24:			; in eax color, edx color, ecx value
	; push	ebx
	; mov	ebx, eax
	; mov	ax, dx
	; and	edx, 00FF00FFH
	; mov	al, bh			; dst
	; and	ebx, 00FF00FFH
	; sub	edx, ebx
	; imul	edx, ecx
	; shr	edx, 8
	; add	edx, ebx		; R & B done
	; mov	bl, al			; bh = 0
	; shr	ax, 8			; ah = 0
	; sub	ax, bx
	; imul	ax, cx
	; add	ah, bl
	; mov	dh, ah
	; mov	eax, edx
	; pop	ebx
	; ret

; mixcol_16:			; in eax color, edx color, ecx value
	; push	ebx		; eax : 00000000`00000000`RRRRRGGG`GGGBBBBB
	; shl	eax, 5		; eax : 00000000`000RRRRR`GGGGGGBB`BBB00000
	; shr	ax, 2		; eax : 00000000`000RRRRR`00GGGGGG'BBBBB000
	; shl	edx, 5		; edx : 00000000`000RRRRR`GGGGGGBB`BBB00000
	; shr	dx, 2		; edx : 00000000`000RRRRR`00GGGGGG'BBBBB000
	; mov	ebx, eax
	; mov	ax, dx
	; and	edx, 001F00FFH
	; mov	al, bh			; dst
	; and	ebx, 001F00FFH
	; sub	edx, ebx
	; imul	edx, ecx
	; shr	edx, 8
	; add	edx, ebx		; R & B done
	; mov	bl, al			; bh = 0
	; shr	ax, 8			; ah = 0
	; sub	ax, bx
	; imul	ax, cx
	; add	ah, bl
	; mov	dh, ah		; edx : 00000000`000RRRRR`00GGGGGG`BBBBB000
	; shl	dx, 2		; edx : 00000000`000RRRRR`GGGGGGBB`BBB00000
	; shr	edx, 5		; edx : 00000000`00000000`RRRRRGGG`GGGBBBBB
	; mov	eax, edx
	; pop	ebx
	; ret

; mixcol_08:			; in eax color, edx color, ecx value
	; xor	ah, ah
	; xor	dh, dh
	; sub	dx, ax
	; imul	ax, cx
	; add	ah, dl
	; shr	ax, 8
	; ret

colcpy_xtr32:
	and	edx, 3
	colcpy_xtr32_loop:
	mov	al, [esi + edx]
	mov	[edi], al
	add	esi, 4
	add	edi, 1
	sub	ecx, 1
	jnz	colcpy_xtr32_loop
	ret

colcpy_lum32:
	push	ebx

 	; .299 * r + .587 * g + .114 * b
	; (00 4C 96 1D) / 255
	; (00  76 150  29) / 255

	colcpy_lum32_loop:
	; mov	eax, [esi]
	; movzx	dx, ah
	; and	eax, 00ff00ffH
	; imul	eax, 004C001DH
	; imul	dx, 096H
	; add	dh, ah
	; shr	eax, 16
	; add	ah, dh

	mov	ebx, [esi]	; blue
	mov	al, 01Dh
	mul	bl
	mov	dx, ax
	shr	ebx, 8		; green
	mov	al, 096h
	mul	bl
	add	dx, ax
	shr	ebx, 8		; red
	mov	al, 04Ch
	mul	bl
	add	ax, dx

	mov	[edi], ah
	add	esi, 4
	add	edi, 1
	sub	ecx, 1
	jnz	colcpy_lum32_loop

	pop	ebx
	ret

; ##############################################################################

colcvt_get:		; in ah dst, al src
	cmp	ah, 8
	je	colcvt_get_TO08
	jb	colcvt_get_fail
	cmp	ah, 16
	je	colcvt_get_TO16
	jb	colcvt_get_fail
	cmp	ah, 24
	je	colcvt_get_TO24
	jb	colcvt_get_fail
	cmp	ah, 32
	je	colcvt_get_TO32

	colcvt_get_fail:
	xor	eax, eax
	ret

colcvt_get_TO08:
	cmp	al, 8
	ja	colcvt_get_0816
	jb	colcvt_get_fail
	mov	eax, offset colcpy_08cpy
	ret
	colcvt_get_0816:
	cmp	al, 16
	ja	colcvt_get_0824
	jb	colcvt_get_fail
	mov	eax, 0	;offset colcpy_08_16
	ret
	colcvt_get_0824:
	cmp	al, 24
	ja	colcvt_get_0832
	jb	colcvt_get_fail
	mov	eax, 0	;offset colcpy_08_24
	ret
	colcvt_get_0832:
	cmp	al, 32
	ja	colcvt_get_fail
	jb	colcvt_get_fail
	mov	eax, 0	;offset colcpy_08_32
	ret

colcvt_get_TO16:
	cmp	al, 8
	ja	colcvt_get_1616
	jb	colcvt_get_fail
	mov	eax, 0	;offset colcpy_16_08
	ret
	colcvt_get_1616:
	cmp	al, 16
	ja	colcvt_get_1624
	jb	colcvt_get_fail
	mov	eax, offset colcpy_16cpy
	ret
	colcvt_get_1624:
	cmp	al, 24
	ja	colcvt_get_1632
	jb	colcvt_get_fail
	mov	eax, offset colcpy_16_24
	ret
	colcvt_get_1632:
	cmp	al, 32
	ja	colcvt_get_fail
	jb	colcvt_get_fail
	mov	eax, offset colcpy_16_32
	ret

colcvt_get_TO24:
	cmp	al, 8
	ja	colcvt_get_2416
	jb	colcvt_get_fail
	mov	eax, 0	;offset colcpy_24_08
	ret
	colcvt_get_2416:
	cmp	al, 16
	ja	colcvt_get_2424
	jb	colcvt_get_fail
	mov	eax, offset colcpy_24_16
	ret
	colcvt_get_2424:
	cmp	al, 24
	ja	colcvt_get_2432
	jb	colcvt_get_fail
	mov	eax, offset colcpy_24cpy
	ret
	colcvt_get_2432:
	cmp	al, 32
	ja	colcvt_get_fail
	jb	colcvt_get_fail
	mov	eax, offset colcpy_24_32
	ret

colcvt_get_TO32:
	cmp	al, 8
	ja	colcvt_get_3216
	jb	colcvt_get_fail
	mov	eax, offset colcpy_32_08
	ret
	colcvt_get_3216:
	cmp	al, 16
	ja	colcvt_get_3224
	jb	colcvt_get_fail
	mov	eax, offset colcpy_32_16
	ret
	colcvt_get_3224:
	cmp	al, 24
	ja	colcvt_get_3232
	jb	colcvt_get_fail
	mov	eax, offset colcpy_32_24
	ret
	colcvt_get_3232:
	cmp	al, 32
	ja	colcvt_get_fail
	jb	colcvt_get_fail
	mov	eax, offset colcpy_32cpy
	ret

colcpy_get:		; in ah dst, al rop [0..4]
	cmp	al, 3
	ja	colcpy_get_fail
	cmp	ah, 8
	je	colcpy_get_DD08
	jb	colcpy_get_fail
	cmp	ah, 16
	je	colcpy_get_DD16
	jb	colcpy_get_fail
	cmp	ah, 24
	je	colcpy_get_DD24
	jb	colcpy_get_fail
	cmp	ah, 32
	je	colcpy_get_DD32
	jmp	colcpy_get_fail
	colcpy_get_DD08:
	and	eax, 000000FFH
	mov	eax, [col_08_cpy + eax*4]
	ret
	colcpy_get_DD16:
	and	eax, 000000FFH
	mov	eax, [col_16_cpy + eax*4]
	ret
	colcpy_get_DD24:
	and	eax, 000000FFH
	mov	eax, [col_24_cpy + eax*4]
	ret
	colcpy_get_DD32:
	and	eax, 000000FFH
	mov	eax, [col_32_cpy + eax*4]
	ret
	colcpy_get_fail:
	xor	eax, eax
	ret

colset_get:		; in ah dst, al rop [0..4]
	cmp	al, 3
	ja	colset_get_fail
	cmp	ah, 8
	je	colset_get_DD08
	jb	colset_get_fail
	cmp	ah, 16
	je	colset_get_DD16
	jb	colset_get_fail
	cmp	ah, 24
	je	colset_get_DD24
	jb	colset_get_fail
	cmp	ah, 32
	je	colset_get_DD32
	jmp	colset_get_fail
	colset_get_DD08:
	; and	eax, 000000FFH
	; mov	eax, [col_08_set + eax*4]
	; ret
	colset_get_DD16:
	; and	eax, 000000FFH
	; mov	eax, [col_16_set + eax*4]
	; ret
	colset_get_DD24:
	; and	eax, 000000FFH
	; mov	eax, [col_24_set + eax*4]
	; ret
	colset_get_DD32:
	; and	eax, 000000FFH
	; mov	eax, [col_32_set + eax*4]
	; ret
	colset_get_fail:
	xor	eax, eax
	ret

gx_getcbltf proc		; gx_cbltf gx_getcbltf(cbltf_type, int depth, int opbpp);
	mov	eax, [esp+0+4*1]		; cbltf_type
	test	eax, eax
	jz	getcbltf_cnv
	dec	eax
	jz	getcbltf_cpy
	dec	eax
	jz	getcbltf_set
	getcbltf_fail:
	xor	eax, eax
	ret

	getcbltf_cnv:
	mov	edx, [esp+0+4*2]
	mov	eax, [esp+0+4*3]
	mov	ah, dl
	jmp	colcvt_get

	getcbltf_cpy:
	mov	edx, [esp+0+4*2]
	mov	eax, [esp+0+4*3]
	mov	ah, dl
	jmp	colcpy_get

	getcbltf_set:
	mov	edx, [esp+0+4*2]
	mov	eax, [esp+0+4*3]
	mov	ah, dl
	; jmp	colset_get
	xor	eax, eax
	ret

gx_getcbltf endp


gx_callcbltf proc		; void gx_callcbltf(gx_cbltf, void*, void*, unsigned cnt, void*);
	os equ 2*4
	push	edi
	push	esi
	mov	edi, [esp+os+4*2]
	mov	esi, [esp+os+4*3]
	mov	ecx, [esp+os+4*4]
	mov	edx, [esp+os+4*5]
	call	dword ptr [esp+os+4*1]
	pop	esi
	pop	edi
	ret

gx_callcbltf endp

;###############################################################################
; Color Convert functions
; extern void callcolcvt(void colcpy(void), void* dst, void* src, unsigned cnt, void* pal);
; extern void callcolcpy(void colcpy(void), void* dst, void* src, unsigned cnt, void* pal);
; extern void callcolset(void colcpy(void), void* dst, void* src, unsigned cnt, long col);
; #pragma aux callcolcvt\
		; parm  [eax] [edi] [esi] [ecx] [edx]\
		; modify [eax edx ecx esi edi]\
		; value [] = "call eax";

; edi : dst
; esi : src
; ecx : cnt
; edx : unused / palette / alpha / ???

colcpy_32_24:
	mov	eax, [esi]
	mov	[edi], eax
	add	esi, 3
	add	edi, 4
	dec	ecx
	jnz	colcpy_32_24
	ret

colcpy_32_16:			; 00000000`RRRRR000`GGGGGG00`BBBBB000 <- RRRRRGGG`GGGBBBBB
	mov	ax, [esi]
	mov	dx, ax
	and	ax, 0F800H	; eax : 00000000`00000000`RRRRR000`00000000
	shl	dx, 3		; edx : 00000000`00000000`RRGGGGGG`BBBBB000
	shl	eax, 8		; eax : 00000000`RRRRR000`00000000`00000000
	shl	dh, 2		; edx : 00000000`00000000`GGGGGG00`BBBBB000
	mov	ax, dx		; eax : 00000000`RRRRR000`GGGGGG00`BBBBB000
	mov	[edi], eax
	add	esi, 2
	add	edi, 4
	dec	ecx
	jnz	colcpy_32_16
	ret

colcpy_32_08:
	test	edx, edx
	jnz	colcpy_32pal

colcpy_32lum:			; 00000000`IIIIIIII`IIIIIIII`IIIIIIII <- IIIIIIII
	mov	al, [esi]
	mov	ah, al		; eax : ????????`????????`IIIIIIII`IIIIIIII
	shl	eax, 8		; eax : ????????`IIIIIIII`IIIIIIII`00000000
	mov	al, ah		; eax : ????????`IIIIIIII`IIIIIIII`IIIIIIII
	mov	[edi], eax
	add	esi, 1
	add	edi, 4
	dec	ecx
	jnz	colcpy_32lum
	ret

colcpy_32pal:
	mov	al, [esi]
	and	eax, 000000FFH
	mov	eax, [edx + eax*4 + 4]
	colcpy_32_pal_skp:
	mov	[edi], eax
	add	esi, 1
	add	edi, 4
	dec	ecx
	jnz	colcpy_32pal
	ret

colcpy_24_32:
	mov	eax, [esi]
	mov	[edi + 0], ax
	; mov	[edi + 1], al
	shr	eax, 16
	mov	[edi + 2], al
	add	esi, 4
	add	edi, 3
	dec	ecx
	jnz	colcpy_24_32
	ret

colcpy_24_16:			; RRRRR000`GGGGGG00`BBBBB000 <- RRRRRGGG`GGGBBBBB
	mov	ax, [esi]
	mov	dx, ax
	shl	ax, 3		; ax : RRGGGGGG`BBBBB000
	and	dh, 0F8H	; dx : RRRRR000`GGGBBBBB
	shl	ah, 2		; ax : GGGGGG00`BBBBB000
	mov	[edi + 0], al
	mov	[edi + 1], ah
	mov	[edi + 2], dh
	add	esi, 2
	add	edi, 3
	dec	ecx
	jnz	colcpy_24_16
	ret

colcpy_24_08:
	test	edx, edx
	jnz	colcpy_24pal

colcpy_24lum:			; IIIIIIII`IIIIIIII`IIIIIIII <- IIIIIIII
	mov	al, [esi]
	mov	[edi + 0], al
	mov	[edi + 1], al
	mov	[edi + 2], al
	add	esi, 1
	add	edi, 3
	dec	ecx
	jnz	colcpy_24lum
	ret

colcpy_24pal:
	mov	al, [esi]
	and	eax, 000000FFH
	mov	eax, [edx + eax*4 + 4]
	mov	[edi + 0], al
	mov	[edi + 1], ah
	shr	eax, 16
	mov	[edi + 2], al
	add	esi, 1
	add	edi, 3
	dec	ecx
	jnz	colcpy_24pal
	ret

colcpy_16_32:			; RRRRRGGG`GGGBBBBB <- ********`RRRRRRRR`GGGGGGGG`BBBBBBBB 
	mov	eax, [esi]
	mov	dx, ax		; dx : GGGGGGGG`BBBBBBBB
	shr	dh, 2		; dx : 00GGGGGG`BBBBBBBB
	shr	eax, 8		; ax : RRRRRRRR'GGGGGGGG
	shr	dx, 3		; dx : 00000GGG'GGGBBBBB
	and	ax, 0f800h	; ax : RRRRR000'00000000
	or	ax, dx		; ax : RRRRRGGG`GGGBBBBB
	mov	[edi], ax
	add	esi, 4
	add	edi, 2
	dec	ecx
	jnz	colcpy_16_32
	ret

colcpy_16_24:			; RRRRRGGG`GGGBBBBB <- RRRRRRRR`GGGGGGGG`BBBBBBBB 
	mov	eax, [esi]
	mov	dx, ax		; dx : GGGGGGGG`BBBBBBBB
	shr	dh, 2		; dx : 00GGGGGG`BBBBBBBB
	shr	eax, 8		; ax : RRRRRRRR'GGGGGGGG
	shr	dx, 3		; dx : 00000GGG'GGGBBBBB
	and	ax, 0f800h	; ax : RRRRR000'00000000
	or	ax, dx		; ax : RRRRRGGG`GGGBBBBB
	mov	[edi], ax
	add	esi, 3
	add	edi, 2
	dec	ecx
	jnz	colcpy_16_24
	ret

colcpy_15_32:			; XRRRRRGG`GGGBBBBB <- ********`RRRRRRRR`GGGGGGGG`BBBBBBBB 
	mov	eax, [esi]
	mov	dx, ax		; dx : GGGGGGGG`BBBBBBBB
	shr	dh, 3		; dx : 000GGGGG`BBBBBBBB
	shr	eax, 9		; ax : 0RRRRRRR'RGGGGGGG
	shr	dx, 3		; dx : 000000GG'GGGBBBBB
	and	ax, 07C00h	; ax : 0RRRRR00'00000000
	or	ax, dx		; ax : 0RRRRRGG`GGGBBBBB
	mov	[edi], ax
	add	esi, 4
	add	edi, 2
	dec	ecx
	jnz	colcpy_15_32
	ret

colcpy_15_24:			; XRRRRRGG`GGGBBBBB <- RRRRRRRR`GGGGGGGG`BBBBBBBB 
	mov	eax, [esi]
	mov	dx, ax		; ax = dx : GGGGGGGG`BBBBBBBB
	shr	dh, 3		; dx : 000GGGGG`BBBBBBBB
	shr	eax, 9		; ax : 0RRRRRRR'RGGGGGGG
	shr	dx, 3		; dx : 000000GG'GGGBBBBB
	and	ax, 07C00h	; ax : 0RRRRR00'00000000
	or	ax, dx		; ax : 0RRRRRGG`GGGBBBBB
	mov	[edi], ax
	add	esi, 3
	add	edi, 2
	dec	ecx
	jnz	colcpy_15_24
	ret

colcpy_15_16:			; XRRRRRGG`GGGBBBBB <- RRRRRGGG`GGGBBBBB
	mov	ax, [esi]
	mov	dx, ax		; ax = dx : RRRRRGGG`GGGBBBBB
	shr	ax, 1		; ax : 0RRRRRGG`GGGGBBBB
	and	dx, 003FFh	; dx : 000000GG`GGGBBBBB
	and	ax, 07C00h	; ax : 0RRRRR00`00000000
	or 	ax, dx
	mov	[edi], ax
	dec	ecx
	jnz	colcpy_15_16
	ret


;###############################################################################
; Color copy functions

colcpy_32cpy:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
	jmp	colcpy_08cpy
colcpy_24cpy:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08cpy
colcpy_16cpy:
	lea	ecx, [ecx*2]				; convert count to bytes : ecx *= 2;
	; jmp	colcpy_08cpy
colcpy_08cpy:
	cmp	ecx, 8
	jb	colcpy_RB_cpy
colcpy_NA_cpy:						; allign destination
	test	edi, 011b				; destination is DWORD alligned ?
	jz	colcpy_DA_cpy
	mov	al, [esi]
	mov	[edi], al
	add	esi, 1
	add	edi, 1
	sub	ecx, 1
	jmp	colcpy_NA_cpy
colcpy_DA_cpy:
	mov	edx, ecx
	shr	edx, 2
	and	ecx, 011b
colcpy_LD_cpy:						; loop on DWORDs
	mov	eax, [esi]
	mov	[edi], eax
	add	esi, 4
	add	edi, 4
	dec	edx
	jnz	colcpy_LD_cpy
colcpy_RB_cpy:						; copy bytes remain
	test	ecx, ecx
	jnz	colcpy_LB_cpy
	ret
colcpy_LB_cpy:
	mov	al, [esi]
	mov	[edi], al
	inc	esi
	inc	edi
	; movsb
	dec	ecx
	jnz	colcpy_LB_cpy
	ret

colcpy_32and:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
	jmp	colcpy_08and
colcpy_24and:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08and
colcpy_16and:
	lea	ecx, [ecx*2]				; convert count to bytes : ecx *= 2;
	; jmp	colcpy_08and
colcpy_08and:						; DWORD allign destination
	cmp	ecx, 8
	jb	colcpy_RB_and
colcpy_NA_and:						; allign destination
	test	edi, 011b				; destination is DWORD alligned
	jz	colcpy_DA_and
	mov	al, [esi]
	and	[edi], al
	add	esi, 1
	add	edi, 1
	sub	ecx, 1
	jmp	colcpy_NA_and
colcpy_DA_and:
	mov	edx, ecx
	shr	edx, 2
	and	ecx, 011b
colcpy_LD_and:						; loop on DWORDs
	mov	eax, [esi]
	and	[edi], eax
	add	esi, 4
	add	edi, 4
	dec	edx
	jnz	colcpy_LD_and
colcpy_RB_and:						; copy bytes remain
	test	ecx, ecx
	jnz	colcpy_LB_and
	ret
colcpy_LB_and:
	mov	al, [esi]
	and	[edi], al
	inc	esi
	inc	edi
	dec	ecx
	jnz	colcpy_LB_and
	ret

colcpy_32ior:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
	jmp	colcpy_08ior
colcpy_24ior:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08ior
colcpy_16ior:
	lea	ecx, [ecx*2]				; convert count to bytes : ecx *= 2;
	; jmp	colcpy_08ior
colcpy_08ior:						; DWORD allign destination
	cmp	ecx, 8
	jb	colcpy_RB_ior
colcpy_NA_ior:						; allign destination
	test	edi, 011b				; destination is DWORD alligned
	jz	colcpy_DA_ior
	mov	al, [esi]
	or	[edi], al
	add	esi, 1
	add	edi, 1
	sub	ecx, 1
	jmp	colcpy_NA_ior
colcpy_DA_ior:
	mov	edx, ecx
	shr	edx, 2
	and	ecx, 011b
colcpy_LD_ior:						; loop on DWORDs
	mov	eax, [esi]
	or	[edi], eax
	add	esi, 4
	add	edi, 4
	dec	edx
	jnz	colcpy_LD_ior
colcpy_RB_ior:						; copy bytes remain
	test	ecx, ecx
	jnz	colcpy_LB_ior
	ret
colcpy_LB_ior:
	mov	al, [esi]
	or	[edi], al
	inc	esi
	inc	edi
	dec	ecx
	jnz	colcpy_LB_ior
	ret

colcpy_32xor:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
	jmp	colcpy_08xor
colcpy_24xor:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08xor
colcpy_16xor:
	lea	ecx, [ecx*2]				; convert count to bytes : ecx *= 2;
	; jmp	colcpy_08xor
colcpy_08xor:						; DWORD allign destination
	cmp	ecx, 8
	jb	colcpy_RB_xor
colcpy_NA_xor:						; allign destination
	test	edi, 011b				; destination is DWORD alligned
	jz	colcpy_DA_xor
	mov	al, [esi]
	xor	[edi], al
	add	esi, 1
	add	edi, 1
	sub	ecx, 1
	jmp	colcpy_NA_xor
colcpy_DA_xor:
	mov	edx, ecx
	shr	edx, 2
	and	ecx, 011b
colcpy_LD_xor:						; loop on DWORDs
	mov	eax, [esi]
	xor	[edi], eax
	add	esi, 4
	add	edi, 4
	dec	edx
	jnz	colcpy_LD_xor
colcpy_RB_xor:						; copy bytes remain
	test	ecx, ecx
	jnz	colcpy_LB_xor
	ret
colcpy_LB_xor:
	mov	al, [esi]
	xor	[edi], al
	inc	esi
	inc	edi
	dec	ecx
	jnz	colcpy_LB_xor
	ret

;###############################################################################
; Color Fill functions

colset_32cpy:
	cmp	ecx, 4
	jb	colset_LDcpy
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
	mov	eax, edx
	jmp	colset_NAcpy

colset_16cpy:
	cmp	ecx, 4
	jb	colset_LDcpy
	lea	ecx, [ecx*2]				; convert count to bytes : ecx *= 2;
	mov	ax, dx
	shl	eax, 16
	mov	ax, dx
	jmp	colset_NAcpy

colset_08cpy:
	cmp	ecx, 4
	jb	colset_RBcpy
	mov	dh, dl
	mov	ax, dx
	shl	edx, 16
	mov	ax, dx

colset_NAcpy:						; allign destination
	test	edi, 001b				; destination is WORD alligned ?
	jz	colset_WAcpy
	mov	[edi], al
	ror	al, 8
	add	edi, 1
	sub	ecx, 1
colset_WAcpy:
	test	edi, 010b				; destination is DWORD alligned ?
	jz	colset_DAcpy
	mov	[edi], ax
	ror	al, 16
	add	edi, 2
	sub	ecx, 2
colset_DAcpy:
	mov	edx, ecx
	shr	edx, 2
colset_LDcpy:						; loop on DWORDs
	mov	[edi], eax
	add	edi, 4
	dec	edx
	jnz	colset_LDcpy
colset_RBcpy:						; copy bytes remain
	and	ecx, 011b
	jnz	colset_LBcpy
	ret

colset_LBcpy:
	mov	[edi], al
	ror	al, 8
	inc	edi
	dec	ecx
	jnz	colset_LBcpy
	ret

;###############################################################################

colcpy_32mix:				; dst += (t * (src - dst)) >> 8;
	test	edx, -1
	je	colcpy_32mixD
; t = alpha(param value : edx)
colcpy_32mixA:
	push	ebx
	push	edx
	colcpy_32_mixA_loop:
	mov	eax, [esi]
	mov	ebx, [edi]		; dst
	mov	edx, eax
	and	edx, 00FF00FFH
	mov	al, bh			; dst
	and	ebx, 00FF00FFH
	sub	edx, ebx
	imul	edx, [esp]
	shr	edx, 8
	add	edx, ebx		; R & B done
	mov	bl, al			; bh = 0
	shr	ax, 8			; ah = 0
	sub	ax, bx
	imul	ax, [esp]
	add	ah, bl
	mov	dh, ah
	mov	[edi], edx
	add	esi, 4
	add	edi, 4
	dec	ecx
	jnz	colcpy_32_mixA_loop
	add	esp, 4
	pop	ebx
	ret
; t = dest.alpha
colcpy_32mixD:
	push	ebx
	push	0
	colcpy_32_mixD_loop:
	mov	edx, [esi]
	mov	ebx, [edi]		; dst
	mov	[esp], edx
	shr	dword ptr [esp], 24
	mov	ax, dx
	and	edx, 00FF00FFH
	mov	al, bh			; dst
	and	ebx, 00FF00FFH
	sub	edx, ebx
	imul	edx, [esp]
	shr	edx, 8
	add	edx, ebx		; R & B done
	mov	bl, al			; bh = 0
	shr	ax, 8			; ah = 0
	sub	ax, bx
	imul	ax, [esp]
	add	ah, bl
	mov	dh, ah
	mov	[edi], edx
	add	esi, 4
	add	edi, 4
	dec	ecx
	jnz	colcpy_32_mixD_loop
	add	esp, 4
	pop	ebx
	ret
; t = dest.alpha * alpha
; colcpy_32mix2:	???

; fill with color(edx) using an alpha map(esi)
colset_32mix:			; in edi dst, esi src, ecx cnt, edx val
	push	ebx
	push	edx
	push	0
	mixset_32_loop:
	mov	al, [esi]		; val
	; test	al, al
	mov	[esp], al
	mov	eax, [esp + 4]		; col
	mov	ebx, [edi]		; dst
	mov	edx, eax
	and	edx, 00FF00FFH
	mov	al, bh			; dst
	and	ebx, 00FF00FFH
	sub	edx, ebx
	imul	edx, [esp]
	shr	edx, 8
	add	edx, ebx		; R & B done
	mov	bl, al			; bh = 0
	shr	ax, 8			; ah = 0
	sub	ax, bx
	imul	ax, [esp]
	add	ah, bl
	mov	dh, ah
	mov	[edi], edx
	add	esi, 1
	add	edi, 4
	dec	ecx
	jnz	mixset_32_loop
	add	esp, 8
	pop	ebx
	ret

end
