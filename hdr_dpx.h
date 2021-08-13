/***************************************************************************
*    Copyright (c) 2018-2020, Broadcom Inc.
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
/** @file hdr_dpx.h
	@brief Defines main interfaces for HDR DPX reader/writer.
	
	IMPORTANT:  This library is not thread-safe! Please ensure that all DPX file-related tasks run in a single thread if you use this library.
*/

#ifndef HDR_DPX_H
#define HDR_DPX_H
#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <utility>
#include <list>

#include "datum.h"
#include "fifo.h"
#include "hdr_dpx_error.h"
#include "file_map.h"

/** Take the maximum of two things */
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

/** Normalized worst-case overhead (beyond the uncompressed size) for an RLE coded image elemnt */
#define RLE_MARGIN   (1.0/127)   // = ~1% margin means we assume that in the worst case an RLE image element might be slightly bigger than uncompressed 
                                  //         (1-component 8-bit IE where RLE flag always indicates no redundancy should be worst case if we require "same" runs to be at least 3 long for 1-component IE case)

/** Round an offset up to a 4-byte (DWORD) boundar */
#define CEIL_DWORD(o)    (((o + 3)>>2)<<2)

#ifndef DPX_H

#ifdef _MSC_VER
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
#endif /* DPX_H */

/** A DPX file can contain up to 8 image elements */
#define NUM_IMAGE_ELEMENTS  8

/** Max length for file name strings in header */
#define FILE_NAME_SIZE	100
/** Max length for time/date strings in header */
#define TIMEDATE_SIZE	24
/** Max length for creator strings in header */
#define CREATOR_SIZE	100
/** Max length for project strings in header */
#define PROJECT_SIZE	200
/** Max length for copyright strings in header */
#define COPYRIGHT_SIZE	200
/** Max length for description strings in header */
#define DESCRIPTION_SIZE	32
/** Max length for input name string in header */
#define INPUTNAME_SIZE	32
/** Max length for input serial number string in header */
#define INPUTSN_SIZE	32

/** DPX file structure */
typedef struct _HDRDPX_GenericFileHeader
{
	DWORD Magic;           ///< Magic number 
	DWORD ImageOffset;     ///< Offset to start of image data (bytes) 
	char Version[8];      ///< Version stamp of header format 
	DWORD FileSize;        ///< Total DPX file size (bytes) 
	DWORD DittoKey;        ///< Image content specifier 
	DWORD GenericSize;     ///< Generic section header length (bytes) 
	DWORD IndustrySize;    ///< Industry-specific header length (bytes) 
	DWORD UserSize;        ///< User-defined data length (bytes) 
	char FileName[FILE_NAME_SIZE];   ///< Name of DPX file 
	char TimeDate[TIMEDATE_SIZE];    ///< Time & date of file creation 
	char Creator[CREATOR_SIZE];    ///< Name of file creator 
	char Project[PROJECT_SIZE];    ///< Name of project
	char Copyright[COPYRIGHT_SIZE];  ///< Copyright information 
	DWORD EncryptKey;      ///< Encryption key 
	DWORD StandardsBasedMetadataOffset; ///< Standards-based metadata offset 
	BYTE  DatumMappingDirection;  ///< Datum mapping direction 
	BYTE  Reserved16_3;    ///< Reserved field 16.3 
	WORD  Reserved16_4;	   ///< Reserved field 16.4 
	BYTE  Reserved16_5[96];   ///< Reserved field 16.5 
} HDRDPX_GENERICFILEHEADER;

/** union to represent either a floating point or DWORD value */
typedef union _HDRDPX_HiLoCode
{
	SINGLE f;   ///< Floating point value (for floating point samples)
	DWORD  d;   ///< Integer value (for integer samples)
} HDRDPX_HILOCODE;

/** Header data structure describing single image element */
typedef struct _HDRDPX_ImageElement
{
	DWORD  DataSign;          ///< Data sign extension 
	HDRDPX_HILOCODE  LowData;           ///< Reference low data code value 
	SINGLE LowQuantity;       ///< reference low quantity represented 
	HDRDPX_HILOCODE  HighData;          ///< Reference high data code value 
	SINGLE HighQuantity;      ///< reference high quantity represented 
	BYTE   Descriptor;        ///< Descriptor for image element 
	BYTE   Transfer;          ///< Transfer characteristics for element 
	BYTE   Colorimetric;      ///< Colorimetric specification for element 
	BYTE   BitSize;           ///< Bit size for element
	WORD   Packing;           ///< Packing for element 
	WORD   Encoding;          ///< Encoding for element 
	DWORD  DataOffset;        ///< Offset to data of element 
	DWORD  EndOfLinePadding;  ///< End of line padding used in element 
	DWORD  EndOfImagePadding; ///< End of image padding used in element 
	char   Description[DESCRIPTION_SIZE];   ///< Description of element 
} HDRDPX_IMAGEELEMENT;

/** General image header */
typedef struct _HDRDPX_GenericImageHeader
{
	WORD         Orientation;        ///< Image orientation 
	WORD         NumberElements;     ///< Number of image elements 
	DWORD        PixelsPerLine;      ///< Pixels per line 
	DWORD        LinesPerElement;    ///< Lines per image element 
	HDRDPX_IMAGEELEMENT ImageElement[NUM_IMAGE_ELEMENTS];  ///< image element data structures
	DWORD		 ChromaSubsampling;  ///< Chroma subsampling descriptor (field 29.1) 
	BYTE         Reserved[48];       ///< Reserved field 29.2
} HDRDPX_GENERICIMAGEHEADER;

/** Source information header */
typedef struct _HDRDPX_GenericSourceInfoHeader
{
	DWORD  XOffset;         ///< X offset 
	DWORD  YOffset;         ///< Y offset 
	SINGLE XCenter;         ///< X center 
	SINGLE YCenter;         ///< Y center 
	DWORD  XOriginalSize;   ///< X original size 
	DWORD  YOriginalSize;   ///< Y original size 
	char SourceFileName[FILE_NAME_SIZE];   ///< Source image file name 
	char SourceTimeDate[TIMEDATE_SIZE];    ///< Source image date & time 
	char InputName[INPUTNAME_SIZE];   ///< Input device name 
	char InputSN[INPUTSN_SIZE];     ///< Input device serial number 
	WORD   Border[4];       ///< Border validity (XL, XR, YT, YB) 
	DWORD  AspectRatio[2];  ///< Pixel aspect ratio H:V 
	SINGLE XScannedSize;    ///< X scanned size 
	SINGLE YScannedSize;    ///< Y scanned size 
	BYTE   Reserved[20];    ///< Reserved 
} HDRDPX_GENERICSOURCEINFOHEADER;

/** Industry-specific film information header */
typedef struct _HDRDPX_IndustryFilmInfoHeader
{
	char FilmMfgId[2];      ///< Film manufacturer ID code 
	char FilmType[2];       ///< Film type 
	char OffsetPerfs[2];    ///< Offset in perfs 
	char Prefix[6];         ///< Prefix 
	char Count[4];          ///< Count 
	char Format[32];        ///< Format 
	DWORD  FramePosition;     ///< Frame position in sequence 
	DWORD  SequenceLen;       ///< Sequence length in frames 
	DWORD  HeldCount;         ///< Held count 
	SINGLE FrameRate;         ///< Frame rate of original in frames/sec 
	SINGLE ShutterAngle;      ///< Shutter angle of camera in degrees 
	char FrameId[32];       ///< Frame identification 
	char SlateInfo[100];    ///< Slate information 
	BYTE   Reserved[56];      ///< Reserved 
} HDRDPX_INDUSTRYFILMINFOHEADER;

/** Industry-specific television information header */
typedef struct _HDRDPX_IndustryTelevisionInfoHeader
{
	DWORD  TimeCode;         ///< SMPTE time code 
	DWORD  UserBits;         ///< SMPTE user bits 
	BYTE   Interlace;        ///< Interlace 
	BYTE   FieldNumber;      ///< Field number 
	BYTE   VideoSignal;      ///< Video signal standard 
	BYTE   Padding;          ///< Structure alignment padding 
	SINGLE HorzSampleRate;   ///< Horizontal sample rate in Hz 
	SINGLE VertSampleRate;   ///< Vertical sample rate in Hz 
	SINGLE FrameRate;        ///< Temporal sampling rate or frame rate in Hz 
	SINGLE TimeOffset;       ///< time offset from sync to first pixel 
	SINGLE Gamma;            ///< gamma value 
	SINGLE BlackLevel;       ///< Black level code value 
	SINGLE BlackGain;        ///< Black gain 
	SINGLE Breakpoint;       ///< Breakpoint 
	SINGLE WhiteLevel;       ///< Reference white level code value 
	SINGLE IntegrationTimes; ///< Integration time(s) 
	BYTE   VideoIdentificationCode; ///< Video Identification Code (field 74.1) 
	BYTE   SMPTETCType;      ///< SMPTE time code type (field 74.2) 
	BYTE   SMPTETCDBB2;		 ///< SMPTE time code DBB2 value (field 74.3) 
	BYTE   Reserved74_4;     ///< Reserved (field 74.4) 
	BYTE   Reserved74_5[72];  ///< Reserved (field 74.5) 
} HDRDPX_INDUSTRYTELEVISIONINFOHEADER;

/** Complete file header structure */
typedef struct _HdrDpxFileFormat
{
	HDRDPX_GENERICFILEHEADER            FileHeader;      ///< File header
	HDRDPX_GENERICIMAGEHEADER           ImageHeader;     ///< Image header
	HDRDPX_GENERICSOURCEINFOHEADER      SourceInfoHeader;  ///< Source information header
	HDRDPX_INDUSTRYFILMINFOHEADER       FilmHeader;     ///< Industry-secific film header
	HDRDPX_INDUSTRYTELEVISIONINFOHEADER TvHeader;     ///< Industry-specific television header
} HDRDPXFILEFORMAT;

/** User-defined data structure (if present) */
typedef struct _HdrDpxUserData
{
	char     UserIdentification[32];	///< User identification 
	std::vector<uint8_t> UserData;		///< User-defined data */
} HDRDPXUSERDATA;

/** Standards-based metadata structure (if present) */
typedef struct _HdrDpxSbmData
{
	char 		SbmFormatDescriptor[128];	///< SBM format decriptor 
	uint32_t	SbmLength;					///< Length of data that follows 
	std::vector<uint8_t> SbmData;			///< Standards-based metadata 
} HDRDPXSBMDATA;

namespace Dpx {

	const uint32_t eOffsetAutoLocate = UINT32_MAX;	///< Set any data offset to this value to have the library automatically determine an offset that works
	const uint32_t eSBMAutoLocate = 0;  ///< Set the standards-based metadata offset to this value to have the library automatically determine an offset that works

	/// Keys to get/set string values in header
	enum HdrDpxFieldsString
	{
		eImageFileName, ///< image file name (max 100 chars)
		eCreationDateTime, ///< creation date & time (max 24 chars)
		eCreator, ///< creator (max 100 chars)
		eProjectName, ///< project name (max 200 chars)
		eRightToUseOrCopyright, ///< right to use or copyright statement (max 200 chars)
		eSourceImageFileName, ///< source image file name (max 100 chars)
		eSourceImageDateTime, ///< source image date & time (max 24 chars)
		eInputDeviceName, ///< input device name (max 32 chars)
		eInputDeviceSN, ///< input device serial number (max 32 chars)
		eFilmMfgIdCode, ///< Film manufacturer ID code (max 2 chars)
		eFilmType, ///< Film type (max 2 chars)
		eOffsetInPerfs, ///< offset in perfs (max 2 chars)
		ePrefix, ///< Film prefix (max 6 chars)
		eCount, ///< Film count (max 4 chars)
		eFormat, ///< Film format (max 32 chars)
		eFrameIdentification, ///<  Film frame identification (max 32 chars)
		eSlateInformation, ///< Slate information (max 100 chars)
		eUserIdentification,   ///< User identification (max 32 chars)
		eUserDefinedData,       ///< User-defined data as a string (max 1000000 chars)
		eSBMFormatDescriptor,   ///< Standards-based metadata format descriptor (ST336, Reg-XML, or XMP)
		eSBMetadata		///< Standards-based metadata as a string (no size limit)
	};

	/// Keys to get/set U32 (32-bit word) values in header
	enum HdrDpxFieldsU32 // No compile-time checks, but run-time checks are needed to validate
	{
		eOffsetToImageData,  ///< Offset to image data
		eTotalImageFileSize,  ///< (get only) total image file size
		eGenericSectionHeaderLength,  ///< generic section header length (must be 1664)
		eIndustrySpecificHeaderLength,  ///< industry-specific section header length (must be 384)
		eUserDefinedHeaderLength,  ///< (get only) user-defined header length. Can only be set by calling SetUserData() or SetHeader() using eUserDefinedData and a string
		eEncryptionKey,  ///< encryption key
		eStandardsBasedMetadataOffset,  ///< standards-based metadata offset
		ePixelsPerLine,  ///< number of pixels per line
		eLinesPerImageElement,  ///< number of lines per image element
		eXOffset,   ///< X offset
		eYOffset,   ///< Y offset
		eXOriginalSize,  ///< X original size
		eYOriginalSize,  ///< Y original sie
		ePixelAspectRatioH,  ///< horizontal pixel aspect ratio
		ePixelAspectRatioV,  ///< vertical pixel aspect ratio
		eFramePositionInSequence,  ///< frame position within a sequence
		eSequenceLength,   ///< sequence length
		eHeldCount,   ///< held count
		eSBMLength   ///< (get only) standards-based metadata length. Can only be set by calling SetStandardsBasedMetadata() or SetHeader() using eSBMetadata and a string 
	};

	/// Keys to get/set U16 (16-bit word) values in header
	enum HdrDpxFieldsU16
	{
		eNumberOfImageElements,   ///< Number of image elements. Refers only to header field and does not allocate or deallocate IE objects. Will be automatically set correctly if unspecified.
		eBorderValidityXL,   ///< Left border validity
		eBorderValidityXR,   ///< Right border validity
		eBorderValidityYT,   ///< Top border validity
		eBorderValidityYB,   ///< Bottom border validity
	};

	/// Keys to get/set R32 (32-bit floating point) values in header
	enum HdrDpxFieldsR32
	{
		eXCenter,   ///< X center
		eYCenter,   ///< Y center
		eXScannedSize,   ///< X scanned size
		eYScannedSize,   ///< Y scanned size
		eFrameRateOfOriginal,   ///< Frame rate of original
		eShutterAngleInDegrees,   ///< Shutter angle in degrees
		eHorizontalSamplingRate,   ///< Horizontal sampling rate
		eVerticalSamplingRate,   ///< Vertical sampling rate
		eTemporalSamplingRate,   ///< Temporal sampling rate
		eTimeOffsetFromSyncToFirstPixel,   ///< Time offset from sync to first pixel
		eGamma,   ///< Gamma value
		eBlackLevelCode,   ///< Black level code value
		eBlackGain,   ///< Black gain
		eBreakpoint,   ///< Breakpoint
		eReferenceWhiteLevelCode,   ///< Reference white level code value
		eIntegrationTime   ///< Integration time
	};

	/// Keys to get/set U32 (32-bit word) values in image element data structure
	enum HdrDpxIEFieldsU32
	{
		eOffsetToData,   ///< Offset to image element data
		eEndOfLinePadding,   ///< Number of end-of-line padding bytes
		eEndOfImagePadding   ///< Number of end-of-image padding bytes
	};

	/// Keys to get/set R32 (32-bit floating point) values in image element data structure
	enum HdrDpxIEFieldsR32
	{
		eReferenceLowDataCode,   ///< Reference low data code value
		eReferenceLowQuantity,   ///< Reference low quantity represented
		eReferenceHighDataCode,   ///< Reference high data code value
		eReferenceHighQuantity   ///< Reference high quantity represented
	};

	/// Keys to get/set string value in image element data structure
	enum HdrDpxIEFieldsString
	{
		eDescriptionOfImageElement   ///< Description of image element
	};

	/// Keys to get/set U8 (byte) values in header
	enum HdrDpxFieldsU8
	{
		eFieldNumber,   ///< Field number
		eSMPTETCDBB2Value   ///< SMPTE time code DBB2 value
	};

	/// Key to get/set version header field
	enum HdrDpxFieldsVersion
	{
		eVersion   ///< version
	};

	/// Key to get/set ditto key header field
	enum HdrDpxFieldsDittoKey
	{
		eDittoKey   ///< ditto key
	};

	/// Key to get/set datum mapping direction header field
	enum HdrDpxFieldsDatumMappingDirection
	{
		eDatumMappingDirection   ///< datum mapping direction
	};

	/// Key to get/set orientation header field
	enum HdrDpxFieldsOrientation
	{
		eOrientation   ///< orientation
	};

	/// Key to get/set data sign image element data structure field
	enum HdrDpxFieldsDataSign
	{
		eDataSign,   ///< data sign
	};

	/// Key to get/set descriptor image element data structure field
	enum HdrDpxFieldsDescriptor
	{
		eDescriptor   ///< descriptor
	};

	/// Key to get/set transfer characteristic image element data structure field
	enum HdrDpxFieldsTransfer
	{
		eTransferCharacteristic   ///< transfer characteristic
	};

	/// Key to get/set colorimetric specification image element data structure field
	enum HdrDpxFieldsColorimetric
	{
		eColorimetricSpecification   ///< colorimetric specification
	};

	/// Key to get/set bit depth image element data structure field
	enum HdrDpxFieldsBitDepth
	{
		eBitDepth   ///< bit depth
	};

	/// Key to get/set packing method image element data structure field
	enum HdrDpxFieldsPacking
	{
		ePacking   ///< packing
	};

	/// Key to get/set encoding method image element data structure field
	enum HdrDpxFieldsEncoding
	{
		eEncoding  ///< encoding (i.e., RLE)
	};

	/// Key to get/set SMPTE time code header field
	enum HdrDpxFieldsTimeCode
	{
		eSMPTETimeCode   ///< SMPTE time code
	};

	/// Key to get/set SMPTE user bits header field
	enum HdrDpxFieldsUserBits
	{
		eSMPTEUserBits   ///< SMPTE user bits
	};

	/// Key to get/set interlace header field
	enum HdrDpxFieldsInterlace
	{
		eInterlace   ///< interlace
	};

	/// Key to get/set video signal standard header field
	enum HdrDpxFieldsVideoSignal
	{
		eVideoSignalStandard  ///< video signal standard
	};

	/// Key to get/set Video Identification Code (VIC) header field
	enum HdrDpxFieldsVideoIdentificationCode
	{
		eVideoIdentificationCode   ///< Video Identification Code (VIC)
	};

	/// Key to get/set byte order of file
	enum HdrDpxFieldsByteOrder
	{
		eByteOrder   ///< byte order of file
	};

	/// Key to get/set color difference siting bits for an image element
	enum HdrDpxFieldsColorDifferenceSiting
	{
		eColorDifferenceSiting   ///< color difference siting
	};

	/// Key to set SMPTE time code type header field
	enum HdrDpxFieldsSMPTETCType
	{
		eSMPTETCType  ///< SMPTE time code type
	};


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Setttings for file byte order
	enum HdrDpxByteOrder
	{
		eNativeByteOrder,   ///< write files in the native order of the machine
		eMSBF,   ///< DPX file is in most-signficant byte first (MSBF) order
		eLSBF   ///< DPX file is in least-significant byte first (LSBF) order
	};

	/// Settings for DPX file version
	enum HdrDpxVersion
	{
		eDPX_1_0,   ///< V1.0 in version field
		eDPX_2_0,   ///< V2.0 in version field
		eDPX_2_0_HDR,   ///< V2.0HDR in version field
		eDPX_Unrecognized   ///< unrecognized string in version field
	};

	/// Settings for ditto key
	enum HdrDpxDittoKey
	{
		eDittoKeySame = 0,   ///< indicates header fields (except frame number & IE offsets) are the same as previous frame in sequence
		eDittoKeyNew = 1,   ///< indicates header fields are not the same as the previous frame in sequence
		eDittoKeyUndefined = UINT32_MAX   ///< ditto key undefined
	};

	/// Settings for datum mapping direction
	enum HdrDpxDatumMappingDirection
	{
		eDatumMappingDirectionR2L = 0,   ///< indicates datum values are mapped right-to-left within image data words (see ST 268-2)
		eDatumMappingDirectionL2R = 1,   ///< indicates datum values are mapped left-to-right within image data words (see ST 268-2)
		eDatumMappingDirectionUndefined = UINT8_MAX   ///< datum mapping direction undefined
	};

	/// Settings for orientation
	enum HdrDpxOrientation
	{
		eOrientationL2R_T2B = 0,   ///< indicates scanning from left-to-right (lines), top-to-bottom (frames)
		eOrientationR2L_T2B = 1,   ///< indicates scanning from right-to-left (lines), top-to-bottom (frames)
		eOrientationL2R_B2T = 2,   ///< indicates scanning from left-to-right (lines), bottom-to-top (frames)
		eOrientationR2L_B2T = 3,   ///< indicates scanning from right-to-left (lines), bottom-to-top (frames)
		eOrientationT2B_L2R = 4,   ///< indicates scanning from top-to-bottom (lines), left-to-right (frames)
		eOrientationT2B_R2L = 5,   ///< indicates scanning from top-to-bottom (lines), right-to-left (frames)
		eOrientationB2T_L2R = 6,   ///< indicates scanning from bottom-to-top (lines), left-to-right (frames)
		eOrientationB2T_R2L = 7,   ///< indiactes scanning from bottom-to-top (lines), right-to-left (frames)
		eOrientationUndefined = UINT16_MAX   ///< orientation undefined
	};

	/// Settings for data sign
	enum HdrDpxDataSign
	{
		eDataSignUnsigned = 0,   ///< indicates samples are unsigned
		eDataSignSigned = 1,    ///< indicates samples are signed
		eDataSignUndefined = UINT32_MAX   ///< data sign undefined
	};

	/// Settings for descriptor
	enum HdrDpxDescriptor
	{
		eDescUser = 0,   ///< IE contains user-defined samples (1 sample/pixel)
		eDescR = 1,   ///< IE contains red (R) samples (1 sample/pixel)
		eDescG = 2,   ///< IE contains green (G) samples (1 sample/pixel)
		eDescB = 3,   ///< IE contains blue (B) samples (1 sample/pixel)
		eDescA = 4,   ///< IE contains alpha (A) samples (1 sample/pixel)
		eDescY = 6,   ///< IE contains luma (Y) samples (1 sample/pixel)
		eDescCbCr = 7,   ///< IE contains 4:2:2 Cb and Cr samples (2 samples/2 pixels)
		eDescZ = 8,		///< IE contains depth (Z) samples (1 sample/pixel)
		eDescComposite = 9,   ///< IE contains composite samples (1 sample/pixel)
		eDescCb = 10,   ///< IE contains 4:2:0 Cb samples (1 sample/4 pixels)
		eDescCr = 11,   ///< IE contains 4:2:0 Cb samples (1 sample/4 pixels)
		eDescRGB_268_1 = 50,   ///< IE contains interleaved R, G, and B samples (3 samples/pixel, deprecated)
		eDescRGBA_268_1 = 51,   ///< IE contains interleaved R, G, B, and A samples (4 samples/pixel, deprecated)
		eDescABGR_268_1 = 52,   ///< IE contains interleaved A, B< G, and R samples (4 samples/pixels deprecated)
		eDescBGR = 53,   ///< IE contains interleaved B, G, and R samples (3 samples/pixel)
		eDescBGRA = 54,   ///< IE contains interleaved B, G, R, and A samples (4 samples/pixel)
		eDescARGB = 55,   ///< IE contains interleaved A, R, G, and B samples (4 samples/pixel)
		eDescRGB = 56,   ///< IE contains interleaved R, G, and B samples (3 samples/pixel)
		eDescRGBA = 57,   ///< IE contains interleaved R, G, B, and A samples (4 samples/pixel)
		eDescABGR = 58,   ///< IE contains interleaved A, B, G, and R samples (4 samples/pixel)
		eDescCbYCrY = 100,   ///< IE contains interleaved 4:2:2 Cb, Y, Cr and Y samples (4 samples/2 pixels)
		eDescCbYACrYA = 101,   ///< IE contains interleaved 4:2:2 Cb, Y, A, Cr, Y, and A samples (6 samples/2 pixels)
		eDescCbYCr = 102,   ///< IE contains interleaved 4:4:4 Cb, Y, and Cr samples (3 samples/pixel)
		eDescCbYCrA = 103,   ///< IE contains interleaved 4:4:4 Cb, Y, Cr, and A samples (4 samples/pixel) 
		eDescCYY = 104,   ///< IE contains interleaved 4:2:0 C (alternating lines Cb/Cr), Y, and Y samples (3 samples/2 pixels)
		eDescCYAYA = 105, ///< IE contains interleaved 4:2:0 C (alternating lines Cb/Cr), Y, A, Y, and A samples (5 samples/2 pixels)
		eDescGeneric2 = 150,  ///< IE contains 2 unspecified samples/pixel
		eDescGeneric3 = 151,  ///< IE contains 3 unspecified samples/pixel
		eDescGeneric4 = 152,  ///< IE contains 4 unspecified samples/pixel
		eDescGeneric5 = 153,  ///< IE contains 5 unspecified samples/pixel
		eDescGeneric6 = 154,  ///< IE contains 6 unspecified samples/pixel
		eDescGeneric7 = 155,  ///< IE contains 7 unspecified samples/pixel
		eDescGeneric8 = 156,  ///< IE contains 8 unspecified samples/pixel
		eDescUndefined = UINT8_MAX
	};

	/// Settings for transfer characteristic
	enum HdrDpxTransfer
	{
		eTransferUser = 0,  ///< user-defined transfer function
		eTransferDensity = 1,  ///< printing density
		eTransferLinear = 2,  ///< linear
		eTransferLogarithmic = 3,  ///< logarithmic
		eTransferUnspecifiedVideo = 4,  ///< unspecified video transfer function
		eTransferST_274 = 5,   ///< SMPTE ST 274 transfer function
		eTransferBT_709 = 6,   ///< ITU-R BT.709 transfer function
		eTransferBT_601_625 = 7,  ///< ITU-R BT.601 625 line system transfer function
		eTransferBT_601_525 = 8,  ///< ITU-R BT.601 525 line system transfer function
		eTransferCompositeNTSC = 9,  ///< NTSC composite video
		eTransferCompositePAL = 10,  ///< PAL composite video
		eTransferZLinear = 11,  ///< Z-depth linear
		eTransferZHomogenous = 12,  ///< Z-depth homogenous
		eTransferST_2065_3 = 13,   ///< SMPTE ST 2065-3 ADX
		eTransferBT_2020_NCL = 14,   ///< ITU-R BT.2020 non-constant luminance transfer function
		eTransferBT_2020_CL = 15,  ///< ITU-R BT.2020 constant luminance transfer function
		eTransferIEC_61966_2_4 = 16,   ///< IEC 61966-2-4 xvYCC transfer function
		eTransferBT_2100_PQ_NCL = 17,   ///< ITU-R BT.2100 PQ non-constant luminance
		eTransferBT_2100_PQ_CI = 18,   ///< ITU-R BT.2100 PQ constant intensity (ICtCp)
		eTransferBT_2100_HLG_NCL = 19,   ///< ITU-R BT.2100 HLG non-constant luminance
		eTransferBT_2100_HLG_CI = 20,   ///< ITU-R BT.2100 HLG constant intensity (ICtCp)
		eTransferRP_431_2 = 21,   ///< RP 431-2 transfer function (gamma 2.6)
		eTransferIEC_61966_2_1 = 22,   ///< IEC 61966-2-1 sRGB transfer function
		eTransferUndefined = UINT8_MAX  ///< undefined transfer function
	};

	/// Settings for colorimetric specification
	enum HdrDpxColorimetric {
		eColorimetricUser = 0,  ///< user-defined colorimetry
		eColorimetricDensity = 1,  ///< printing density
		eColorimetricNA_2 = 2,   ///< not applicable
		eColorimetricNA_3 = 3,   ///< not applicable
		eColorimetricUnspecifiedVideo = 4,   ///< unspecified video colorimetry
		eColorimetricST_274 = 5,   ///< SMPTE ST 274 colorimetry
		eColorimetricBT_709 = 6,   ///< ITU-R BT.709 colorimetry
		eColorimetricBT_601_625 = 7,  ///< ITU-R BT.601 625 line system colorimetry
		eColorimetricBT_601_525 = 8,   ///< ITU-R BT.601 525 line system colorimetry
		eColorimetricCompositeNTSC = 9,   ///< NTSC composite video
		eColorimetricCompositePAL = 10,   ///< PAL composite video
		eColorimetricNA_11 = 11,   ///< not applicable
		eColorimetricNA_12 = 12,   ///< not applicable
		eColorimetricST_2065_3 = 13,   ///< SMPTE ST 2065-3 ADX
		eColorimetricBT_2020 = 14,   ///< ITU-R BT.2020 colorimetry (used with BT.2100 modes as well)
		eColorimetricST_2113_P3D65 = 15,  ///< SMPTE ST 2113 P3D65 RGB color space
		eColorimetricST_2113_P3DCI = 16,   ///< SMPTE ST 2113 P3DCI RGB color space
		eColorimetricST_2113_P3D60 = 17,   ///< SMPTE ST 2113 P3D60 RGB color sapce
		eColorimetricST_2065_1_ACES = 18,   ///< SMPTE ST 2065-1 ACES color space
		eColorimetricUndefined = UINT8_MAX   ///< undefined colorimetry
	};

	/// Settings for IE bit depth
	enum HdrDpxBitDepth {
		eBitDepth1 = 1,   ///< 1 bit/sample (datum)
		eBitDepth8 = 8,   ///< 8 bits/sample
		eBitDepth10 = 10,   ///< 10 bits/sample
		eBitDepth12 = 12,   ///< 12 bits/sample
		eBitDepth16 = 16,   ///< 16 bits/sample
		eBitDepthR32 = 32,  ///< 32 bit floating point
		eBitDepthR64 = 64,  ///< 64 bit floating point
		eBitDepthUndefined = UINT8_MAX  ///< undefined bit depth
	};

	/// Settings for datum packing
	enum HdrDpxPacking {
		ePackingPacked = 0,   ///< Packed
		ePackingFilledMethodA = 1,  ///< Filled using method A (10/12 bit only)
		ePackingFilledMethodB = 2,  ///< Filled using method B (10/12 bit only)
		ePackingUndefined = UINT16_MAX  ///< undefined packing
	};

	/// Settings for encoding (RLE)
	enum HdrDpxEncoding {
		eEncodingNoEncoding = 0,  ///< uncompressed (no encoding)
		eEncodingRLE = 1,   ///< compressed using run-length encoding (RLE)
		eEncodingUndefined = UINT16_MAX  ///< undefined encoding
	};

	/// Settings for interlace field
	enum HdrDpxInterlace {
		eInterlaceNone = 0,   ///< not interlaced
		eInterlace2_1 = 1,   ///< 2:1 interlaced
		eInterlaceUndefined = UINT8_MAX  ///< undefined interlacing
	};

	/// Settings for video signal standard
	enum HdrDpxVideoSignal {
		eVideoSignalUndefined0 = 0,  ///< video signal undefined
		eVideoSignalNTSC = 1,   ///< NTSC video signal
		eVideoSignalPAL = 2,  ///< PAL video signal
		eVideoSignalPAL_M = 3,  ///< PAL-M video signal
		eVideoSignalSECAM = 4,   ///< SECAM video signal
		eVideoSignalBT_601_525_4x3 = 50,  ///< ITU-R BT.601 525 line 4x3 video signal
		eVideoSignalBT_601_625_4x3 = 51,  ///< ITU-R BT.601 625 line 4x3 video signal
		eVideoSignalBT_601_525_16x9 = 100,   ///< ITU-R BT.601 525 line 16x9 video signal
		eVideoSignalBT_601_625_16x9 = 101,  ///< ITU-R BT.601 625 line 16x9 video signal
		eVideoSignalYCbCr_int_1050ln_16x9 = 150,  ///< interlaced 1050 line 16x9 video signal (YCbCr)
		eVideoSignalYCbCr_int_1125ln_16x9_ST_274 = 151,  ///< interlaced 1125 line 16x9 video signal (SMPTE ST 274)
		eVideoSignalYCbCr_int_1250ln_16x9 = 152,   ///< interlaced 1250 line 16x9 video signal (YCbCr)
		eVideoSignalYCbCr_int_1125ln_16x9_ST_240 = 153,  ///< interlaced 1125 line 16x9 video signal (SMPTE ST 240)
		eVideoSignalYCbCr_prog_525ln_16x9 = 200,  ///< progressive 525 line 16x9 video signal (YCbCr)
		eVideoSignalYCbCr_prog_625ln_16x9 = 201,  ///< progresssive 625 line 16x9 video signal (YCbCr)
		eVideoSignalYCbCr_prog_750ln_16x9_ST_296 = 202,  ///< progressive 750 lines 16x9 video signal (SMPTE ST 296)
		eVideoSignalYCbCr_prog_1125ln_16x9_ST_274 = 203,  ///< progressive 1125 line 16x9 video signal (SMPTE ST 274)
		eVideoSignalCTA_VIC = 250,  ///< video signal defined by CTA VIC code (861-G)
		eVideoSignalUndefined = UINT8_MAX  ///< undefined video signal
	};

	/// Settings for CTA video identification code (VIC)
	enum HdrDpxVideoIdentificationCode {
		eVIC_DMT0659 = 1,	///< 640x480p 59.94/60Hz, 4:3 picture, 1:1 pixel
		eVIC_480p = 2,		///< 720x480p 59.94/60Hz, 4:3 picture, 8:9 pixel
		eVIC_480pH = 3,		///< 720x480p 59.94/60Hz, 16:9 picture, 32:27 pixel
		eVIC_720p = 4,		///< 1280x720p 59.94/60Hz, 16:9 picture, 1:1 pixel
		eVIC_1080i = 5,		///< 1920x1080i 59.94/60Hz, 16:9 picture, 1:1 pixel
		eVIC_480i = 6,		///< 720x480i 59.94/60Hz, 4:3 picture, 8:9 pixel
		eVIC_480iH = 7,		///< 720(1440)x480i 59.94/60Hz, 16:9 picture, 32:27 pixel
		eVIC_240p = 8,		///< 720(1440)x480i 59.94/60Hz, 4:3 picture, 4:9 pixel
		eVIC_240pH = 9,		///< 720(1440)x240p 59.94/60Hz, 16:9 picture, 16:27 pixel
		eVIC_480i4x = 10,	///< 2880x480i 59.94/60Hz, 4:3 picture, 2:9-20:9 pixel
		eVIC_480i4xH = 11,	///< 2880x480i 59.94/60Hz, 16:9 picture, 8:27-80:27 pixel
		eVIC_240p4x = 12,	///< 2880x240p 59.94/60Hz, 4:3 picture, 1:9-10:9 pixel
		eVIC_240p4xH = 13,	///< 2880x240p 59.94/60Hz, 16:9 picture, 4:27-40:27 pixel
		eVIC_480p2x = 14,	///< 1440x480p 59.94/60Hz, 4:3 picture, 4:9 or 8:9 pixel
		eVIC_480p2xH = 15,	///< 1440x480p 59.94/60Hz, 16:9 picture, 16:27 or 32:27 pixel
		eVIC_1080p = 16,	///< 1920x1080p 59.94/60Hz, 16:9 picture, 1:1 pixel
		eVIC_576p = 17,		///< 720x576p 50Hz, 4:3 picture, 16:15 pixel
		eVIC_576pH = 18,	///< 720x576p 50Hz, 16:9 picture, 64:45 pixel
		eVIC_720p50 = 19,	///< 1280x720p 50Hz, 16:9 picture, 1:1 pixel
		eVIC_1080i25_20 = 20, ///< 1920x1080i 50Hz, 16:9 picture, 1:1 pixel
		eVIC_576i = 21,		///< 720(1440)x576i 50Hz, 4:3 picture, 16:15 pixel
		eVIC_576iH = 22,	///< 720(1440)x576i 50Hz, 16:9 picture, 64:45 pixel
		eVIC_288p = 23,		///< 720(1440)x288p 50Hz, 4:3 picture, 8:15 pixel
		eVIC_288pH = 24,	///< 720(1440)x288p 50Hz, 16:9 picture, 32:45 pixel
		eVIC_576i4x = 25,	///< 2880x576i 50Hz, 4:3 picture, 2:15-20:15 pixel
		eVIC_576i4xH = 26,	///< 2880x576i 50Hz, 16:9 picture, 16:45-160:45 pixel
		eVIC_288p4x = 27,	///< 2880x288p 50Hz, 4:3 picture, 1:15-10:15 pixel
		eVIC_288p4xH = 28,	///< 2880x288p 50Hz, 16:9 picture, 8:45-80:45 pixel
		eVIC_576p2x = 29,	///< 1440x576p 50Hz, 4:3 picture, 8:15 or 16:15 pixel
		eVIC_576p2xH = 30,	///< 1440x576p 50Hz, 16:9 picture, 32:45 or 64:45 pixel
		eVIC_1080p50 = 31,	///< 1920x1080p 50Hz, 16:9 picture, 1:1 pixel
		eVIC_1080p24 = 32,	///< 1920x1080p 23.98/24Hz, 16:9 picture, 1:1 pixel
		eVIC_1080p25 = 33,	///< 1920x1080p 25Hz, 16:9 picture, 1:1 pixel
		eVIC_1080p30 = 34,	///< 1920x1080p 29.97/30Hz, 16:9 picture, 1:1 pixel
		eVIC_480p4x = 35,	///< 2880x480p 59.94/60Hz, 4:3 picture, 2:9, 4:9 or 8:9 pixel
		eVIC_480p4xH = 36,	///< 2880x480p 59.94/60Hz, 16:9 picture, 8:27, 16:27 or 32:27 pixel
		eVIC_576p4x = 37,	///< 2880x576p 50Hz, 4:3 picture, 4:15, 8:15, or 16:15 pixel
		eVIC_576p4xH = 38,	///< 2880x576p 50Hz, 16:9 picture, 16:45, 32:45, or 64:45 pixel
		eVIC_1080i25_39 = 39, ///< 1920x1080i 50Hz, 16:9 picture, 1:1 pixel
		eVIC_1080i50 = 40,	///< 1920x1080i 100Hz, 16:9 picture, 1:1 pixel
		eVIC_720p100 = 41,	///< 1280x720p 100Hz, 16:9 picture, 1:1 pixel
		eVIC_576p100 = 42,	///< 720x576p 100Hz, 4:3 picture, 16:15 pixel
		eVIC_576p100H = 43,	///< 720x576p 100Hz, 16:9 picture, 64:45 pixel
		eVIC_576i50 = 44,	///< 720(1440)x576i 100Hz, 4:3 picture, 16:15 pixel
		eVIC_576i50H = 45,	///< 720(1440)x576i 100Hz, 16:9 picture, 64:45 pixel
		eVIC_1080i60 = 46,	///< 1920x1080i 119.88/120Hz, 16:9 picture, 1:1 pixel
		eVIC_720p120 = 47,	///< 1280x720p 119.88/120Hz, 16:9 picture, 1:1 pixel
		eVIC_480p119 = 48,	///< 720x480p 119.88/120Hz, 4:3 picture, 8:9 pixel
		eVIC_480p119H = 49,	///< 720x480p 119.88/120Hz, 16:9 picture, 32:27 pixel
		eVIC_480i59 = 50,	///< 720(1440)x480i 119.88/120Hz, 4:3 picture, 8:9 pixel
		eVIC_480i59H = 51,	///< 720(1440)x480i 119.88/120Hz, 16:9 picture, 32:27 pixel
		eVIC_576p200 = 52,	///< 720x576p 200Hz, 4:3 picture, 16:15 pixel
		eVIC_576p200H = 53,	///< 720x576p 200Hz, 16:9 picture, 64:45 pixel
		eVIC_576i100 = 54,	///< 720(1440)x576i 200Hz, 4:3 picture, 16:15 pixel
		eVIC_576i100H = 55,	///< 720(1440)x576i 200Hz, 16:9 picture, 64:45 pixel
		eVIC_480p239 = 56,	///< 720x480p 239.76/240Hz, 4:3 picture, 8:9 pixel
		eVIC_480p239H = 57,	///< 720x480p 239.76/240Hz, 16:9 picture, 32:27 pixel
		eVIC_480i119 = 58,	///< 720(1440)x480i 239.76/240Hz, 4:3 picture, 8:9 pixel
		eVIC_480i119H = 59,	///< 720(1440)x480i 239.76/240Hz, 16:9 picture, 32:27 pixel
		eVIC_720p24 = 60,	///< 1280x720p 23.98/24Hz, 16:9 picture, 1:1 pixel
		eVIC_720p25 = 61,	///< 1280x720p 25Hz, 16:9 picture, 1:1 pixel
		eVIC_720p30 = 62,	///< 1280x720p 29.97/30Hz, 16:9 picture, 1:1 pixel
		eVIC_1080p120 = 63,	///< 1920x1080p 119.88/120Hz, 16:9 picture, 1:1 pixel
		eVIC_1080p100 = 64,	///< 1920x1080p 100Hz, 16:9 picture, 1:1 pixel
		eVIC_720p24W = 65,	///< 1280x720p 23.98/24Hz, 64:27 picture, 4:3 pixel
		eVIC_720p25W = 66,	///< 1280x720p 25Hz, 64:27 picture, 4:3 pixel
		eVIC_720p30W = 67,	///< 1280x720p 29.97/30Hz, 64:27 picture, 4:3 pixel
		eVIC_720p50W = 68,	///< 1280x720p 50Hz, 64:27 picture, 4:3 pixel
		eVIC_720pW = 69,	///< 1280x720p 59.94/60Hz, 64:27 picture, 4:3 pixel
		eVIC_720p100W = 70,	///< 1280x720p 100Hz, 64:27 picture, 4:3 pixel
		eVIC_720p120W = 71,	///< 1280x720p 119.88/120Hz, 64:27 picture, 4:3 pixel
		eVIC_1080p24W = 72,	///< 1920x1080p 23.98/24Hz, 64:27 picture, 4:3 pixel
		eVIC_1080p25W = 73,	///< 1920x1080p 25Hz, 64:27 picture, 4:3 pixel
		eVIC_1080p30W = 74,	///< 1920x1080p 29.97/30Hz, 64:27 picture, 4:3 pixel
		eVIC_1080p50W = 75,	///< 1920x1080p 50Hz, 64:27 picture, 4:3 pixel
		eVIC_1080pW = 76,	///< 1920x1080p 59.94/60Hz, 64:27 picture, 4:3 pixel
		eVIC_1080p100W = 77, ///< 1920x1080p 100Hz, 64:27 picture, 4:3 pixel
		eVIC_1080p120W = 78, ///< 1920x1080p 119.88/120Hz, 64:27 picture, 4:3 pixel
		eVIC_720p2x24 = 79,	///< 1680x720p 23.97/30Hz, 64:27 picture, 64:63 pixel
		eVIC_720p2x25 = 80,	///< 1680x720p 25Hz, 64:27 picture, 64:63 pixel
		eVIC_720p2x30 = 81,	///< 1680x720p 29.97/30Hz, 64:27 picture, 64:63 pixel
		eVIC_720p2x50 = 82,	///< 1680x720p 50Hz, 64:27 picture, 64:63 pixel
		eVIC_720p2x = 83,	///< 1680x720p 59.94/60Hz, 64:27 picture, 64:63 pixel
		eVIC_720p2x100 = 84, ///< 1680x720p 100Hz, 64:27 picture, 64:63 pixel
		eVIC_720p2x120 = 85, ///< 1680x720p 119.88/120Hz, 64:27 picture, 64:63 pixel
		eVIC_1080p2x24 = 86, ///< 2560x1080p 23.98/24Hz, 64:27 picture, 1:1 pixel
		eVIC_1080p2x25 = 87, ///< 2560x1080p 25Hz, 64:27 picture, 1:1 pixel
		eVIC_1080p2x30 = 88, ///< 2560x1080p 29.97/30Hz, 64:27 picture, 1:1 pixel
		eVIC_1080p2x50 = 89, ///< 2560x1080p 50Hz, 64:27 picture, 1:1 pixel
		eVIC_1080p2x = 90,	///< 2560x1080p 59.94/60Hz, 64:27 picture, 1:1 pixel
		eVIC_1080p2x100 = 91, ///< 2560x1080p 100Hz, 64:27 picture, 1:1 pixel
		eVIC_1080p2x120 = 92, ///< 2560x1080p 119.88/120Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p24 = 93,	///< 3840x2160p 23.98/24Hz, 16:9 picture, 1:1 pixel
		eVIC_2160p25 = 94,	///< 3840x2160p 25Hz, 16:9 picture, 1:1 pixel
		eVIC_2160p30 = 95,	///< 3840x2160p 29.97/30Hz, 16:9 picture, 1:1 pixel
		eVIC_2160p50 = 96,	///< 3840x2160p 50Hz, 16:9 picture, 1:1 pixel
		eVIC_2160p60 = 97,	///< 3840x2160p 59.94/60Hz, 16:9 picture, 1:1 pixel
		eVIC_4096x2160p24 = 98, ///< 4096x2160p 23.98/24Hz, 256:135 picture, 1:1 pixel
		eVIC_4096x2160p25 = 99, ///< 4096x2160p 25Hz, 256:135 picture, 1:1 pixel
		eVIC_4096x2160p30 = 100, ///< 4096x2160p 29.97/30Hz, 256:135 picture, 1:1 pixel
		eVIC_4096x2160p50 = 101, ///< 4096x2160p 50Hz, 256:135 picture, 1:1 pixel
		eVIC_4096x2160p = 102, ///< 4096x2160p 59.94/60Hz, 256:135 picture, 1:1 pixel
		eVIC_2160p24W = 103, ///< 3840x2160p 23.98/24Hz, 64:27 picture, 4:3 pixel
		eVIC_2160p25W = 104, ///< 3840x2160p 25Hz, 64:27 picture, 4:3 pixel
		eVIC_2160p30W = 105, ///< 3840x2160p 29.97/30Hz, 64:27 picture, 4:3 pixel
		eVIC_2160p50W = 106, ///< 3840x2160p 50Hz, 64:27 picture, 4:3 pixel
		eVIC_2160pW = 107,	///< 3840x2160p 59.94/60Hz, 64:27 picture, 4:3 pixel
		eVIC_720p48 = 108,	///< 1280x720p 47.95/48Hz, 16:9 picture, 1:1 pixel
		eVIC_720p48W = 109, ///< 1280x720p 47.95/48Hz, 64:27 picture, 4:3 pixel
		eVIC_720p2x48 = 110, ///< 1680x720p 47.95/48Hz, 64:27 picture, 64:63 pixel
		eVIC_1080p48 = 111,	///< 1920x1080p 47.95/48Hz, 16:9 picture, 1:1 pixel
		eVIC_1080p48W = 112, ///< 1920x1080p 47.95/48Hz, 64:27 picture, 4:3 pixel
		eVIC_1080p2x48 = 113, ///< 2560x1080p 47.95/48Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p48 = 114, ///< 3840x2160p 47.95/48Hz, 16:9 picture, 1:1 pixel
		eVIC_4096x2160p48 = 115, ///< 4096x2160p 47.95/48Hz, 256:135 picture, 1:1 pixel
		eVIC_2160p48W = 116, ///< 3840x2160p 47.95/48Hz, 64:27 picture, 4:3 pixel
		eVIC_2160p100 = 117, ///< 3840x2160p 100Hz, 16:9 picture, 1:1 pixel
		eVIC_2160p120 = 118, ///< 3840x2160p 119.88/120Hz, 16:9 picture, 1:1 pixel
		eVIC_2160p100W = 119, ///< 3840x2160p 100Hz, 64:27 picture, 4:3 pixel
		eVIC_2160p120W = 120, ///< 3840x2160p 119.88/120Hz, 64:27 picture, 4:3 pixel
		eVIC_2160p2x24 = 121, ///< 5120x2160p 23.98/24Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p2x25 = 122, ///< 5120x2160p 25Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p2x30 = 123, ///< 5120x2160p 29.97/30Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p2x48 = 124, ///< 5120x2160p 47.95/48Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p2x50 = 125, ///< 5120x2160p 50Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p2x = 126,	///< 5120x2160p 59.94/60Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p2x100 = 127, ///< 5120x2160p 100Hz, 64:27 picture, 1:1 pixel
		eVIC_2160p2x120 = 193, ///< 5120x2160p 119.88/120Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p24 = 194,	///< 7680x4320p 23.98/24Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p25 = 195,	///< 7680x4320p 25Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p30 = 196, ///< 7680x4320p 29.97/30Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p48 = 197, ///< 7680x4320p 47.95/48Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p50 = 198, ///< 7680x4320p 50Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p = 199,	///< 7680x4320p 59.94/60Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p100 = 200, ///< 7680x4320p 100Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p120 = 201, ///< 7680x4320p 119.88/120Hz, 16:9 picture, 1:1 pixel
		eVIC_4320p24W = 202, ///< 7680x4320p 23.98/24Hz, 64:27 picture, 4:3 pixel
		eVIC_4320p25W = 203, ///< 7680x4320p 25Hz, 64:27 picture, 4:3 pixel
		eVIC_4320p30W = 204, ///< 7680x4320p 29.97/30Hz, 64:27 picture, 4:3 pixel
		eVIC_4320p48W = 205, ///< 7680x4320p 47.95/48Hz, 64:27 picture, 4:3 pixel
		eVIC_4320p50W = 206, ///< 7680x4320p 50Hz, 64:27 picture, 4:3 pixel
		eVIC_4320pW = 207,	///< 7680x4320p 59.94/60Hz, 64:27 picture, 4:3 pixel
		eVIC_4320p100W = 208, ///< 7680x4320p 100Hz, 64:27 picture, 4:3 pixel
		eVIC_4320p120W = 209, ///< 7680x4320p 119.88/120Hz, 64:27 picture, 4:3 pixel
		eVIC_4320p2x24 = 210, ///< 10240x4320p 23.98/24Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p2x25 = 211, ///< 10240x4320p 25Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p2x30 = 212, ///< 10240x4320p 29.97/30Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p2x48 = 213, ///< 10240x4320p 47.95/48Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p2x50 = 214, ///< 10240x4320p 50Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p2x = 215,	///< 10240x4320p 59.94/60Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p2x100 = 216, ///< 10240x4320p 100Hz, 64:27 picture, 1:1 pixel
		eVIC_4320p2x120 = 217, ///< 10240x4320p 119.88/120Hz, 64:27 picture, 1:1 pixel
		eVIC_4096x2160p100 = 218, ///< 4096x2160p 100Hz, 256:135 picture, 1:1 pixel
		eVIC_4096x2160p120 = 219, ///< 4096x2160p 119.88/120Hz, 256:135 picture, 1:1 pixel
		eVIC_undefined = UINT8_MAX  ///< undefined VIC
	};

	/// Settings for color difference siting
	enum HdrDpxColorDifferenceSiting
	{
		eSitingCositedHCositedV = 0,  ///< Horizontally cosited, Vertically cosited
		eSitingInterstitialHCositedV = 1,  ///< Horizontally interstitial, Vertically cosited
		eSitingCositedHinterstitialV = 2,  ///< Horizontally cosited, vertically interstitial
		eSitingInterstitialHInterstitialV = 3,  ///< Horizontally interstitial, vertically interstitial
		eSitingUndefined = 15  ///< Undefined color difference siting
	};

	/// Settings for SMPTE time code type
	enum HdrDpxSMPTETCType
	{
		eSMPTETCTypeST268_1 = 0,  ///< SMPTE ST 268-1 time code
		eSMPTETCTypeST12_1_LTC = 1,  ///< SMPTE ST 12-1 LTC
		eSMPTETCTypeST12_1_VITC = 2,  ///< SMPTE ST 12-1 VITC
		eSMPTETCTypeST12_3_Codeword = 3,  ///< SMPTE ST 12-3 codeword
		eSMPTETCTypeUndefined = 255   ///< Undefined time code type
	};

	/// options for dumping data as bytes or strings
	enum HdrDpxDumpFormat
	{
		eDumpFormatDefault,   ///< Use default for data type
		eDumpFormatAsBytes,   ///< Dump data as bytes
		eDumpFormatAsStrings, ///< Dump data as strings
	};

	/// Data structure for SMPTE time code
	struct SMPTETimeCode
	{
		// Each of these fields are actually 4-bits (nibbles)
		unsigned int h_tens : 4;   ///< tens of hours
		unsigned int h_units : 4;  ///< units of hours
		unsigned int m_tens : 4;   ///< tens of minutes
		unsigned int m_units : 4;  ///< units of minutes
		unsigned int s_tens : 4;   ///< tens of seconds
		unsigned int s_units : 4;  ///< units of seconds
		unsigned int F_tens : 4;   ///< tens of frames
		unsigned int F_units : 4;  ///< units of frames
	};

	/// Data structure for SMPTE user bits
	struct SMPTEUserBits
	{
		unsigned int UB8 : 4;  ///< UB8 (bits 31:28)
		unsigned int UB7 : 4;  ///< UB7 (bits 27:24)
		unsigned int UB6 : 4;  ///< UB6 (bits 23:20)
		unsigned int UB5 : 4;  ///< UB5 (bits 19:16)
		unsigned int UB4 : 4;  ///< UB4 (bits 15:12)
		unsigned int UB3 : 4;  ///< UB3 (bits 11:8)
		unsigned int UB2 : 4;  ///< UB2 (bits 7:4)
		unsigned int UB1 : 4;  ///< UB1 (bits 3:0)
	};

}

/** Undefined value for U32 fields */
#define UNDEFINED_U32	UINT32_MAX
/** Undefined value for U16 fields */
#define UNDEFINED_U16   UINT16_MAX
/** Undefined value for U8 fields */
#define UNDEFINED_U8    UINT8_MAX




namespace Dpx {
	/** Convert a descriptor to a list (vector) of datum labels
		@param desc	Descriptor
		@return		Vector of datum labels
	*/
	std::vector<DatumLabel> DescriptorToDatumList(uint8_t desc);

	/** Convert a list (vector) of datum labels to a descriptor 
		@param datum_list	Vector of datum labels
		@return				Descriptor corresponding to datum label list, or 255 if no descriptor matches the list */
	uint8_t DatumListToDescriptor(std::vector<DatumLabel> datum_list);

	class HdrDpxFile;

	/** Interface for handling a single image element within a DPX file
	*
	* A DPX file can contain up to 8 image elements, and each one containing image data is
	* mapped to an HdrDpxImageElement object.
	*/
	class HdrDpxImageElement
	{
	public:
		~HdrDpxImageElement();

		// Writing functions:
		/** Write a row of integer pixels from a specified pointer to a file. IE must be configured with bit depth of 16 or less (integer samples).
			@param row				row number to write
			@param[in]	datum_ptr	pointer to buffer that contains a line's worth of samples to write */
		void App2DpxPixels(uint32_t row, int32_t *datum_ptr);
		/** Write a row of 32-bit float pixels from a specified pointer to a file. IE must be configured with bit depth = 32 (float samples).
			@param row				row number to write
			@param[in]	datum_ptr	pointer to buffer that contains a line's worth of samples to write */
		void App2DpxPixels(uint32_t row, float *datum_ptr);
		/** Write a row of integer pixels from a specified pointer to a file. IE must be configured with bit depth = 64 (double samples).
			@param row				row number to write
			@param[in]	datum_ptr	pointer to buffer that contains a line's worth of samples to write */
		void App2DpxPixels(uint32_t row, double *datum_ptr);

		/** Set a U32 header field to a specific value
			@param field			field to write to 
			@param value			value to write to the field */
		void SetHeader(HdrDpxIEFieldsU32 field, uint32_t value);
		/** Set a R32 header field to a specific value
			@param field			field to write to
			@param value			value to write to the field */
		void SetHeader(HdrDpxIEFieldsR32 field, float value);
		/** Set a string header field to a specific value
			@param field			field to write to
			@param value			value to write to the field */
		void SetHeader(HdrDpxIEFieldsString field, std::string value);
		/** Set the data sign header field
			@param field			eDataSign
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsDataSign field, HdrDpxDataSign value);
		/** Set the descriptor header field
			@param field			eDescriptor
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsDescriptor field, HdrDpxDescriptor value);
		/** Set the transfer characteristic header field
			@param field			eTransferCharacteristic
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsTransfer field, HdrDpxTransfer value);
		/** Set the colorimetric specification header field
			@param field			eColorimetricSpecifcation
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsColorimetric field, HdrDpxColorimetric value);
		/** Set the bit depth header field
			@param field			eBitDepth
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsBitDepth field, HdrDpxBitDepth value);
		/** Set the packing header field
			@param field			ePacking
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsPacking field, HdrDpxPacking value);
		/** Set the encoding header field
			@param field			eEncoding
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsEncoding field, HdrDpxEncoding value);
		/** Set the color difference siting header field
			@param field			eColorDifferenceSiting
			@param value			enum value to write to the field */
		void SetHeader(HdrDpxFieldsColorDifferenceSiting field, HdrDpxColorDifferenceSiting value);

		/** Copy the image element header info from another image element object
			@param	ie				pointer to other image element object to copy from */
		void CopyHeaderFrom(HdrDpxImageElement *ie);

		// Reading functions:
		/** Read a row of integer pixels from a DPX file to a specified pointer. Fails if file contains floating point samples.
			@param row				row number to read
			@param[out] datum_ptr	pointer to buffer to write samples to */
		void Dpx2AppPixels(uint32_t row, int32_t *datum_ptr);
		/** Read a row of 32-bit float pixels from a DPX file to a specified pointer. Fails if file does not contain 32-bit float samples.
			@param row				row number to read
			@param[out] datum_ptr	pointer to buffer to write samples to */
		void Dpx2AppPixels(uint32_t row, float *datum_ptr);
		/** Read a row of 64-bit float pixels from a DPX file to a specified pointer. Fails if file does not contain 64-bit float samples.
			@param row				row number to read
			@param[out] datum_ptr	pointer to buffer to write samples to */
		void Dpx2AppPixels(uint32_t row, double *datum_ptr);
		/** Return the number of pixels per row in the file
			@return					pixels per row */
		uint32_t GetWidth(void) const;
		/** Return the number of rows of pixels in the file
			@return					number of rows of pixels */
		uint32_t GetHeight(void) const;
		/** Return the number of bytes per row for the image element
			@param include_padding	if true, include end-of-line padding bytes in the count
			@return					row size in bytes */
		uint32_t GetRowSizeInBytes(bool include_padding = false) const;
		/** Return the number of datums in each row for the image element
			@return					number of datums per row */
		uint32_t GetRowSizeInDatums() const;
		/** Return the total image data size for the image element in bytes
			@return					image data size in bytes */
		uint32_t GetImageDataSizeInBytes() const;

		/** Get the value of the specified U32 header field 
			@return					header field value */
		uint32_t GetHeader(HdrDpxIEFieldsU32 field) const;
		/** Get the value of the specified R32 header field
			@return					header field value */
		float GetHeader(HdrDpxIEFieldsR32 field) const;
		/** Get the value of the specified string header field
			@return					string value */
		std::string GetHeader(HdrDpxIEFieldsString field) const;
		/** Get the value of the data sign header field
			@return					data sign */
		HdrDpxDataSign GetHeader(HdrDpxFieldsDataSign field) const;
		/** Get the value of the descriptor header field
			@return					descriptor */
		HdrDpxDescriptor GetHeader(HdrDpxFieldsDescriptor field) const;
		/** Get the value of the transfer characteristic header field
			@return					transfer characteristic */
		HdrDpxTransfer GetHeader(HdrDpxFieldsTransfer field) const;
		/** Get the value of the colorimetric specification header field
			@return					colorimetric specifcation */
		HdrDpxColorimetric GetHeader(HdrDpxFieldsColorimetric field) const;
		/** Get the value of the bit depth header field
			@return					bit depth */
		HdrDpxBitDepth GetHeader(HdrDpxFieldsBitDepth field) const;
		/** Get the value of the packing header field 
			@return					packing */
		HdrDpxPacking GetHeader(HdrDpxFieldsPacking field) const;
		/** Get the value of the encoding header field
			@return					encoding */
		HdrDpxEncoding GetHeader(HdrDpxFieldsEncoding field) const;
		/** Get the value of the color difference siting header field
			@return					color difference siting */
		HdrDpxColorDifferenceSiting GetHeader(HdrDpxFieldsColorDifferenceSiting field) const;

		/** Get a list of datum labels based on the descriptor
			@return					vector of datum labels */
		std::vector<DatumLabel> GetDatumLabels(void) const;
		/** Get the index of the specified datum label
			@return					index of specified datum label */
		uint8_t GetDatumLabelIndex(DatumLabel dl) const;
		/** Get the number of components based on the descriptor
			@return					number of components */
		uint8_t GetNumberOfComponents(void) const;
		/** Convert a datum label to a printable string
			@return					string description of datum label */
		std::string DatumLabelToName(Dpx::DatumLabel datum_label) const;


	private:
		HdrDpxImageElement();
		friend class HdrDpxFile;
		///////////////////////////////////////// called from HdrDpxFile class:
		HdrDpxImageElement(uint8_t ie_idx, std::fstream *fstream_ptr, HDRDPXFILEFORMAT *dpxf_ptr, FileMap *file_map_ptr);
		/** Initializes the IE if the blank constructor was used
			@param[in] ie_idx		Index number (0-7) of the image element in the file
			@param[in] fstream_ptr	Pointer to the open fstream for accessing the DPX file
			@param[in] dpxf_ptr		Pointer to the header structure for the DPX file
			@param[in] file_map_ptr	Pointer to the file map for the DPX file */
		void Initialize(uint8_t ie_idx, std::fstream *fstream_ptr, HDRDPXFILEFORMAT *dpxie_ptr, FileMap *file_map_ptr);
		/** Deinitialize the IE */
		void Deinitialize();
		/** Lock the header so it can't be modified */
		void LockHeader();
		/** Unlock the header so it can be modified */
		void UnlockHeader();
		/** Call if file is open for reading */
		void OpenForReading(bool bswap);
		/** Call if file is open for writing */
		void OpenForWriting(bool bswap);

		// Internal functions
		/** Write a single pixel to a Fifo 
			@param xpos				Pixel x position within line */
		void WritePixel(uint32_t xpos);
		/** Write a single datum value to a Fifo 
			@param datum			Datum value to write */
		void WriteDatum(int32_t datum);
		/** Finish writing any data still left in FIFO */
		void WriteFlush();
		/** Finish writing a line, performing any necessary padding */
		void WriteLineEnd();
		/** Function to determine if next pixel is the same (for RLE) 
			@param xpos				X position within line
			@param pixel			Pixel value to match */
		bool IsNextSame(uint32_t xpos, int32_t pixel[]);
		/** Compute the width and height values from the header info */
		void ComputeWidthAndHeight(void);

		/** Reset any accumulated warning messages */
		void ResetWarnings(void);


		// Pointers back to HdrDpxFile
		HDRDPXFILEFORMAT *m_dpx_hdr_ptr; //!< Pointer to main DPX file data structure
		HDRDPX_IMAGEELEMENT *m_dpx_ie_ptr;  //!< Pointer to IE data structure
		std::fstream *m_filestream_ptr;  //!< Pointer to file stream in HdrDpx object
		FileMap *m_file_map_ptr;   //!< Pointer to file map in HdrDpx object

		uint32_t GetOffsetForRow(uint32_t row) const; //!< Return file offset (seek pointer) for specific row
		void ReadRow(uint32_t row);  //!< Read the row from a file
		void WriteRow(uint32_t row);  //!< Write the row to a file
		uint32_t BytesUsed(void);   //!< Returns the number of bytes used for the IE
		
		uint32_t m_width;  //!< width of IE (in pixels)
		uint32_t m_height;  //!< height of IE (in pixels)
		bool m_byte_swap;   //!< flag indicating whether byte swap is needed
		bool m_direction_r2l;   //!< 0 = left-to-right datum order, 1 = right-to-left datum order
		std::list<std::string> m_warnings;   //!< List of warning mesages
		ErrorObject m_err;   //!< Error object (for tracking errors)
		Fifo m_fifo;   //!< FIFO object

		uint8_t m_ie_index = 0xff;  //!< indicates which IE index corresponds to this IE
		float *m_float_row;  //!< pointer to floating point pixel data
		double *m_double_row;  //!< pointer to double precision pixel data
		int32_t *m_int_row;  //!< pointer to integer pixel datat
		uint32_t m_row_rd_idx;  //!< which row is being read
		bool m_is_open_for_write;  //!< flag indicating if file is open for writing
		bool m_is_open_for_read;  //!< flag indicating if file is open for reading
		bool m_is_header_locked = false;  //!< flag indicating header is locked
		bool m_isinitialized = false;  //!< flag indicating whether header is initialized
		uint32_t m_previous_row;   //!< which row was last read
		uint32_t m_previous_file_offset;  //!< keeps track of where we're reading for IE in case another IE is read and changes seek position
		bool m_is_h_subsampled;  //!< flag indicating if chroma is horizontally subsampled by 2
		bool m_is_v_subsampled;  //!< Flag indicating if chroma is vertically subsampled by 2

		bool m_warn_unexpected_nonzero_data_bits;  //!< flag indicating unexepected nonzero data bits were encountered
		uint32_t m_warn_image_data_word_mask;  //!< indicates which bit positions unexpected nonzero data bits were found in
		bool m_warn_rle_same_past_eol;  //!< flag indicating a "same" run went past the end of the line
		bool m_warn_rle_diff_past_eol;  //!< flag indicating a "different" run went past the end of the line
		bool m_warn_zero_run_length;  //!< flag indicating a zero run length was signaled
	};

	/** Main interface for reading or writing a DPX file
	*/
	class HdrDpxFile
	{
	public:
		/** Blank constructor (always use for writing, can be used for reading) */
		HdrDpxFile(); 
		/** Special constructor to open a file for reading 
			@param filename			Filename of DPX file to read */
		HdrDpxFile(std::string filename);			// Shortcut to open a file for reading
		~HdrDpxFile();
		/** Overload of << that allows DPX header information to be dumped to specified ostream (e.g., cout << dpxfileobject) */
		friend std::ostream& operator<<(std::ostream & os, const HdrDpxFile &dpxf)
		{
			os << dpxf.DumpHeader();
			return os;
		}
		/** Open the specified DPX file for reading. Do not call this if the filename was passed to the constructor already 
			@param filename			Filename of DPX file to read */
		void OpenForReading(std::string filename);
		/** Close the DPX file */
		void Close();
		/** Open the specified DPX file for writing. 
		    @param filename			Filename of DPX file to write */
		void OpenForWriting(std::string filename);
		/** Dump the DPX header information to a string 
		    @return					string containing DPX header information */
		std::string DumpHeader() const;
		/** Validates the DPX header. Errors/warnings/informational messages can be retrieved using GetError() after calling this function.
		    @return					true = errors and/or warnings found, false = no errors found */
		bool Validate();
		/** Returns true if the file contains the ST 268-2 version string. */
		bool IsHdr() const;
		/** Returns true if the file read ok (no fatal errors) */
		bool IsOk() const;
		/** Returns the number of errors/warnings/messages that have been logged so far */
		int GetNumErrors() const;
		/** This function is called to retrieve a single error message 
			@param[in] index			index (from 0 to GetNumErrors() - 1) of the error to retrieve 
			@param[out] errcode			Error code of the error
			@param[out] severity		Severity of the error
			@param[out] errmsg			Description of the error as a string */
		void GetError(int index, ErrorCode &errcode, ErrorSeverity &severity, std::string &errmsg) const;
		/** Clears the list of errors logged */
		void ClearErrors();
		/** Get a list (vector) of indices of which IEs are present */
		std::vector<uint8_t> GetIEIndexList() const;
		/** Copy header info from another DPX file object */
		void CopyHeaderFrom(const HdrDpxFile &src);
		/*** Indicate whether user data is dumped with header dump (default no) and if dumped as bytes or string (default bytes) */
		void DumpUserDataOptions(bool dump_ud, HdrDpxDumpFormat format = eDumpFormatDefault);
		/*** Indicate whether standards-based metadata is dumped with header dump (default no) and if dumped as bytes or strings */
		void DumpStandardsBasedMetedataOptions(bool dump_sbmd, HdrDpxDumpFormat format = eDumpFormatDefault);

		/** Gets the value of a string header field
			@param field			which header field to get
			@return					string value */
		std::string GetHeader(HdrDpxFieldsString field) const;
		/** Gets the value of a U32 header field
			@param field			which header field to get
			@return					U32 value */
		uint32_t GetHeader(HdrDpxFieldsU32 field) const;
		/** Gets the value of a U16 header field
			@param field			which header field to get
			@return					U16 value */
		uint16_t GetHeader(HdrDpxFieldsU16 field) const;
		/** Gets the value of a R32 header field
			@param field			which header field to get
			@return					R32 value */
		float GetHeader(HdrDpxFieldsR32 field) const;
		/** Gets the value of a U8 header field
			@param field			which header field to get
			@return					U8 value */
		uint8_t GetHeader(HdrDpxFieldsU8 field) const;
		/** Gets the value of the ditto key header field
			@param field			selects ditto key field
			@return					ditto key value */
		HdrDpxDittoKey GetHeader(HdrDpxFieldsDittoKey field) const;
		/** Gets the value of the datum mapping direction header field
			@param field			selects datum mapping direction field
			@return					datum mapping direction value */
		HdrDpxDatumMappingDirection GetHeader(HdrDpxFieldsDatumMappingDirection field) const;
		/** Gets the value of the orientation header field
			@param field			selects orientation field
			@return					orientation value */
		HdrDpxOrientation GetHeader(HdrDpxFieldsOrientation field) const;
		/** Gets the value of the time code header field
			@param field			selects time code field
			@return					time code value */
		SMPTETimeCode GetHeader(HdrDpxFieldsTimeCode field) const;
		/** Gets the value of the user bits header field
			@param field			selects user bits field
			@return					user bits value */
		SMPTEUserBits GetHeader(HdrDpxFieldsUserBits field) const;
		/** Gets the value of the interlace header field
			@param field			selects interlace field
			@return					interlace value */
		HdrDpxInterlace GetHeader(HdrDpxFieldsInterlace field) const;
		/** Gets the value of the video signal header field
			@param field			selects video signal field
			@return					video signal field value */
		HdrDpxVideoSignal GetHeader(HdrDpxFieldsVideoSignal field) const;
		/** Gets the value of the VIC header field
			@param field			selects VIC field
			@return					VIC value */
		HdrDpxVideoIdentificationCode GetHeader(HdrDpxFieldsVideoIdentificationCode field) const;
		/** Gets the value of the time code type header field
			@param field			selects time code type
			@return					time code type value */
		HdrDpxSMPTETCType GetHeader(HdrDpxFieldsSMPTETCType field) const;
		/** Gets the byte order of the file
			@param field			selects byte order
			@return					byte order of the file */
		HdrDpxByteOrder GetHeader(HdrDpxFieldsByteOrder field) const;
		/** Gets the value of the version header field
			@param field			selects version header field
			@return					version field value */
		HdrDpxVersion GetHeader(HdrDpxFieldsVersion field) const;
		/** Gets the user-defined data section
			@param[out] userid		returns the user identification string
			@param[out] userdata	returns the user data bytes
			@return					true if user data present, false if not */
		bool GetUserData(std::string &userid, std::vector<uint8_t> &userdata) const;
		/** Gets the standards-based metadata section. Note that a string of standards-based metadata can also be retrieved by using GetHeader(eSBMetadata).
			@param[out] sbm_descriptor	returns the standards-based metadata descriptor ("ST336", "Reg-XML", or "XMP")
			@param[out] sbmdata			returns the standards-based metadata bytes */
		bool GetStandardsBasedMetadata(std::string &sbm_descriptor, std::vector<uint8_t> &sbmdata) const;


		/** Sets a string header field
			@param field			which header field to set
			@param value			string value */
		void SetHeader(HdrDpxFieldsString field, const std::string &value);
		/** Sets a U32 header field
			@param field			which header field to set
			@param value			U32 value */
		void SetHeader(HdrDpxFieldsU32 field, uint32_t value);
		/** Sets a U16 header field
			@param field			which header field to set
			@param value			U16 value */
		void SetHeader(HdrDpxFieldsU16 field, uint16_t value);
		/** Sets a R32 header field
			@param field			which header field to set
			@param value			R32 value */
		void SetHeader(HdrDpxFieldsR32 field, float value);
		/** Sets a U8 header field
			@param field			which header field to set
			@param value			U8 value */
		void SetHeader(HdrDpxFieldsU8 field, uint8_t value);
		/** Sets the ditto key header field
			@param field			selects ditto key field
			@param value			ditto key value */
		void SetHeader(HdrDpxFieldsDittoKey field, HdrDpxDittoKey value);
		/** Sets the datum mapping direction header field
			@param field			selects datum mapping direction field
			@param value			datum mapping direction value */
		void SetHeader(HdrDpxFieldsDatumMappingDirection field, HdrDpxDatumMappingDirection value);
		/** Sets the orientation header field
			@param field			selects orientation field
			@param value			orientation value */
		void SetHeader(HdrDpxFieldsOrientation field, HdrDpxOrientation value);
		/** Sets the time code header field
			@param field			selects time code field
			@param value			time code value */
		void SetHeader(HdrDpxFieldsTimeCode field, SMPTETimeCode value);
		/** Sets the user bits header field
			@param field			selects user bits field
			@param value			user bits value */
		void SetHeader(HdrDpxFieldsUserBits field, SMPTEUserBits value);
		/** Sets the interlace header field
			@param field			selects interlace field
			@param value			interlace value */
		void SetHeader(HdrDpxFieldsInterlace field, HdrDpxInterlace value);
		/** Sets the video signal header field
			@param field			selects video signal field
			@param value			video signal value */
		void SetHeader(HdrDpxFieldsVideoSignal field, HdrDpxVideoSignal value);
		/** Sets the VIC header field
			@param field			selects VIC field
			@param value			VIC value */
		void SetHeader(HdrDpxFieldsVideoIdentificationCode field, HdrDpxVideoIdentificationCode value);
		/** Sets the timecode type header field
			@param field			selects timecode type field
			@param value			timecode type value */
		void SetHeader(HdrDpxFieldsSMPTETCType field, HdrDpxSMPTETCType value);
		/** Sets the byte order of the file for writing
			@param field			selects byte order
			@param value			byte order to write */
		void SetHeader(HdrDpxFieldsByteOrder field, HdrDpxByteOrder value);
		/** Sets the version field
			@param field			selects version field
			@param value			version field value */
		void SetHeader(HdrDpxFieldsVersion field, HdrDpxVersion value);
		/** Sets the data in the user-defined data section
			@param userid			user identification string
			@param userdata			user data bytes */
		void SetUserData(std::string userid, std::vector<uint8_t> userdata);
		/** Sets the data in the standards-based metadata section. Can also be set as a string using SetHeader(eSBMetadata, metadataString) and SetHeader(eSBMFormatDescriptor, descString)
			@param sbm_descriptor	standards-based metadata descriptor ("ST336", "Reg-XML", or "XMP")
			@param sbmdata			standards-based metadata bytes */
		void SetStandardsBasedMetadata(std::string sbm_descriptor, std::vector<uint8_t> sbmdata);


		/** Get a pointer to an image element data structure that corresponds to a specific IE index.
			@param ie_index				IE index
			@return						pointer to HdrDpxImageElement structure that can be used to access IE pixel and header data */
		HdrDpxImageElement *GetImageElement(uint8_t ie_index);
		/** Get a list of any warnings that have been generated. */
		std::list<std::string> GetWarningList();

	private:
		//friend class HdrDpxImageElement;
		/** Clear header or just IE data structure
			@param ie_index				IE index (if specified, only clears the corresponding IE header; if unspecified, clears the entire DPX header) */
		void ClearHeader(uint8_t ie_index = 0xff);

		HdrDpxImageElement m_IE[8];  ///< Array of image element objects
		void ByteSwapHeader(void);   ///< Byte swap the header fields
		void ByteSwapSbmHeader(void);   ///< Byte swap the standards-based metadata fields
		bool ByteSwapToMachine(void);   ///< Byte swap the header (if needed) to match machine endianness
		void ComputeOffsets();   ///< Compute offsets to data for writing file
		void FillCoreFields();   ///< Fill in any missing core fields

		void ReadUserData();    ///< Read the user data from the file
		void ReadSbmData();    ///< Read the standards-based metadata from the file

		std::list<std::string> m_warn_messages;  ///< list of warnings
		std::string m_file_name;      ///< File name
		bool m_file_is_hdr_version = false;   ///< Flag indicating file contains ST 268-2 version string
		bool m_machine_is_msbf = false;   ///< Flag indicating machine is big-endian
		bool m_sbm_present = false;   ///< Flag indicating standards-based metadata is present
		bool m_open_for_write = false;   ///< Flag indicating file is open for writing
		bool m_open_for_read = false;   ///< Flag indicating file is open for reading
		bool m_is_header_locked = false;   ///< Flag indicating header is locked
		std::fstream m_file_stream;    ///< File stream handle
		bool m_ud_dump = false;    ///< indicates whether to dump user data with header
		HdrDpxDumpFormat m_ud_dump_format = eDumpFormatDefault; ///< user data dump format
		bool m_sbm_dump = false;    ///< indicates whether to dump standards-based metadata with header
		HdrDpxDumpFormat m_sbm_dump_format = eDumpFormatDefault; ///< user data dump format

		HdrDpxByteOrder m_byteorder = eNativeByteOrder;  ///< Byte order of file
		HDRDPXFILEFORMAT m_dpx_header;   ///< DPX header
		HDRDPXSBMDATA m_dpx_sbmdata;    ///< Standards-based metadata structure
		HDRDPXUSERDATA m_dpx_userdata;   ///< User data header
		FileMap m_filemap;    ///< File map object

		ErrorObject m_err;   ///< Error tracking object
	};

	/** Copy a string to a char * buffer with null termination, without exceeding a maximum size
		@param[out] dest				Buffer to copy to
		@param[in] src				Source string
		@param[in] field_length		Maximum number of bytes to copy
		@return						true if string was clipped to fit into buffer */
	bool CopyStringN(char *dest, std::string src, unsigned int field_length);
	/** Copy a char * buffer with a specified max length to a string
		@param src					Buffer to copy from
		@param field_length			Maximum size of string
		@return						string containing copied characters */
	std::string CopyToStringN(const char * src, unsigned int field_length);

	
	/** Generic 32-bit byte swap
		@param[in/out] v			pointer to 32-bit word to byte swap */
	static inline void ByteSwap32(void *v)
	{
		uint8_t *u8;
		u8 = (uint8_t *)v;
		std::swap(u8[0], u8[3]);
		std::swap(u8[1], u8[2]);
	}

	/** Generic 16-bit byte swap
		@param[in/out] v			pointer to 16-bit word to byte swap */
	static inline void ByteSwap16(void *v)
	{
		uint8_t *u8;
		u8 = (uint8_t *)v;
		std::swap(u8[0], u8[1]);
	}

}
#endif

