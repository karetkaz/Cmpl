;###############################################################################
; Color Convert functions
; extern void callcolcvt(void colcpy(void), void* dst, void* src, unsigned cnt, void* pal);
; #pragma aux callcolcvt\
		; parm  [eax] [edi] [esi] [ecx] [edx]\
		; modify [eax edx ecx esi edi]\
		; value [] = "call eax";

; edi : dst
; esi : src
; ecx : cnt
; edx : unused / palette

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
; extern void callcolcpy(void colcpy(void), void* dst, void* src, unsigned cnt);
; #pragma aux callcolcpy\
		; parm  [eax] [edi] [esi] [ecx]\
		; modify [eax edx ecx esi edi]\
		; value [] = "call eax";

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
; extern void callcolset(void colcpy(void), void* dst, unsigned cnt, long col);
; #pragma aux callcolset\
		; parm  [eax] [edi] [ecx] [edx]\
		; modify [eax edx ecx edi]\
		; value [] = "call eax";


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
	test	edi, 010b				; destination is WORD alligned ?
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
