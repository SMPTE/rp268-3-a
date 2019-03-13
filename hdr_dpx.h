/***************************************************************************
*    Copyright (c) 2018-2019, Broadcom Inc.
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
#ifndef HDR_DPX_H
#define HDR_DPX_H
#include <stdint.h>
#include <stdio.h>

/*** hdr_dpx.h ***/
#include "vdo.h"

#define DPX_ERROR_UNRECOGNIZED_CHROMA  -1
#define DPX_ERROR_UNSUPPORTED_BPP      -2
#define DPX_ERROR_NOT_IMPLEMENTED      -3
#define DPX_ERROR_MISMATCH             -4
#define DPX_ERROR_BAD_FILENAME         -5
#define DPX_ERROR_CORRUPTED_FILE       -6
#define DPX_ERROR_MALLOC_FAIL          -7
#define DPX_ERROR_BAD_PARAMETER		   -8
#define DPX_ERROR_PIC_MAP_FAILED       -9

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

#define NUM_IMAGE_ELEMENTS  8

#define FILE_NAME_SIZE	100
#define TIMEDATE_SIZE	24
#define CREATOR_SIZE	100
#define PROJECT_SIZE	200
#define COPYRIGHT_SIZE	200
#define DESCRIPTION_SIZE	32
#define INPUTNAME_SIZE	32
#define INPUTSN_SIZE	32

#define HDRDPX_DESC_USER		0
#define HDRDPX_DESC_R			1
#define HDRDPX_DESC_G			2
#define HDRDPX_DESC_B			3
#define HDRDPX_DESC_A			4
#define HDRDPX_DESC_Y			6
#define HDRDPX_DESC_CbCr		7
#define HDRDPX_DESC_Z			8
#define HDRDPX_DESC_COMP		9
#define HDRDPX_DESC_Cb			10
#define HDRDPX_DESC_Cr			11
#define HDRDPX_DESC_RGB_OLD		50
#define HDRDPX_DESC_RGBA_OLD	51
#define HDRDPX_DESC_ABGR_OLD	52
#define HDRDPX_DESC_BGR			53
#define HDRDPX_DESC_BGRA		54
#define HDRDPX_DESC_ARGB		55
#define HDRDPX_DESC_RGB			56
#define HDRDPX_DESC_RGBA		57
#define HDRDPX_DESC_ABGR		58
#define HDRDPX_DESC_CbYCrY		100
#define HDRDPX_DESC_CbYACrYA	101
#define HDRDPX_DESC_CbYCr		102
#define HDRDPX_DESC_CbYCrA		103
#define	HDRDPX_DESC_CYY			104
#define HDRDPX_DESC_CYAYA		105
#define HDRDPX_DESC_G2C			150
#define HDRDPX_DESC_G3C			151
#define HDRDPX_DESC_G4C			152
#define HDRDPX_DESC_G5C			153
#define HDRDPX_DESC_G6C			154
#define HDRDPX_DESC_G7C			155
#define HDRDPX_DESC_G8C			156

#define HDRDPX_TF_USER				0
#define HDRDPX_TF_DENSITY			1
#define HDRDPX_TF_LINEAR			2
#define HDRDPX_TF_LOGARITHMIC		3
#define HDRDPX_TF_UNSPECIFIED_VIDEO	4
#define HDRDPX_TF_ST_274			5
#define HDRDPX_TF_BT_709			6
#define HDRDPX_TF_BT_601_625		7
#define HDRDPX_TF_BT_601_525		8
#define HDRDPX_TF_COMPOSITE_NTSC	9
#define HDRDPX_TF_COMPOSITE_PAL		10
#define HDRDPX_TF_Z_LINEAR			11
#define HDRDPX_TF_Z_HOMOGENOUS		12
#define HDRDPX_TF_ST_2065_3			13
#define HDRDPX_TF_BT_2020_NCL		14
#define HDRDPX_TF_BT_2020_CL		15
#define HDRDPX_TF_IEC_61966_2_4		16
#define HDRDPX_TF_BT_2100_PQ_NCL	17
#define HDRDPX_TF_BT_2100_PQ_CI		18
#define HDRDPX_TF_BT_2100_HLG_NCL	19
#define HDRDPX_TF_BT_2100_HLG_CI	20
#define HDRDPX_TF_RP_231_2			21
#define HDRDPX_TF_IEC_61966_2_1		22

#define HDRDPX_C_USER				0
#define HDRDPX_C_DENSITY			1
#define HDRDPX_C_NA_2				2
#define HDRDPX_C_NA_3				3
#define HDRDPX_C_UNSPECIFIED_VIDEO	4
#define HDRDPX_C_ST_274				5
#define HDRDPX_C_BT_709				6
#define HDRDPX_C_BT_601_625			7
#define HDRDPX_C_BT_601_525			8
#define HDRDPX_C_COMPOSITE_NTSC		9
#define HDRDPX_C_COMPOSITE_PAL		10
#define HDRDPX_C_NA_11				11
#define HDRDPX_C_NA_12				12
#define HDRDPX_C_ST_2065_3			13
#define HDRDPX_C_BT_2020			14
#define HDRDPX_C_ST_2113_P3D65		15
#define HDRDPX_C_ST_2113_P3DCI		16
#define HDRDPX_C_ST_2113_P3D60		17
#define HDRDPX_C_ST_2065_1_ACES		18

typedef union _datum
{
	float sf;
	double df;
	uint32_t d[2];
} datum_t;

typedef struct _datum_list_s
{
	uint32_t *d;
	uint8_t *eol_flag;
	int ndatum;
	int bit_depth;
} datum_list_t;

typedef struct _HDRDPX_Metadata
{
	int file_is_be;		/* 0=BE, 1=LE */
	int override_image_offset;	/* 0=use automatic image data offsets, 1=override */
	unsigned int image_element_data_offsets[8];  /* If overriding image offset, use these offsets */
} sHDRDPXMetadata;

typedef struct _HDRDPX_GenericFileHeader
{
	DWORD Magic;           /* Magic number */
	DWORD ImageOffset;     /* Offset to start of image data (bytes) */
	char Version[8];      /* Version stamp of header format */
	DWORD FileSize;        /* Total DPX file size (bytes) */
	DWORD DittoKey;        /* Image content specifier */
	DWORD GenericSize;     /* Generic section header length (bytes) */
	DWORD IndustrySize;    /* Industry-specific header length (bytes) */
	DWORD UserSize;        /* User-defined data length (bytes) */
	char FileName[FILE_NAME_SIZE];   /* Name of DPX file */
	char TimeDate[TIMEDATE_SIZE];    /* Time & date of file creation */
	char Creator[CREATOR_SIZE];    /* Name of file creator */
	char Project[PROJECT_SIZE];    /* Name of project */
	char Copyright[COPYRIGHT_SIZE];  /* Copyright information */
	DWORD EncryptKey;      /* Encryption key */
	DWORD StandardsBasedMetadataOffset; /* Standards-based metadata offset */
	BYTE  DatumMappingDirection;  /* Datum mapping direction */
	BYTE  Reserved16_3;    /* Reserved field 16.3 */
	WORD  Reserved16_4;	   /* Reserved field 16.4 */
	int8_t Reserved16_5[96];   /* Reserved field 16.5 */
} HDRDPX_GENERICFILEHEADER;

typedef union _HDRDPX_HiLoCode
{
	SINGLE f;
	DWORD  d;
} HDRDPX_HILOCODE;

typedef struct _HDRDPX_ImageElement
{
	DWORD  DataSign;          /* Data sign extension */
	HDRDPX_HILOCODE  LowData;           /* Reference low data code value */
	SINGLE LowQuantity;       /* reference low quantity represented */
	HDRDPX_HILOCODE  HighData;          /* Reference high data code value */
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
	char   Description[DESCRIPTION_SIZE];   /* Description of element */
} HDRDPX_IMAGEELEMENT;

typedef struct _HDRDPX_GenericImageHeader
{
	WORD         Orientation;        /* Image orientation */
	WORD         NumberElements;     /* Number of image elements */
	DWORD        PixelsPerLine;      /* Pixels per line */
	DWORD        LinesPerElement;    /* Lines per image element */
	HDRDPX_IMAGEELEMENT ImageElement[NUM_IMAGE_ELEMENTS];
	DWORD		 ChromaSubsampling;  /* Chroma subsampling descriptor (field 29.1) */
	BYTE         Reserved[48];       /* Reserved field 29.2 */
} HDRDPX_GENERICIMAGEHEADER;

typedef struct _HDRDPX_GenericSourceInfoHeader
{
	DWORD  XOffset;         /* X offset */
	DWORD  YOffset;         /* Y offset */
	SINGLE XCenter;         /* X center */
	SINGLE YCenter;         /* Y center */
	DWORD  XOriginalSize;   /* X original size */
	DWORD  YOriginalSize;   /* Y original size */
	char SourceFileName[FILE_NAME_SIZE];   /* Source image file name */
	char SourceTimeDate[TIMEDATE_SIZE];    /* Source image date & time */
	char InputName[INPUTNAME_SIZE];   /* Input device name */
	char InputSN[INPUTSN_SIZE];     /* Input device serial number */
	WORD   Border[4];       /* Border validity (XL, XR, YT, YB) */
	DWORD  AspectRatio[2];  /* Pixel aspect ratio H:V */
	BYTE   Reserved[28];    /* Reserved */
} HDRDPX_GENERICSOURCEINFOHEADER;

typedef struct _HDRDPX_IndustryFilmInfoHeader
{
	char FilmMfgId[2];      /* Film manufacturer ID code */
	char FilmType[2];       /* Film type */
	char OffsetPerfs[2];    /* Offset in perfs */
	char Prefix[6];         /* Prefix */
	char Count[4];          /* Count */
	char Format[32];        /* Format */
	DWORD  FramePosition;     /* Frame position in sequence */
	DWORD  SequenceLen;       /* Sequence length in frames */
	DWORD  HeldCount;         /* Held count */
	SINGLE FrameRate;         /* Frame rate of original in frames/sec */
	SINGLE ShutterAngle;      /* Shutter angle of camera in degrees */
	char FrameId[32];       /* Frame identification */
	char SlateInfo[100];    /* Slate information */
	BYTE   Reserved[56];      /* Reserved */
} HDRDPX_INDUSTRYFILMINFOHEADER;

typedef struct _HDRDPX_IndustryTelevisionInfoHeader
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
	BYTE   VideoIdentificationCode; /* Video Identification Code (field 74.1) */
	BYTE   SMPTETCType;      /* SMPTE time code type (field 74.2) */
	BYTE   SMPTETCDBB2;		 /* SMPTE time code DBB2 value (field 74.3) */
	BYTE   Reserved74_4;     /* Reserved (field 74.4) */
	BYTE   Reserved74_5[72];  /* Reserved (field 74.5) */
} HDRDPX_INDUSTRYTELEVISIONINFOHEADER;

typedef struct _HdrDpxFileFormat
{
	HDRDPX_GENERICFILEHEADER            FileHeader;
	HDRDPX_GENERICIMAGEHEADER           ImageHeader;
	HDRDPX_GENERICSOURCEINFOHEADER      SourceInfoHeader;
	HDRDPX_INDUSTRYFILMINFOHEADER       FilmHeader;
	HDRDPX_INDUSTRYTELEVISIONINFOHEADER TvHeader;
} HDRDPXFILEFORMAT;

typedef struct _HdrDpxUserData
{
	char     UserIdentification[32];	/* User identification */
	uint8_t *UserData;		/* User data */
} HDRDPXUSERDATA;

typedef struct _HdrDpxSbmData
{
	char 		SbmFormatDescriptor[128];	/* SBM format decriptor */
	uint32_t	SbmLength;					/* Length of data that follows */
	uint8_t		*SbmData;			/* Standards-based metadata */
} HDRDPXSBMDATA;

#define UNDEFINED_U32	(~((DWORD)0))
#define UNDEFINED_U16   (~((WORD)0))
#define UNDEFINED_U8    (~((BYTE)0))


extern format_t determine_field_format(char* file_name);
extern int      ends_in_percentd(char* str, int length);

int      hdr_dpx_read(char *fname, pic_t **p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm);
int      hdr_dpx_write(char *fname, pic_t *p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm, int write_as_be, HDRDPXFILEFORMAT *dpxh_written);
int		 hdr_dpx_determine_file_type(char *fname);

void print_hdrdpx_header(FILE *fp, char *fname, HDRDPXFILEFORMAT *dpx_file, HDRDPXUSERDATA *dpx_u, HDRDPXSBMDATA *dpx_sbm);

#endif
