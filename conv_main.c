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

/*! \file conv_main.c
 *    Main loop
 *  \author Frederick Walls (frederick.walls@broadcom.com) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "vdo.h"
#include "dpx.h"
#include "utl.h"
#include "cmd_parse.h"
#include "logging.h"
#include "hdr_dpx.h"

#define PATH_MAX 1024
#define MAX_OPTNAME_LEN 200
#define CFGLINE_LEN ((PATH_MAX) + MAX_OPTNAME_LEN)
#define RANGE_CHECK(s,a,b,c) { if(((a)<(b))||((a)>(c))) { UErr("%s out of range, needs to be between %d and %d\n",s,b,c); } }
#define SET_FLOAT(d,s)  if((s)==FLT_MAX) { memset(&(d), 0xff, 4); } else { d = s; } 

static int assign_line (char* line, cmdarg_t *cmdargs);


static int      help = 0;
static char fn_i[PATH_MAX+1] = "";
static char fn_o[PATH_MAX+1] = "";
static char fn_log[PATH_MAX+1] = "";
static char filepath[PATH_MAX+1] = "";
static char option[PATH_MAX+1]   = "";
static int use422;
static int use420;
static int useYuvInput;
static int bitsPerComponent;
static int function;
static int rbSwap;
static int rbSwapOut;
static int cbcrSwap;
static int cbcrSwapOut;
static int picWidth;
static int picHeight;
static int dpxRForceBe;
static int dpxRDatumOrder;
static int dpxRPadEnds;
static int dpxWPadEnds;
static int dpxWDatumOrder;
static int dpxWForcePacking;
static int dpxWByteSwap;
static int hdrdpxWriteAsBe;
static int ppmFileOutput;
static int dpxFileOutput;
static int hdrDpxFileOutput;
static int yuvFileOutput;
static int yuvFileFormat;
static int sbmFileOutput;
static int udFileOutput;
static int logHeaderInput;
static int logHeaderOutput;
static int createAlpha;
static int deleteAlpha;
static char sbmExtension[PATH_MAX + 1] = "klv";

// DPX header fields
static unsigned int imageOffset = UINT32_MAX;
static unsigned int dittoKey = UINT32_MAX;
static char fileName[PATH_MAX + 1] = "";
static char timeDate[PATH_MAX + 1] = "";
static char creator[PATH_MAX + 1] = "";
static char project[PATH_MAX + 1] = "";
static char copyright[PATH_MAX + 1] = "";
static unsigned int encryptKey = UINT32_MAX;
static unsigned int sbmOffset = UINT32_MAX;
static int datumMappingDirection = UINT8_MAX;
static int orientation = UINT16_MAX;
static int numberElements = UINT16_MAX;
static unsigned int pixelsPerLine = UINT32_MAX;
static unsigned int linesPerElement = UINT32_MAX;
static int chromaSubsampling[NUM_IMAGE_ELEMENTS];

static unsigned int dataSign[NUM_IMAGE_ELEMENTS];
static float lowData[NUM_IMAGE_ELEMENTS];
static float lowQuantity[NUM_IMAGE_ELEMENTS];
static float highData[NUM_IMAGE_ELEMENTS];
static float highQuantity[NUM_IMAGE_ELEMENTS];
static int descriptor[NUM_IMAGE_ELEMENTS];
static int transfer[NUM_IMAGE_ELEMENTS];
static int colorimetric[NUM_IMAGE_ELEMENTS];
static int bitSize[NUM_IMAGE_ELEMENTS];
static int packing[NUM_IMAGE_ELEMENTS];
static int encoding[NUM_IMAGE_ELEMENTS];
static int endOfLinePadding[NUM_IMAGE_ELEMENTS];
static int endOfImagePadding[NUM_IMAGE_ELEMENTS];
static char description[NUM_IMAGE_ELEMENTS][DESCRIPTION_SIZE];

static unsigned int xOffset = UINT32_MAX;
static unsigned int yOffset = UINT32_MAX;
static float xCenter = FLT_MAX;
static float yCenter = FLT_MAX;
static unsigned int xOriginalSize = UINT32_MAX;
static unsigned int yOriginalSize = UINT32_MAX;
static char sourceFileName[PATH_MAX + 1] = "";
static char sourceTimeDate[PATH_MAX + 1] = "";
static char inputName[PATH_MAX + 1] = "";
static char inputSN[PATH_MAX + 1] = "";
static int border[4] = { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX };
static unsigned int aspectRatio[2] = { UINT32_MAX, UINT32_MAX };

static char filmMfgId[PATH_MAX + 1]= ""; 
static char filmType[PATH_MAX + 1] = "";
static char offsetPerfs[PATH_MAX + 1] = "";
static char prefix[PATH_MAX + 1]= "";
static char count[PATH_MAX + 1] = "";
static char format[PATH_MAX + 1] = "";
static unsigned int framePosition = UINT32_MAX;
static unsigned int sequenceLen = UINT32_MAX;
static unsigned int heldCount = UINT32_MAX;
static float mpFrameRate = FLT_MAX;
static float shutterAngle = FLT_MAX;
static char frameId[PATH_MAX + 1] = "";
static char slateInfo[PATH_MAX + 1] = "";

static unsigned int timeCode = UINT32_MAX;
static unsigned int userBits = UINT32_MAX;
static int interlace = UINT8_MAX;
static int fieldNumber = UINT8_MAX;
static int videoSignal = UINT8_MAX;
static float horzSampleRate = FLT_MAX;
static float vertSampleRate = FLT_MAX;
static float tvFrameRate = FLT_MAX;
static float timeOffset = FLT_MAX;
static float tvGamma = FLT_MAX;
static float blackLevel = FLT_MAX;
static float blackGain = FLT_MAX;
static float breakpoint = FLT_MAX;
static float whiteLevel = FLT_MAX; 
static float integrationTimes = FLT_MAX;
static int videoIdentificationCode = UINT8_MAX;
static int smpteTCType = UINT8_MAX;
static int smpteTCDBB2 = UINT8_MAX;

static char userIdentification[PATH_MAX + 1] = "";
static char sbmDescriptor[PATH_MAX + 1] = "";

static  cmdarg_t cmd_args[] = {

	{ UARG,  &imageOffset,        "IMAGE_OFFSET",         "-imgofs", 0, 0},   // Image offset
	{ UARG,  &dittoKey,           "DITTO_KEY",            "-ditto", 0, 0},  // Ditto key
	{ SARG,  &fileName,           "FILE_NAME",            "-fname", 0,  0},
	{ SARG,  &timeDate,			  "TIME_DATE",			  "-timdat", 0, 0},
	{ SARG,  &creator,			  "CREATOR",			  "-creatr", 0, 0},
	{ SARG,  &project,            "PROJECT",			  "-projct", 0, 0},
	{ SARG,  &copyright,		  "COPYRIGHT",			  "-copyrt", 0, 0},
	{ UARG,  &encryptKey,         "ENCRYPT_KEY",		  "-enckey", 0, 0},
	{ UARG,  &sbmOffset,          "SBM_OFFSET",		      "-sbmofs", 0, 0},
	{ PARG,  &datumMappingDirection, "DATUM_MAPPING_DIRECTION", "-dmdir", 0, 0},

	{ PARG,  &(dataSign[0]),	  "DATA_SIGN_1",		  "-dsign1", 0, 0},
	{ PARG,  &(dataSign[1]),	  "DATA_SIGN_2",		  "-dsign2", 0, 0 },
	{ PARG,  &(dataSign[2]),	  "DATA_SIGN_3",		  "-dsign3", 0, 0 },
	{ PARG,  &(dataSign[3]),	  "DATA_SIGN_4",		  "-dsign4", 0, 0 },
	{ PARG,  &(dataSign[4]),	  "DATA_SIGN_5",		  "-dsign5", 0, 0 },
	{ PARG,  &(dataSign[5]),	  "DATA_SIGN_6",		  "-dsign6", 0, 0 },
	{ PARG,  &(dataSign[6]),	  "DATA_SIGN_7",		  "-dsign7", 0, 0 },
	{ PARG,  &(dataSign[7]),	  "DATA_SIGN_8",		  "-dsign8", 0, 0 },
	{ FARG,  &(lowData[0]),  	  "LOW_DATA_1",			  "-ldata1", 0, 0 },
	{ FARG,  &(lowData[1]),	      "LOW_DATA_2",			  "-ldata2", 0, 0 },
	{ FARG,  &(lowData[2]),	      "LOW_DATA_3",			  "-ldata3", 0, 0 },
	{ FARG,  &(lowData[3]),	      "LOW_DATA_4",			  "-ldata4", 0, 0 },
	{ FARG,  &(lowData[4]),	      "LOW_DATA_5",			  "-ldata5", 0, 0 },
	{ FARG,  &(lowData[5]),	      "LOW_DATA_6",			  "-ldata6", 0, 0 },
	{ FARG,  &(lowData[6]),	      "LOW_DATA_7",			  "-ldata7", 0, 0 },
	{ FARG,  &(lowData[7]),	      "LOW_DATA_8",			  "-ldata8", 0, 0 },
	{ FARG,  &(lowQuantity[0]),   "LOW_QUANTITY_1",		  "-lquan1", 0, 0 },
	{ FARG,  &(lowQuantity[1]),   "LOW_QUANTITY_2",		  "-lquan2", 0, 0 },
	{ FARG,  &(lowQuantity[2]),   "LOW_QUANTITY_3",		  "-lquan3", 0, 0 },
	{ FARG,  &(lowQuantity[3]),   "LOW_QUANTITY_4",		  "-lquan4", 0, 0 },
	{ FARG,  &(lowQuantity[4]),   "LOW_QUANTITY_5",		  "-lquan5", 0, 0 },
	{ FARG,  &(lowQuantity[5]),   "LOW_QUANTITY_6",		  "-lquan6", 0, 0 },
	{ FARG,  &(lowQuantity[6]),   "LOW_QUANTITY_7",		  "-lquan7", 0, 0 },
	{ FARG,  &(lowQuantity[7]),   "LOW_QUANTITY_8",		  "-lquan8", 0, 0 },
	{ FARG,  &(highData[0]),	  "HIGH_DATA_1",	      "-hdata1", 0, 0 },
	{ FARG,  &(highData[1]),	  "HIGH_DATA_2",	      "-hdata2", 0, 0 },
	{ FARG,  &(highData[2]),	  "HIGH_DATA_3",	      "-hdata3", 0, 0 },
	{ FARG,  &(highData[3]),	  "HIGH_DATA_4",	      "-hdata4", 0, 0 },
	{ FARG,  &(highData[4]),	  "HIGH_DATA_5",	      "-hdata5", 0, 0 },
	{ FARG,  &(highData[5]),	  "HIGH_DATA_6",	      "-hdata6", 0, 0 },
	{ FARG,  &(highData[6]),	  "HIGH_DATA_7",	      "-hdata7", 0, 0 },
	{ FARG,  &(highData[7]),	  "HIGH_DATA_8",	      "-hdata8", 0, 0 },
	{ FARG,  &(highQuantity[0]),  "HIGH_QUANTITY_1",	  "-hquan1", 0, 0 },
	{ FARG,  &(highQuantity[1]),  "HIGH_QUANTITY_2",	  "-hquan2", 0, 0 },
	{ FARG,  &(highQuantity[2]),  "HIGH_QUANTITY_3",	  "-hquan3", 0, 0 },
	{ FARG,  &(highQuantity[3]),  "HIGH_QUANTITY_4",	  "-hquan4", 0, 0 },
	{ FARG,  &(highQuantity[4]),  "HIGH_QUANTITY_5",	  "-hquan5", 0, 0 },
	{ FARG,  &(highQuantity[5]),  "HIGH_QUANTITY_6",	  "-hquan6", 0, 0 },
	{ FARG,  &(highQuantity[6]),  "HIGH_QUANTITY_7",	  "-hquan7", 0, 0 },
	{ FARG,  &(highQuantity[7]),  "HIGH_QUANTITY_8",	  "-hquan8", 0, 0 },
	{ PARG,  &(descriptor[0]),    "DESCRIPTOR_1",		  "-dscor1", -1, 0 },
	{ PARG,  &(descriptor[1]),    "DESCRIPTOR_2",		  "-dscor2", -1, 0 },
	{ PARG,  &(descriptor[2]),    "DESCRIPTOR_3",         "-dscor3", -1, 0 },
	{ PARG,  &(descriptor[3]),    "DESCRIPTOR_4",		  "-dscor4", -1, 0 },
	{ PARG,  &(descriptor[4]),    "DESCRIPTOR_5",		  "-dscor5", -1, 0 },
	{ PARG,  &(descriptor[5]),    "DESCRIPTOR_6",		  "-dscor6", -1, 0 },
	{ PARG,  &(descriptor[6]),    "DESCRIPTOR_7",		  "-dscor7", -1, 0 },
	{ PARG,  &(descriptor[7]),    "DESCRIPTOR_8",		  "-dscor8", -1, 0 },
	{ PARG,  &(transfer[0]),	  "TRANSFER_1",		      "-trans1", -1, 0 },
	{ PARG,  &(transfer[1]),	  "TRANSFER_2",		      "-trans2", -1, 0 },
	{ PARG,  &(transfer[2]),	  "TRANSFER_3",		      "-trans3", -1, 0 },
	{ PARG,  &(transfer[3]),	  "TRANSFER_4",		      "-trans4", -1, 0 },
	{ PARG,  &(transfer[4]),	  "TRANSFER_5",		      "-trans5", -1, 0 },
	{ PARG,  &(transfer[5]),	  "TRANSFER_6",		      "-trans6", -1, 0 },
	{ PARG,  &(transfer[6]),	  "TRANSFER_7",		      "-trans7", -1, 0 },
	{ PARG,  &(transfer[7]),	  "TRANSFER_8",		      "-trans8", -1, 0 },
	{ PARG,  &(colorimetric[0]),  "COLORIMETRIC_1",		  "-color1", -1, 0 },
	{ PARG,  &(colorimetric[1]),  "COLORIMETRIC_2",		  "-color2", -1, 0 },
	{ PARG,  &(colorimetric[2]),  "COLORIMETRIC_3",		  "-color3", -1, 0 },
	{ PARG,  &(colorimetric[3]),  "COLORIMETRIC_4",		  "-color4", -1, 0 },
	{ PARG,  &(colorimetric[4]),  "COLORIMETRIC_5",		  "-color5", -1, 0 },
	{ PARG,  &(colorimetric[5]),  "COLORIMETRIC_6",		  "-color6", -1, 0 },
	{ PARG,  &(colorimetric[6]),  "COLORIMETRIC_7",		  "-color7", -1, 0 },
	{ PARG,  &(colorimetric[7]),  "COLORIMETRIC_8",		  "-color8", -1, 0 },
	{ PARG,  &(bitSize[0]),		  "BITSIZE_1",			  "-bitsz1", -1, 0 },
	{ PARG,  &(bitSize[1]),		  "BITSIZE_2",			  "-bitsz2", -1, 0 },
	{ PARG,  &(bitSize[2]),		  "BITSIZE_3",			  "-bitsz3", -1, 0 },
	{ PARG,  &(bitSize[3]),		  "BITSIZE_4",			  "-bitsz4", -1, 0 },
	{ PARG,  &(bitSize[4]),		  "BITSIZE_5",			  "-bitsz5", -1, 0 },
	{ PARG,  &(bitSize[5]),		  "BITSIZE_6",			  "-bitsz6", -1, 0 },
	{ PARG,  &(bitSize[6]),		  "BITSIZE_7",			  "-bitsz7", -1, 0 },
	{ PARG,  &(bitSize[7]),		  "BITSIZE_8",			  "-bitsz8", -1, 0 },
	{ PARG,  &(packing[0]),		  "PACKING_1",			  "-pack1", -1, 0 },
	{ PARG,  &(packing[1]),		  "PACKING_2",			  "-pack2", -1, 0 },
	{ PARG,  &(packing[2]),		  "PACKING_3",			  "-pack3", -1, 0 },
	{ PARG,  &(packing[3]),		  "PACKING_4",			  "-pack4", -1, 0 },
	{ PARG,  &(packing[4]),		  "PACKING_5",			  "-pack5", -1, 0 },
	{ PARG,  &(packing[5]),		  "PACKING_6",			  "-pack6", -1, 0 },
	{ PARG,  &(packing[6]),		  "PACKING_7",			  "-pack7", -1, 0 },
	{ PARG,  &(packing[7]),		  "PACKING_8",			  "-pack8", -1, 0 },
	{ PARG,  &(encoding[0]),	  "ENCODING_1",		      "-encod1", -1, 0 },
	{ PARG,  &(encoding[1]),	  "ENCODING_2",		      "-encod2", -1, 0 },
	{ PARG,  &(encoding[2]),	  "ENCODING_3",		      "-encod3", -1, 0 },
	{ PARG,  &(encoding[3]),	  "ENCODING_4",		      "-encod4", -1, 0 },
	{ PARG,  &(encoding[4]),	  "ENCODING_5",		      "-encod5", -1, 0 },
	{ PARG,  &(encoding[5]),	  "ENCODING_6",		      "-encod6", -1, 0 },
	{ PARG,  &(encoding[6]),	  "ENCODING_7",		      "-encod7", -1, 0 },
	{ PARG,  &(encoding[7]),	  "ENCODING_8",		      "-encod8", -1, 0 },
	{ PARG,  &(endOfLinePadding[0]), "END_OF_LINE_PADDING_1", "-eolp1", -1, 0 },
	{ PARG,  &(endOfLinePadding[1]), "END_OF_LINE_PADDING_2", "-eolp2", -1, 0 },
	{ PARG,  &(endOfLinePadding[2]), "END_OF_LINE_PADDING_3", "-eolp3", -1, 0 },
	{ PARG,  &(endOfLinePadding[3]), "END_OF_LINE_PADDING_4", "-eolp4", -1, 0 },
	{ PARG,  &(endOfLinePadding[4]), "END_OF_LINE_PADDING_5", "-eolp5", -1, 0 },
	{ PARG,  &(endOfLinePadding[5]), "END_OF_LINE_PADDING_6", "-eolp6", -1, 0 },
	{ PARG,  &(endOfLinePadding[6]), "END_OF_LINE_PADDING_7", "-eolp7", -1, 0 },
	{ PARG,  &(endOfLinePadding[7]), "END_OF_LINE_PADDING_8", "-eolp8", -1, 0 },
	{ PARG,  &(endOfImagePadding[0]), "END_OF_IMAGE_PADDING_1", "-eoip1", -1, 0 },
	{ PARG,  &(endOfImagePadding[1]), "END_OF_IMAGE_PADDING_2", "-eoip2", -1, 0 },
	{ PARG,  &(endOfImagePadding[2]), "END_OF_IMAGE_PADDING_3", "-eoip3", -1, 0 },
	{ PARG,  &(endOfImagePadding[3]), "END_OF_IMAGE_PADDING_4", "-eoip4", -1, 0 },
	{ PARG,  &(endOfImagePadding[4]), "END_OF_IMAGE_PADDING_5", "-eoip5", -1, 0 },
	{ PARG,  &(endOfImagePadding[5]), "END_OF_IMAGE_PADDING_6", "-eoip6", -1, 0 },
	{ PARG,  &(endOfImagePadding[6]), "END_OF_IMAGE_PADDING_7", "-eoip7", -1, 0 },
	{ PARG,  &(endOfImagePadding[7]), "END_OF_IMAGE_PADDING_8", "-eoip8", -1, 0 },
	{ SARG,  description[0],		"DESCRIPTION_1",	  "-dsctn1", 0, 0 },
	{ SARG,  description[1],		"DESCRIPTION_2",	  "-dsctn2", 0, 0 },
	{ SARG,  description[2],		"DESCRIPTION_3",	  "-dsctn3", 0, 0 },
	{ SARG,  description[3],		"DESCRIPTION_4",	  "-dsctn4", 0, 0 },
	{ SARG,  description[4],		"DESCRIPTION_5",	  "-dsctn5", 0, 0 },
	{ SARG,  description[5],		"DESCRIPTION_6",	  "-dsctn6", 0, 0 },
	{ SARG,  description[6],		"DESCRIPTION_7",	  "-dsctn7", 0, 0 },
	{ SARG,  description[7],		"DESCRIPTION_8",	  "-dsctn8", 0, 0 },
	{ PARG,  &orientation,		    "ORIENTATION",		  "-orient", 0, 0 },
	{ PARG,  &numberElements,		"NUMBER_ELEMENTS",	  "-numele", 1, 0 },
	{ UARG,  &pixelsPerLine,		"PIXELS_PER_LINE",	  "-pixpl", 0, 0 },
	{ UARG,  &linesPerElement,		"LINES_PER_ELEMENT",  "-lpelem", 0, 0 },
	{ PARG,  &(chromaSubsampling[0]), "CHROMA_SUBSAMPLING_1", "-chrsb1", 0, 0 },
	{ PARG,  &(chromaSubsampling[1]), "CHROMA_SUBSAMPLING_2", "-chrsb2", 0, 0 },
	{ PARG,  &(chromaSubsampling[2]), "CHROMA_SUBSAMPLING_3", "-chrsb3", 0, 0 },
	{ PARG,  &(chromaSubsampling[3]), "CHROMA_SUBSAMPLING_4", "-chrsb4", 0, 0 },
	{ PARG,  &(chromaSubsampling[4]), "CHROMA_SUBSAMPLING_5", "-chrsb5", 0, 0 },
	{ PARG,  &(chromaSubsampling[5]), "CHROMA_SUBSAMPLING_6", "-chrsb6", 0, 0 },
	{ PARG,  &(chromaSubsampling[6]), "CHROMA_SUBSAMPLING_7", "-chrsb7", 0, 0 },
	{ PARG,  &(chromaSubsampling[7]), "CHROMA_SUBSAMPLING_8", "-chrsb8", 0, 0 },

	{ UARG,  &xOffset,				 "X_OFFSET",			  "-xoffs", 0, 0 },
	{ UARG,  &yOffset,               "Y_OFFSET",			  "-yoffs", 0, 0 },
	{ FARG,  &xCenter,			     "X_CENTER",			  "-xcentr", 0, 0 },
	{ FARG,  &yCenter,				 "Y_CENTER",			  "-ycentr", 0, 0 },
	{ UARG,  &xOriginalSize,		 "X_ORIGINAL_SIZE",       "-xorgsz", 0, 0 },
	{ UARG,  &yOriginalSize,		 "Y_ORIGINAL_SIZE",		  "-yorgsz", 0, 0 },
	{ SARG,  sourceFileName,		 "SOURCE_FILE_NAME",	  "-srcfn", 0, 0 },
	{ SARG,  sourceTimeDate,		 "SOURCE_TIME_DATE",	  "-srctd", 0, 0 },
	{ SARG,  inputName,				 "INPUT_NAME",			  "-inptnm", 0, 0 },
	{ SARG,  inputSN,				 "INPUT_SN",			  "-inptsn", 0, 0 },
	{ PARG,  &(border[0]),           "BORDER_XL",		      "-bordxl", 0, 0 },
	{ PARG,  &(border[1]),           "BORDER_XR",		      "-bordxr", 0, 0 },
	{ PARG,  &(border[2]),           "BORDER_YT",		      "-bordyt", 0, 0 },
	{ PARG,  &(border[3]),           "BORDER_YB",		      "-bordyb", 0, 0 },
	{ UARG,  &(aspectRatio[0]),		 "ASPECT_RATIO_H",	      "-aspcth", 0, 0 },
	{ UARG,  &(aspectRatio[1]),		 "ASPECT_RATIO_V",		  "-aspctv", 0, 0 },

	{ SARG,  filmMfgId,              "FILM_MFG_ID",			  "-filmid", 0, 0 },
	{ SARG,  filmType,				 "FILM_TYPE",			  "-filmty", 0, 0 },
	{ SARG,  offsetPerfs,			 "OFFSET_PERFS",	      "-ofsprf", 0, 0 },
	{ SARG,  prefix,				 "PREFIX",				  "-prefix", 0, 0 },
	{ SARG,  count,					 "COUNT",				  "-count", 0, 0 },
	{ SARG,  format,				 "FORMAT",			      "-format", 0, 0 },
	{ UARG,  &framePosition,		 "FRAME_POSITION",		  "-frmpos", 0, 0 },
	{ UARG,  &sequenceLen,			 "SEQUENCE_LEN",		  "-seqlen", 0, 0 },
	{ UARG,  &heldCount,			 "HELD_COUNT",			  "-heldct", 0, 0 },
	{ FARG,  &mpFrameRate,			 "MP_FRAME_RATE",		  "-mpfrat", 0, 0 },
	{ FARG,  &shutterAngle,			 "SHUTTER_ANGLE",		  "-shutan", 0, 0 },
	{ SARG,  frameId,			     "FRAME_ID",			  "-frmid", 0, 0 },
	{ SARG,  slateInfo,				 "SLATE_INFO",		      "-sltinf", 0, 0 },

	{ UARG,  &timeCode,              "TIME_CODE",             "-timeco", 0, 0 },
	{ UARG,  &userBits,              "USER_BITS",             "-usrbts", 0, 0 },
	{ PARG,  &interlace,             "INTERLACE",             "-ilace", 0, 0 },
	{ PARG,  &fieldNumber,           "FIELD_NUMBER",          "-fldnum", 0, 0 },
	{ PARG,  &videoSignal,           "VIDEO_SIGNAL",		  "-vidsig", 0, 0 },
	{ FARG,  &horzSampleRate,        "HORZ_SAMPLE_RATE",      "-horzsr", 0, 0 },
	{ FARG,  &vertSampleRate,        "VERT_SAMPLE_RATE",      "-vertsr", 0, 0 },
	{ FARG,  &tvFrameRate,           "TV_FRAME_RATE",		  "-tvfrat", 0, 0 },
	{ FARG,  &timeOffset,            "TIME_OFFSET",           "-timofs", 0, 0 },
	{ FARG,  &tvGamma,               "GAMMA",                 "-gamma", 0, 0 },
	{ FARG,  &blackLevel,            "BLACK_LEVEL",		      "-blklev", 0, 0 },
	{ FARG,  &blackGain,             "BLACK_GAIN",            "-blkgai", 0, 0 },
	{ FARG,  &breakpoint,            "BREAKPOINT",			  "-brkpnt", 0, 0 },
	{ FARG,  &whiteLevel,            "WHITE_LEVEL",			  "-whtlvl", 0, 0 },
	{ FARG,  &integrationTimes,      "INTEGRATION_TIMES",	  "-inttms", 0, 0 },
	{ PARG,  &videoIdentificationCode, "VIDEO_IDENTIFICATION_CODE", "-vic", 0, 0 },
	{ PARG,  &smpteTCType,	         "SMPTE_TC_TYPE",		  "-sttype", 0, 0 },
	{ PARG,  &smpteTCDBB2,           "SMPTE_TC_DBB2",	      "-stdbb2", 0, 0 },

	{ SARG,  &userIdentification, "USER_IDENTIFICATION",      "-userid", 0, 0 },
	{ SARG,  &sbmDescriptor,      "SBM_DESCRIPTOR",			  "-sbmdsc", 0, 0 }, 

	{ PARG,  &use422,             "USE_422",              "-u422",  0,  0},  // enable_422/simple_422
	{ PARG,  &use420,             "USE_420",              "-u420",  0,  0},  // native 420
	{ PARG,  &useYuvInput,        "USE_YUV_INPUT",        "-uyi",   0,  0},   // Use YUV input (convert if necessary)
	{ PARG,  &bitsPerComponent,   "BITS_PER_COMPONENT",   "-bpc",   0,  0 },  // Bits per component
	{ PARG,  &createAlpha,        "CREATE_ALPHA",         "-crealp", 0, 0 },  // Create alpha plane
	{ PARG,  &deleteAlpha,        "DELETE_ALPHA",         "-delalp", 0, 0 },  // Delete alpha plane
	{ PARG,  &rbSwap,             "SWAP_R_AND_B",         "-rbswp", 0,  0},  // Swap red & blue components
	{ PARG,  &rbSwapOut,          "SWAP_R_AND_B_OUT",     "-rbswpo", 0,  0},  // Swap red & blue components
	{ PARG,  &cbcrSwap,           "SWAP_CB_AND_CR",       "-cbcrswp", 0,  0 },  // Swap Cb & Cr components for 4:4:4
	{ PARG,  &cbcrSwapOut,        "SWAP_CB_AND_CR_OUT",   "-cbcrswpo", 0,  0 },  // Swap Cb & Cr components for 4:4:4
	{ PARG,  &function,           "FUNCTION",             "-do",    0,  0},   // 0=convert, 1=read only (for dumping headers)
	{ PARG,  &dpxRForceBe,        "DPXR_FORCE_BE",        "-dpxrfbe", 0, 0},  // Force big-endian DPX read
	{ PARG,  &dpxRPadEnds,        "DPXR_PAD_ENDS",        "-dpxrpad",  0, 0},  // Pad line ends for DPX input
	{ PARG,  &dpxRDatumOrder,     "DPXR_DATUM_ORDER",     "-dpxrdo",  0, 0},   // DPX input datum order
	{ PARG,  &dpxWPadEnds,        "DPXW_PAD_ENDS",        "-dpxwpad",  0, 0},  // Pad line ends for DPX writing
	{ PARG,  &dpxWDatumOrder,     "DPXW_DATUM_ORDER",     "-dpxwdo",  0, 0},   // DPX output datum order
	{ PARG,  &dpxWForcePacking,   "DPXW_FORCE_PACKING",   "-dpxwfp",  0, 0},   // DPX output force packing method
	{ PARG,  &dpxWByteSwap,       "DPXW_BYTE_SWAP",       "-dpxwbs",  0, 0},   // DPX output write in LE order
	{ PARG,  &hdrdpxWriteAsBe,    "HDRDPX_WRITE_AS_BE",   "-hdpxwbe",  0, 0 },   // 0 = write HDR DPX in LE order, 1 = write HDR DPX in BE order
	{ PARG,  &picWidth,           "PIC_WIDTH",            "-pw",  0,  0},    // picture width for YUV input
	{ PARG,  &picHeight,          "PIC_HEIGHT",           "-ph",  0,  0},    // picture height for YUV input
	{ PARG,  &ppmFileOutput,      "PPM_FILE_OUTPUT",      "-ppm",  0,  0},    // output PPM files
	{ PARG,  &dpxFileOutput,      "DPX_FILE_OUTPUT",      "-dpx",  0,  0},    // output DPX files
	{ PARG,  &hdrDpxFileOutput,   "HDRDPX_FILE_OUTPUT",   "-hdrdpx",  0,  0 },    // output HDR DPX files
	{ PARG,  &yuvFileOutput,      "YUV_FILE_OUTPUT",      "-yuvo", 0,  0},   // Enable YUV file output for 4:2:0
	{ PARG,  &yuvFileFormat,      "YUV_FILE_FORMAT",      "-yuvff", 0,  0},   // YUV file format (0=planar 420, 1=UYVY, 2=planar 444)
	{ PARG,  &sbmFileOutput,      "SBM_FILE_OUTPUT",      "-sbmo", 0,  0 },   // Enable output of standards-based metadata file to sidecar file
	{ PARG,  &udFileOutput,       "UD_FILE_OUTPUT",       "-udo", 0,  0 },   // Enable output of user data section to sidecar file

	{ PARG,  &logHeaderInput,     "LOG_HEADER_INPUT",     "-loghi", 0, 0}, // log HDR DPX header for input files
	{ PARG,  &logHeaderOutput,    "LOG_HEADER_OUTPUT",    "-logho", 0, 0 }, // log HDR DPX header for output files

	 {NARG,  &help,               "",                     "-help"   , 0, 0}, // video format
     {SARG,   filepath,           "INCLUDE",              "-F"      , 0, 0}, // Cconfig file
     {SARG,   option,             "",                     "-O"      , 0, 0}, // key/value pair
     {SARG,   fn_i,               "SRC_LIST",              ""        , 0, 0}, // Input file list
     {SARG,   fn_o,               "OUT_DIR",               ""       , 0, 0}, // Output directory
     {SARG,   fn_log,             "LOG_FILENAME",         ""        , 0, 0}, // Log file name
	 {SARG,   sbmExtension,       "SBM_EXTENSION",        ""        , 0, 0 }, // File extension for standards-based metadata input files

     {PARG,   NULL,               "",                     "",       0, 0 }
};

/*!
 ************************************************************************
 * \brief
 *    set_defaults() - Set default configuration values
 ************************************************************************
 */
void set_defaults(void)
{
	int i;

	useYuvInput = 0;
	rbSwap = 0;
	rbSwapOut = 0;
	cbcrSwap = 0;
	cbcrSwapOut = 0;
	function = 0;
	ppmFileOutput = 0;
	yuvFileOutput = 0;
	dpxFileOutput = 0;
	hdrDpxFileOutput = 1;
	yuvFileFormat = 0;

	// Standards compliant DPX settings:
	dpxRForceBe = 0;
	dpxRDatumOrder = 0;
	dpxRPadEnds = 0;
	dpxWPadEnds = 0;
	dpxWDatumOrder = 0;
	dpxWForcePacking = 1;   // Method B is non-standard per DPX spec
	use420 = 0;
	use422 = 0;
	bitsPerComponent = UINT8_MAX;
	createAlpha = 0;
	deleteAlpha = 0;
	hdrdpxWriteAsBe = 1;   // Write as big-endian by default

	picWidth = 1920;  // Default for YUV input
	picHeight = 1080;  
	logHeaderInput = 0;
	logHeaderOutput = 0;

	for (i = 0; i < NUM_IMAGE_ELEMENTS; ++i)
	{
		chromaSubsampling[i] = 0xf;
		dataSign[i] = UINT32_MAX;
		lowData[i] = FLT_MAX;
		lowQuantity[i] = FLT_MAX;
		highData[i] = FLT_MAX;
		highQuantity[i] = FLT_MAX;
		descriptor[i] = UINT8_MAX;
		transfer[i] = UINT8_MAX;
		colorimetric[i] = UINT8_MAX;
		bitSize[i] = UINT8_MAX;
		packing[i] = UINT16_MAX;
		encoding[i] = UINT16_MAX;
		endOfLinePadding[i] = UINT32_MAX;
		endOfImagePadding[i] = UINT32_MAX;
		memset(description[i], 0, DESCRIPTION_SIZE);
	}

	strcpy(fn_o, ".");
}


/*!
************************************************************************
* \brief
*    sanity_checks() - Ensure (some) parameters are valid
************************************************************************
*/
void sanity_checks(void)
{
	int i;

	if (strlen(fileName) > FILE_NAME_SIZE)
		UErr("Specified file name (%s) is too many characters\n", fileName);
	if (strlen(timeDate) > TIMEDATE_SIZE)
		UErr("Specified time & date (%s) is too many characters\n", timeDate);
	if (strlen(creator) > CREATOR_SIZE)
		UErr("Specified creator (%s) is too many characters\n", creator);
	if (strlen(project) > PROJECT_SIZE)
		UErr("Specified project (%s) is too many characters\n", project);
	if (strlen(copyright) > COPYRIGHT_SIZE)
		UErr("Specified copyright (%s) is too many characters\n", copyright);
	for (i = 0; i < 8; ++i)
	{
		if (strlen(description[i]) > DESCRIPTION_SIZE)
			UErr("Specified description (%s) for image element %d is too many characters\n", description[i], i + 1);
	}
	if (strlen(sourceFileName) > FILE_NAME_SIZE)
		UErr("Specified source file name (%s) is too many characters\n", sourceFileName);
	if (strlen(sourceTimeDate) > TIMEDATE_SIZE)
		UErr("Specified source time & date (%s) is too many characters\n", sourceTimeDate);
	if (strlen(inputName) > INPUTNAME_SIZE)
		UErr("Specified input name (%s) is too many characters\n", inputName);
	if (strlen(inputSN) > INPUTSN_SIZE)
		UErr("Specified input SN (%s) is too many characters\n", inputSN);
	if (strlen(filmMfgId) > 2)
		UErr("Specified film mfg id (%s) is too many characters\n", filmMfgId);
	if (strlen(filmType) > 2)
		UErr("Specified film type (%s) is too many characters\n", filmType);
	if (strlen(offsetPerfs) > 2)
		UErr("Specified offset perfs (%s) is too many characters\n", offsetPerfs);
	if (strlen(prefix) > 6)
		UErr("Specified prefix (%s) is too many characters\n", prefix);
	if (strlen(count) > 4)
		UErr("Specified count (%s) is too many characters\n", count);
	if (strlen(format) > 32)
		UErr("Specified format (%s) is too many characters\n", format);
	if (strlen(frameId) > 32)
		UErr("Specified frame ID (%s) is too many characters\n", frameId);
	if (strlen(slateInfo) > 100)
		UErr("Specified slate info (%s) is too many characters\n", slateInfo);
	if (strlen(userIdentification) > 32)
		UErr("Specified user identification (%s) is too many characters\n", userIdentification);
}




/*!
************************************************************************
* \brief
*    read_user_data_file() - Read a user-data file
*
* \param infname
*    Image filename (user data filename replaces extension with .user extension)
* \param dpxuserdata
*    User data structure to fill (assumes data pointer is unallocated)
* \param length
*    Returns length to populate into main DPX header
*
************************************************************************
*/
void read_user_data_file(char *infname, HDRDPXUSERDATA *dpxuserdata, unsigned int *length)
{
	FILE *fp;
	unsigned int i, fsize;
	char udfname[PATH_MAX + 1];
	int j, extloc = -1;

	for(j=strlen(infname)-1; j>=0; --j)
		if (infname[j] == '.')
		{
			extloc = j;
			break;
		}

	if (extloc < 0)
		UErr("read_user_data_file() could not find extension for file name %s", infname);
	strcpy(udfname, infname);
	sprintf(&(udfname[extloc + 1]), "user");  // .user extension for user-data files

	fp = fopen(udfname, "rb");

	if (fp == NULL)
	{
		if (strlen(userIdentification) == 0)
			*length = 0;
		else
			*length = 32; // No data, just user ID
		return;
	}

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	dpxuserdata->UserData = (unsigned char *)malloc(fsize);
	for (i = 0; i < fsize; ++i)
		dpxuserdata->UserData[i] = fgetc(fp);
	fclose(fp);
	*length = 32 + fsize;
}


/*!
************************************************************************
* \brief
*    read_sbm_data_file() - Read standards-based metadata file
*
* \param infname
*    Standards-based metadata filename
* \param dpxsbmdata
*    Standards-based metadata structure to fill (assumes data pointer is unallocated)
*
************************************************************************
*/
void read_sbm_data_file(char *infname, HDRDPXSBMDATA *dpxsbmdata)
{
	FILE *fp;
	unsigned int i, fsize;
	char sbmfname[PATH_MAX + 1];
	int j, extloc = -1;

	dpxsbmdata->SbmLength = 0;
	for (j = strlen(infname) - 1; j >= 0; --j)
		if (infname[j] == '.')
		{
			extloc = j;
			break;
		}

	if (extloc < 0)
		UErr("read_sbm_data_file() could not find extension for file name %s", infname);
	strcpy(sbmfname, infname);
	sprintf(&(sbmfname[extloc + 1]), "%s", sbmExtension);

	fp = fopen(sbmfname, "rb");

	if (fp == NULL)
		return;

	if(strcmp(sbmDescriptor, "ST336") != 0 && strcmp(sbmDescriptor, "Reg-XML") != 0 && strcmp(sbmDescriptor, "XMP") != 0)
		UErr("Unrecognized standards-based metadata descriptor\n");
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	copy_string_n(dpxsbmdata->SbmFormatDescriptor, sbmDescriptor, 128);
	dpxsbmdata->SbmLength = fsize;
	dpxsbmdata->SbmData = (unsigned char *)malloc(fsize);
	for (i = 0; i < fsize; ++i)
		dpxsbmdata->SbmData[i] = fgetc(fp);
	fclose(fp);
}


/*!
************************************************************************
* \brief
*    write_user_data_file() - Write a user-data file from a DPX file
*
* \param fname
*    Image filename (user data filename replaces extension with .user extension)
* \param hdrdpx_f
*    HDR DPX file header structure
* \param length
*    Returns length to populate into main DPX header
*
************************************************************************
*/
void write_user_data_file(char *fname, HDRDPXFILEFORMAT *hdrdpx_f, HDRDPXUSERDATA *hdrdpx_user)
{
	FILE *fp;
	char udfname[PATH_MAX + 1];
	uint32_t i;
	int j, extloc = -1;

	for (j = strlen(fname) - 1; j >= 0; --j)
		if (fname[j] == '.')
		{
			extloc = j;
			break;
		}

	if (extloc < 0)
		UErr("write_user_data_file() could not find extension for file name %s", fname);
	strcpy(udfname, fname);
	sprintf(&(udfname[extloc + 1]), "user");  // .user extension for user-data files

	if (hdrdpx_f->FileHeader.UserSize <= 32 || hdrdpx_f->FileHeader.UserSize == UINT32_MAX)
		return;		// Nothing to do, user data not present

	if ((fp = fopen(udfname, "wb")) == NULL)
		UErr("Can't open user data file %s for output\n", udfname);

	for (i = 0; i < hdrdpx_f->FileHeader.UserSize - 32; ++i)
		fputc(hdrdpx_user->UserData[i], fp);
	fclose(fp);
}


/*!
************************************************************************
* \brief
*    write_sbm_data_file() - Write a standards-based metadata file from HDR DPX file
*
* \param fname
*    Image filename (user data filename replaces extension with appropriate extension)
* \param hdrdpx_f
*    HDR DPX file header structure
* \param dpxsbmdata
*    Standards-based metadata read from HDR DPX file
*
************************************************************************
*/
void write_sbm_data_file(char *fname, HDRDPXFILEFORMAT *hdrdpx_f, HDRDPXSBMDATA *dpxsbmdata)
{
	FILE *fp;
	char sbmfname[PATH_MAX];
	uint32_t i;
	int j, extloc = -1;

	for (j = strlen(fname) - 1; j >= 0; --j)
		if (fname[j] == '.')
		{
			extloc = j;
			break;
		}

	if (extloc < 0)
		UErr("write_sbm_data_file() could not find extension for file name %s", fname);
	strcpy(sbmfname, fname);
	sprintf(&(sbmfname[extloc + 1]), "user");  // .user extension for user-data files

	if (hdrdpx_f->FileHeader.StandardsBasedMetadataOffset == UINT32_MAX || dpxsbmdata->SbmLength == 0)
		return;  // Nothing to do, standards-based metadata not present

	if (!strcmp(dpxsbmdata->SbmFormatDescriptor, "ST336"))
		strcat(sbmfname, ".klv");
	else if (!strcmp(dpxsbmdata->SbmFormatDescriptor, "Reg-XML"))
		strcat(sbmfname, ".xml");
	else if (!strcmp(dpxsbmdata->SbmFormatDescriptor, "XMP"))
		strcat(sbmfname, ".xmp");
	else
		UErr("Unrecognized standards-based metadata type:  %s", dpxsbmdata->SbmFormatDescriptor);

	if ((fp = fopen(sbmfname, "wb")) == NULL)
		UErr("Can't open standards-based metadata file %s for output\n", sbmfname);

	for (i = 0; i < dpxsbmdata->SbmLength; ++i)
		fputc(dpxsbmdata->SbmData[i], fp);
	fclose(fp);
}


/*!
************************************************************************
* \brief
*    populate_hdr_dpx_header() - Populate HDR DPX header items based on cfg file params
*
* \param infname
*    Image filename (used to figure out user data and standards-based metadata filenames)
* \param dpxh
*    Pointer to allocataed HDR DPX file header structure to output
* \param dpxud
*    Pointer to allocated user data structure (data pointer is not preallocated since size is unknown)
* \param dpxsbm
*    Pointer to allocated SBM data structure (data pointer is not preallocated since size is unknown)
*
************************************************************************
*/
void populate_hdr_dpx_header(char *infname, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxud, HDRDPXSBMDATA *dpxsbm)
{
	unsigned int ud_length;
	unsigned int min_image_offset, recommended_image_offset;
	unsigned int sbm_offset;
	unsigned int approx_image_size = 0;
	int i, d;

	read_user_data_file(infname, dpxud, &ud_length);
	sbm_offset = sbmOffset;
	read_sbm_data_file(infname, dpxsbm);  // length = 0 means no SBM
	min_image_offset = ((2048 + ud_length + 3) >> 2) << 2;  // Required by spec: 2048 byte header + user data at 4-byte boundary
	recommended_image_offset = ((2048 + ud_length + 511) >> 9) << 9; // Not required by spec, but if offset is unspecified, chooses 512-byte boundary
	memset(dpxh, 0xff, sizeof(HDRDPXFILEFORMAT));

	if (sbm_offset != UINT32_MAX)
	{
		if (dpxsbm->SbmLength == 0)
		{
			printf("Warning: SBM offset specified but no standards-based metadata is present");
			sbm_offset = UINT32_MAX;
		}
		else		// SBM offset specified, sanity checks
		{
			if (sbm_offset < min_image_offset)
				UErr("SBM offset (%d) must be greater than header size (%d)\n", sbm_offset, min_image_offset);
			// This check is approximate:
			for (i = 0; i < numberElements; ++i)
			{
				d = descriptor[i];
				if (d <= 9)
					approx_image_size += (pixelsPerLine * linesPerElement);
				else if (d == 10 || d == 11)
					approx_image_size += (pixelsPerLine * linesPerElement / 4);
				else if (d == 50 || d == 53 || d == 56 || d == 101 || d == 102)
					approx_image_size += (3 * pixelsPerLine * linesPerElement);
				else if (d == 51 || d == 52 || d == 54 || d == 55 || d == 57 || d == 58 || d == 103)
					approx_image_size += (4 * pixelsPerLine * linesPerElement);
				else if (d == 100)
					approx_image_size += (2 * pixelsPerLine * linesPerElement);
				else if (d == 104)
					approx_image_size += (3 * pixelsPerLine * linesPerElement / 2);
				else if (d == 105)
					approx_image_size += (5 * pixelsPerLine * linesPerElement / 2);
				else if (d >= 150 && d <= 156)
					approx_image_size += ((d - 148) * pixelsPerLine * linesPerElement);

				if (sbmOffset < min_image_offset + ud_length + approx_image_size)  // SBM goes before image data
				{
					min_image_offset += ((sbmOffset + 132 + dpxsbm->SbmLength + 3) >> 2) << 2;
					recommended_image_offset = ((sbmOffset + 132 + dpxsbm->SbmLength + 511) >> 9) << 9;
				}
			}
		} // else fill in SBM offset once image data is written
	}

	// Fill in file header
	dpxh->FileHeader.Magic = 0x53445058;
	if (imageOffset < min_image_offset)
	{
		if (imageOffset > 0)
		{
			printf("WARNING: specified image offset too small, changed to %d", min_image_offset);
			dpxh->FileHeader.ImageOffset = min_image_offset;
		}
		else
			dpxh->FileHeader.ImageOffset = recommended_image_offset;  // use recommended value if unspecified
	}
	else
		dpxh->FileHeader.ImageOffset = imageOffset;
	copy_string_n(dpxh->FileHeader.Version, "V2.0HDR", 8);
	// Skipping file size (based on how big file is after it has been written)
	dpxh->FileHeader.DittoKey = dittoKey;
	dpxh->FileHeader.GenericSize = 1664;	// Mandated by spec
	dpxh->FileHeader.IndustrySize = 384;	// Mandated by spec
	dpxh->FileHeader.UserSize = ud_length;
	copy_string_n(dpxh->FileHeader.FileName, fileName, FILE_NAME_SIZE);
	copy_string_n(dpxh->FileHeader.TimeDate, timeDate, TIMEDATE_SIZE);
	copy_string_n(dpxh->FileHeader.Creator, creator, CREATOR_SIZE);
	copy_string_n(dpxh->FileHeader.Project, project, PROJECT_SIZE);
	copy_string_n(dpxh->FileHeader.Copyright, copyright, COPYRIGHT_SIZE);
	dpxh->FileHeader.EncryptKey = encryptKey;
	dpxh->FileHeader.StandardsBasedMetadataOffset = sbm_offset; 
	dpxh->FileHeader.DatumMappingDirection = datumMappingDirection;
	dpxh->FileHeader.UserSize = ud_length;

	// Fill in generic image header
	dpxh->ImageHeader.Orientation = orientation;
	dpxh->ImageHeader.NumberElements = numberElements;
	dpxh->ImageHeader.PixelsPerLine = pixelsPerLine;
	dpxh->ImageHeader.LinesPerElement = linesPerElement;
	for (i = 0; i < MIN(numberElements, 8); ++i)
	{
		dpxh->ImageHeader.ImageElement[i].DataSign = dataSign[i];
		if (bitSize[i] < 32)
		{
			if (lowData[i] == FLT_MAX)
				dpxh->ImageHeader.ImageElement[i].LowData.d = UINT32_MAX;
			else
				dpxh->ImageHeader.ImageElement[i].LowData.d = (int)(lowData[i] + 0.5);
			if (highData[i] == FLT_MAX)
				dpxh->ImageHeader.ImageElement[i].HighData.d = UINT32_MAX;
			else
				dpxh->ImageHeader.ImageElement[i].HighData.d = (int)(highData[i] + 0.5);
		}
		else
		{
			SET_FLOAT(dpxh->ImageHeader.ImageElement[i].LowData.f,  lowData[i]);
			SET_FLOAT(dpxh->ImageHeader.ImageElement[i].HighData.f, highData[i]);
		}

		SET_FLOAT(dpxh->ImageHeader.ImageElement[i].LowQuantity, lowQuantity[i]);
		SET_FLOAT(dpxh->ImageHeader.ImageElement[i].HighQuantity, highQuantity[i]);
		dpxh->ImageHeader.ImageElement[i].Descriptor = descriptor[i];
		dpxh->ImageHeader.ImageElement[i].Transfer = transfer[i];
		dpxh->ImageHeader.ImageElement[i].Colorimetric = colorimetric[i];
		dpxh->ImageHeader.ImageElement[i].BitSize = bitSize[i];
		dpxh->ImageHeader.ImageElement[i].Packing = packing[i];
		dpxh->ImageHeader.ImageElement[i].Encoding = encoding[i];
		dpxh->ImageHeader.ImageElement[i].Descriptor = descriptor[i];
		dpxh->ImageHeader.ImageElement[i].Transfer = transfer[i];
		dpxh->ImageHeader.ImageElement[i].Colorimetric = colorimetric[i];
		dpxh->ImageHeader.ImageElement[i].BitSize = bitSize[i];
		dpxh->ImageHeader.ImageElement[i].Packing = packing[i];
		dpxh->ImageHeader.ImageElement[i].Encoding = encoding[i];
		// DataOffset is derived as the data is written
		dpxh->ImageHeader.ImageElement[i].EndOfLinePadding = endOfLinePadding[i];
		dpxh->ImageHeader.ImageElement[i].EndOfImagePadding = endOfImagePadding[i];
		copy_string_n(dpxh->ImageHeader.ImageElement[i].Description, description[i], DESCRIPTION_SIZE);
		dpxh->ImageHeader.ChromaSubsampling &= ~(0xf << (i * 4));
		dpxh->ImageHeader.ChromaSubsampling |= (chromaSubsampling[i] << (i * 4)) & 0xf;
	}

	dpxh->SourceInfoHeader.XOffset = xOffset;
	dpxh->SourceInfoHeader.YOffset = yOffset;
	SET_FLOAT(dpxh->SourceInfoHeader.XCenter, xCenter);
	SET_FLOAT(dpxh->SourceInfoHeader.YCenter, yCenter);
	dpxh->SourceInfoHeader.XOriginalSize = xOriginalSize;
	dpxh->SourceInfoHeader.YOriginalSize = yOriginalSize;
	copy_string_n(dpxh->SourceInfoHeader.SourceFileName, sourceFileName, FILE_NAME_SIZE);
	copy_string_n(dpxh->SourceInfoHeader.SourceTimeDate, sourceTimeDate, TIMEDATE_SIZE);
	copy_string_n(dpxh->SourceInfoHeader.InputName, inputName, INPUTNAME_SIZE);
	copy_string_n(dpxh->SourceInfoHeader.InputSN, inputSN, INPUTSN_SIZE);
	for(i=0; i<4; ++i)
		dpxh->SourceInfoHeader.Border[i] = border[i];
	dpxh->SourceInfoHeader.AspectRatio[0] = aspectRatio[0];
	dpxh->SourceInfoHeader.AspectRatio[1] = aspectRatio[1];

	copy_string_n(dpxh->FilmHeader.FilmMfgId, filmMfgId, 2);
	copy_string_n(dpxh->FilmHeader.FilmType, filmType, 2);
	copy_string_n(dpxh->FilmHeader.OffsetPerfs, offsetPerfs, 2);
	copy_string_n(dpxh->FilmHeader.Prefix, prefix, 6);
	copy_string_n(dpxh->FilmHeader.Count, count, 4);
	copy_string_n(dpxh->FilmHeader.Format, format, 32);
	dpxh->FilmHeader.FramePosition = framePosition;
	dpxh->FilmHeader.SequenceLen = sequenceLen;
	dpxh->FilmHeader.HeldCount = heldCount;
	SET_FLOAT(dpxh->FilmHeader.FrameRate, mpFrameRate);
	SET_FLOAT(dpxh->FilmHeader.ShutterAngle, shutterAngle);
	copy_string_n(dpxh->FilmHeader.FrameId, frameId, 32);
	copy_string_n(dpxh->FilmHeader.SlateInfo, slateInfo, 100);

	dpxh->TvHeader.TimeCode = timeCode;
	dpxh->TvHeader.UserBits = userBits;
	dpxh->TvHeader.Interlace = interlace;
	dpxh->TvHeader.FieldNumber = fieldNumber;
	dpxh->TvHeader.VideoSignal = videoSignal;
	SET_FLOAT(dpxh->TvHeader.HorzSampleRate, horzSampleRate);
	SET_FLOAT(dpxh->TvHeader.VertSampleRate, vertSampleRate);
	SET_FLOAT(dpxh->TvHeader.FrameRate, tvFrameRate);
	SET_FLOAT(dpxh->TvHeader.TimeOffset, timeOffset);
	SET_FLOAT(dpxh->TvHeader.Gamma, tvGamma);
	SET_FLOAT(dpxh->TvHeader.BlackLevel, blackLevel);
	SET_FLOAT(dpxh->TvHeader.BlackGain, blackGain);
	SET_FLOAT(dpxh->TvHeader.Breakpoint, breakpoint);
	SET_FLOAT(dpxh->TvHeader.WhiteLevel, whiteLevel);
	SET_FLOAT(dpxh->TvHeader.IntegrationTimes, integrationTimes);
	dpxh->TvHeader.VideoIdentificationCode = videoIdentificationCode;
	dpxh->TvHeader.SMPTETCType = smpteTCType;
	dpxh->TvHeader.SMPTETCDBB2 = smpteTCDBB2;
}


/*!
 ************************************************************************
 * \brief
 *    parse_cfgfile() - loop over a file's lines and process
 *
 * \param fn
 *    Config file filename
 * \param cmdargs
 *    Command argument structure
 *
 ************************************************************************
 */
static int parse_cfgfile (char* fn, cmdarg_t *cmdargs)
{
	FILE *fd;
    char line[CFGLINE_LEN+1] = "";

    if (NULL == fn)
    {
        Err("%s called with NULL file name", __FUNCTION__);
    }
    if (0 == fn[0])
    {
        Err("empty configuration file name");
    }
    fd = fopen(fn, "r");
    if (NULL == fd)
    {
        PErr("cannot open file '%s'", fn);
    }
    fn[0] = '\0';
    while (NULL != fgets(line, CFGLINE_LEN, fd)) // we re-use the storage
    {
        assign_line(line, cmdargs);    
    }
    if (!feof(fd))
    {
        PErr("while reading from file '%s'", fn);
    }
    fclose(fd);
    return 0;
}


/*!
 ************************************************************************
 * \brief
 *    assign_line() - process a single line
 *
 * \param line
 *    String containing line
 * \param cmdargs
 *    Command argument structure
 *
 ************************************************************************
 */
static int assign_line (char* line, cmdarg_t *cmdargs)
{
    if (!parse_line (line, cmdargs))
    {
        UErr("unknown configuration field '%s'", line);
    }
    if ('\0' != filepath[0])   // config file
    {
        parse_cfgfile(filepath, cmdargs);
    }
    return 0;
}

/*!
 ************************************************************************
 * \brief
 *    usage() - Print the usage message
 ************************************************************************
 */
void usage(void)
{
	printf("Usage: conv_format <options>\n");
	printf(" Option list:\n");
	printf("  -help => print this message\n");
	printf("  -F <cfg_file> => Specify configuration file (required)\n");
	printf("  -O\"PARAMETER <value>\" => override config file parameter PARAMETER with new value\n");
	printf("  See README.txt for a list of parameters.\n");
	exit(1);
}

/*!
 ************************************************************************
 * \brief
 *    process_args() - set defaults and process command line
 *
 * \param argc
 *    Argument count (from main)
 * \param argv
 *    Arguments (from main)
 * \param cmddargs
 *    Command arguemnts structure
 *
 ************************************************************************
 */
static int process_args(int argc, char *argv[], cmdarg_t *cmdargs)
{
    enum arg_order_e
    {
        IN_ARG, OUT_ARG, DONE
    };
    int expected_arg = IN_ARG; // track if we've seen these args
    int incr;
	int i;
	char *arg1;

    if (1 == argc) // no arguments
    {
        usage();
    }

    /* process each argument */
    for (i=1; i<argc; i+=incr)
    {
        arg1 = (i<argc-1)? argv[i+1]: NULL;
        incr = parse_cmd(argv[i], arg1, cmdargs);

        if (incr)
        {
            if (help) // help
            {
                usage();
            }
            if (0 != filepath[0])   // config file
            {
                parse_cfgfile(filepath, cmdargs);
            }
            else if (0 != option[0]) // config line
            {
                assign_line(option, cmdargs);
                option[0] = '\0';
            }
            continue;
        }
        if (argv[i][0]=='-')
        {
            fprintf(stderr, "ERROR: Unknown option '%s'!\n", argv[i]);
            usage();
        }
        incr = 1;
        switch (expected_arg)
        {
        case IN_ARG:  // input file name */
            expected_arg = OUT_ARG;
            strcpy(fn_i, argv[i]);
            break;
        case OUT_ARG:  // output file name */
            expected_arg = DONE;
            strcpy(fn_o, argv[i]);
            break;
        default:  /* input file name */
            fprintf(stderr, "ERROR: Unexpected argument %s (in=%s, out=%s)", argv[i], fn_i, fn_o);
            usage();
        }
    }

    return 0;
}


/*!
 ************************************************************************
 * \brief
 *    split_base_and_ext() - separate base file name and extension
 *
 * \param infname
 *    Filename with (optional) path and extension
 * \param base_name
 *    Allocated string to copy base filename to
 * \param extension
 *    Returns pointer to extension (uses part of base_name storage)
 *
 ************************************************************************
 */
void split_base_and_ext(char *infname, char *base_name, char **extension)
{
	int zz;
	int dot=-1, slash=-1;

	// Find last . and / (or \) in filename
	for (zz = strlen(infname)-1; zz >= 0; --zz)
	{
		if ((slash<0) && ((infname[zz] == '\\') || (infname[zz] == '/')))
			slash = zz;
		if ((dot<0) && (infname[zz] == '.'))
			dot = zz;
	}
	if ((dot < slash) || (dot < 0))
	{
		printf("ERROR: picture format unrecognized\n");
		exit(1);
	}
	strcpy(base_name, &(infname[slash+1]));
	*extension = &(base_name[dot-slash]);
	base_name[dot-slash-1] = '\0';  // terminate base_name string
}


/*!
************************************************************************
* \brief
*    set_alpha_to_1() - overwrites alpha channel for a picture to 1 (all opaque)
*
* \param p
*    Picture to process
*
************************************************************************
*/
void set_alpha_to_1(pic_t *p)
{
	int i, j;

	p->alpha = 1;

	if (p->color == RGB)
	{
		for(i=0; i<p->h; ++i)
			for (j = 0; j < p->w; ++j)
			{
				if (p->bits < 32)
					p->data.rgb.a[i][j] = (1 << p->bits) - 1;
				else if (p->bits == 32)
					p->data.rgb_s.a[i][j] = (float)1.0;
				else  // 64-bit
					p->data.rgb_d.a[i][j] = (double)1.0;
			}
	}
	else if (p->color == YUV_HD || p->color == YUV_SD) {
		for (i = 0; i<p->h; ++i)
			for (j = 0; j < p->w; ++j)
			{
				if (p->bits < 32)
					p->data.yuv.a[i][j] = (1 << p->bits) - 1;
				else if (p->bits == 32)
					p->data.yuv_s.a[i][j] = (float)1.0;
				else  // 64-bit
					p->data.yuv_d.a[i][j] = (double)1.0;
			}
	}  // Don't do anything for GC formats
}



/*!
 ************************************************************************
 * \brief
 *    conv_main() - Mainline
 *
 * \param argc
 *    Argument count (from main)
 * \param argv
 *    Arguments (from main)
 ************************************************************************
 */
#ifdef WIN32
int conv_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	pic_t *ip=NULL, *ip2, *ref_pic, *op;
	char f[PATH_MAX], infname[PATH_MAX];
	char *extension;
	FILE *list_fp, *logfp = NULL;
	char base_name[PATH_MAX];
	int num_frames;
	int multiFrameYuvFile = 0;
	int frameNumber = 0;
	char frmnumstr[20];
	HDRDPXFILEFORMAT hdrdpx_f, hdrdpx_f_written;
	HDRDPXSBMDATA    hdrdpx_sbm;
	HDRDPXUSERDATA   hdrdpx_user;
	int type;

	printf("conv_format HDR DPX reference model version 0.1\n");
	printf("Copyright 2019 Broadcom Inc.  All rights reserved.\n\n");

	// Allocate variables
	//cmd_args[0].var_ptr = rcOffset = (int *)malloc(sizeof(int)*15);

	set_defaults();

	/* process input arguments */
    process_args(argc, argv, cmd_args);

	if (NULL == (list_fp=fopen(fn_i, "rt")))
	{
		fprintf(stderr, "Cannot open list file %s for input\n", fn_i);
		exit(1);
	}

	if (strlen(fn_log) > 0)
	{
		if (NULL == (logfp = fopen(fn_log, "wt")))
		{
			fprintf(stderr, "Cannot open list file log.txt for output\n");
			exit(1);
		}
	}

	if ((use422 || use420) && !useYuvInput)
	{
		fprintf(stderr, "4:2:2/4:2:0 are only supported in YCbCr mode (USE_YUV_INPUT = 1)\n");
		exit(1);
	}

	if (dpxFileOutput && hdrDpxFileOutput)
	{
		fprintf(stderr, "Only one of DPX_FILE_OUTPUT and HDRDPX_FILE_OUTPUT can be enabled\n");
		exit(1);
	}

	infname[0] = '\0';
	if (fgets(infname, 512, list_fp) == NULL)
		UErr("Unexpected end of list file\n");

	while ((strlen(infname)>0) || !feof(list_fp) || multiFrameYuvFile)
	{
		sprintf(frmnumstr, "_%06d", frameNumber);
		ip = NULL;
		while ((strlen(infname)>0) && (infname[strlen(infname)-1] < '0'))
			infname[strlen(infname)-1] = '\0';

		if (strlen(infname)==0)  // Skip blank lines
		{
			infname[0] = '\0';
			if (fgets(infname, 512, list_fp) == NULL)
				break;   // reached end of file
			continue;
		}

		split_base_and_ext(infname, base_name, &extension);

		memset(&hdrdpx_user, 0, sizeof(HDRDPXUSERDATA));
		memset(&hdrdpx_sbm, 0, sizeof(HDRDPXSBMDATA));
		if (!strcmp(extension, "dpx") || !strcmp(extension, "DPX"))
		{
			type = hdr_dpx_determine_file_type(infname);
			if (type == -1)
				UErr("hdr_dpx_determine_file_type failed\n");
			if (type == 1 || type == 2)
			{
				if (dpx_read(infname, &ip, dpxRPadEnds, dpxRForceBe, dpxRDatumOrder, rbSwap | (cbcrSwap << 1)))
				{
					fprintf(stderr, "Error read DPX file %s\n", infname);
					exit(1);
				}
			}
			else {
				if (hdr_dpx_read(infname, &ip, &hdrdpx_f, &hdrdpx_user, &hdrdpx_sbm))
				{
					fprintf(stderr, "Error read HDR DPX file %s\n", infname);
					exit(1);
				}
				if (logHeaderInput)
					print_hdrdpx_header(logfp, infname, &hdrdpx_f, &hdrdpx_user, &hdrdpx_sbm);
				if (udFileOutput)
					write_user_data_file(infname, &hdrdpx_f, &hdrdpx_user);
				if (sbmFileOutput)
					write_sbm_data_file(infname, &hdrdpx_f, &hdrdpx_sbm);
			}
		}
		else if (!strcmp(extension, "ppm") || !strcmp(extension, "PPM"))
		{
			if (useYuvInput)
			{
				printf("PPM format is RGB only, converting source to YUV\n");
			}
			
			if (ppm_read(infname, &ip))
			{
				fprintf(stderr, "Error read PPM file %s\n", infname);
				exit(1);
			}
		}
		else if (!strcmp(extension, "yuv") || !strcmp(extension, "YUV"))
		{
			if (!useYuvInput)
			{
				printf("Warning: YUV format is YUV only, upsampling source to 4:4:4 RGB\n");
			}

			// Right now just reads first frame of a YUV file, could be extended to read multiple frames
#if 1 // OLD CODE
			if (yuv_read(infname, &ip, picWidth, picHeight, frameNumber, bitsPerComponent, &num_frames, yuvFileFormat))
			{
				fprintf(stderr, "Error read YUV file %s\n", infname);
				exit(1);
			}
#else // NEW CODE
			if (yuv_read(infname, &ip, picWidth, picHeight, frameNumber, 16, &num_frames, yuvFileFormat))
			{
				fprintf(stderr, "Error read YUV file %s\n", infname);
				exit(1);
			}
			ip2 = convertbits(ip, bitsPerComponent);
			pdestroy(ip);
			ip = ipi2;
#endif
			multiFrameYuvFile = (frameNumber < num_frames-1);  // will be false for last frame of multi-frame file, which is ok since frameNumber will be nonzero
		}
		else if (!strcmp(extension, "rgba") || !strcmp(extension, "RGBA") || !strcmp(extension, "rgb") || !strcmp(extension, "RGB"))
		{
			if (useYuvInput)
			{
				printf("Warning: RGB will be converted to YUV\n");
			}

			// Right now just reads first frame of a YUV file, could be extended to read multiple frames
			if (rgba_read(infname, &ip, picWidth, picHeight, bitsPerComponent, !strcmp(extension, "rgba") || !strcmp(extension, "RGBA")))
			{
				fprintf(stderr, "Error read YUV file %s\n", infname);
				exit(1);
			}
		}
		else 
		{
			fprintf(stderr, "Unrecognized file format .%s\n", extension);
			exit(1);
		}

		if (ip)
		{
			if (bitsPerComponent != UINT8_MAX && bitsPerComponent > ip->bits)
			{
				printf("WARNING: Source picture bit depth is %d, converting to %d bits\n", ip->bits, bitsPerComponent);
				ip2 = convertbits(ip, bitsPerComponent);
				pdestroy(ip);
				ip = ip2;
		    }
		}

		// 4:2:0 to 4:2:2 conversion
		if (ip && (ip->chroma == YUV_420) && !use420)
		{
			printf("WARNING: Source picture is 4:2:0, converting to 4:2:2\n");
			ip2 = pcreate_ext(FRAME, ip->color, YUV_422, ip->w, ip->h, ip->bits);
			ip2->alpha = ip->alpha;
			yuv_420_422(ip, ip2);
			pdestroy(ip);
			ip = ip2;
		}			

		// 4:2:2 to 4:4:4 coversion
		if (ip && (ip->chroma == YUV_422) && !(use422 || use420))
		{
			printf("WARNING: Converting source picture to 4:4:4\n");
			ip2 = pcreate_ext(FRAME, ip->color, YUV_444, ip->w, ip->h, ip->bits);
			ip2->alpha = ip->alpha;
			yuv_422_444(ip, ip2);
			pdestroy(ip);
			ip = ip2;
		}

		// RGB to YUV conversion
		if (ip && (ip->color == RGB) && useYuvInput)
		{
			printf("WARNING: Source picture is RGB, converting to YCbCr\n");
			ip2 = pcreate_ext(FRAME, YUV_HD, ip->chroma, ip->w, ip->h, ip->bits);
			ip2->alpha = ip->alpha;
			rgb2yuv(ip, ip2);
			pdestroy(ip);
			ip = ip2;
		}
		else if (ip && ((ip->color == YUV_HD) || (ip->color == YUV_SD)) && !useYuvInput)      // YUV to RGB conversion
		{
			printf("WARNING: Source picture is YCbCr, converting to RGB\n");
			ip2 = pcreate_ext(FRAME, RGB, YUV_444, ip->w, ip->h, ip->bits);
			ip2->alpha = ip->alpha;
			yuv2rgb(ip, ip2);
			pdestroy(ip);
			ip = ip2;
		}

		if (ip && (ip->chroma == YUV_444) && (use422 || use420))
		{
			printf("WARNING: Converting source picture to 4:2:2\n");
			ip2 = pcreate_ext(FRAME, ip->color, YUV_422, ip->w, ip->h, ip->bits);
			ip2->alpha = ip->alpha;
			yuv_444_422(ip, ip2);
			pdestroy(ip);
			ip = ip2;
		}

		if (ip && (ip->chroma == YUV_422) && use420)
		{
			printf("WARNING: Converting source picture to 4:2:0\n");
			ip2 = pcreate_ext(FRAME, ip->color, YUV_420, ip->w, ip->h, ip->bits);
			ip2->alpha = ip->alpha;
			yuv_422_420(ip, ip2);
			pdestroy(ip);
			ip = ip2;
		}

		if (ip && (bitsPerComponent < ip->bits))
		{
			printf("WARNING: Source bit depth is %d, converting to %d bits\n", ip->bits, bitsPerComponent);
			ip2 = convertbits(ip, bitsPerComponent);
			pdestroy(ip);
			ip = ip2;
		}

		if (ip && (use422 || use420) && (ip->w%2))
		{
			ip->w--;
			printf("WARNING: 4:2:2 picture width is constrained to be a multiple of 2.\nThe image %s will be cropped to %d pixels wide.\n", base_name, ip->w);
		}

		if (ip && (use420) && (ip->h%2))
		{
			ip->h--;
			printf("WARNING: 4:2:0 picture height is constrained to be a multiple of 2.\nThe image %s will be cropped to %d pixels high.\n", base_name, ip->h);
		}

		ref_pic = ip;
		
		if(use422 && use420)
			UErr("ERROR: USE_420 and USE_422 modes cannot both be enabled at the same time\n");
		if (createAlpha && deleteAlpha)
			UErr("ERROR: CREATE_ALPHA and DELETE_ALPHA are mutually exclusive\n");


		// Set alpha
		if (createAlpha)
			set_alpha_to_1(ip);

		if (deleteAlpha)
			ip->alpha = 0;

		// Initial version copies exact samples from input to output. This could be expanded to support
		// color gamut conversions (e.g., 709 <-> 2020), transfer function conversions, etc.
		op = pcopy(ip);

		if (function!=1)  // Don't write if read only
		{
			if (yuvFileOutput)
			{
				strcpy(f, fn_o);
#ifdef WIN32
				strcat(f, "\\");
#else
				strcat(f, "/");
#endif
				strcat(f, base_name);
				strcat(f, ".out.yuv");
				if (yuv_write(f, op, frameNumber, yuvFileFormat))
				{
					fprintf(stderr, "Error writing YUV file %s\n", f);
					exit(1);
				}
			}
			if (dpxFileOutput)
			{
				strcpy(f, fn_o);
#ifdef WIN32
				strcat(f, "\\");
#else
				strcat(f, "/");
#endif
				strcat(f, base_name);
				if (multiFrameYuvFile || frameNumber > 0)
					strcat(f, frmnumstr);
				strcat(f, ".dpx");
				if (dpx_write(f, op, dpxWPadEnds, dpxWDatumOrder, dpxWForcePacking, rbSwapOut | (cbcrSwapOut << 1), dpxWByteSwap))
				{
					fprintf(stderr, "Error writing DPX file %s\n", f);
					exit(1);
				}
			}
			if (hdrDpxFileOutput)
			{
				strcpy(f, fn_o);
#ifdef WIN32
				strcat(f, "\\");
#else
				strcat(f, "/");
#endif
				strcat(f, base_name);
				if (multiFrameYuvFile || frameNumber > 0)
					strcat(f, frmnumstr);
				strcat(f, ".dpx");
				populate_hdr_dpx_header(infname, &hdrdpx_f, &hdrdpx_user, &hdrdpx_sbm);
				if (hdr_dpx_write(f, op, &hdrdpx_f, &hdrdpx_user, &hdrdpx_sbm, hdrdpxWriteAsBe, &hdrdpx_f_written))
				{
					fprintf(stderr, "Error writing DPX file %s\n", f);
					exit(1);
				}
				if (logHeaderOutput)
					print_hdrdpx_header(logfp, infname, &hdrdpx_f_written, &hdrdpx_user, &hdrdpx_sbm);
			}
			if (ppmFileOutput)
			{
				strcpy(f, fn_o);
#ifdef WIN32
				strcat(f, "\\");
#else
				strcat(f, "/");
#endif
				strcat(f, base_name);
				if (multiFrameYuvFile || frameNumber > 0)
					strcat(f, frmnumstr);
				strcat(f, ".ppm");
				if (ppm_write(f, op))
				{
					fprintf(stderr, "Error writing PPM file %s\n", f);
					exit(1);
				}
			}
		}


		if (ref_pic != ip)
			pdestroy(ref_pic);
		pdestroy(ip);
		pdestroy(op);
		if (hdrdpx_user.UserData != NULL)
			free(hdrdpx_user.UserData);
		if (hdrdpx_sbm.SbmData != NULL)
			free(hdrdpx_sbm.SbmData);

		if (multiFrameYuvFile)
			frameNumber++;		// Increment frame counter
		else
		{
			infname[0] = '\0';
			frameNumber = 0;
			if (fgets(infname, 512, list_fp) == NULL)
				break;  // end of file
		}
	}

	fclose(list_fp);
	if (logfp)
		fclose(logfp);
	return(0);
}
