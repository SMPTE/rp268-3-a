/***************************************************************************
*    Copyright (c) 2013-2019, Broadcom Inc.
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are
*  met:
*
*  1. Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*
*  3. Neither the name of the copyright holder nor the names of its
*     contributors may be used to endorse or promote products derived from
*     this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
*  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
*  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
*  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
*  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#ifndef DPX_H
#define DPX_H


/*** dpx.h ***/
#include <stdint.h>
#include "vdo.h"

#ifndef HDR_DPX_H
#define DPX_ERROR_UNRECOGNIZED_CHROMA  -1
#define DPX_ERROR_UNSUPPORTED_BPP      -2
#define DPX_ERROR_NOT_IMPLEMENTED      -3
#define DPX_ERROR_MISMATCH             -4
#define DPX_ERROR_BAD_FILENAME         -5
#define DPX_ERROR_CORRUPTED_FILE       -6
#define DPX_ERROR_MALLOC_FAIL          -7

enum DPX_VER_e { STD_DPX_VER = 0, DVS_DPX_VER = 1};

#ifdef WIN32
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef float SINGLE;
#define snprintf sprintf_s
#else
#if (defined(SOLARIS) || defined(__sparc__))
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef float SINGLE;
#else
#if (defined(__linux__) || defined(__APPLE__))
#include <stdint.h>
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef float SINGLE;
#endif /* linux */
#endif /* !sparc */
#endif /* !win */
#endif /* HDR_DPX_H */

#define FILENAME_SIZE 99

typedef struct _DPX_GenericFileHeader
{
	DWORD Magic;           /* Magic number */
	DWORD ImageOffset;     /* Offset to start of image data (bytes) */
	char  Version[8];      /* Version stamp of header format */
	DWORD FileSize;        /* Total DPX file size (bytes) */
	DWORD DittoKey;        /* Image content specifier */
	DWORD GenericSize;     /* Generic section header length (bytes) */
	DWORD IndustrySize;    /* Industry-specific header length (bytes) */
	DWORD UserSize;        /* User-defined data length (bytes) */
	char  FileName[FILENAME_SIZE + 1];   /* Name of DPX file */
	char  TimeDate[24];    /* Time & date of file creation */
	char  Creator[100];    /* Name of file creator */
	char  Project[200];    /* Name of project */
	char  Copyright[200];  /* Copyright information */
	DWORD EncryptKey;      /* Encryption key */
	char  Reserved16[104];   /* Reserved field 16 */
} DPX_GENERICFILEHEADER;

typedef struct _DPX_ImageElement
{
	DWORD  DataSign;          /* Data sign extension */
	DWORD  LowData;           /* Reference low data code value */
	SINGLE LowQuantity;       /* reference low quantity represented */
	DWORD  HighData;          /* Reference high data code value */
	SINGLE HighQuantity;      /* reference high quantity represented */
	BYTE   Descriptor;        /* Descriptor for image element */
	BYTE   Transfer;          /* Transfer characteristics for element */
	BYTE   Colorimetric;      /* Colorimetric specification for element */
	BYTE   BitSize;           /* Bit size for element */
	WORD   Packing;           /* Packing for element */
	WORD   Encoding;          /* Encoding for element */
	DWORD  DataOffset;        /* Offset to data of element */
	DWORD  EndOfLinePadding;  /* End of line padding used in element */
	DWORD  EndOfImagePadding; /* End of image padding used in element */
	char   Description[32];   /* Description of element */
} DPX_IMAGEELEMENT;

typedef struct _DPX_GenericImageHeader
{
	WORD         Orientation;        /* Image orientation */
	WORD         NumberElements;     /* Number of image elements */
	DWORD        PixelsPerLine;      /* Pixels per line */
	DWORD        LinesPerElement;    /* Lines per image element */
	DPX_IMAGEELEMENT ImageElement[8];
	BYTE         Reserved29[52];       /* Reserved field 29 */
} DPX_GENERICIMAGEHEADER;

typedef struct _DPX_GenericSourceInfoHeader
{
	DWORD  XOffset;         /* X offset */
	DWORD  YOffset;         /* Y offset */
	SINGLE XCenter;         /* X center */
	SINGLE YCenter;         /* Y center */
	DWORD  XOriginalSize;   /* X original size */
	DWORD  YOriginalSize;   /* Y original size */
	char   SourceFileName[FILENAME_SIZE + 1];   /* Source image file name */
	char   SourceTimeDate[24];    /* Source image date & time */
	char   InputName[32];   /* Input device name */
	char   InputSN[32];     /* Input device serial number */
	WORD   Border[4];       /* Border validity (XL, XR, YT, YB) */
	DWORD  AspectRatio[2];  /* Pixel aspect ratio H:V */
	BYTE   Reserved[28];    /* Reserved */
} DPX_GENERICSOURCEINFOHEADER;

typedef struct _DPX_IndustryFilmInfoHeader
{
	char   FilmMfgId[2];      /* Film manufacturer ID code */
	char   FilmType[2];       /* Film type */
	char   OffsetPerfs[2];    /* Offset in perfs */
	char   Prefix[6];         /* Prefix */
	char   Count[4];          /* Count */
	char   Format[32];        /* Format */
	DWORD  FramePosition;     /* Frame position in sequence */
	DWORD  SequenceLen;       /* Sequence length in frames */
	DWORD  HeldCount;         /* Held count */
	SINGLE FrameRate;         /* Frame rate of original in frames/sec */
	SINGLE ShutterAngle;      /* Shutter angle of camera in degrees */
	char   FrameId[32];       /* Frame identification */
	char   SlateInfo[100];    /* Slate information */
	BYTE   Reserved[56];      /* Reserved */
} DPX_INDUSTRYFILMINFOHEADER;

typedef struct _DPX_IndustryTelevisionInfoHeader
{
	DWORD  TimeCode;         /* SMPTE time code */
	DWORD  UserBits;         /* SMPTE user bits */
	BYTE   Interlace;        /* Interlace */
	BYTE   FieldNumber;      /* Field number */
	BYTE   VideoSignal;      /* Video signal standard */
	BYTE   Padding;          /* Structure alignment padding */
	SINGLE HorzSampleRate;   /* Horizontal sample rate in Hz */
	SINGLE VertSampleRate;   /* Vertical sample rate in Hz */
	SINGLE FrameRate;        /* Temporal sampling rate or frame rate in Hz */
	SINGLE TimeOffset;       /* time offset from sync to first pixel */
	SINGLE Gamma;            /* gamma value */
	SINGLE BlackLevel;       /* Black level code value */
	SINGLE BlackGain;        /* Black gain */
	SINGLE Breakpoint;       /* Breakpoint */
	SINGLE WhiteLevel;       /* Reference white level code value */
	SINGLE IntegrationTimes; /* Integration time(s) */
	BYTE   Reserved74[76];  /* Reserved (field 74) */
} DPX_INDUSTRYTELEVISIONINFOHEADER;

typedef struct _DpxFileFormat
{
	DPX_GENERICFILEHEADER            FileHeader;
	DPX_GENERICIMAGEHEADER           ImageHeader;
	DPX_GENERICSOURCEINFOHEADER      SourceInfoHeader;
	DPX_INDUSTRYFILMINFOHEADER       FilmHeader;
	DPX_INDUSTRYTELEVISIONINFOHEADER TvHeader;
} DPXFILEFORMAT;

#define UNDEFINED_U32	UINT32_MAX
#define UNDEFINED_U16   UINT16_MAX
#define UNDEFINED_U8    UINT8_MAX


format_t determine_field_format(char* file_name);
int      ends_in_percentd(char* str, int length);

int      dpx_write(char *fname, pic_t *p, int pad_ends, int datum_order, int force_packing, int swaprb, int wbswap);
int      dpx_read(char *fname, pic_t **p, int pad_ends, int noswap, int datum_order, int swap_r_and_b);
void     set_dpx_colorspace(int color);

int      dpx_read_hl(char *fname, pic_t **p, int *high, int *low, int dpx_bugs, int noswap, int datum_order, int swap_r_and_b);

int      write_dpx_ver(char *fname, pic_t *p, int ar1, int ar2, int frameno, int seqlen, float framerate, int interlaced, int bpp, int ver, int pad_ends, int datum_order, int force_packing, int swaprb, int wbswap);
int      write_dpx(char *fname, pic_t *p, int ar1, int ar2, int frameno, int seqlen, float framerate, int interlaced, int bpp, int pad_ends, int datum_order, int force_packing, int swaprb, int wbswap);
int      read_dpx(char *fname, pic_t **p, int *ar1, int *ar2, int *frameno, int *seqlen, float *framerate, int *interlaced, int *bpp, int pad_ends, int noswap, int datum_order, int swap_r_and_b);

#endif
