; low level raster operations

global gx_getpaddr		; void* gx_getpaddr(gx_Surf, int, int);					Get Pixel Address
global gx_getpixel		; long	gx_getpixel(gx_Surf, int, int);					Get Pixel
global gx_getpix16		; long	gx_getpnear(gx_Surf, fixed16, fixed16, filter);			Get Pixel

global gx_setpixel		; void	gx_setpixel(gx_Surf, int, int, long);				Set Pixel
; global gx_setpline		; void	gx_setpline(gx_Surf, int x1, int y, int x2, long, Pattern);	Pattern Line
global gx_fillrect		; void	gx_fillrect(gx_Surf, int x1, int y1, int x2, int y2, long);	Block (fill rect)
global gx_getcbltf		; gx_cbltf gx_getcbltf(cbltf_type, int depth, int opbpp);
global gx_callcbltf		; void gx_callcbltf(gx_cbltf, void*, void*, unsigned, void*);

;####################################################################################
global colcpy_32_24
global colcpy_32_16
; global colcpy_32_15
global colcpy_32_08
global colcpy_32pal	;rgbcpy_32_08 where: dst[i] = pal[src[i]].rgb
global colcpy_32lum	;rgbcpy_32_08 where: dst[i].r = dst[i].g = dst[i].b = src[i]

global colcpy_24_32
global colcpy_24_16
; global colcpy_24_15
global colcpy_24_08
; global colcpy_24pal		;colcpy_24__08 with edx != 0
; global colcpy_24lum		;colcpy_24__08 with edx == 0

global colcpy_16_32
global colcpy_16_24
; global colcpy_16_15
; global colcpy_16_08
; global colcpy_16pal		;colcpy_16__08 with edx != 0
; global colcpy_16lum		;colcpy_16__08 with edx == 0

global colcpy_15_32
global colcpy_15_24
global colcpy_15_16
; global colcpy_15_08
; global colcpy_15pal		;colcpy_16__08 with edx != 0
; global colcpy_15lum		;colcpy_16__08 with edx == 0

; global colcpy_08_32
; global colcpy_08_24
; global colcpy_08_16
global colcpy_lum32
global colcpy_xtr32

global colcpy_32cpy	;[edi] := [esi]
global colcpy_32mix	;[edi] += (edx * ([esi] - [edi])) >> 8
global colcpy_32and	;[edi] &= [esi]
global colcpy_32ior	;[edi] |= [esi]
global colcpy_32xor	;[edi] ^= [esi]

global colcpy_24cpy
global colcpy_24and
global colcpy_24ior
global colcpy_24xor
global colcpy_16cpy
global colcpy_16and
global colcpy_16ior
global colcpy_16xor
; global colcpy_15cpy
; global colcpy_15and
; global colcpy_15ior
; global colcpy_15xor
global colcpy_08cpy
global colcpy_08and
global colcpy_08ior
global colcpy_08xor

; global colset_32cpy	;[edi] := edx
global colset_32mix	;[edi] += ([esi] * (edx - [edi])) >> 8
; global colset_32and	;[edi] &= edx
; global colset_32ior	;[edi] |= edx
; global colset_32xor	;[edi] ^= edx

; global colset_24cpy
; global colset_24and
; global colset_24ior
; global colset_24xor
; global colset_16cpy
; global colset_16and
; global colset_16ior
; global colset_16xor
; global colset_15cpy
; global colset_15and
; global colset_15ior
; global colset_15xor
; global colset_08cpy
; global colset_08and
; global colset_08ior
; global colset_08xor

;misc

;#global colcpy_32msk	; [edi] := ([edi] & ~edx) | ([esi] | edx)
;#global colcpy_32lut	; [edi] := [edx[esi]]
;#global colcpy_32key	; if ([esi] != edx) {[edi] := [esi]}

struc gx_Clip				; Clip Region
	.clipL:			resw 1		; Horizontal Start	(minX)
	.clipT:			resw 1		; Vertical   Start	(minY)
	.clipR:			resw 1		; Horizontal End	(maxX)
	.clipB:			resw 1		; Vertical   End	(maxY)
endstruc

struc gx_Surf				; Surface
	.clipPtr:			resd 1		; + 04
	.horzRes:			resw 1		; + 06
	.vertRes:			resw 1		; + 08
	.flags:				resw 1		; + 10
	.depth:				resb 1		; + 11
	.pixeLen:			resb 1		; + 12
	.scanLen:			resd 1		; + 16
	.basePtr:			resd 1		; + 20
	.clutPtr:			resd 1		; + 24
	;...
endstruc
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
;\--------------------------------------------------------------/.32

segment data

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

segment code

proc_dummi:ret

; Get Pixel Address
; void* gx_getpaddr(gx_Surf, int x, int y);
gx_getpaddr:
	push	ebx
	mov	ebx, [esp + 4 + 4*1]
	mov	ecx, [esp + 4 + 4*2]
	mov	edx, [esp + 4 + 4*3]

	movzx	eax, word[ebx + gx_Surf.vertRes]
	cmp	edx, eax
	jae	gx_getpaddr_fail
	movzx	eax, word[ebx + gx_Surf.horzRes]
	cmp	ecx, eax
	jae	gx_getpaddr_fail


	movzx	eax, byte[ebx + gx_Surf.depth]
	cmp		eax, 32
	jz		gx_getpaddr_32
	cmp		eax, 8
	jz		gx_getpaddr_08

	gx_getpaddr_fail:
	xor	eax, eax
	pop	ebx
	ret

	gx_getpaddr_08:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*1]
	add	eax, [ebx + gx_Surf.basePtr]
	pop	ebx
	ret

	gx_getpaddr_32:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*4]
	add	eax, [ebx + gx_Surf.basePtr]
	pop	ebx
	ret

; endp

; Get Pixel
; long getpixel(gx_Surf, int x, int y);
gx_getpixel:
	push	ebx
	xor	eax, eax
	mov	ebx, [esp + 4 + 4*1]
	mov	ecx, [esp + 4 + 4*2]
	mov	edx, [esp + 4 + 4*3]

	mov	ax, [ebx + gx_Surf.vertRes]
	cmp	edx, eax
	jae	gx_getpixel_fail
	mov	ax, [ebx + gx_Surf.horzRes]
	cmp	ecx, eax
	jae	gx_getpixel_fail

	movzx	eax, byte[ebx + gx_Surf.depth]
	cmp		eax, 32
	jz		gx_getpixel_32
	cmp		eax, 8
	jz		gx_getpixel_08

	gx_getpixel_fail:
	xor	eax, eax
	pop	ebx
	ret

	gx_getpixel_08:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*1]
	add	eax, [ebx + gx_Surf.basePtr]
	pop	ebx
	movzx	eax, byte[eax]
	ret

	gx_getpixel_32:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*4]
	add	eax, [ebx + gx_Surf.basePtr]
	pop	ebx
	mov	eax, [eax]
	ret

; gx_getpixel endp

; Get Pixel using filter
; long gx_getpix16(gx_Surf, fixed16, fixed16, filter);
gx_getpix16:
	push	esi
	push	ebx
	mov	ebx, [esp + 8 + 4*1]
	mov	ecx, [esp + 8 + 4*2]
	mov	edx, [esp + 8 + 4*3]

	cmp	byte[ebx + gx_Surf.depth], 32
	jnz	gx_getpix16_fail

	cmp	dword[esp + 8 + 4*4], 0
	je	gx_getpix16_near

	movzx	eax, word[ebx + gx_Surf.horzRes]
	sar	ecx, 16
	dec	eax
	; TODO ;inc	ecx
	cmp	ecx, eax
	ja	gx_getpix16_fail
	je	gx_getpix16_lrpY

	movzx	eax, word[ebx + gx_Surf.vertRes]
	sar	edx, 16
	dec	eax
	cmp	edx, eax
	ja	gx_getpix16_fail
	je	gx_getpix16_lrpX

	mov	eax, [ebx + gx_Surf.scanLen]
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx + gx_Surf.basePtr]

	mov	eax, [esi]
	mov	edx, [esi + 4]
	movzx	ecx, byte[esp + 8 + 4*2+1]

	add	esi, [ebx + gx_Surf.scanLen]
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

	movzx	ecx, byte[esp + 8 + 4*3+1]			; alpha y

	gx_getpix16_lrp2:
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

	gx_getpix16_lrpY:
	movzx	eax, word[ebx + gx_Surf.vertRes]
	sar	edx, 16
	dec	eax
	cmp	edx, eax
	ja	gx_getpix16_fail
	je	gx_getpix16_lrp0

	mov	eax, [ebx + gx_Surf.scanLen]
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx + gx_Surf.basePtr]

	mov	eax, [esi]					; first pixel
	add	esi, [ebx + gx_Surf.scanLen]
	movzx	ecx, byte[esp + 8 + 4*2+1]			; alpha x
	mov	edx, [esi]
	jmp	gx_getpix16_lrp2

	gx_getpix16_lrpX:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx + gx_Surf.basePtr]

	mov	eax, [esi]					; first pixel
	movzx	ecx, byte[esp + 8 + 4*3+1]			; alpha y
	mov	edx, [esi + 4]
	jmp	gx_getpix16_lrp2

	gx_getpix16_lrp0:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	edx
	lea	esi, [eax + ecx * 4]
	add	esi, [ebx + gx_Surf.basePtr]
	mov	eax, [esi]					; first pixel
	pop	ebx
	pop	esi
	ret

	gx_getpix16_near:
	sar	edx, 16
	movzx	eax, word[ebx + gx_Surf.vertRes]
	cmp	edx, eax
	jae	gx_getpix16_fail

	sar	ecx, 16
	movzx	eax, word[ebx + gx_Surf.horzRes]
	cmp	ecx, eax
	jae	gx_getpix16_fail

	mov	eax, [ebx + gx_Surf.scanLen]
	mul	edx
	lea	eax, [eax + ecx * 4]
	add	eax, [ebx + gx_Surf.basePtr]
	mov	eax, [eax]
	pop	ebx
	pop	esi
	ret

	gx_getpix16_fail:
	xor	eax, eax
	pop	ebx
	pop	esi
	ret

; gx_getpix16 endp

; Set Pixel
; void setpixel(Surface, int, int, long);
gx_setpixel:
	push	ebx
	push	edi
	%define param(x) [esp + 8 + (4 * x)]

	mov	ebx, param(1)			; surf
	mov	ecx, param(2)			; x
	mov	edx, param(3)			; y

	mov	edi, ebx						; clip
	cmp	dword [edi], 0
	je	gx_setpixel_clip
	mov	edi, [edi]
	gx_setpixel_clip:

	movzx	eax, word[edi + gx_Clip.clipL]
	cmp	ecx, eax
	jl	gx_setpixel_fail

	movzx	eax, word[edi + gx_Clip.clipT]
	cmp	edx, eax
	jl	gx_setpixel_fail

	movzx	eax, word[edi + gx_Clip.clipR]
	cmp	ecx, eax
	jae	gx_setpixel_fail

	movzx	eax, word[edi + gx_Clip.clipB]
	cmp	edx, eax
	jae	gx_setpixel_fail

	movzx	eax, byte[ebx + gx_Surf.depth]
	cmp		eax, 32
	jz		gx_setpixel_32
	cmp		eax, 8
	jz		gx_setpixel_08

	gx_setpixel_fail:
	pop	edi
	pop	ebx
	ret

	gx_setpixel_08:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*1]
	add	eax, [ebx + gx_Surf.basePtr]
	mov edx, param(4)
	mov [eax], dl
	pop	edi
	pop	ebx
	ret

	gx_setpixel_32:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	dx
	shl	edx, 16
	mov	dx, ax
	lea	eax, [edx + ecx*4]
	add	eax, [ebx + gx_Surf.basePtr]
	mov edx, param(4)
	mov [eax], edx
	pop	edi
	pop	ebx
	ret


; void gx_fillrect(gx_Surf dst, int x1, int y1, int x2, int y2, long col);
gx_fillrect:
	push	ebx
	push	edi
	%define param(x) [esp + 8 + (4 * x)]
	%define tempw param(4)
	%define temph param(5)
	; arg x  eq 8 + 4 * x
	mov	ebx, param(1)			; surf
	mov	ecx, param(2)			; x1
	mov	edx, param(3)			; y1

	cmp	ecx, param(4)			; x2
	jl	gx_fillrect_nosx		; x1 < x2
	xchg	ecx, param(4)
	gx_fillrect_nosx:

	cmp	edx, param(5)			; y2
	jl	gx_fillrect_nosy		; y1 < y2
	xchg	edx, param(5)
	gx_fillrect_nosy:

	mov	edi, ebx
	cmp	dword[edi], 0
	je	gx_fillrect_clip
	mov	edi, [edi]
	gx_fillrect_clip:

	movzx	eax, word[edi + gx_Clip.clipL]	; if (x1 < clip.L) x1 = clip.L
	cmp	ecx, eax
	jge	gx_fillrect_x1ok
	mov	ecx, eax
	gx_fillrect_x1ok:
	movzx	eax, word[edi + gx_Clip.clipT]	; if (y1 < clip.T) y1 = clip.T
	cmp	edx, eax
	jge	gx_fillrect_y1ok
	mov	edx, eax
	gx_fillrect_y1ok:
	movzx	eax, word[edi + gx_Clip.clipR]	; if (x2 > clip.R) x2 = clip.R
	cmp	param(4), eax
	jl	gx_fillrect_x2ok
	mov	param(4), eax
	gx_fillrect_x2ok:
	movzx	eax, word[edi + gx_Clip.clipB]	; if (y2 > clip.B) y2 = clip.B
	cmp	param(5), eax
	jl	gx_fillrect_y2ok
	mov	param(5), eax
	gx_fillrect_y2ok:

	sub	param(4), ecx			; if ((x2 -= x1) <= 0) return;
	jle	gx_fillrect_fail		; if CF OR ZF is set
	sub	param(5), edx			; if ((y2 -= y1) <= 0) return;
	jle	gx_fillrect_fail		; if CF OR ZF is set

	; x: ecx
	; y: edx
	; w: [esp + 8 + 4*4]
	; h: [esp + 8 + 5*4]

	movzx	eax, byte[ebx + gx_Surf.depth]
	cmp	eax, 32
	jz	gx_fillrect_4byte
	; cmp	eax, 1
	; je	gx_fillrect_8bpp
	; cmp	eax, 2
	; je	gx_fillrect_16bpp
	; cmp	eax, 3
	; je	gx_fillrect_24bpp
	gx_fillrect_fail:
	pop	edi
	pop	ebx
	ret

	gx_fillrect_4byte:
	mov	eax, [ebx + gx_Surf.scanLen]
	mul	edx
	lea	edi, [eax + ecx * 4]
	add	edi, [ebx + gx_Surf.basePtr]

	mov	eax, param(6)
	mov	ebx, [ebx + gx_Surf.scanLen]

	cmp	dword param(5), 1
	je	gx_sethline_4byte

	cmp	dword param(4), 1
	je	gx_setvline_4byte

	mov	param(1), edi
	gx_fillrect_4loop:
	mov	edi, param(1)
	add	param(1), ebx
	mov	ecx, param(4)
	rep stosd
	dec	dword param(5)
	jnz	gx_fillrect_4loop
	jmp	gx_fillrect_done

	gx_setvline_4byte:
	mov	ecx, param(5)
	gx_setvline_4loop:
	mov	[edi], eax
	add edi, ebx
	dec	ecx
	jnz	gx_setvline_4loop
	jmp	gx_fillrect_done

	gx_sethline_4byte:
	mov	ecx, param(4)
	rep stosd

	gx_fillrect_done:
	pop	edi
	pop	ebx
	ret

; gx_fillrect endp


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
	mov	eax, colcpy_08cpy
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
	mov	eax, colcpy_16cpy
	ret
	colcvt_get_1624:
	cmp	al, 24
	ja	colcvt_get_1632
	jb	colcvt_get_fail
	mov	eax, colcpy_16_24
	ret
	colcvt_get_1632:
	cmp	al, 32
	ja	colcvt_get_fail
	jb	colcvt_get_fail
	mov	eax, colcpy_16_32
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
	mov	eax, colcpy_24_16
	ret
	colcvt_get_2424:
	cmp	al, 24
	ja	colcvt_get_2432
	jb	colcvt_get_fail
	mov	eax, colcpy_24cpy
	ret
	colcvt_get_2432:
	cmp	al, 32
	ja	colcvt_get_fail
	jb	colcvt_get_fail
	mov	eax, colcpy_24_32
	ret

colcvt_get_TO32:
	cmp	al, 8
	ja	colcvt_get_3216
	jb	colcvt_get_fail
	mov	eax, colcpy_32_08
	ret
	colcvt_get_3216:
	cmp	al, 16
	ja	colcvt_get_3224
	jb	colcvt_get_fail
	mov	eax, colcpy_32_16
	ret
	colcvt_get_3224:
	cmp	al, 24
	ja	colcvt_get_3232
	jb	colcvt_get_fail
	mov	eax, colcpy_32_24
	ret
	colcvt_get_3232:
	cmp	al, 32
	ja	colcvt_get_fail
	jb	colcvt_get_fail
	mov	eax, colcpy_32cpy
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

gx_getcbltf: ;proc		; gx_cbltf gx_getcbltf(cbltf_type, int depth, int opbpp);
	mov	eax, [esp+0+4*1]		; cbltf_type
	test	eax, eax
	jz	getcbltf_cnv
	dec	eax
	jz	getcbltf_cpy
	; dec	eax
	; jz	getcbltf_set
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

gx_callcbltf:; proc		; void gx_callcbltf(gx_cbltf, void*, void*, unsigned cnt, void*);
	push	edi
	push	esi
	locals equ 2*4
	mov	edi, [esp+locals+4*2]
	mov	esi, [esp+locals+4*3]
	mov	ecx, [esp+locals+4*4]
	mov	edx, [esp+locals+4*5]
	call dword[esp+locals+4*1]
	pop	esi
	pop	edi
	ret

; gx_callcbltf endp

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

colcpy_24cpy:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08cpy
colcpy_32cpy:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
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

colcpy_24and:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08and
colcpy_32and:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
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

colcpy_24ior:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08ior
colcpy_32ior:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
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

colcpy_24xor:
	lea	ecx, [ecx + ecx*2]			; convert count to bytes : ecx *= 3;
	jmp	colcpy_08xor
colcpy_32xor:
	lea	ecx, [ecx*4]				; convert count to bytes : ecx *= 4;
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
	shr	dword[esp], 24
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
	mov	eax, [esp + 4]	; col
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

; end
