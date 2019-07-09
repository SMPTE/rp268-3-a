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
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "hdr_dpx.h"


static const uint32_t undefined_4bytes = UINT32_MAX;  /* 4-byte block 0xffffffff used to check if floats are undefined */

#define PRINT_FIELD_U32(c,n,v) if(c || (v)!=UNDEFINED_U32) { fprintf(fp, "%s\t%d\n", n, v); }
#define PRINT_FIELD_U32_HEX(c,n,v) if(c || (v)!=UNDEFINED_U32) { fprintf(fp, "%s\t0x%08x\n", n, v); }
#define PRINT_RO_FIELD_U32(c,n,v) if(c || (v)!=UNDEFINED_U32) { fprintf(fp, "// %s\t%d\n", n, v); }
#define PRINT_RO_FIELD_U32_HEX(c,n,v) if(c || (v)!=UNDEFINED_U32) { fprintf(fp, "// %s\t0x%08x\n", n, v); }
#define PRINT_FIELD_U8(c,n,v) if(c || (v)!=UNDEFINED_U8) { fprintf(fp, "%s\t%d\n", n, v); }
#define PRINT_RO_FIELD_U8(c,n,v) if(c || (v)!=UNDEFINED_U8) { fprintf(fp, "// %s\t%d\n", n, v); }
#define PRINT_FIELD_U16(c,n,v) if(c || (v)!=UNDEFINED_U16) { fprintf(fp, "%s\t%d\n", n, v); }
#define PRINT_FIELD_R32(c,n,v) if(c || memcmp(&v, &undefined_4bytes, 4)!=0) { fprintf(fp, "%s\t%f\n", n, v); }
#define PRINT_FIELD_ASCII(c,n,v,l) if(c || (v[0])!='\0') { char s[l+1]; s[l] = '\0'; memcpy(s, v, l); fprintf(fp, "%s\t%s\n", n, s); }

void print_hdrdpx_header(FILE *fp, char *fname, HDRDPXFILEFORMAT *dpx_file, HDRDPXUSERDATA *dpx_u, HDRDPXSBMDATA *dpx_sbm)
{
	int i;

	fprintf(fp, "\n\n//////////////////////////////////////////////////////////////////\n");
	fprintf(fp, "// File information header for %s\n", fname);
	fprintf(fp, "//////////////////////////////////////////////////////////////////\n");
	PRINT_RO_FIELD_U32_HEX(1, "// Magic", dpx_file->FileHeader.Magic);
	PRINT_FIELD_U32(1, "Image_Offset", dpx_file->FileHeader.ImageOffset);
	PRINT_FIELD_ASCII(1, "// Version", dpx_file->FileHeader.Version, 8);
	PRINT_RO_FIELD_U32(1, "File_Size", dpx_file->FileHeader.FileSize);
	PRINT_FIELD_U32(0, "Ditto_Key", dpx_file->FileHeader.DittoKey);
	PRINT_RO_FIELD_U32(0, "Generic_Size", dpx_file->FileHeader.GenericSize);
	PRINT_RO_FIELD_U32(0, "Industry_Size", dpx_file->FileHeader.IndustrySize);
	PRINT_RO_FIELD_U32(0, "User_Size", dpx_file->FileHeader.UserSize);
	PRINT_FIELD_ASCII(0, "File_Name", dpx_file->FileHeader.FileName, FILE_NAME_SIZE + 1);
	PRINT_FIELD_ASCII(0, "Time_Date", dpx_file->FileHeader.TimeDate, 24);
	PRINT_FIELD_ASCII(0, "Creator", dpx_file->FileHeader.Creator, 100);
	PRINT_FIELD_ASCII(0, "Project", dpx_file->FileHeader.Project, 200);
	PRINT_FIELD_ASCII(0, "Copyright", dpx_file->FileHeader.Copyright, 200);
	PRINT_FIELD_U32(0, "Encrypt_Key", dpx_file->FileHeader.EncryptKey);
	PRINT_FIELD_U32(0, "Standards_Based_Metadata_Offset", dpx_file->FileHeader.StandardsBasedMetadataOffset);
	PRINT_FIELD_U8(1, "Datum_Mapping_Direction", dpx_file->FileHeader.DatumMappingDirection);

	fprintf(fp, "\n// Image information header\n");
	PRINT_FIELD_U16(1, "Orientation", dpx_file->ImageHeader.Orientation);
	PRINT_FIELD_U16(1, "Number_Elements", dpx_file->ImageHeader.NumberElements);
	PRINT_RO_FIELD_U32(1, "Pixels_Per_Line", dpx_file->ImageHeader.PixelsPerLine);
	PRINT_RO_FIELD_U32(1, "Lines_Per_Element", dpx_file->ImageHeader.LinesPerElement);

	for (i = 0; i < dpx_file->ImageHeader.NumberElements; ++i)
	{
		char fld_name[256];
		fprintf(fp, "//// Image element data structure %d\n", i + 1);
		sprintf(fld_name, "Data_Sign_%d", i + 1);		PRINT_FIELD_U32(1, fld_name, dpx_file->ImageHeader.ImageElement[i].DataSign);
		if (dpx_file->ImageHeader.ImageElement[i].BitSize < 32)
		{
			sprintf(fld_name, "Low_Data_%d", i + 1);			PRINT_FIELD_U32(1, fld_name, dpx_file->ImageHeader.ImageElement[i].LowData.d);
		}
		else
		{
			sprintf(fld_name, "Low_Data_%d", i + 1);			PRINT_FIELD_R32(1, fld_name, dpx_file->ImageHeader.ImageElement[i].LowData.f);
		}
		sprintf(fld_name, "Low_Quantity_%d", i + 1);		PRINT_FIELD_R32(0, fld_name, dpx_file->ImageHeader.ImageElement[i].LowQuantity);
		if (dpx_file->ImageHeader.ImageElement[i].BitSize < 32)
		{
			sprintf(fld_name, "High_Data_%d", i + 1);		PRINT_FIELD_U32(1, fld_name, dpx_file->ImageHeader.ImageElement[i].HighData.d);
		}
		else
		{
			sprintf(fld_name, "High_Data_%d", i + 1);		PRINT_FIELD_R32(1, fld_name, dpx_file->ImageHeader.ImageElement[i].HighData.f);
		}
		sprintf(fld_name, "High_Quantity_%d", i + 1);	PRINT_FIELD_R32(0, fld_name, dpx_file->ImageHeader.ImageElement[i].HighQuantity);
		sprintf(fld_name, "Descriptor_%d", i + 1);		PRINT_FIELD_U8(1, fld_name, dpx_file->ImageHeader.ImageElement[i].Descriptor);
		sprintf(fld_name, "Transfer_%d", i + 1);		PRINT_FIELD_U8(1, fld_name, dpx_file->ImageHeader.ImageElement[i].Transfer);
		sprintf(fld_name, "Colorimetric_%d", i + 1);	PRINT_FIELD_U8(1, fld_name, dpx_file->ImageHeader.ImageElement[i].Colorimetric);
		sprintf(fld_name, "BitSize_%d", i + 1);			PRINT_FIELD_U8(1, fld_name, dpx_file->ImageHeader.ImageElement[i].BitSize);
		sprintf(fld_name, "Packing_%d", i + 1);			PRINT_FIELD_U16(1, fld_name, dpx_file->ImageHeader.ImageElement[i].Packing);
		sprintf(fld_name, "Encoding_%d", i + 1);		PRINT_FIELD_U16(1, fld_name, dpx_file->ImageHeader.ImageElement[i].Encoding);
		sprintf(fld_name, "DataOffset_%d", i + 1);		PRINT_RO_FIELD_U32(1, fld_name, dpx_file->ImageHeader.ImageElement[i].DataOffset);
		sprintf(fld_name, "End_Of_Line_Padding_%d", i + 1); PRINT_FIELD_U32(0, fld_name, dpx_file->ImageHeader.ImageElement[i].EndOfLinePadding);
		sprintf(fld_name, "End_Of_Image_Padding_%d", i + 1); PRINT_FIELD_U32(0, fld_name, dpx_file->ImageHeader.ImageElement[i].EndOfImagePadding);
		sprintf(fld_name, "Description_%d", i + 1);		PRINT_FIELD_ASCII(0, fld_name, dpx_file->ImageHeader.ImageElement[i].Description, 32);
		sprintf(fld_name, "Chroma_Subsampling_%d", i + 1); PRINT_FIELD_U8(0, fld_name, 0xf & (dpx_file->ImageHeader.ChromaSubsampling >> (i*4)));
	}

	fprintf(fp, "\n// Image source information header\n");
	PRINT_FIELD_U32(0, "X_Offset", dpx_file->SourceInfoHeader.XOffset);
	PRINT_FIELD_U32(0, "Y_Offset", dpx_file->SourceInfoHeader.YOffset);
	PRINT_FIELD_R32(0, "X_Center", dpx_file->SourceInfoHeader.XCenter);
	PRINT_FIELD_R32(0, "Y_Center", dpx_file->SourceInfoHeader.YCenter);
	PRINT_FIELD_U32(0, "X_Original_Size", dpx_file->SourceInfoHeader.XOriginalSize);
	PRINT_FIELD_U32(0, "Y_Original_Size", dpx_file->SourceInfoHeader.YOriginalSize);
	PRINT_FIELD_ASCII(0, "Source_File_Name", dpx_file->SourceInfoHeader.SourceFileName, FILE_NAME_SIZE + 1);
	PRINT_FIELD_ASCII(0, "Source_Time_Date", dpx_file->SourceInfoHeader.SourceTimeDate, 24);
	PRINT_FIELD_ASCII(0, "Input_Name", dpx_file->SourceInfoHeader.InputName, 32);
	PRINT_FIELD_ASCII(0, "Input_SN", dpx_file->SourceInfoHeader.InputSN, 32);
	PRINT_FIELD_U16(0, "Border_XL", dpx_file->SourceInfoHeader.Border[0]);
	PRINT_FIELD_U16(0, "Border_XR", dpx_file->SourceInfoHeader.Border[1]);
	PRINT_FIELD_U16(0, "Border_YT", dpx_file->SourceInfoHeader.Border[2]);
	PRINT_FIELD_U16(0, "Border_YB", dpx_file->SourceInfoHeader.Border[3]);
	PRINT_FIELD_U32(0, "Aspect_Ratio_H", dpx_file->SourceInfoHeader.AspectRatio[0]);
	PRINT_FIELD_U32(0, "Aspect_Ratio_V", dpx_file->SourceInfoHeader.AspectRatio[1]);

	fprintf(fp, "\n// Motion-picture film information header\n");
	PRINT_FIELD_ASCII(0, "Film_Mfg_Id", dpx_file->FilmHeader.FilmMfgId, 2);
	PRINT_FIELD_ASCII(0, "Film_Type", dpx_file->FilmHeader.FilmType, 2);
	PRINT_FIELD_ASCII(0, "Offset_Perfs", dpx_file->FilmHeader.OffsetPerfs, 2);
	PRINT_FIELD_ASCII(0, "Prefix", dpx_file->FilmHeader.Prefix, 6);
	PRINT_FIELD_ASCII(0, "Count", dpx_file->FilmHeader.Count, 4);
	PRINT_FIELD_ASCII(0, "Format", dpx_file->FilmHeader.Format, 32);
	PRINT_FIELD_U32(0, "Frame_Position", dpx_file->FilmHeader.FramePosition);
	PRINT_FIELD_U32(0, "Sequence_Len", dpx_file->FilmHeader.SequenceLen);
	PRINT_FIELD_U32(0, "Held_Count", dpx_file->FilmHeader.HeldCount);
	PRINT_FIELD_R32(0, "MP_Frame_Rate", dpx_file->FilmHeader.FrameRate);
	PRINT_FIELD_R32(0, "Shutter_Angle", dpx_file->FilmHeader.ShutterAngle);
	PRINT_FIELD_ASCII(0, "Frame_Id", dpx_file->FilmHeader.FrameId, 32);
	PRINT_FIELD_ASCII(0, "Slate_Info", dpx_file->FilmHeader.SlateInfo, 100);

	fprintf(fp, "\n// Television information header\n");
	PRINT_FIELD_U32_HEX(0, "Time_Code", dpx_file->TvHeader.TimeCode);
	PRINT_FIELD_U32_HEX(0, "User_Bits", dpx_file->TvHeader.UserBits);
	PRINT_FIELD_U8(0, "Interlace", dpx_file->TvHeader.Interlace);
	PRINT_FIELD_U8(0, "Field_Number", dpx_file->TvHeader.FieldNumber);
	PRINT_FIELD_U8(0, "Video_Signal", dpx_file->TvHeader.VideoSignal);
	PRINT_FIELD_R32(0, "Horz_Sample_Rate", dpx_file->TvHeader.HorzSampleRate);
	PRINT_FIELD_R32(0, "Vert_Sample_Rate", dpx_file->TvHeader.VertSampleRate);
	PRINT_FIELD_R32(0, "TV_Frame_Rate", dpx_file->TvHeader.FrameRate);
	PRINT_FIELD_R32(0, "Time_Offset", dpx_file->TvHeader.TimeOffset);
	PRINT_FIELD_R32(0, "Gamma", dpx_file->TvHeader.Gamma);
	PRINT_FIELD_R32(0, "Black_Level", dpx_file->TvHeader.BlackLevel);
	PRINT_FIELD_R32(0, "Black_Gain", dpx_file->TvHeader.BlackGain);
	PRINT_FIELD_R32(0, "Breakpoint", dpx_file->TvHeader.Breakpoint);
	PRINT_FIELD_R32(0, "White_Level", dpx_file->TvHeader.WhiteLevel);
	PRINT_FIELD_R32(0, "Integration_Times", dpx_file->TvHeader.IntegrationTimes);
	PRINT_FIELD_U8(0, "Video_Identification_Code", dpx_file->TvHeader.VideoIdentificationCode);
	PRINT_FIELD_U8(0, "SMPTE_TC_Type", dpx_file->TvHeader.SMPTETCType);
	PRINT_FIELD_U8(0, "SMPTE_TC_DBB2", dpx_file->TvHeader.SMPTETCDBB2);

	if(dpx_u)
	{	
		if (dpx_file->FileHeader.UserSize > 0 && dpx_file->FileHeader.UserSize != UINT32_MAX)
		{
			fprintf(fp, "\n// User data header\n");
			PRINT_FIELD_ASCII(0, "User_Identification", dpx_u->UserIdentification, 32);
		}
	}

	if (dpx_sbm)
	{
		if (dpx_sbm->SbmLength > 0)
		{
			fprintf(fp, "\n// Standards-based metadata header\n");
			PRINT_FIELD_ASCII(0, "Sbm_Descriptor", dpx_sbm->SbmFormatDescriptor, 128);
			PRINT_FIELD_U32(0, "Sbm_Length", dpx_sbm->SbmLength);
		}
	}
}





