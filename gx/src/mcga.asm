.486p
.model flat, c

public MCGA_open		;void initG(void);
public MCGA_exit		;void closeG(void);
public setpixel			;void putpixel(int x,int y,int c);
public getpixel			;void putpixel(int x,int y,int c);
public blurscr			;void blurscr(void*);
public blurcpy			;void blurscr(void*);
; public copyscr			;void copyscr(void*, rect*);
public setpalette		;void setpal(argb*);
public waitretrace

public initMouse		;int initMouse(void handler(int, mouseinf*));
public exitMouse		;void exitMouse();
public resetMouse		;void resetMouse();
public showMouse		;void showMouse();
public hideMouse		;void hideMouse();
public setMouse			;void setMouse(int x, int y);
public getMouse			;void getMouse(mouse_info*);
public bndMouse			;void bndMouse(mouse_rect*);
public hndMouse			;void bndMouse(mouseinf*);


mouse_info struc
	posx	dw	?
	posy	dw	?
	relx	dw	?
	rely	dw	?
	button	db	?
	whell	db	?
mouse_info ends

rect struc
	mminx	dw	?
	mminy	dw	?
	mmaxx	dw	?
	mmaxy	dw	?
rect ends

.data

	lastMode	dw	?
	mouseInf	mouse_info <?>
	mouseFun	dd	?
	rmcb_seg	dw	0
	rmcb_ofs	dw	0

RmRegs	label			dword
	_edi			label	dword	;+4
	_di			dw	?
				dw	?
	_esi			label	dword	;+8
	_si			dw	?
				dw	?
	_ebp			label	dword	;+12
	_bp			dw	?
				dw	?
	_Reserved		dd	0	;+16
	_ebx			label	dword	;+20
	_bx			label	word
	_bl			db	?
	_bh			db	?
				dw	?
	_edx			label	dword	;+24
	_dx			label	word
	_dl			db	?
	_dh			db	?
				dw	?
	_ecx			label	dword	;+28
	_cx			label	word
	_cl			db	?
	_ch			db	?
				dw	?
	_eax			label	dword	;+32
	_ax			label	word
	_al			db	?
	_ah			db	?
				dw	?
	_flags			dw	?	;+34
	_es			dw	?	;+36
	_ds			dw	?	;+38
	_fs			dw	?	;+40
	_gs			dw	?	;+42
	_ip			dw	?	;+44
	_cs			dw	?	;+46
	_sp			dw	?	;+48
	_ss			dw	0	;+50


.code

MCGA_open proc
	pushad
	mov	ax, 04F03H
	int	010H
	mov	lastMode, bx
	mov	ax, 0013H
	int	010H
	popad
	ret
MCGA_open endp

MCGA_exit proc
	pushad
	mov	ax,lastMode
	int	10H
	popad
	ret
MCGA_exit endp

setpixel proc
	pushad
	cmp	dword ptr [esp+32+4*2], 0
	jl	setpixel_exit
	cmp	dword ptr [esp+32+4*3], 0
	jl	setpixel_exit
	cmp	dword ptr [esp+32+4*2], 320
	jge	setpixel_exit
	cmp	dword ptr [esp+32+4*3], 200
	jge	setpixel_exit
	mov	edi, [esp+32+4*1]
	test	edi, edi
	jnz	setpixel_buff
	mov	edi, 0A0000H
	setpixel_buff:
	add	edi, [esp+32+4*2]
	mov	eax, [esp+32+4*3]
	shl	eax, 6
	add	edi, eax
	shl	eax, 2
	add	edi, eax
	mov	al, [esp+32+4*4]
	mov	[edi], al
	setpixel_exit:
	popad
	ret
setpixel endp

getpixel proc
	xor	eax, eax
	pushad
	cmp	dword ptr [esp+32+4*2], 320
	jae	getpixel_exit
	cmp	dword ptr [esp+32+4*3], 200
	jae	getpixel_exit
	mov	edi, [esp+32+4*1]
	test	edi, edi
	jnz	getpixel_buff
	mov	edi, 0A0000H
	getpixel_buff:
	add	edi, [esp+32+4*2]
	mov	ebx, [esp+32+4*3]
	shl	ebx, 6
	add	edi, ebx
	shl	ebx, 2
	add	edi, ebx
	mov	al, [edi]
	mov	[esp+28], eax
	getpixel_exit:
	popad
	ret
getpixel endp

blurscr proc
	push	ebx
	push	ecx
	push	edx
	mov	ebx, [esp+4*3+4*1]
	mov	edx, [esp+4*3+4*2]
	xor	ax, ax
;Top Left
	mov	al, [ebx+1]
	add	al, [ebx+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_TL_nc
	xor	ax, ax
	blurscr_TL_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx
;Top
	mov	ecx, 318
	blurscr_T_loop:
	mov	al, [ebx-1]
	add	al, [ebx+1]
	adc	ah, 0
	add	al, [ebx+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_T_nc
	xor	ax, ax
	blurscr_T_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx
	dec	ecx
	jnz	blurscr_T_loop
;Top Right
	mov	al, [ebx-1]
	add	al, [ebx+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_TR_nc
	xor	ax, ax
	blurscr_TR_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx
;Interior
	push	198
	blurscr_loop_V:
	mov	al, [ebx-320]
	add	al, [ebx+1]
	adc	ah, 0
	add	al, [ebx+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_L_nc
	xor	ax, ax
	blurscr_L_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx
	mov	ecx, 318
	blurscr_loop_H:
	mov	al, [ebx-320]
	add	al, [ebx-1]
	adc	ah, 0
	add	al, [ebx+1]
	adc	ah, 0
	add	al, [ebx+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_nc
	xor	ax, ax
	blurscr_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx
	dec	ecx
	jnz	blurscr_loop_H
	mov	al, [ebx-320]
	add	al, [ebx-1]
	adc	ah, 0
	add	al, [ebx+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_R_nc
	xor	ax, ax
	blurscr_R_nc:
	shr	ax,2
	mov	[ebx], al
	inc	ebx
	sub	byte ptr [esp], 1
	jnz	blurscr_loop_V
;Botton Left
	mov	al, [ebx-320]
	add	al, [ebx+1]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_BL_nc
	xor	ax, ax
	blurscr_BL_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx
;Botton
	mov	ecx, 318
	blurscr_B_loop:
	mov	al, [ebx-320]
	add	al, [ebx-1]
	adc	ah, 0
	add	al, [ebx+1]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_B_nc
	xor	ax, ax
	blurscr_B_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx
	dec	ecx
	jnz	blurscr_B_loop
;Botton Right
	mov	al, [ebx-320]
	add	al, [ebx-1]
	adc	ah, 0
	sub	ax, dx
	jnc	blurscr_BR_nc
	xor	ax, ax
	blurscr_BR_nc:
	shr	ax, 2
	mov	[ebx], al
	inc	ebx

	pop	edx
	pop	edx
	pop	ecx
	pop	ebx
	ret

blurscr endp

blurcpy proc
	push	edi
	push	esi
	push	ebx
	push	ecx
	push	edx
	xor	eax, eax
	mov	edi, [esp+4*5+4*1]		; dst
	mov	esi, [esp+4*5+4*2]		; src
	mov	edx, [esp+4*5+4*3]		; val
	mov	al, [esi+1]
	add	al, [esi+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_TL_nc
	xor	ax, ax
	blurcpy_TL_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_TL_max
	mov	[edi], al
	blurcpy_TL_max:
	inc	esi
	inc	edi
	mov	ecx, 318
	blurcpy_T_loop:
	mov	al, [esi-1]
	add	al, [esi+1]
	adc	ah, 0
	add	al, [esi+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_T_nc
	xor	ax, ax
	blurcpy_T_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_T_max
	mov	[edi], al
	blurcpy_T_max:
	inc	esi
	inc	edi
	dec	ecx
	jnz	blurcpy_T_loop
	mov	al, [esi-1]
	add	al, [esi+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_TR_nc
	xor	ax, ax
	blurcpy_TR_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_TR_max
	mov	[edi], al
	blurcpy_TR_max:
	inc	esi
	inc	edi
	mov	ebx, 198
	blurcpy_loop_V:
	mov	al, [esi-320]
	add	al, [esi+1]
	adc	ah, 0
	add	al, [esi+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_L_nc
	xor	ax, ax
	blurcpy_L_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_L_max
	mov	[edi], al
	blurcpy_L_max:
	inc	esi
	inc	edi
	mov	ecx, 318
	blurcpy_loop_H:
	mov	al, [esi-320]
	add	al, [esi-1]
	adc	ah, 0
	add	al, [esi+1]
	adc	ah, 0
	add	al, [esi+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_nc
	xor	ax, ax
	blurcpy_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_max
	mov	[edi], al
	blurcpy_max:
	inc	esi
	inc	edi
	dec	ecx
	jnz	blurcpy_loop_H
	mov	al, [esi-320]
	add	al, [esi-1]
	adc	ah, 0
	add	al, [esi+320]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_R_nc
	xor	ax, ax
	blurcpy_R_nc:
	shr	ax,2
	; cmp	[edi], al
	; ja	blurcpy_R_max
	mov	[edi], al
	blurcpy_R_max:
	inc	esi
	inc	edi
	dec	ebx
	jnz	blurcpy_loop_V
	mov	al, [esi-320]
	add	al, [esi+1]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_BL_nc
	xor	ax, ax
	blurcpy_BL_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_BL_max
	mov	[edi], al
	blurcpy_BL_max:
	inc	esi
	inc	edi
	mov	ecx, 318
	blurcpy_B_loop:
	mov	al, [esi-320]
	add	al, [esi-1]
	adc	ah, 0
	add	al, [esi+1]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_B_nc
	xor	ax, ax
	blurcpy_B_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_B_max
	mov	[edi], al
	blurcpy_B_max:
	inc	esi
	inc	edi
	dec	ecx
	jnz	blurcpy_B_loop
	mov	al, [esi-320]
	add	al, [esi-1]
	adc	ah, 0
	sub	ax, dx
	jnc	blurcpy_BR_nc
	xor	ax, ax
	blurcpy_BR_nc:
	shr	ax, 2
	; cmp	[edi], al
	; ja	blurcpy_BR_max
	mov	[edi], al
	blurcpy_BR_max:
	; inc	esi
	; inc	edi

	pop	edx
	pop	ecx
	pop	ebx
	pop	esi
	pop	edi
	ret

blurcpy endp

setpalette proc
	pushad
	mov	ax, 4f08H
	mov	bx, 0800H
	int	010H

	mov	esi, [esp+32+4*1]
	xor	ecx, ecx
	mov	dx, 03C8H
	xor	eax, eax
	out	dx, al
	inc	dx
	setpal_loop:
	lodsd
	rol	eax, 16
	; shr	al, 2
	out	dx, al
	rol	eax, 8
	; shr	al, 2
	out	dx, al
	rol	eax, 8
	; shr	al, 2
	out	dx, al
	inc	cl
	jnz	setpal_loop
	popad
	ret
setpalette endp

waitretrace proc
	push	edx
	mov	dx, 03DAH
	waitvretrace_lp1:
	in	al, dx
	test	al, 08H
	jnz	waitvretrace_lp1
	waitvretrace_lp2:
	in	al, dx
	test	al, 08H
	jz	waitvretrace_lp2
	pop	edx
	ret

waitretrace endp

initMouse proc
	xor	eax, eax
	pushad
	int	033H
	test	ax, ax
	jz	initMouse_exit
	mov	dword ptr [esp+28], 1
	mov	eax, [esp+32+4*1]
	mov	mouseFun, eax
	test	eax, eax
	jz	initMouse_exit

	; allocate real mode callback
	push	ds
	mov	ax, ds
	mov	es, ax
	mov	ax, cs
	mov	ds, ax
	mov	esi, offset mouse_srt_func
	mov	edi, offset RmRegs
	mov	ax, 0303H
	int	031H
	pop	ds
	jc	initMouse_exit			;DPMIError
	mov	dword ptr [esp+28], 2

	mov	rmcb_seg, cx
	mov	rmcb_ofs, dx
	mov	ax, ds
	mov	es, ax
	mov	_ss, ss
	mov	_es, cx
	mov	_dx, dx
	mov	_ax, 0CH
	mov	ecx, [esp+32+4*2]
	mov	_ecx, ecx
	mov	ax, 0300H
	mov	bx, 033H
	mov	cx, 0
	mov	edi, offset RmRegs
	int	031H

	initMouse_exit:
	popad
	ret

mouse_srt_func:
	push	ds				; save stack and transfer
	mov	ax, es
	mov	ds, ax
	mov	ax, [_cx]
	mov	mouseInf.posx, ax
	mov	ax, [_dx]
	mov	mouseInf.posy, ax
	mov	ax, [_si]
	mov	mouseInf.relx, ax
	mov	ax, [_di]
	mov	mouseInf.rely, ax
	mov	ax, [_bx]
	mov	mouseInf.button, al
	mov	mouseInf.whell, ah
	movzx	eax, [_ax]
	push	offset mouseInf
	push	eax
	call	mouseFun
	add	esp, 8
	pop	ds
	cld
	lodsw
	mov	es:[_ip], ax
	lodsw
	mov	es:[_cs], ax
	add	es:[_sp], 4
	iretd

initMouse endp

exitMouse proc
	pushad
	mov	cx, rmcb_seg
	mov	dx, rmcb_ofs			; CX : DX Call Back Function
	mov	ax, 0304H			; DPMI Function 0304h
	int	31H				; Free Real Mode Call Back
	popad
	ret
exitMouse endp

resetMouse proc
	pushad
	mov	ax, 021H
	int	033H
	popad
	ret

resetMouse endp

showMouse proc
	pushad
	mov	ax, 1
	int	033H
	popad
	ret

showMouse endp

hideMouse proc
	pushad
	mov	ax, 2
	int	033H
	popad
	ret

hideMouse endp

setMouse proc
	pushad
	mov	ax, 04h
	mov	cx, [esp+32+4*1]
	mov	dx, [esp+32+4*2]
	int	033H
	popad
	ret

setMouse endp

getMouse proc
	pushad
	mov	ax, 3
	int	033h
	mov	eax, [esp+32+4*1]
	mov	[eax].mouse_info.posx, cx
	mov	[eax].mouse_info.posy, dx
	mov	[eax].mouse_info.button, bl
	mov	[eax].mouse_info.whell, bh
	popad
	ret

getMouse endp

bndMouse proc
	pushad
	mov	ebp, [esp+32+4*1]
	mov	ax, 7
	mov	cx, [ebp].rect.mminx
	mov	dx, [ebp].rect.mmaxx
	int	033H
	mov	ax, 8
	mov	cx, [ebp].rect.mminy
	mov	dx, [ebp].rect.mmaxy
	int	033H
	popad
	ret

bndMouse endp

hndMouse proc
	mov	eax, [esp+32+4*1]
	test	eax, eax
	jnz	hndMouse_set
	mov	eax, offset dummy
	hndMouse_set:
	mov	mouseFun, eax
	dummy:
	ret

hndMouse endp

end
