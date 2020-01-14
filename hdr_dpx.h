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
#include <cstdint>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <utility>

/*** hdr_dpx.h ***/
#include "vdo.h"
#include "datum.h"
#include "fifo.h"

#ifndef DPX_H
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
#endif /* DPX_H */

#define NUM_IMAGE_ELEMENTS  8

#define FILE_NAME_SIZE	100
#define TIMEDATE_SIZE	24
#define CREATOR_SIZE	100
#define PROJECT_SIZE	200
#define COPYRIGHT_SIZE	200
#define DESCRIPTION_SIZE	32
#define INPUTNAME_SIZE	32
#define INPUTSN_SIZE	32


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

namespace Dpx {

	// No compile-time checks for length
	enum HdrDpxFieldsString
	{
		eFileName = 0, // FILE_NAME_SIZE
		eTimeDate, // TIMEDATE_SIZE
		eCreator, // CREATOR_SIZE
		eProject, // PROJECT_SIZE
		eCopyright, // COPYRIGHT_SIZE
		eSourceFileName, // FILE_NAME_SIZE
		eSourceTimeDate, // TIMEDATE_SIZE
		eInputName, // INPUTNAME_SIZE
		eInputSN, // INPUTSN_SIZE 
		eFilmMfgId, // 2
		eFilmType, // 2
		eOffsetPerfs, // 4
		ePrefix, // 6
		eCount, // 4
		eFormat, // 32
		eFrameId, // 32
		eSlateInfo // 100
	};

	enum HdrDpxFieldsU32 // No compile-time checks, but run-time checks are needed to validate
	{
		eImageOffset,
		eFileSize,
		eGenericSize,
		eIndustrySize,
		eUserSize,
		eEncryptKey,
		eStandardsBasedMetadataOffset,
		ePixelsPerLine,
		eLinesPerElement,
		eXOffset,
		eYOffset,
		eXOriginalSize,
		eYOriginalSize,
		eAspectRatioH,
		eAspectRatioV,
		eFramePosition,
		eSequenceLen,
		eHeldCount,
	};

	enum HdrDpxFieldsU16
	{
		eNumberElements,
		eBorderXL,
		eBorderXR,
		eBorderYT,
		eBorderYB,
	};

	enum HdrDpxFieldsR32
	{
		eXCenter,
		eYCenter,
		eFilmFrameRate,
		eShutterAngle,
		eHorzSampleRate,
		eVertSampleRate,
		eTvFrameRate,
		eTimeOffset,
		eGamma,
		eBlackLevel,
		eBlackGain,
		eBreakpoint,
		eWhiteLevel,
		eIntegrationTimes
	};

	enum HdrDpxIEFieldsU32
	{
		eOffsetToData,
		eEndOfLinePadding,
		eEndOfImagePadding
	};

	enum HdrDpxIEFieldsR32
	{
		eReferenceLowDataCode,
		eReferenceLowQuantity,
		eReferenceHighDataCode,
		eReferenceHighQuantity
	};

	enum HdrDpxFieldsU8
	{
		eFieldNumber
	};
	
	enum HdrDpxFieldsDittoKey
	{
		eDittoKey
	};

	enum HdrDpxFieldsDatumMappingDirection
	{
		eDatumMappingDirection
	};

	enum HdrDpxFieldsOrientation
	{
		eOrientation
	};

	enum HdrDpxFieldsDataSign
	{
		eDataSign,
	};

	enum HdrDpxFieldsDescriptor
	{
		eDescriptor
	};

	enum HdrDpxFieldsTransfer
	{
		eTransfer
	};

	enum HdrDpxFieldsColorimetric
	{
		eColorimetric
	};

	enum HdrDpxFieldsBitDepth
	{
		eBitDepth
	};
	
	enum HdrDpxFieldsPacking
	{
		ePacking
	};

	enum HdrDpxFieldsEncoding
	{
		eEncoding
	};

	enum HdrDpxFieldsInterlace
	{
		eInterlace
	};

	enum HdrDpxFieldsVideoSignal
	{
		eVideoSignal
	};

	enum HdrDpxFieldsVideoIdentificationCode
	{
		eVideoIdentificationCode
	};

	enum HdrDpxFieldsByteOrder
	{
		eByteOrder
	};

	/* enum HdrDpxFields
	{
		eMagic,
		eImageOffset,
		eVersion,
		eFileSize,
		eDittoKey,
		eGenericSize,
		eIndustrySize,
		eUserSize,
		eFileName,
		eTimeDate,
		eCreator,
		eProject,
		eCopyright,
		eEncryptKey,
		eStandardsBasedMetadataOffset,
		eDatumMappingDirection,
		eOrientation,
		eNumberElements,
		ePixelsPerLine,
		eLinesPerElement,
		eXOffset,
		eYOffset,
		eXCenter,
		eYCenter,
		eXOriginalSize,
		eYOriginalSize,
		eSourceFileName,
		eSourceTimeDate,
		eInputName,
		eInputSN,
		eBorderXL,
		eBorderXR,
		eBorderYT,
		eBorderYB,
		eAspectRatioH,
		eAspectRatioV,
		eFilmMfgId,
		eFilmType,
		eOffsetPerfs,
		ePrefix,
		eCount,
		eFormat,
		eFramePosition,
		eSequenceLene,
		eHeldCount,
		eFrameRate,
		eShutterAngle,
		eFrameId,
		eSlateInfo,
		eTimeCode,
		eUserBits,
		eInterlace,
		eFieldNumber,
		eVideoSignal,
		ePadding,
		eHorzSampleRate,
		eVertSampleRate,
		eFrameRate,
		eTimeOffset,
		eGamma,
		eBlackLevel,
		eBlackGain,
		eBreakpoint,
		eWhiteLevel,
		eIntegrationTimes,
		eVideoIdentificationCode,
		eSMPTETCType,
		eSMPTETCDBB2
	}; */

	enum HdrDpxFieldDtypes
	{
		eU32,
		eU16,
		eU8,
		eR32,
		eASCII
	};

	enum HdrDpxByteOrder
	{
		eNativeByteOrder,
		eMSBF,
		eLSBF
	};

	enum HdrDpxVersion
	{
		eDPX_1_0,
		eDPX_2_0,
		eDPX_2_0_HDR
	};

	enum HdrDpxDittoKey
	{
		eDittoKeySame = 0,
		eDittoKeyNew = 1,
		eDittoKeyUndefined = UINT32_MAX
	};

	enum HdrDpxDatumMappingDirection
	{
		eDatumMappingDirectionR2L = 0,
		eDatumMappingDirectionL2R = 1,
		eDatumMappingDirectionUndefined = UINT8_MAX
	};

	enum HdrDpxOrientation
	{
		eOrientationL2R_T2B = 0,
		eOrientationR2L_T2B = 1,
		eOrientationL2R_B2T = 2,
		eOrientationR2L_B2T = 3,
		eOrientationT2B_L2R = 4,
		eOrientationT2B_R2L = 5,
		eOrientationB2T_L2R = 6,
		eOrientationB2T_R2L = 7,
		eOrientationUndefined = UINT16_MAX
	};

	enum HdrDpxDataSign
	{
		eDataSignUnsigned = 0,
		eDataSignSigned = 1,
		eDataSignUndefined = UINT8_MAX
	};

	enum HdrDpxDescriptor
	{
		eDescUser = 0,
		eDescR = 1,
		eDescG = 2,
		eDescB = 3,
		eDescA = 4,
		eDescY = 6,
		eDescCbCr = 7,
		eDescZ = 8,
		eDescComposite = 9,
		eDescCb = 10,
		eDescCr = 11,
		eDescRGB_268_1 = 50,
		eDescRGBA_268_1 = 51,
		eDescABGR_268_1 = 52,
		eDescBGR = 53,
		eDescBGRA = 54,
		eDescARGB = 55,
		eDescRGB = 56,
		eDescRGBA = 57,
		eDescABGR = 58,
		eDescCbYCrY = 100,
		eDescCbYACrYA = 101,
		eDescCbYCr = 102,
		eDescCbYCrA = 103,
		eDescCYY = 104,
		eDescCYAYA = 105,
		eDescGeneric2 = 150,
		eDescGeneric3 = 151,
		eDescGeneric4 = 152,
		eDescGeneric5 = 153,
		eDescGeneric6 = 154,
		eDescGeneric7 = 155,
		eDescGeneric8 = 156,
		eDescUndefined = UINT8_MAX
	};

	enum HdrDpxTransfer
	{
		eTransferUser = 0,
		eTransferDensity = 1,
		eTransferLinear = 2,
		eTransferLogarithmic = 3,
		eTransferUnspecifiedVideo = 4,
		eTransferST_274 = 5,
		eTransferBT_709 = 6,
		eTransferBT_601_625 = 7,
		eTransferBT_601_525 = 8,
		eTransferCompositeNTSC = 9,
		eTransferCompositePAL = 10,
		eTransferZLinear = 11,
		eTransferZHomogenous = 12,
		eTransferST_2065_3 = 13,
		eTransferBT_2020_NCL = 14,
		eTransferBT_2020_CL = 15,
		eTransferIEC_61966_2_4 = 16,
		eTransferBT_2100_PQ_NCL = 17,
		eTransferBT_2100_PQ_CI = 18,
		eTransferBT_2100_HLG_NCL = 19,
		eTransferBT_2100_HLG_CI = 20,
		eTransferRP_231_2 = 21,
		eTransferIEC_61966_2_1 = 22,
		eTransferUndefined = UINT8_MAX
	};

	enum HdrDpxColorimetric {
		eColorimetricUser = 0,
		eColorimetricDensity = 1,
		eColorimetricNA_2 = 2,
		eColorimetricNA_3 = 3,
		eColorimetricUnspecifiedVideo = 4,
		eColorimetricST_274 = 5,
		eColorimetricBT_709 = 6,
		eColorimetricBT_601_625 = 7,
		eColorimetricBT_601_525 = 8,
		eColorimetricCompositeNTSC = 9,
		eColorimetricCompositePAL = 10,
		eColorimetricNA_11 = 11,
		eColorimetricNA_12 = 12,
		eColorimetricST2065_3 = 13,
		eColorimetricBT_2020 = 14,
		eColorimetricST_2113_P3D65 = 15,
		eColorimetricST_2113_P3DCI = 16,
		eColorimetricST_2113_P3D60 = 17,
		eColorimetricST_2065_1_ACES = 18,
		eColorimetricUndefined = UINT8_MAX
	};

	enum HdrDpxBitDepth {
		eBitDepth1 = 1,
		eBitDepth8 = 8,
		eBitDepth10 = 10,
		eBitDepth12 = 12,
		eBitDepth16 = 16,
		eBitDepth32 = 32,
		eBitDepth64 = 64,
		eBitDepthUndefined = UINT8_MAX
	};

	enum HdrDpxPacking {
		ePackingPacking = 0,
		ePackingFilledMethodA = 1,
		ePackingFilledMethodB = 2,
		ePackingUndefined = UINT16_MAX
	};

	enum HdrDpxEncoding {
		eEncodingNoEncoding = 0,
		eEncodingRLE = 1,
		eEncodingUndefined = UINT16_MAX
	};

	enum HdrDpxInterlace {
		eInterlaceNone = 0,
		eInterlace2_1 = 1,
		eInterlaceUndefined = UINT8_MAX
	};

	enum HdrDpxVideoSignal {
		eVideoSignalUndefined0 = 0,
		eVideoSignalNTSC = 1,
		eVideoSignalPAL = 2,
		eVideoSignalPAL_M = 3,
		eVideoSignalSECAM = 4,
		eVideoSignalBT_601_525_4x3 = 50,
		eVideoSignalBT_601_625_4x3 = 51,
		eVideoSignalBT_601_525_16x9 = 100,
		eVideoSignalBT_601_625_16x9 = 101,
		eVideoSignalYCbCr_int_1050ln_16x9 = 150,
		eVideoSignalYCbCr_int_1125ln_16x9_ST_274 = 151,
		eVideoSignalYCbCr_int_1250ln_16x9 = 152,
		eVideoSignalYCbCr_int_1125ln_16x9_ST_240 = 153,
		eVideoSignalYCbCr_prog_525ln_16x9 = 200,
		eVideoSignalYCbCr_prog_625ln_16x9 = 201,
		eVideoSignalYCbCr_prog_750ln_16x9_ST_296 = 202,
		eVideoSignalYCbCr_prog_1125ln_16x9_ST_274 = 203,
		eVideoSignalCTA_VIC = 250,
		eVideoSignalUndefined = UINT8_MAX
	};

	enum HdrDpxVideoIdentificationCode {
		eVIC_DMT0659 = 1,
		eVIC_480p = 2,
		eVIC_480pH = 3,
		eVIC_720p = 4,
		eVIC_1080i = 5,
		eVIC_480i = 6,
		eVIC_480iH = 7,
		eVIC_240p = 8,
		eVIC_240pH = 9,
		eVIC_480i4x = 10,
		eVIC_480i4xH = 11,
		eVIC_240p4x = 12,
		eVIC_240p4xH = 13,
		eVIC_480p2x = 14,
		eVIC_480p2xH = 15,
		eVIC_1080p = 16,
		eVIC_576p = 17,
		eVIC_576pH = 18,
		eVIC_720p50 = 19,
		eVIC_1080i25_20 = 20,
		eVIC_576i = 21,
		eVIC_576iH = 22,
		eVIC_288p = 23,
		eVIC_288pH = 24,
		eVIC_576i4x = 25,
		eVIC_576i4xH = 26,
		eVIC_288p4x = 27,
		eVIC_288p4xH = 28,
		eVIC_576p2x = 29,
		eVIC_576p2xH = 30,
		eVIC_1080p50 = 31,
		eVIC_1080p24 = 32,
		eVIC_1080p25 = 33,
		eVIC_1080p30 = 34,
		eVIC_480p4x = 35,
		eVIC_480p4xH = 36,
		eVIC_576p4x = 37,
		eVIC_576p4xH = 38,
		eVIC_1080i25_39 = 39,
		eVIC_1080i50 = 40,
		eVIC_720p100 = 41,
		eVIC_576p100 = 42,
		eVIC_576p100H = 43,
		eVIC_576i50 = 44,
		eVIC_576i50H = 45,
		eVIC_1080i60 = 46,
		eVIC_720p120 = 47,
		eVIC_480p119 = 48,
		eVIC_480p119H = 49,
		eVIC_480i59 = 50,
		eVIC_480i59H = 51,
		eVIC_576p200 = 52,
		eVIC_576p200H = 53,
		eVIC_576i100 = 54,
		eVIC_576i100H = 55,
		eVIC_480p239 = 56,
		eVIC_480p239H = 57,
		eVIC_480i119 = 58,
		eVIC_480i119H = 59,
		eVIC_720p24 = 60,
		eVIC_720p25 = 61,
		eVIC_720p30 = 62,
		eVIC_1080p120 = 63,
		eVIC_1080p100 = 64,
		eVIC_720p24W = 65,
		eVIC_720p25W = 66,
		eVIC_720p30W = 67,
		eVIC_720p50W = 68,
		eVIC_720pW = 69,
		eVIC_720p100W = 70,
		eVIC_720p120W = 71,
		eVIC_1080p24W = 72,
		eVIC_1080p25W = 73,
		eVIC_1080p30W = 74,
		eVIC_1080p50W = 75,
		eVIC_1080pW = 76,
		eVIC_1080p100W = 77,
		eVIC_1080p120W = 78,
		eVIC_720p2x24 = 79,
		eVIC_720p2x25 = 80,
		eVIC_720p2x30 = 81,
		eVIC_720p2x50 = 82,
		eVIC_720p2x = 83,
		eVIC_720p2x100 = 84,
		eVIC_720p2x120 = 85,
		eVIC_1080p2x24 = 86,
		eVIC_1080p2x25 = 87,
		eVIC_1080p2x30 = 88,
		eVIC_1080p2x50 = 89,
		eVIC_1080p2x = 90,
		eVIC_1080p2x100 = 91,
		eVIC_1080p2x120 = 92,
		eVIC_2160p24 = 93,
		eVIC_2160p25 = 94,
		eVIC_2160p30 = 95,
		eVIC_2160p50 = 96,
		eVIC_2160p60 = 97,
		eVIC_4096x2160p24 = 98,
		eVIC_4096x2160p25 = 99,
		eVIC_4096x2160p30 = 100,
		eVIC_4096x2160p50 = 101,
		eVIC_4096x2160p = 102,
		eVIC_2160p24W = 103,
		eVIC_2160p25W = 104,
		eVIC_2160p30W = 105,
		eVIC_2160p50W = 106,
		eVIC_2160pW = 107,
		eVIC_720p48 = 108,
		eVIC_720p48W = 109,
		eVIC_720p2x48 = 110,
		eVIC_1080p48 = 111,
		eVIC_1080p48W = 112,
		eVIC_1080p2x48 = 113,
		eVIC_2160p48 = 114,
		eVIC_4096x2160p48 = 115,
		eVIC_2160p48W = 116,
		eVIC_2160p100 = 117,
		eVIC_2160p120 = 118,
		eVIC_2160p100W = 119,
		eVIC_2160p120W = 120,
		eVIC_2160p2x24 = 121,
		eVIC_2160p2x25 = 122,
		eVIC_2160p2x30 = 123,
		eVIC_2160p2x48 = 124,
		eVIC_2160p2x50 = 125,
		eVIC_2160p2x = 126,
		eVIC_2160p2x100 = 127,
		eVIC_2160p2x120 = 193,
		eVIC_4320p24 = 194,
		eVIC_4320p25 = 195,
		eVIC_4320p30 = 196,
		eVIC_4320p48 = 197,
		eVIC_4320p50 = 198,
		eVIC_4320p = 199,
		eVIC_4320p100 = 200,
		eVIC_4320p120 = 201,
		eVIC_4320p24W = 202,
		eVIC_4320p25W = 203,
		eVIC_4320p30W = 204,
		eVIC_4320p48W = 205,
		eVIC_4320p50W = 206,
		eVIC_4320pW = 207,
		eVIC_4320p100W = 208,
		eVIC_4320p120W = 209,
		eVIC_4320p2x24 = 210,
		eVIC_4320p2x25 = 211,
		eVIC_4320p2x30 = 212,
		eVIC_4320p2x48 = 213,
		eVIC_4320p2x50 = 214,
		eVIC_4320p2x = 215,
		eVIC_4320p2x100 = 216,
		eVIC_4320p2x120 = 217,
		eVIC_4096x2160p100 = 218,
		eVIC_4096x2160p120 = 219,
		eVIC_undefined = UINT8_MAX
	};

	enum ErrorSeverity
	{
		eInformational,
		eWarning,
		eFatal
	};

	enum ErrorCode
	{
		eNoError = 0,
		eValidationError,
		eMissingCoreField,
		eUnexpectedNonzeroBit,
		eFileOpenError,
		eFileReadError,
		eFileWriteError,
		eBadParameter
	};

	class ErrorObject
	{
	public:
		explicit operator bool() const {  // Makes return object testable
			return (m_code.size() != 0);
		}
		ErrorObject operator + (ErrorObject const &obj)   // Combine the errors from 2 objects
		{
			ErrorObject err;
			Dpx::ErrorCode code;
			Dpx::ErrorSeverity severity;
			std::string message;
			int i;

			for (i=0; i<GetNumErrors(); ++i)
			{
				GetError(i, code, severity, message);
				err.LogError(code, severity, message);
			}
			for (i = 0; i < obj.GetNumErrors(); ++i)
			{
				obj.GetError(i, code, severity, message);
				err.LogError(code, severity, message);
			}
			return err;
		}
		ErrorObject& operator += (ErrorObject const &obj)
		{
			Dpx::ErrorCode code;
			Dpx::ErrorSeverity severity;
			std::string message;
			int i;

			for (i = 0; i < obj.GetNumErrors(); ++i)
			{
				obj.GetError(i, code, severity, message);
				this->LogError(code, severity, message);
			}
			return *this;
		}
		void LogError(Dpx::ErrorCode errorcode, Dpx::ErrorSeverity severity, std::string errmsg)
		{
			m_code.push_back(errorcode);
			m_severity.push_back(severity);
			m_message.push_back(errmsg);
			if (severity > m_worst_severity)
				m_worst_severity = severity;
		}
		void GetError(int index, Dpx::ErrorCode &errcode, Dpx::ErrorSeverity &severity, std::string &errmsg) const
		{
			if (index >= m_code.size())
			{
				errcode = eNoError;
				errmsg = "";
				return;
			}
			errcode = m_code[index];
			severity = m_severity[index];
			errmsg = m_message[index];
		}
		int GetNumErrors() const
		{
			return static_cast<int>(m_severity.size());
		}
		Dpx::ErrorSeverity GetWorstSeverity() const
		{
			return m_worst_severity;
		}
	private:
		std::vector<Dpx::ErrorSeverity> m_severity;
		std::vector<Dpx::ErrorCode> m_code;
		std::vector<std::string> m_message;
		Dpx::ErrorSeverity m_worst_severity = Dpx::eInformational;
	};
}



struct SMPTETimeCode
{
	// Each of these fields are actually 4-bits (nibbles)
	unsigned int h_tens : 4;
	unsigned int h_units : 4;
	unsigned int m_tens : 4;
	unsigned int m_units : 4;
	unsigned int s_tens : 4;
	unsigned int s_units : 4;
	unsigned int F_tens : 4;
	unsigned int F_units : 4;
};

struct SMPTEUserBits
{
	unsigned int UB7 : 4;  // UB7 => 31:28, UB6 =>27:24, .. UB0 => 3:0
	unsigned int UB6 : 4;
	unsigned int UB5 : 4;
	unsigned int UB4 : 4;
	unsigned int UB3 : 4;
	unsigned int UB2 : 4;
	unsigned int UB1 : 4;
	unsigned int UB0 : 4;
};

/*
struct sHrdDpxDataTypes {
	HdrDpxFields f;
	HdrDpxFieldDtypes dt;
	int sl;   // length for ascii
};


sHdrDpxDataTypes hdr_dpx_fields[] =
{
	{ Dpx::eMagic, Dpx::eU32, 0 },
	{ Dpx::eImageOffset, Dpx::eU32, 0 },
	{ Dpx::eVersion, Dpx::eASCII, 8 },
	{ Dpx::eFileSize, Dpx::eU32, 0 },
	{ Dpx::eDittoKey, Dpx::eU32, 0 },
	{ Dpx::eGenericSize, Dpx::eU32, 0 },
	{ Dpx::eIndustrySize, Dpx::eU32, 0 },
	{ Dpx::eUserSize, Dpx::eU32, 0 },
	{ Dpx::eFileName, Dpx::eASCII, FILE_NAME_SIZE },
	{ Dpx::eTimeDate, Dpx::eASCII, TIMEDATE_SIZE },
	{ Dpx::eCreator, Dpx::eASCII, CREATOR_SIZE },
	{ Dpx::eProject, Dpx::eASCII, PROJECT_SIZE },
	{ Dpx::eCopyright, Dpx::eASCII, COPYRIGHT_SIZE },
	{ Dpx::eEncryptKey, Dpx::eU32, 0 },
	{ Dpx::eStandardsBasedMetadataOffset, Dpx::eU32, 0 },
	{ Dpx::eDatumMappingDirection, Dpx::eU8, 0 },

	{ Dpx::eOrientation, Dpx::eU16, 0 },
	{ Dpx::eNumberElements, Dpx::eU16, 0 },
	{ Dpx::ePixelsPerLine, Dpx::eU32, 0 },
	{ Dpx::eLinesPerElement, Dpx::eU32, 0 },
	{ Dpx::eXOffset, Dpx::eU32, 0 },
	{ Dpx::eYOffset, Dpx::eU32, 0 },
	{ Dpx::eXCenter, Dpx::eR32, 0 },
	{ Dpx::eYCenter, Dpx::eR32, 0 },
	{ Dpx::eXOriginalSize, Dpx::eU32, 0 },
	{ Dpx::eYOriginalSize, Dpx::eU32, 0 },
	{ Dpx::eSourceFileName, Dpx::eASCII, FILE_NAME_SIZE },
	{ Dpx::eSourceTimeDate, Dpx::eASCII, TIMEDATE_SIZE },
	{ Dpx::eInputName, Dpx::eASCII, INPUTNAME_SIZE },
	{ Dpx::eInputSN, Dpx::eASCII, INPUTSN_SIZE },
	{ Dpx::eBorderXL, Dpx::eU16, 0 },
	{ Dpx::eBorderXR, Dpx::eU16, 0 },
	{ Dpx::eBorderYT, Dpx::eU16, 0 },
	{ Dpx::eBorderYB, Dpx::eU16, 0 },
	{ Dpx::eAspectRatioH, Dpx::eU32, 0 },
	{ Dpx::eAspectRatioV, Dpx::eU32, 0 },

	{ Dpx::eFilmMfgId, Dpx::eASCII, 2 },
	{ Dpx::eFilmType, Dpx::eASCII, 2 },
	{ Dpx::eOffsetPerfs, Dpx::eASCII, 4 },
	{ Dpx::ePrefix, Dpx::eASCII, 6 },
	{ Dpx::eCount, Dpx::eASCII, 4 },
	{ Dpx::eFormat, Dpx::eASCII, 32 },
	{ Dpx::eFramePosition, Dpx::eU32, 0 },
	{ Dpx::eSequenceLene, Dpx::eU32, 0 },
	{ Dpx::eHeldCount, Dpx::eU32, 0 },
	{ Dpx::eFrameRate, Dpx::eR32, 0 },
	{ Dpx::eShutterAngle, Dpx::eR32, 0 },
	{ Dpx::eFrameId, Dpx::eASCII, 32 },
	{ Dpx::eSlateInfo, Dpx::eASCII, 100 },
	
	{ Dpx::eTimeCode, Dpx:eU32, 0 },
	{ Dpx::eUserBits, Dpx:eU32, 0 },
	{ Dpx::eInterlace, Dpx::eU8, 0 },
	{ Dpx::eFieldNumber, Dpx::eU8, 0 },
	{ Dpx::eVideoSignal, Dpx::eU8, 0 },
	{ Dpx::ePadding, Dpx::eU8, 0 },
	{ Dpx::eHorzSampleRate, Dpx::eR32, 0 },
	{ Dpx::eVertSampleRate, Dpx::eR32, 0 },
	{ Dpx::eFrameRate, Dpx::eR32, 0 },
	{ Dpx::eTimeOffset, Dpx::eR32, 0 },
	{ Dpx::eGamma, Dpx::eR32, 0 },
	{ Dpx::eBlackLevel, Dpx::eR32, 0 },
	{ Dpx::eBlackGain, Dpx::eR32, 0 },
	{ Dpx::eBreakpoint, Dpx::eR32, 0 },
	{ Dpx::eWhiteLevel, Dpx::eR32, 0 },
	{ Dpx::eIntegrationTimes, Dpx::eR32, 0 },
	{ Dpx::eVideoIdentificationCode, Dpx::eU8, 0 },
	{ Dpx::eSMPTETCType, Dpx::eU8, 0 },
	{ Dpx::eSMPTETCDBB2, Dpx::eU8, 0 }

}; */


struct HdrDpxIEFields
{
	Dpx::HdrDpxOrientation m_orientation = Dpx::eOrientationUndefined;
};

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
	BYTE  Reserved16_5[96];   /* Reserved field 16.5 */
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

//typedef struct _HdrDpxImageData
//{
//	int n_elements;
//	datum_image_element_t  ie[8];
//} HDRDPXIMAGEDATA;

#define UNDEFINED_U32	UINT32_MAX
#define UNDEFINED_U16   UINT16_MAX
#define UNDEFINED_U8    UINT8_MAX


extern format_t determine_field_format(char* file_name);
extern int      ends_in_percentd(char* str, int length);

////int      hdr_dpx_read(char *fname, pic_t **p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm);
////int      hdr_dpx_write(char *fname, pic_t *p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm, int write_as_be, HDRDPXFILEFORMAT *dpxh_written);
//int      hdr_dpx_header_init(HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm);

//int      hdr_dpx_read(char *fname, HDRDPXIMAGEDATA **p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm);
//int      hdr_dpx_write(char *fname, HDRDPXIMAGEDATA *p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm, int write_as_be, HDRDPXFILEFORMAT *dpxh_written);
//int		 hdr_dpx_determine_file_type(char *fname);  // returns: -1 = error, 1 = V1.0, 2 = V2.0, 3 = V2.0HDR

void print_hdrdpx_header(FILE *fp, char *fname, HDRDPXFILEFORMAT *dpx_file, HDRDPXUSERDATA *dpx_u, HDRDPXSBMDATA *dpx_sbm);
inline void hdr_dpx_copy_string_n(char *dest, char *src, int n);

//// Example calling sequence for HDR DPX read:
//
//   char *fname = "mydpxfile.dpx";
//   HDRDPXFILEFORMAT dpx_header;
//   HDRDPXUSERDATA dpx_user_data;
//   HDRDPXSBMDATA dpx_sbmdata;
//   HDRDPXIMAGEDATA *output_image;
//   int error_code;
//
//   if(hdr_dpx_determine_file_type(fname) == 3)
//   {
//     error_code = hdr_dpx_read(fname, &output_image, &dpx_header, &dpx_user_data, &dpx_sbmdata);
//     if(error_code)
//       error_handler(error_code);
//   } else {
//     // Code to handle V1.0/V2.0 DPX files
//   }
//
//// Example calling sequence for HDR DPX write (single image element case):
//
//   char *fname = "mydpxfile.dpx";
//   HDRDPXFILEFORMAT dpx_header;
//   HDRDPXUSERDATA dpx_user_data;
//   HDRDPXSBMDATA dpx_sbmdata;
//   HDRDPXIMAGEDATA dpx_imgdata;
//   datum_image_element_t *image_element;
//   datum_label_t labels;
//   int error_code;
//   int i, j;
//   const int width = 1920, height = 1080;
//
//   // Create a picture
//   label[0] = DATUM_R;  label[1] = DATUM_G;  label[2] = DATUM_B;
//   image_element = datum_image_element_create(3, label, width, height, DATUM_UINT32, 8);
//   for(i = 0; i < height; i++)
//   {
//     for(j = 0; j < width; j++)
//     {
//       datum_image_set_pixel_ui(image_element, DATUM_R, j, i, 50);  // Set all pixels to R,G,B = (50, 60, 70)
//       datum_image_set_pixel_ui(image_element, DATUM_G, j, i, 60);
//       datum_image_set_pixel_ui(image_element, DATUM_B, j, i, 70);
//    }
//  }
//
//  // Set header values as desired
//  dpx_imgdata.n_elements = 1;
//  dpx_imgdata.ie[0] = image_element;
//  error_code = hdr_dpx_header_init(&dpx_header, &dpx_user_data, &dpx_sbmdata);
//  if(error_code)
//    error_handler(error_code);
//  // Set fields as desired, for example:
//  hdr_dpx_copy_string_n(dpx_header.Copyright, "(C) 2019 XYZ Productions", COPYRIGHT_SIZE);
//  
//  // Write the file
//  error_code = hdr_dpx_write(fname, &dpx_imgdata, &dpx_header, &dpx_user_data, &dpx_sbmdata,  
//  if(error_code)
//    error_handler(error_code);
//
///////////////////////////////////////////////////////////////////////////////////////////////


class HdrDpxImageElement
{
public:
	~HdrDpxImageElement();

	// Writing functions:
	HdrDpxImageElement(uint32_t width, uint32_t height, Dpx::HdrDpxDescriptor desc, int8_t bpc);
	void AddPlane(DatumLabel dtype);
	void SetPixelI(int x, int y, DatumLabel dtype, int32_t d);
	void SetPixelLf(int x, int y, DatumLabel dtype, double d);

	void SetHeader(Dpx::HdrDpxIEFieldsU32 f, uint32_t d);
	void SetHeader(Dpx::HdrDpxIEFieldsR32 f, float d);
	void SetHeader(Dpx::HdrDpxFieldsDataSign f, Dpx::HdrDpxDataSign d);
	void SetHeader(Dpx::HdrDpxFieldsDescriptor f, Dpx::HdrDpxDescriptor d);
	void SetHeader(Dpx::HdrDpxFieldsTransfer f, Dpx::HdrDpxTransfer d);
	void SetHeader(Dpx::HdrDpxFieldsColorimetric f, Dpx::HdrDpxColorimetric d);
	void SetHeader(Dpx::HdrDpxFieldsBitDepth f, Dpx::HdrDpxBitDepth d);
	void SetHeader(Dpx::HdrDpxFieldsPacking f, Dpx::HdrDpxPacking d);
	void SetHeader(Dpx::HdrDpxFieldsEncoding f, Dpx::HdrDpxEncoding d);

	// Reading functions:
	HdrDpxImageElement(void);
	uint32_t GetWidth(void);
	uint32_t GetHeight(void);

	int32_t GetPixelI(int x, int y, DatumLabel dtype);
	double GetPixelLf(int x, int y, DatumLabel dtype);
	
	uint32_t GetHeader(Dpx::HdrDpxIEFieldsU32 f);
	float GetHeader(Dpx::HdrDpxIEFieldsR32 f);
	Dpx::HdrDpxDataSign GetHeader(Dpx::HdrDpxFieldsDataSign f);
	Dpx::HdrDpxDescriptor GetHeader(Dpx::HdrDpxFieldsDescriptor f);
	Dpx::HdrDpxTransfer GetHeader(Dpx::HdrDpxFieldsTransfer f);
	Dpx::HdrDpxColorimetric GetHeader(Dpx::HdrDpxFieldsColorimetric f);
	Dpx::HdrDpxBitDepth GetHeader(Dpx::HdrDpxFieldsBitDepth f);
	Dpx::HdrDpxPacking GetHeader(Dpx::HdrDpxFieldsPacking f);
	Dpx::HdrDpxEncoding GetHeader(Dpx::HdrDpxFieldsEncoding f);

	std::vector<DatumLabel> GetDatumLabels(void);
	std::string DatumLabelToName(DatumLabel dl);
	std::vector<std::string> GetChannelList();
	std::list<std::string> GetErrorList();
	std::list<std::string> GetWarningList();

	bool m_isinitialized = false;

private:
	friend class HdrDpxFile;
	///////////////////////////////////////// called from HdrDpxFile class:
	HdrDpxImageElement(HDRDPXFILEFORMAT header, int ie);
	Dpx::ErrorObject ReadImageData(std::ifstream &infile, bool direction_r2l, bool bswap);
	Dpx::ErrorObject WriteImageData(std::ofstream &outfile, bool direction_r2l, bool bswap);
	void WritePixel(Fifo *fifo, uint32_t xpos, uint32_t ypos, std::ofstream &ofile);
	void WriteDatum(Fifo *fifo, int32_t datum, std::ofstream &ofile);
	void WriteFlush(Fifo *fifo, std::ofstream &ofile);
	void WriteLineEnd(Fifo *fifo, std::ofstream &ofile);
	bool IsNextSame(uint32_t xpos, uint32_t ypos);

	void ResetWarnings(void);
	HDRDPX_IMAGEELEMENT m_dpx_imageelement;  // Header data structure

	void CompileDatumPlanes(void);

	uint32_t ComputeWidth(uint32_t w);
	uint32_t ComputeHeight(uint32_t h);
	int GetNumberOfComponents(void);
	uint32_t m_width;
	uint32_t m_height;
	int m_bpc;
	Dpx::HdrDpxDescriptor m_descriptor;
	DatumDType m_datum_type;
	std::vector<std::shared_ptr<DatumPlane>> m_planes;
	bool m_h_subsampled;
	bool m_v_subsampled;
	std::shared_ptr<Fifo> m_fifoptr;
	bool m_byte_swap;
	bool m_direction_r2l;

	std::list<std::string> m_warnings;
	bool m_warn_unexpected_nonzero_data_bits;
	uint32_t m_warn_image_data_word_mask;
	bool m_warn_rle_same_past_eol;
	bool m_warn_rle_diff_past_eol;
	bool m_warn_zero_run_length;
};

class HdrDpxFile
{
public:
	HdrDpxFile(std::string filename);
	HdrDpxFile();
	~HdrDpxFile();
	Dpx::ErrorObject Open(std::string filename);
	Dpx::ErrorObject WriteFile(std::string filename);
	std::string DumpHeader();
	Dpx::ErrorObject Validate();
	bool IsHdr();
	std::shared_ptr<HdrDpxImageElement> CreateImageElement(uint32_t width, uint32_t height, Dpx::HdrDpxDescriptor desc, int8_t bpc);

	// overloaded functions for header field access
	// TODO: rename variables
	void SetHeader(Dpx::HdrDpxFieldsString f, std::string s);
	std::string GetHeader(Dpx::HdrDpxFieldsString f);
	void SetHeader(Dpx::HdrDpxFieldsU32 f, uint32_t d);
	uint32_t GetHeader(Dpx::HdrDpxFieldsU32 f);
	void SetHeader(Dpx::HdrDpxFieldsU16 f, uint16_t d);
	uint16_t GetHeader(Dpx::HdrDpxFieldsU16 f);
	void SetHeader(Dpx::HdrDpxFieldsR32 f, float d);
	float GetHeader(Dpx::HdrDpxFieldsR32 f);
	void SetHeader(Dpx::HdrDpxFieldsU8 f, uint8_t d);
	uint8_t GetHeader(Dpx::HdrDpxFieldsU8 f);
	void SetHeader(Dpx::HdrDpxFieldsDittoKey f, Dpx::HdrDpxDittoKey d);
	Dpx::HdrDpxDittoKey GetHeader(Dpx::HdrDpxFieldsDittoKey f);
	void SetHeader(Dpx::HdrDpxFieldsDatumMappingDirection f, Dpx::HdrDpxDatumMappingDirection d);
	Dpx::HdrDpxDatumMappingDirection GetHeader(Dpx::HdrDpxFieldsDatumMappingDirection f);
	void SetHeader(Dpx::HdrDpxFieldsOrientation f, Dpx::HdrDpxOrientation d);
	Dpx::HdrDpxOrientation GetHeader(Dpx::HdrDpxFieldsOrientation f);
	void SetHeader(Dpx::HdrDpxFieldsInterlace f, Dpx::HdrDpxInterlace d);
	Dpx::HdrDpxInterlace GetHeader(Dpx::HdrDpxFieldsInterlace f);
	void SetHeader(Dpx::HdrDpxFieldsVideoSignal f, Dpx::HdrDpxVideoSignal d);
	Dpx::HdrDpxVideoSignal GetHeader(Dpx::HdrDpxFieldsVideoSignal f);
	void SetHeader(Dpx::HdrDpxFieldsVideoIdentificationCode f, Dpx::HdrDpxVideoIdentificationCode d);
	Dpx::HdrDpxVideoIdentificationCode GetHeader(Dpx::HdrDpxFieldsVideoIdentificationCode f);
	void SetHeader(Dpx::HdrDpxFieldsByteOrder f, Dpx::HdrDpxByteOrder d);
	Dpx::HdrDpxByteOrder GetHeader(Dpx::HdrDpxFieldsByteOrder f);

	std::vector<std::shared_ptr<HdrDpxImageElement>> m_IE;
	std::list<std::string> GetWarningList();

private:
	bool CopyStringN(char *dest, std::string src, int l);
	std::string CopyToStringN(char * src, int l);
	void ByteSwapHeader(void);
	void ByteSwapSbmHeader(void);
	bool ByteSwapToMachine(void);
	Dpx::ErrorObject ComputeOffsets();
	Dpx::ErrorObject FillCoreFields();
	Dpx::ErrorObject CheckDataCollisions(uint32_t *ie_data_block_ends);

	std::list<std::string> m_warn_messages;
	std::string m_file_name;
	SMPTETimeCode m_smptetimecode;
	SMPTEUserBits m_smpteuserbits;
	bool m_file_is_hdr_version = false;
	bool m_machine_is_mbsf = false;
	bool m_sbm_present = false;

	Dpx::HdrDpxByteOrder m_byteorder = Dpx::eNativeByteOrder;
	Dpx::HdrDpxVersion m_version = Dpx::eDPX_2_0_HDR;
	Dpx::HdrDpxDittoKey m_dittokey = Dpx::eDittoKeyUndefined;
	Dpx::HdrDpxDatumMappingDirection m_datummappingdirection = Dpx::eDatumMappingDirectionUndefined;
	Dpx::HdrDpxInterlace m_interlaced = Dpx::eInterlaceUndefined;
	Dpx::HdrDpxVideoSignal m_videosignal = Dpx::eVideoSignalUndefined;
	Dpx::HdrDpxVideoIdentificationCode m_videoidentificationcode = Dpx::eVIC_undefined;
	HDRDPXFILEFORMAT m_dpx_header;
	HDRDPXSBMDATA m_dpx_sbmdata;
	HDRDPXUSERDATA m_dpx_userdata;
};


// Generic byte swap functions
static inline void ByteSwap32(void *v)
{
	uint8_t *u8;
	u8 = (uint8_t *)v;
	std::swap(u8[0], u8[3]);
	std::swap(u8[1], u8[2]);
}

static inline void ByteSwap16(void *v)
{
	uint8_t *u8;
	u8 = (uint8_t *)v;
	std::swap(u8[0], u8[1]);
}


#endif

