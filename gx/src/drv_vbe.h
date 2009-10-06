#ifndef __gx_VBE_h
#define __gx_VBE_h

#include "g2_surf.h"

#define MD_LINEAR 0x4000
#define MD_DNTCLR 0x8000

typedef struct CtrlInfo_t {
	unsigned char	VbeSignature[4];
	unsigned short	VbeVersion;
	unsigned char*	OemStringPtr;
	unsigned long	Capabilities;
	unsigned short*	VideoModePtr;
	unsigned short	TotalMemory;
	unsigned short	OemSoftwareRev;
	unsigned char*	OemVendorNamePtr;
	unsigned char*	OemProductNamePtr;
	unsigned char*	OemProductRevPtr;
	unsigned char	Reserved[222];
	unsigned char	OemData[256];
} CtrlInfo_s, *CtrlInfo;

typedef struct ModeInfo_t {
	unsigned short	ModeAttributes;
	unsigned char	WinAAttributes;
	unsigned char	WinBAttributes;
	unsigned short	WinGranularity;
	unsigned short	WinSize;
	unsigned short	WinASegment;
	unsigned short	WinBSegment;
	unsigned long	WinFuncPtr;
	unsigned short	BytesPerScanLine;
	unsigned short	XResolution;
	unsigned short	YResolution;
	unsigned char	XCharSize;
	unsigned char	YCharSize;
	unsigned char	NumberOfPlanes;
	unsigned char	BitsPerPixel;
	unsigned char	NumberOfBanks;
	unsigned char	MemoryModel;
	unsigned char	BankSize;
	unsigned char	NumberOfImagePages;
	unsigned char	_Reserved;
	unsigned char	RedMaskSize;
	unsigned char	RedFieldPosition;
	unsigned char	GreenMaskSize;
	unsigned char	GreenFieldPosition;
	unsigned char	BlueMaskSize;
	unsigned char	BlueFieldPosition;
	unsigned char	RsvdMaskSize;
	unsigned char	RsvdFieldPosition;
	unsigned char	DirectColorModeInfo;
	unsigned long	PhysBasePtr;
	unsigned long	OffScreenMemOffset;
	unsigned short	OffScreenMemSize;
	unsigned char	__Reserved[206];
} ModeInfo_s, *ModeInfo;

typedef struct CRTCInfo_t {
	unsigned short	HorizontalTotal;
	unsigned short	HorizontalSyncStart;
	unsigned short	HorizontalSyncEnd;
	unsigned short	VerticalTotal;
	unsigned short	VerticalSyncStart;
	unsigned short	VerticalSyncEnd;
	unsigned char	Flags;
	unsigned long	PixelClock;
	unsigned short	RefreshRate;
	unsigned char	Reserved[40];
} CRTCInfo_s, *CRTCInfo;

#pragma pack(1)

#if __cplusplus
extern "C" {
#endif	// __cplusplus

/**	gx_vbeinit : initialize vbe
 * \param	: none
 * \return	: 
 *	= 0 on success
 *	< 0 on error :
 *		-1 DPMI error
 *		...
**/

extern int gx_vbeinit();

/**	gx_vbeexit : exit vbe
 * \param	: none
 * \return	: none
**/

extern void gx_vbeexit();
//~ extern int gx_vbeinfo(VesaInfoBlock* dst);
//~ extern int gx_modeinfo(ModeInfoBlock* dst, int mode);

/**	gx_modeopen : open video mode
 * \param	: X Resolution
 *		: Y Resolution
 *		: Depth
 *		: CRTC info for advanced timmings 
 *		  pass 0 for defaults
 * \return	: = 0 on success
 *			: < 0 on error
 *			:  -1 on mode not found in list
 *			:  -2 if vbeinit was not called
 *			:  -3 if Physical Address Mapping failed
 *			: > 0 dpmi error : hiword(0x31), loword(?)
**/

extern int gx_modeopen(gx_Surf*, int XRes, int YRes, int Flgs, CRTCInfo);

extern void gx_wait_retr();

//~ extern int gx_vbesurf(gx_Surf dst, int page);

extern int gx_vgacopy(gx_Surf *src, int page);

#if __cplusplus
}
#endif	// __cplusplus

#endif	// __gx_VBE_h
