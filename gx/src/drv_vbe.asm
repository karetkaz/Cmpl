;VESA BIOS EXTENSION (VBE)
;Core Functions Standard

.386p
.model flat, c
; option casemap : none
public gx_vbeinit		; int gx_vbeinit();
public gx_vbeexit		; int gx_vbeexit();
; public gx_vbeinfo		; int gx_vbeinfo(VesaInfoBlock* dst);
; public gx_modeinfo		; int gx_modeinfo(ModeInfoBlock* dst, int mode);
public gx_modeopen		; int gx_modeopen(gx_Surf, int Hres, int Vres, int Depth, CRTCInfo crtc = 0);
; public gx_modequit		; int gx_modequit();
; public gx_setmode
; public gx_getmode

; public gx_setscnlen
; public gx_getscnlen

; public gx_setdsp_st
; public gx_getdsp_st

; public gx_setdsp_pm
; public gx_detdsp_pm

public gx_wait_retr

; public gx_vbesurf		; int gx_vbesurf(durface*, int page = 0);
public gx_vgacopy		; int gx_vbesurf(durface*, int page = 0);

public VbeVInf
public VbeMInf

VesaInfoBlock struc
	VbeSignature		dd 0		; +4	VBE Signature
	VbeVersion		dw 0300h	; +6	VBE Version
	OemStringPtr		dd ?		; +10	VbeFarPtr to OEM String
	Capabilities		db 4 dup (?)	; +14	Capabilities of graphics controller
	VideoModePtr		dd ?		; +18	VbeFarPtr to VideoModeList
	TotalMemory		dw ?		; +20	Number of 64kb memory blocks
						; Added for VBE 2.0+
	OemSoftwareRev		dw ?		; +22	VBE implementation Software revision
	OemVendorNamePtr	dd ?		; +26	VbeFarPtr to Vendor Name String
	OemProductNamePtr	dd ?		; +30	VbeFarPtr to Product Name String
	OemProductRevPtr	dd ?		; +34	VbeFarPtr to Product Revision String
	Reserved		db 222 dup (?)	; +256	Reserved for VBE implementation scratch
	OemData 		db 256 dup (?)	; +512. Data Area for OEM Strings
VesaInfoBlock ends

ModeInfoBlock struc
	; Mandatory information for all VBE revisions
	ModeAttributes		dw ?		; +2	mode attributes
	WinAAttributes		db ?		; +3	window A attributes
	WinBAttributes		db ?		; +4	window B attributes
	WinGranularity		dw ?		; +6	window granularity
	WinSize 		dw ?		; +8	window size
	WinASegment		dw ?		; +10	window A start segment
	WinBSegment		dw ?		; +12	window B start segment
	WinFuncPtr		dd ?		; +16	real mode pointer to window function
	BytesPerScanLine	dw ?		; +18	bytes per scan line
	; Mandatory information for VBE 1.2 and above
	XResolution		dw ?		; +20	horizontal resolution in pixels or characters3
	YResolution		dw ?		; +22	vertical resolution in pixels or characters
	XCharSize		db ?		; +23	character cell width in pixels
	YCharSize		db ?		; +24	character cell height in pixels
	NumberOfPlanes		db ?		; +25	number of memory planes
	BitsPerPixel		db ?		; +26	bits per pixel
	NumberOfBanks		db ?		; +27	number of banks
	MemoryModel		db ?		; +28	memory model type
	BankSize		db ?		; +29	bank size in KB
	NumberOfImagePages	db ?		; +30	number of images
	Reserved		db 1		; +31	reserved for page function
	; Direct Color fields (required for direct/6 and YUV/7 memory models)
	RedMaskSize		db ?		; +32	size of direct color red mask in bits
	RedFieldPosition	db ?		; +33	bit position of lsb of red mask
	GreenMaskSize		db ?		; +34	size of direct color green mask in bits
	GreenFieldPosition	db ?		; +35	bit position of lsb of green mask
	BlueMaskSize		db ?		; +36	size of direct color blue mask in bits
	BlueFieldPosition	db ?		; +37	bit position of lsb of blue mask
	RsvdMaskSize		db ?		; +38	size of direct color reserved mask in bits
	RsvdFieldPosition	db ?		; +39	bit position of lsb of reserved mask
	DirectColorModeInfo	db ?		; +40	direct color mode attributes
	; Mandatory information for VBE 2.0 and above
	PhysBasePtr		dd ?		; +44	physical address for flat memory frame buffer
	Reserved1		dd 0		; +48	Reserved - always set to 0
	Reserved2		dw 0		; +50	Reserved - always set to 0
	; Mandatory information for VBE 3.0 and above
	LinBytesPerScanLine	dw ?		; +52	bytes per scan line for linear modes
	BnkNumberOfImagePages	db ?		; +53	number of images for banked modes
	LinNumberOfImagePages	db ?		; +54	number of images for linear modes
	LinRedMaskSize		db ?		; +55	size of direct color red mask (linear modes)
	LinRedFieldPosition	db ?		; +56	bit position of lsb of red mask (linear modes)
	LinGreenMaskSize	db ?		; +57	size of direct color green mask (linear modes)
	LinGreenFieldPosition	db ?		; +58	bit position of lsb of green mask (linear modes)
	LinBlueMaskSize 	db ?		; +59	size of direct color blue mask (linear modes)
	LinBlueFieldPosition	db ?		; +60	bit position of lsb of blue mask (linear modes)
	LinRsvdMaskSize 	db ?		; +61	size of direct color reserved mask (linear modes)
	LinRsvdFieldPosition	db ?		; +62	bit position oflsbof reserved mask (linear modes)
	MaxPixelClock		dd ?		; +66	maximum pixel clock (in Hz) for graphics mode
	Reserved3		db 189 dup (?)	; remainder of ModeInfoBlock
ModeInfoBlock ends

CRTCInfoBlock struc
	HorizontalTotal 	dw ?		; +2	Horizontal total in pixels
	HorizontalSyncStart	dw ?		; +4	Horizontal sync start in pixels
	HorizontalSyncEnd	dw ?		; +6	Horizontal sync end in pixels
	VerticalTotal		dw ?		; +8	Vertical total in lines
	VerticalSyncStart	dw ?		; +10	Vertical sync start in lines
	VerticalSyncEnd 	dw ?		; +12	Vertical sync end in lines
	CRTCFlags		db ?		; +13	Flags (Interlaced, Double Scan etc)
	PixelClock		dd ?		; +17	Pixel clock in units of Hz
	RefreshRate		dw ?		; +19	Refresh rate in units of 0.01 Hz
	Reserved		db 40 dup (?)	;	remainder of ModeInfoBlock
CRTCInfoBlock ends

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
	offsPtr			dd ?		; + 28
	movePtr			dd ?		; + 32
gx_Surf ends

; extern gx_initdraw : proc

.data

;-------------------------------------------------------------------------------
;	Real Mode Registers
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
;	Real Mode Registers

	dos_memsel		dw 0
	dos_memseg		dw 0
	dos_memadr		dd 0

	VbeBuff			db 1024 dup (0) ; Buffer for vbe
	VbeEofB			label dword	; end of vbe buffer
	VbeMInf			ModeInfoBlock	<0>
	VbeVInf			VesaInfoBlock	<0>

	; VbeBPtr			dd 0
	; VbeBPtr			dd 0

	PageBase		dd 0
	PageSize		dd 0
	ScanSize		dd 0

	LastMode		dw 0
	LastBank		dw 0

	LastPage		db 0		; Last video page

	; VbeBankSize		dw 0		; Size of bank
	; VbeActvBank		dw 0		; Active bank nr


;VbeFlags : -----------	Mask ----------	Bit ---	Meaning(1/0) -------------------
; F_opn		equ	0x01		;0	Mode is Open (y/n)
; F_vbe		equ	0x02		;0	VBE Detect happend(y/n)
; F_lin		equ	0x04		;1	Linear / Banked mode
; F_wpr		equ	0x08		;2	Wait for page retrace (y/n)
; F_bym		equ	0x10		;3	Byte / Pixel mode (def 0)
; F_1st		equ	0x20		;5	first Mode opening happend
		; equ	0x40		;6	?
; F_err		equ	0x80		;7	?
;-------------------------------------------------------------------------------

; //~ FUNCTION 00H - RETURN VBE CONTROLLER INFORMATION
; //~ FUNCTION 01H - RETURN VBE MODE INFORMATION
; //~ FUNCTION 02H - SET VBE MODE
; //~ FUNCTION 03H - RETURN CURRENT VBE MODE
; //X FUNCTION 04H - SAVE/RESTORE STATE
; //X FUNCTION 05H - DISPLAY WINDOW CONTROL
; //~ FUNCTION 06H - SET/GET LOGICAL SCAN LINE LENGTH
; //~ FUNCTION 07H - SET/GET DISPLAY START
; //~ FUNCTION 08H - SET/GET DAC PALETTE FORMAT
; //~ FUNCTION 09H - SET/GET PALETTE DATA
; //~ FUNCTION 0AH - RETURN VBE PROTECTED MODE INTERFACE
; //~ FUNCTION 0BH - GET/SET PIXEL CLOCK
; //~ FUNCTION 10H - GET/SET DISPLAY POWER STATE

_int macro int_nr
	pushad
	mov	edi, offset RmRegs
	mov	ax, 0300H
	mov	bl, int_nr
	xor	bh, bh
	xor	cx, cx
	int	031H
	popad
endm

lpa macro reg32, mem32			;seg:offs => (seg<<4)+offs
	push	mem32
	movzx	reg32, word ptr [esp+2]
	mov	word ptr [esp+2], 0
	shl	reg32, 4
	add	reg32, dword ptr [esp]
	add	esp, 4
endm

.code

gx_vbeinit:		; int gx_vbeinit();
	; Test for first time call
	cmp	dos_memadr, 0
	je	gx_vbeinit_init
	xor	eax, eax
	ret

	gx_vbeinit_init:
	pushad
	mov	ax, 0100H		; get 512 bytes of dos memory
	mov	bx, 512 shr 4
	int	031H
	jnc	gx_vbeinit_memgd
	popad
	mov	eax, 0310100H
	ret

	gx_vbeinit_memgd:		; dos mem alloc done
	mov	dos_memseg, ax		; Initial real mode segment of allocated block
	and	eax, 0000FFFFH
	mov	dos_memsel, dx		; Selector for allocated block
	shl	eax, 4
	mov	dos_memadr, eax		; 32 bit addres
	
	; Init VBE ax = 04F00H int 10h
	mov	ax, dos_memseg
	mov	_es, ax
	mov	_di, 0
	mov	_ax, 04F00H
	_int	010H
	jnc	gx_vbeinit_dpmid
	; DPMI error
	popad
	mov	eax, 0310300H
	ret

	gx_vbeinit_dpmid:
	cmp	_ah, 00H		; ah = 0 fnc sucessful, al = 4f fn suported
	je	gx_vbeinit_vbeid
	; VESA error			_ax
	popad
	mov	eax, 0104F00H
	ret

	gx_vbeinit_vbeid:
	mov	esi, dos_memadr
	; cmp	dword ptr [esi], 'ASEV'			; this wont happend
	; seterr	0xffff				; no Vesa signature present
	; jne	getvbeinfo_fail

	; Copy the VBE information block
	mov	edi, offset VbeVInf
	mov	ecx, 256
	rep movsb
	mov	edi, offset VbeBuff

	; Copy the OEM string to local mem
	lpa	esi, VbeVInf.OemStringPtr
	mov	VbeVInf.OemStringPtr, edi
	gx_vbeinit_scpy1:
	lodsb
	stosb
	test	al, al
	jnz	gx_vbeinit_scpy1

	; Set Oem Vendor Name
	lpa	esi,VbeVInf.OemVendorNamePtr
	mov	VbeVInf.OemVendorNamePtr, edi
	gx_vbeinit_scpy2:
	lodsb
	stosb
	test	al, al
	jnz	gx_vbeinit_scpy2

	; Set Oem Product Name
	lpa	esi, VbeVInf.OemProductNamePtr
	mov	VbeVInf.OemProductNamePtr, edi
	gx_vbeinit_scpy3:
	lodsb
	stosb
	test	al, al
	jnz	gx_vbeinit_scpy3

	; Set Oem Product Rev
	lpa	esi, VbeVInf.OemProductRevPtr
	mov	VbeVInf.OemProductRevPtr, edi
	gx_vbeinit_scpy4:
	lodsb
	stosb
	test	al, al
	jnz	gx_vbeinit_scpy4

	; Copy the mode list
	lpa	esi, VbeVInf.VideoModePtr
	mov	VbeVInf.VideoModePtr, edi
	gx_vbeinit_mcpy1:
	lodsw
	stosw
	cmp	ax, -1
	jne	gx_vbeinit_mcpy1

	; Get curent mode	!!!
	mov	_ax, 04F03H			; Get Current VESA Mode
	_int	010H
	; cmp	_ax, 004FH			; ah = 0 fnc sucessful, al = 4f fn suported
	; jnz	gx_vbeinit_stlm
	; gx_vbeinit_stlm:			; set last mode
	mov	ax, _bx
	mov	LastMode, ax
	popad
	xor	eax, eax
	ret

gx_vbeexit:		; int gx_vbeexit();
	; Test for gx_vbeinit
	cmp	dos_memadr, 0
	jne	gx_vbeexit_exit
	ret
	; Relase dos Memory
	gx_vbeexit_exit:
	pushad
	mov	ax, 0101H
	mov	dx, dos_memsel
	int	031H
	; Remove phisical address mapping, if it was used
	cmp	PageBase, 0
	jz	gx_vbeexit_rlvm
	mov	edi, PageBase
	mov	cx, di
	shr	edi, 16
	mov	bx, di
	mov	ax, 0801H
	int	031H
	gx_vbeexit_rlvm:
	; Restore last Mode
	mov	_ax, 04F02H
	mov	ax, lastMode
	; mov	_bx, ax
	mov	_bx, 3
	_int	010H
	mov	dos_memseg, 0
	mov	dos_memsel, 0
	mov	dos_memadr, 0
	mov	PageBase, 0
	popad
	ret

gx_modeopen:		; int gx_modeopen(gx_Surf, int Hres, int Vres, int Depth, CRTCInfo crtc = 0);
	; Test for gx_vbeinit
	cmp	dos_memadr, 0
	jne	gx_modeopen_open
	mov	eax, -2
	ret

	gx_modeopen_open:			; Load next mode from list and its ModeInfoBlock
	pushad
	mov	esi, VbeVInf.VideoModePtr
	mov	edx, dos_memadr

	gx_modeopen_next:
	lodsw
	cmp	ax, -1
	je	gx_modeopen_fail
	mov	_ax, 04F01H			; Get ModeInfoBlock for mode
	mov	_cx, ax
	_int	010H
	jnc	gx_modeopen_test
	; DPMI error		0xff11
	popad
	mov	eax, 0310300H
	ret

	gx_modeopen_test:
	mov	ax, [edx].ModeInfoBlock.XResolution
	cmp	ax, [esp + 32 + 4*2]
	jne	gx_modeopen_next
	mov	ax, [edx].ModeInfoBlock.YResolution
	cmp	ax, [esp + 32 + 4*3]
	jne	gx_modeopen_next
	mov	al, [edx].ModeInfoBlock.BitsPerPixel
;	add	al, [edx].ModeInfoBlock.LinRsvdMaskSize		;RsvdFieldPosition
	cmp	al, [esp + 32 + 4*4]
	jne	gx_modeopen_next
	; test for graphical screen ?

	; Copy ModeInfoBlock
	mov	edi, offset VbeMInf
	mov	esi, dos_memadr
	mov	ecx, 256 / 4		; 64 dwords
	rep	movsd

	; Remove phisical address mapping, if it was used
	cmp	PageBase, 0
	jz	gx_modeopen_setm
	mov	edi, PageBase
	mov	cx, di
	shr	edi, 16
	mov	bx, di
	mov	ax, 0801H
	int	031H
	jnc	gx_modeopen_setm
	popad
	mov	eax, 0310801H
	ret

	; Set the mode
	gx_modeopen_setm:
	mov	PageBase, 0
	mov	PageSize, 0
	mov	ax, _cx
	mov	bx, [esp + 32 + 4*4]
	and	ax, 0000000111111111b
	and	bx, 1100000000000000b
	or	ax, bx
	mov	_bx, ax
	; use custom CRTC timmings ?
	mov	esi, [esp + 32 + 4*5]
	test	esi, esi
	jz	gx_modeopen_crtc
	mov	edi, dos_memadr
	mov	ecx, 59
	rep	movsb				; copy 59 bytes
	or	_bx, 0000100000000000b		; D11 Use user specified CRTC values for refresh rate
	mov	ax, dos_memsel
	mov	_es, ax
	mov	_di, 0

	gx_modeopen_crtc:			; default timmings
	mov	_ax, 04F02H
	_int	010H
	; jnc	????
	cmp	_ah, 0
	je	gx_modeopen_save
	; VESA error				_ax
	popad
	mov	eax, 0104F02H
	ret

	gx_modeopen_save:
	mov	al, VbeMInf.NumberOfImagePages
	mov	LastPage, al

	mov	ax, VbeMInf.BytesPerScanLine
	mul	VbeMInf.YResolution
	shl	edx, 16
	mov	dx, ax
	mov	PageSize, edx

	movzx	edx, VbeMInf.BytesPerScanLine
	mov	ScanSize, edx

	test	word ptr [esp + 32 + 4*4], 0100000000000000b		; D14 Use linear/flat frame buffer model
	jz	gx_modeopen_exit

	; Set Physical Address Mapping + Linear Addressing
	mov	ebx, VbeMInf.PhysBasePtr
	mov	ecx, ebx
	shr	ebx, 16
	movzx	esi, VbeVInf.TotalMemory
	shl	esi, 16
	mov	edi, esi
	shr	esi, 16				; SI:DI = region size
	mov	ax, 0800H
	int	031H
	jnc	gx_modeopen_pamd
	; DPMI error
	popad
	mov	eax, 0310800H
	ret

	gx_modeopen_pamd:
	shl	ebx, 16
	mov	bx, cx
	mov	PageBase, ebx
	; movzx	edx, VbeMInf.LinBytesPerScanLine
	; mov	ScanSize, edx

	; gx_modeopen_surf:

	gx_modeopen_exit:
	mov	edx, [esp + 32 + 4*1]
	test	edx, edx
	jz	gx_modeopen_exit1

	mov	[edx].gx_Surf.clipPtr, 0
	mov	ax, VbeMInf.XResolution
	mov	[edx].gx_Surf.horzRes, ax
	mov	ax, VbeMInf.YResolution
	mov	[edx].gx_Surf.vertRes, ax
	mov	[edx].gx_Surf.flags, 3
	mov	al, VbeMInf.BitsPerPixel
	mov	[edx].gx_Surf.depth, al
	inc	al
	shr	al, 3		;/8
	mov	[edx].gx_Surf.pixeLen, al
	mov	eax, ScanSize
	mov	[edx].gx_Surf.scanLen, eax
	mov	[edx].gx_Surf.clutPtr, 0
	mov	eax, PageBase
	mov	[edx].gx_Surf.basePtr, eax
	; push	0
	; push	edx
	; call	gx_initdraw
	; add	esp, 8
	gx_modeopen_exit1:
	popad
	xor	eax, eax
	ret

	gx_modeopen_fail:			; video mode not found
	popad
	mov	eax, 0FF0000H
	ret

gx_wait_retr:
	push	edx
	mov	dx, 03DAH
	waitretrace_lp1:
	in	al, dx
	test	al, 08H
	jnz	waitretrace_lp1
	waitretrace_lp2:
	in	al, dx
	test	al, 08H
	jz	waitretrace_lp2
	pop	edx
	ret

; gx_vbesurf:
	; mov	edx, [esp + 4*1]
	; mov	[edx].gx_Surf.clipPtr, 0
	; mov	ax, VbeMInf.XResolution
	; mov	[edx].gx_Surf.horzRes, ax
	; mov	ax, VbeMInf.YResolution
	; mov	[edx].gx_Surf.vertRes, ax
	; mov	[edx].gx_Surf.flags, 3
	; mov	al, VbeMInf.BitsPerPixel
	; mov	[edx].gx_Surf.depth, al
	; inc	al
	; shr	al, 3		;/8
	; mov	[edx].gx_Surf.pixelLen, al
	; mov	ax, VbeMInf.BytesPerScanLine
	; and	eax, 0000FFFFH
	; mov	[edx].gx_Surf.scnLnLen, eax
	; mov	[edx].gx_Surf.clutPtr, 0
	; mov	eax, [esp + 4*2]		; page
	; imul	eax, PageSize
	; add	eax, PageBase
	; mov	[edx].gx_Surf.basePtr, eax
	; push	0
	; push	edx
	; call	gx_initdraw
	; add	esp, 8
	; ret

gx_vgacopy:	;int gx_vgacopy(gx_Surf src, int page);
	mov	eax, [esp + 4*1]		; src Surface
	mov	edx, [esp + 4*2]		; page number
	; cmp	dl, LastPage
	; ja	gx_vgacopy_fail

	; mov	edx, [esp + 4*2]		; page
	; imul	edx, PageSize
	; add	edx, PageBase

	cmp	PageBase, 0
	jnz	gx_vgacopy_flat
	; gx_vgacopy_bank
	push	edi
	push	esi
	push	ebx
	mov	edx, PageSize
	mov	esi, [eax].gx_Surf.basePtr
	mov	LastBank, 0
	copy_bank:
	mov	ax, LastBank
	mov	_ax, 04F05H
	mov	_bx, 00000H
	mov	_dx, ax
	_int	010H
	inc	LastBank

	mov	ecx, 65536		; !!!
	sub	edx, ecx
	jle	gx_vgacopy_last
	mov	edi, 0A000H shl 4
	shr	ecx, 2
	rep	movsd
	jmp	copy_bank

	gx_vgacopy_last:
	add	edx, ecx
	mov	ecx, edx
	mov	edi, 0A000H shl 4
	shr	ecx, 2
	rep	movsd
	gx_vgacopy_byte:
	and	edx, 11b
	jz	gx_vgacopy_done
	movsb
	dec	ecx
	jmp	gx_vgacopy_byte

	gx_vgacopy_done:
	pop	ebx
	pop	esi
	pop	edi
	ret
	
	gx_vgacopy_flat:
	push	edi
	push	esi
	mov	ecx, PageSize
	mov	edi, PageBase
	mov	esi, [eax].gx_Surf.basePtr
	shr	ecx, 2
	rep	movsd
	pop	esi
	pop	edi
	ret

end
