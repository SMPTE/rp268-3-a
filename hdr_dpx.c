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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "vdo.h"
#include "utl.h"
#include "hdr_dpx.h"
#include "cmd_parse.h"
#include "fifo.h"

#define READ_DPX_32(x)   (bswap ? (((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | ((x) >> 24))) : (x))
#define READ_DPX_16(x)   (bswap ? ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8)) : (x))
//#define SINGLE_BS(x)   data32b = *((DWORD *)(&(x)));  *((DWORD *)(&(x))) = READ_DPX_32(data32b);
//#define SINGLE_BS(x)   { char t; union { SINGLE s; BYTE b[4]; } d; d.s = (x); t=d.b[0]; d.b[0]=d.b[3]; d.b[3]=t; t=d.b[1]; d.b[1]=d.b[2]; d.b[2]=t; (x)=d.s; } 
//#define BYTE_SWAP(x)  if(bswap) x = (((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | (x >> 24));

#define BS_32(f,x)   { uint8_t *f_ptr; int o; uint8_t t; f_ptr=(uint8_t *)&(f); o=offsetof(HDRDPXFILEFORMAT,x); t=f_ptr[o]; f_ptr[o]=f_ptr[o+3]; f_ptr[o+3]=t; t=f_ptr[o+1]; f_ptr[o+1]=f_ptr[o+2]; f_ptr[o+2]=t; }
#define BS_16(f,x)   { uint8_t *f_ptr; int o; uint8_t t; f_ptr=(uint8_t *)&(f); o=offsetof(HDRDPXFILEFORMAT,x); t=f_ptr[o]; f_ptr[o]=f_ptr[o+1]; f_ptr[o]=t; }




static const uint32_t undefined_4bytes = UINT32_MAX;  /* 4-byte block 0xffffffff used to check if floats are undefined */

// local data structures:
typedef struct rle_ll_s
{
	uint32_t *d;
	uint8_t f;
	uint8_t eol_flag;
	int32_t count;
	struct rle_ll_s *next;
} rle_ll_t;

typedef struct datum_ptrs_s
{
	int **datum[8][8];
	int **alt_chroma[8];
	int ndatum[8];
	int stride[8][8], offset[8][8];
	int wbuff[8];
	int hbuff[8];
} datum_ptrs_t;



void hdr_dpx_rle_encode(datum_list_t *dlist, int d_per_pixel)
{
	int i, p;
	int n, nn;
	rle_ll_t *head, *cur, *prev;
	unsigned char eol;
	int same, match, diff;
	rle_ll_t ref;
	int maxrun;
	datum_list_t dlist_out;
	int yidx = 0;

	head = cur = (rle_ll_t *)malloc(sizeof(rle_ll_t));

	dlist_out.ndatum = 0;
	dlist_out.bit_depth = dlist->bit_depth;
	n = 0;
	maxrun = (1 << (dlist->bit_depth - 1)) - 1;
	ref.d = (uint32_t *)malloc(sizeof(uint32_t) * d_per_pixel);

	while (n < dlist->ndatum)
	{
		// populate datum values
		cur->d = (uint32_t *)malloc(sizeof(uint32_t) * d_per_pixel);
		for (i = 0; i < d_per_pixel; ++i)
			cur->d[i] = dlist->d[n++];
		eol = dlist->eol_flag[n - 1];

		// Determine how many subsequent pixels are identical
		nn = n;
		same = 0;
		diff = 1;
		match = 1;
		while (nn < dlist->ndatum && match && same < maxrun - 1 && !eol)
		{
			for (i = 0; i < d_per_pixel; ++i)
			{
				if (cur->d[i] != dlist->d[nn])
				{
					match = 0;
				}
				nn++;
			}
			eol = dlist->eol_flag[nn - 1];
			if (match)
			{
				same++;
			}
		}
		if (!match)		// If we terminated early, EOL indication has to be backed up a step
			eol = dlist->eol_flag[nn - 4];

		// Determine how many subsequent pixels are different
		if (same == 0 && !dlist->eol_flag[n - 1])
		{
			if (n > 0)
				eol = dlist->eol_flag[n - 1];  // run of 1 pixel if end of line
			else
				eol = 0;
			for (i = 0; i < d_per_pixel; ++i)
				ref.d[i] = cur->d[i];
			nn = n;
			while (nn < dlist->ndatum && !match && diff < maxrun && !eol)
			{
				match = 1;
				for (i = 0; i < d_per_pixel; ++i)
				{
					if (ref.d[i] != dlist->d[nn + i])
						match = 0;
					eol = dlist->eol_flag[nn];
					ref.d[i] = dlist->d[nn++];
				}
				if (!match || eol)  // Treat a single match at the end of a line as different
					diff++;
			}
		}

		if (same)
		{
			cur->count = same + 1;
			cur->f = 1;
			n += same * d_per_pixel;
			cur->eol_flag = eol;
			dlist_out.ndatum += 1 + d_per_pixel;
		}
		else
		{
			cur->d = realloc(cur->d, sizeof(int *) * d_per_pixel * diff);
			cur->count = diff;
			cur->f = 0;
			cur->eol_flag = eol;
			for (p = 1; p < diff; ++p)
			{
				for (i = 0; i < d_per_pixel; ++i)
				{
					cur->d[p*d_per_pixel + i] = dlist->d[n++];
				}
			}
			dlist_out.ndatum += 1 + (d_per_pixel * diff);
		}
		//printf("%d: Run flag = %d, count = %d\n", yidx, cur->f, cur->count);
		if (cur->eol_flag)
			yidx++;
		//if (yidx == 328)
		//	printf("debug\n");
		if (cur->count > maxrun || cur->count == 0)
		{
			fprintf(stderr, "hdr_dpx_rle_encode(): something went wrong\n");
			exit(1);
		}
		cur->next = (rle_ll_t *)malloc(sizeof(rle_ll_t));
		cur = cur->next;
		cur->next = NULL;  // last element with NULL pointer won't have valid data
	}

	dlist_out.d = (uint32_t *)malloc(sizeof(uint32_t) * dlist_out.ndatum);
	dlist_out.eol_flag = (uint8_t *)malloc(sizeof(uint8_t) * dlist_out.ndatum);

	cur = head;
	n = 0;
	while (cur->next != NULL)
	{
		dlist_out.d[n] = (cur->count << 1) | cur->f;
		dlist_out.eol_flag[n++] = 0;
		for (p = 0; p < (cur->f ? 1 : cur->count); ++p)
		{
			for (i = 0; i < d_per_pixel; ++i)
			{
				dlist_out.d[n] = cur->d[p*d_per_pixel + i];
				dlist_out.eol_flag[n++] = 0;
			}
		}
		if (cur->eol_flag)
			dlist_out.eol_flag[n - 1] = 1;

		free(cur->d);
		prev = cur;
		cur = cur->next;
		free(prev);
	}
	free(cur);

	if (n != dlist_out.ndatum)
	{
		fprintf(stderr, "hdr_dpx_rle_encode(): something went wrong\n");
		exit(1);
	}

	free(dlist->d);
	free(dlist->eol_flag);
	(*dlist) = dlist_out;
}

// Note that some valid DPX files are rejected by this function because of the constraints imposed by mapping to the pic_t structure
int hdr_dpx_create_pic(HDRDPXFILEFORMAT *f, pic_t **out_pic)
{
	int i;
	pic_t *p;
	int      alpha; // 0: not used, 1: used (same resolution as Y)
	int w, h;
	int bits;
	format_t format;
	color_t  color;
	chroma_t chroma;
	int chroma_siting;

	chroma = YUV_444;
	color = UNDEFINED_COLOR;
	alpha = 0;
	w = f->ImageHeader.PixelsPerLine;
	h = f->ImageHeader.LinesPerElement;
	bits = f->ImageHeader.ImageElement[0].BitSize;
	if (f->TvHeader.Interlace == 1)
		format = f->TvHeader.FieldNumber;
	else
		format = FRAME;
	chroma_siting = 0;

	for (i = 0; i < f->ImageHeader.NumberElements; ++i)
	{
		if (f->ImageHeader.ImageElement[i].BitSize != bits)
		{
			fprintf(stderr, "hdr_dpx_create_pic(): This implementation does not support different bit depths for different image elements\n");
			return(DPX_ERROR_PIC_MAP_FAILED);
		}
		if(f->ImageHeader.ImageElement[i].Transfer != f->ImageHeader.ImageElement[0].Transfer)
		{
			fprintf(stderr, "hdr_dpx_create_pic(): This implementation does not support different transfer functions for different image elements\n");
			return(DPX_ERROR_PIC_MAP_FAILED);
		}
		if (f->ImageHeader.ImageElement[i].Colorimetric != f->ImageHeader.ImageElement[0].Colorimetric)
		{
			fprintf(stderr, "hdr_dpx_create_pic(): This implementation does not support different colorimetry for different image elements\n");
			return(DPX_ERROR_PIC_MAP_FAILED);
		}

		switch (f->ImageHeader.ImageElement[i].Descriptor)
		{
		case HDRDPX_DESC_USER:
		case HDRDPX_DESC_Z:
		case HDRDPX_DESC_COMP:
			if (color == UNDEFINED_COLOR)
				color = G_1C;
			else if (color >= G_1C && color <= G_7C)
				color++;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): too many generic components\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_R:
		case HDRDPX_DESC_G:
		case HDRDPX_DESC_B:
		case HDRDPX_DESC_RGB_OLD:
		case HDRDPX_DESC_BGR:
		case HDRDPX_DESC_RGB:
			if (color == UNDEFINED_COLOR || color == RGB)
				color = RGB;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): incompatible image element types\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			if (chroma != YUV_444 && chroma != YUV_4444)
			{
				fprintf(stderr, "hdr_dpx_create_pic(): incompatible image element types\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_A:
			alpha = 1;
			if (chroma == YUV_444)
				chroma = YUV_4444;
			break;
		case HDRDPX_DESC_Y:
		case HDRDPX_DESC_CbYCr:
			if (color == UNDEFINED_COLOR || color == YUV_HD)
				color = YUV_HD;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): Y component encountered for RGB picture\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_CbCr:
		case HDRDPX_DESC_CbYCrY:
			if (color == UNDEFINED_COLOR || color == YUV_HD)
				color = YUV_HD;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): CbCr 4:2:2 component encountered for RGB picture\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			chroma = YUV_422;
			chroma_siting = (f->ImageHeader.ChromaSubsampling >> (4 * i)) & 0xf;
			break;
		case HDRDPX_DESC_Cb:
		case HDRDPX_DESC_Cr:
		case HDRDPX_DESC_CYY:
			if (color == UNDEFINED_COLOR || color == YUV_HD)
				color = YUV_HD;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): Cb or Cr 4:2:0 component encountered for RGB picture\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			chroma = YUV_420;
			chroma_siting = (f->ImageHeader.ChromaSubsampling >> (4 * i)) & 0xf;
			break;
		case HDRDPX_DESC_RGBA_OLD:
		case HDRDPX_DESC_ABGR_OLD:
		case HDRDPX_DESC_BGRA:
		case HDRDPX_DESC_ARGB:
		case HDRDPX_DESC_RGBA:
		case HDRDPX_DESC_ABGR:
			if (color == UNDEFINED_COLOR || color == RGB)
				color = RGB;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): incompatible image element types\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			if (chroma != YUV_444 && chroma != YUV_4444)
			{
				fprintf(stderr, "hdr_dpx_create_pic(): incompatible image element types\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			chroma = YUV_4444;
			alpha = 1;
			break;
		case HDRDPX_DESC_CbYACrYA:
			if (color == UNDEFINED_COLOR || color == YUV_HD)
				color = YUV_HD;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): CbCr 4:2:2 component encountered for RGB picture\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			chroma = YUV_422;
			alpha = 1;
			chroma_siting = (f->ImageHeader.ChromaSubsampling >> (4 * i)) & 0xf;
			break;
		case HDRDPX_DESC_CbYCrA:
			if (color == UNDEFINED_COLOR || color == YUV_HD)
				color = YUV_HD;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): Y component encountered for RGB picture\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			alpha = 1;
			break;
		case HDRDPX_DESC_CYAYA:
			if (color == UNDEFINED_COLOR || color == YUV_HD)
				color = YUV_HD;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): Cb or Cr 4:2:0 component encountered for RGB picture\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			chroma = YUV_420;
			alpha = 1;
			chroma_siting = (f->ImageHeader.ChromaSubsampling >> (4 * i)) & 0xf;
			break;
		case HDRDPX_DESC_G2C:
			if (color == UNDEFINED_COLOR)
				color = G_2C;
			else if (color >= G_1C && color <= G_6C)
				color += 2;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): too many generic components\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_G3C:
			if (color == UNDEFINED_COLOR)
				color = G_3C;
			else if (color >= G_1C && color <= G_5C)
				color += 3;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): too many generic components\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_G4C:
			if (color == UNDEFINED_COLOR)
				color = G_4C;
			else if (color >= G_1C && color <= G_4C)
				color += 4;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): too many generic components\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_G5C:
			if (color == UNDEFINED_COLOR)
				color = G_5C;
			else if (color >= G_1C && color <= G_3C)
				color += 5;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): too many generic components\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_G6C:
			if (color == UNDEFINED_COLOR)
				color = G_6C;
			else if (color >= G_1C && color <= G_2C)
				color += 6;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): too many generic components\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_G7C:
			if (color == UNDEFINED_COLOR)
				color = G_7C;
			else if (color == G_1C)
				color = G_8C;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): too many generic components\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		case HDRDPX_DESC_G8C:
			if (color == UNDEFINED_COLOR)
				color = G_8C;
			else
			{
				fprintf(stderr, "hdr_dpx_create_pic(): unsupported image element configuration\n");
				return(DPX_ERROR_PIC_MAP_FAILED);
			}
			break;
		}
	}
	p = pcreate_ext(format, color, chroma, w, h, bits);

	switch (f->ImageHeader.ImageElement[0].Transfer)
	{
	case HDRDPX_TF_USER:
		p->transfer = TF_USER;
		break;
	case HDRDPX_TF_DENSITY:
		p->transfer = TF_DENSITY;
		break;
	case HDRDPX_TF_LINEAR:
		p->transfer = TF_LINEAR;
		break;
	case HDRDPX_TF_LOGARITHMIC:
		p->transfer = TF_LOG;
		break;
	case HDRDPX_TF_UNSPECIFIED_VIDEO:
		p->transfer = TF_UNSPEC_VID;
		break;
	case HDRDPX_TF_ST_274:
	case HDRDPX_TF_BT_709:
	case HDRDPX_TF_BT_601_625:
	case HDRDPX_TF_BT_601_525:
	case HDRDPX_TF_COMPOSITE_NTSC:
	case HDRDPX_TF_COMPOSITE_PAL:
	case HDRDPX_TF_IEC_61966_2_4:
		p->transfer = TF_BT_709;
		break;
	case HDRDPX_TF_Z_LINEAR:
		p->transfer = TF_Z_LINEAR;
		break;
	case HDRDPX_TF_Z_HOMOGENOUS:
		p->transfer = TF_Z_HOMOG;
		break;
	case HDRDPX_TF_ST_2065_3:
		p->transfer = TF_ADX;
		break;
	case HDRDPX_TF_BT_2020_NCL:
		p->transfer = TF_BT_2020_NCL;
		break;
	case HDRDPX_TF_BT_2020_CL:
		p->transfer = TF_BT_2020_CL;
		break;
	case HDRDPX_TF_BT_2100_PQ_NCL:
		p->transfer = TF_BT_2100_PQ_NCL;
		break;
	case HDRDPX_TF_BT_2100_PQ_CI:
		p->transfer = TF_BT_2100_PQ_CI;
		break;
	case HDRDPX_TF_BT_2100_HLG_NCL:
		p->transfer = TF_BT_2100_HLG_NCL;
		break;
	case HDRDPX_TF_BT_2100_HLG_CI:
		p->transfer = TF_BT_2100_HLG_CI;
		break;
	case HDRDPX_TF_RP_231_2:
		p->transfer = TF_P3_GAMMA;
		break;
	case HDRDPX_TF_IEC_61966_2_1:
		p->transfer = TF_SRGB;
		break;
	default:
		p->transfer = UNDEFINED_TRANSFER;
	}

	switch (f->ImageHeader.ImageElement[0].Colorimetric)
	{
	case HDRDPX_C_USER:
		p->colorimetry = C_USER;
		break;
	case HDRDPX_C_DENSITY:
		p->colorimetry = C_DENSITY;
		break;
	case HDRDPX_C_UNSPECIFIED_VIDEO:
		p->colorimetry = C_UNSPEC_VID;
		break;
	case HDRDPX_C_ST_274:
	case HDRDPX_C_BT_709:
		p->colorimetry = C_REC_709;
		break;
	case HDRDPX_C_BT_601_625:
	case HDRDPX_C_COMPOSITE_PAL:
		p->colorimetry = C_REC_601_625;
		break;
	case HDRDPX_C_BT_601_525:
	case HDRDPX_C_COMPOSITE_NTSC:
		p->colorimetry = C_REC_601_525;
		break;
	case HDRDPX_C_ST_2065_3:
		p->colorimetry = C_ADX;
		break;
	case HDRDPX_C_BT_2020:
		p->colorimetry = C_REC_2020;
		break;
	case HDRDPX_C_ST_2113_P3D65:
		p->colorimetry = C_P3D65;
		break;
	case HDRDPX_C_ST_2113_P3DCI:
		p->colorimetry = C_P3DCI;
		break;
	case HDRDPX_C_ST_2113_P3D60:
		p->colorimetry = C_P3D60;
		break;
	case HDRDPX_C_ST_2065_1_ACES:
		p->colorimetry = C_ACES;
		break;
	default:
		p->colorimetry = UNDEFINED_GAMUT;
	}
	if(f->SourceInfoHeader.AspectRatio[0] != UINT32_MAX)
		p->ar1 = f->SourceInfoHeader.AspectRatio[0];
	if(f->SourceInfoHeader.AspectRatio[1] != UINT32_MAX)
		p->ar2 = f->SourceInfoHeader.AspectRatio[1];
	if(f->FilmHeader.FramePosition != UINT32_MAX)
		p->frm_no = f->FilmHeader.FramePosition;
	if(f->FilmHeader.SequenceLen != UINT32_MAX)
		p->seq_len = f->FilmHeader.SequenceLen;
	if(memcmp(&(f->FilmHeader.FrameRate), &undefined_4bytes, 4) == 0)
		p->framerate = f->FilmHeader.FrameRate;
	else if (memcmp(&(f->TvHeader.FrameRate), &undefined_4bytes, 4) == 0)
		p->framerate = f->TvHeader.FrameRate;
	else
		p->framerate = 0;  // Unknown
	if(f->TvHeader.Interlace != UINT8_MAX)
		p->interlaced = f->TvHeader.Interlace;
	if (f->ImageHeader.ImageElement[0].BitSize >= 32)
		p->limited_range = 0;
	else
		p->limited_range = f->ImageHeader.ImageElement[0].LowData.d > ((unsigned int)15 << (f->ImageHeader.ImageElement[0].BitSize - 8));
	if(chroma_siting != 0xf)
		p->chroma_siting = chroma_siting;
	p->alpha = alpha;

	*out_pic = p;
	return(0);
}


// TBD: Drop all the bit size if's in this function:
int hdr_dpx_map_datum_to_pic(HDRDPXFILEFORMAT *f, pic_t *p, datum_ptrs_t *dptr)
{
	int i, j;

	for (i = 0; i < f->ImageHeader.NumberElements; ++i)
	{
		dptr->wbuff[i] = p->w;
		dptr->hbuff[i] = p->h;
		dptr->alt_chroma[i] = NULL;
		for (j = 0; j < 8; ++j)
		{
			dptr->offset[i][j] = 0;
			dptr->stride[i][j] = 1;
		}
		switch (f->ImageHeader.ImageElement[i].Descriptor)
		{
		case HDRDPX_DESC_USER:		// generic 1 component
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.gc[i];
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.gc_s[i];
			else
				dptr->datum[i][0] = (int **)p->data.gc_d[i];
			dptr->ndatum[i] = 1;
			break;
		case HDRDPX_DESC_R:		// Red
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.rgb.r;
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.rgb_s.r;
			else
				dptr->datum[i][0] = (int **)p->data.rgb_d.r;
			dptr->ndatum[i] = 1;
			break;
		case HDRDPX_DESC_G:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.rgb.g;
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.rgb_s.g;
			else
				dptr->datum[i][0] = (int **)p->data.rgb_d.g;
			dptr->ndatum[i] = 1;
			break;
		case HDRDPX_DESC_B:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.rgb.b;
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.rgb_s.b;
			else
				dptr->datum[i][0] = (int **)p->data.rgb_d.b;
			dptr->ndatum[i] = 1;
			break;
		case HDRDPX_DESC_A:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = (p->color == RGB) ? p->data.rgb.a : p->data.yuv.a;
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (p->color == RGB) ? (int **)p->data.rgb_s.a : (int **)p->data.yuv_s.a;
			else
				dptr->datum[i][0] = (p->color == RGB) ? (int **)p->data.rgb_d.a : (int **)p->data.yuv_d.a;
			break;
		case HDRDPX_DESC_Y:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.yuv.y;
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.yuv_s.y;
			else
				dptr->datum[i][0] = (int **)p->data.yuv_d.y;
			dptr->ndatum[i] = 1;
			break;
		case HDRDPX_DESC_CbCr:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.yuv.u;
				dptr->datum[i][1] = p->data.yuv.v;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
				dptr->datum[i][1] = (int **)p->data.yuv_s.v;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
				dptr->datum[i][1] = (int **)p->data.yuv_d.v;
			}
			dptr->ndatum[i] = 2;
			dptr->wbuff[i] /= 2;
			break;
		case HDRDPX_DESC_Z:
		case HDRDPX_DESC_COMP:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.gc[0];
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.gc_s[0];
			else
				dptr->datum[i][0] = (int **)p->data.gc_d[0];
			dptr->ndatum[i] = 1;
			break;
		case HDRDPX_DESC_Cb:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.yuv.u;
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
			else
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
			dptr->ndatum[i] = 1;
			dptr->wbuff[i] /= 2;
			dptr->hbuff[i] /= 2;
			break;
		case HDRDPX_DESC_Cr:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
				dptr->datum[i][0] = p->data.yuv.v;
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
				dptr->datum[i][0] = (int **)p->data.yuv_s.v;
			else
				dptr->datum[i][0] = (int **)p->data.yuv_d.v;
			dptr->ndatum[i] = 1;
			dptr->wbuff[i] /= 2;
			dptr->hbuff[i] /= 2;
			break;
		case HDRDPX_DESC_RGB_OLD:
			fprintf(stderr, "hdr_dpx_write() WARNING: descriptor 50 has differing interpretations among implementations. Suggest to use 53 or 56 instead\n");
		case HDRDPX_DESC_BGR:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.rgb.b;
				dptr->datum[i][1] = p->data.rgb.g;
				dptr->datum[i][2] = p->data.rgb.r;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.rgb_s.b;
				dptr->datum[i][1] = (int **)p->data.rgb_s.g;
				dptr->datum[i][2] = (int **)p->data.rgb_s.r;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.rgb_d.b;
				dptr->datum[i][1] = (int **)p->data.rgb_d.g;
				dptr->datum[i][2] = (int **)p->data.rgb_d.r;
			}
			dptr->ndatum[i] = 3;
			break;
		case HDRDPX_DESC_RGBA_OLD:
			fprintf(stderr, "hdr_dpx_write() WARNING: descriptor 51 has differing interpretations among implementations. Suggest to use 54, 55, 57, or 58.\n");
		case HDRDPX_DESC_BGRA:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.rgb.b;
				dptr->datum[i][1] = p->data.rgb.g;
				dptr->datum[i][2] = p->data.rgb.r;
				dptr->datum[i][3] = p->data.rgb.a;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.rgb_s.b;
				dptr->datum[i][1] = (int **)p->data.rgb_s.g;
				dptr->datum[i][2] = (int **)p->data.rgb_s.r;
				dptr->datum[i][3] = (int **)p->data.rgb_s.a;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.rgb_d.b;
				dptr->datum[i][1] = (int **)p->data.rgb_d.g;
				dptr->datum[i][2] = (int **)p->data.rgb_d.r;
				dptr->datum[i][3] = (int **)p->data.rgb_d.a;
			}
			dptr->ndatum[i] = 4;
			break;
		case HDRDPX_DESC_ABGR_OLD:
			fprintf(stderr, "hdr_dpx_write() WARNING: descriptor 52 has differing interpretations among implementations. Suggest to use 54, 55, 57, or 58.\n");
		case HDRDPX_DESC_ARGB:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.rgb.a;
				dptr->datum[i][1] = p->data.rgb.r;
				dptr->datum[i][2] = p->data.rgb.g;
				dptr->datum[i][3] = p->data.rgb.b;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.rgb_s.a;
				dptr->datum[i][1] = (int **)p->data.rgb_s.r;
				dptr->datum[i][2] = (int **)p->data.rgb_s.g;
				dptr->datum[i][3] = (int **)p->data.rgb_s.b;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.rgb_d.a;
				dptr->datum[i][1] = (int **)p->data.rgb_d.r;
				dptr->datum[i][2] = (int **)p->data.rgb_d.g;
				dptr->datum[i][3] = (int **)p->data.rgb_d.b;
			}
			dptr->ndatum[i] = 4;
			break;
		case HDRDPX_DESC_RGB:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.rgb.r;
				dptr->datum[i][1] = p->data.rgb.g;
				dptr->datum[i][2] = p->data.rgb.b;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.rgb_s.r;
				dptr->datum[i][1] = (int **)p->data.rgb_s.g;
				dptr->datum[i][2] = (int **)p->data.rgb_s.b;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.rgb_d.r;
				dptr->datum[i][1] = (int **)p->data.rgb_d.g;
				dptr->datum[i][2] = (int **)p->data.rgb_d.b;
			}
			dptr->ndatum[i] = 3;
			break;
		case HDRDPX_DESC_RGBA:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.rgb.r;
				dptr->datum[i][1] = p->data.rgb.g;
				dptr->datum[i][2] = p->data.rgb.b;
				dptr->datum[i][3] = p->data.rgb.a;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.rgb_s.r;
				dptr->datum[i][1] = (int **)p->data.rgb_s.g;
				dptr->datum[i][2] = (int **)p->data.rgb_s.b;
				dptr->datum[i][3] = (int **)p->data.rgb_s.a;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.rgb_d.r;
				dptr->datum[i][1] = (int **)p->data.rgb_d.g;
				dptr->datum[i][2] = (int **)p->data.rgb_d.b;
				dptr->datum[i][3] = (int **)p->data.rgb_d.a;
			}
			dptr->ndatum[i] = 4;
			break;
		case HDRDPX_DESC_ABGR:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.rgb.a;
				dptr->datum[i][1] = p->data.rgb.b;
				dptr->datum[i][2] = p->data.rgb.g;
				dptr->datum[i][3] = p->data.rgb.r;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.rgb_s.a;
				dptr->datum[i][1] = (int **)p->data.rgb_s.b;
				dptr->datum[i][2] = (int **)p->data.rgb_s.g;
				dptr->datum[i][3] = (int **)p->data.rgb_s.r;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.rgb_d.a;
				dptr->datum[i][1] = (int **)p->data.rgb_d.b;
				dptr->datum[i][2] = (int **)p->data.rgb_d.g;
				dptr->datum[i][3] = (int **)p->data.rgb_d.r;
			}
			dptr->ndatum[i] = 4;
			break;
		case HDRDPX_DESC_CbYCrY:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.yuv.u;
				dptr->datum[i][1] = p->data.yuv.y;
				dptr->datum[i][2] = p->data.yuv.v;
				dptr->datum[i][3] = p->data.yuv.y;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
				dptr->datum[i][1] = (int **)p->data.yuv_s.y;
				dptr->datum[i][2] = (int **)p->data.yuv_s.v;
				dptr->datum[i][3] = (int **)p->data.yuv_s.y;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
				dptr->datum[i][1] = (int **)p->data.yuv_d.y;
				dptr->datum[i][2] = (int **)p->data.yuv_d.v;
				dptr->datum[i][3] = (int **)p->data.yuv_d.y;
			}
			dptr->stride[i][1] = dptr->stride[i][3] = 2;
			dptr->offset[i][3] = 1;
			dptr->wbuff[i] /= 2;
			dptr->ndatum[i] = 4;
			break;
		case HDRDPX_DESC_CbYACrYA:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.yuv.u;
				dptr->datum[i][1] = p->data.yuv.y;
				dptr->datum[i][2] = p->data.yuv.v;
				dptr->datum[i][3] = p->data.yuv.y;
				dptr->datum[i][4] = p->data.yuv.a;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
				dptr->datum[i][1] = (int **)p->data.yuv_s.y;
				dptr->datum[i][2] = (int **)p->data.yuv_s.v;
				dptr->datum[i][3] = (int **)p->data.yuv_s.y;
				dptr->datum[i][4] = (int **)p->data.yuv_s.a;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
				dptr->datum[i][1] = (int **)p->data.yuv_d.y;
				dptr->datum[i][2] = (int **)p->data.yuv_d.v;
				dptr->datum[i][3] = (int **)p->data.yuv_d.y;
				dptr->datum[i][4] = (int **)p->data.yuv_d.a;
			}
			dptr->stride[i][1] = dptr->stride[i][3] = 2;
			dptr->offset[i][3] = 1;
			dptr->wbuff[i] /= 2;
			dptr->ndatum[i] = 5;
			break;
		case HDRDPX_DESC_CbYCr:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.yuv.u;
				dptr->datum[i][1] = p->data.yuv.y;
				dptr->datum[i][2] = p->data.yuv.v;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
				dptr->datum[i][1] = (int **)p->data.yuv_s.y;
				dptr->datum[i][2] = (int **)p->data.yuv_s.v;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
				dptr->datum[i][1] = (int **)p->data.yuv_d.y;
				dptr->datum[i][2] = (int **)p->data.yuv_d.v;
			}
			dptr->ndatum[i] = 3;
			break;
		case HDRDPX_DESC_CbYCrA:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.yuv.u;
				dptr->datum[i][1] = p->data.yuv.y;
				dptr->datum[i][2] = p->data.yuv.v;
				dptr->datum[i][3] = p->data.yuv.a;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
				dptr->datum[i][1] = (int **)p->data.yuv_s.y;
				dptr->datum[i][2] = (int **)p->data.yuv_s.v;
				dptr->datum[i][3] = (int **)p->data.yuv_s.a;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
				dptr->datum[i][1] = (int **)p->data.yuv_d.y;
				dptr->datum[i][2] = (int **)p->data.yuv_d.v;
				dptr->datum[i][3] = (int **)p->data.yuv_d.a;
			}
			dptr->ndatum[i] = 4;
			break;
		case HDRDPX_DESC_CYY:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.yuv.u;
				dptr->alt_chroma[i] = p->data.yuv.v;
				dptr->datum[i][1] = p->data.yuv.y;
				dptr->datum[i][2] = p->data.yuv.y;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
				dptr->alt_chroma[i] = (int **)p->data.yuv_s.v;
				dptr->datum[i][1] = (int **)p->data.yuv_s.y;
				dptr->datum[i][2] = (int **)p->data.yuv_s.y;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
				dptr->alt_chroma[i] = (int **)p->data.yuv_d.v;
				dptr->datum[i][1] = (int **)p->data.yuv_d.y;
				dptr->datum[i][2] = (int **)p->data.yuv_d.y;
			}
			dptr->stride[i][1] = dptr->stride[i][2] = 2;
			dptr->offset[i][2] = 1;
			dptr->wbuff[i] /= 2;
			dptr->ndatum[i] = 3;
			break;
		case HDRDPX_DESC_CYAYA:
			if (f->ImageHeader.ImageElement[i].BitSize < 32)
			{
				dptr->datum[i][0] = p->data.yuv.u;
				dptr->alt_chroma[i] = p->data.yuv.v;
				dptr->datum[i][1] = p->data.yuv.y;
				dptr->datum[i][2] = p->data.yuv.a;
				dptr->datum[i][3] = p->data.yuv.y;
				dptr->datum[i][4] = p->data.yuv.a;
			}
			else if (f->ImageHeader.ImageElement[i].BitSize == 32)
			{
				dptr->datum[i][0] = (int **)p->data.yuv_s.u;
				dptr->alt_chroma[i] = (int **)p->data.yuv_s.v;
				dptr->datum[i][1] = (int **)p->data.yuv_s.y;
				dptr->datum[i][2] = (int **)p->data.yuv_s.a;
				dptr->datum[i][3] = (int **)p->data.yuv_s.y;
				dptr->datum[i][4] = (int **)p->data.yuv_s.a;
			}
			else
			{
				dptr->datum[i][0] = (int **)p->data.yuv_d.u;
				dptr->alt_chroma[i] = (int **)p->data.yuv_d.v;
				dptr->datum[i][1] = (int **)p->data.yuv_d.y;
				dptr->datum[i][2] = (int **)p->data.yuv_d.a;
				dptr->datum[i][3] = (int **)p->data.yuv_d.y;
				dptr->datum[i][4] = (int **)p->data.yuv_d.a;
			}
			dptr->stride[i][1] = dptr->stride[i][2] = dptr->stride[i][3] = dptr->stride[i][4] = 2;
			dptr->offset[i][3] = dptr->offset[i][4] = 1;
			dptr->wbuff[i] /= 2;
			dptr->ndatum[i] = 5;
			break;
		case HDRDPX_DESC_G2C:
		case HDRDPX_DESC_G3C:
		case HDRDPX_DESC_G4C:
		case HDRDPX_DESC_G5C:
		case HDRDPX_DESC_G6C:
		case HDRDPX_DESC_G7C:
		case HDRDPX_DESC_G8C:
			dptr->ndatum[i] = f->ImageHeader.ImageElement[i].Descriptor - HDRDPX_DESC_G2C + 2;
			for (j = 0; j < dptr->ndatum[i]; ++j)
			{
				if (f->ImageHeader.ImageElement[i].BitSize < 32)
					dptr->datum[i][j] = p->data.gc[j];
				else if (f->ImageHeader.ImageElement[i].BitSize == 32)
					dptr->datum[i][j] = (int **)p->data.gc_s[j];
				else
					dptr->datum[i][j] = (int **)p->data.gc_d[j];
			}
			break;
		default:
			fprintf(stderr, "hdr_dpx_write() error: unrecognized descriptor %d", f->ImageHeader.ImageElement[i].Descriptor);
			return(DPX_ERROR_BAD_PARAMETER);
		}
	}
	return(0);
}

int hdr_dpx_pic_to_datum_list(HDRDPXFILEFORMAT *f, datum_list_t *dlist, pic_t *p)
{
	int xindex[8], yindex[8];
	int yidx;
	int i, j, k, e, n, err;
	datum_ptrs_t dptr;
	int **cur_datum;
	int dwords_per_datum;

	if ((err = hdr_dpx_map_datum_to_pic(f, p, &dptr)) != 0)
		return(err);

	for (e = 0; e < f->ImageHeader.NumberElements; e++) // Loop over image elements
	{
		dwords_per_datum = (f->ImageHeader.ImageElement[e].BitSize == 64) ? 2 : 1;
		dlist[e].d = (uint32_t *)malloc(sizeof(uint32_t) * dptr.hbuff[e] * dptr.wbuff[e] * dptr.ndatum[e] * dwords_per_datum);
		dlist[e].eol_flag = (uint8_t *)malloc(sizeof(uint8_t) * dptr.hbuff[e] * dptr.wbuff[e] * dptr.ndatum[e] * dwords_per_datum);
		// init indices
		for (k = 0; k < 8; ++k)
		{
			yindex[k] = 0;
		}
		n = 0;

		for (i = 0; i < dptr.hbuff[e]; ++i) // every row
		{
			for (k = 0; k < 8; ++k)
				xindex[k] = dptr.offset[e][k] * dwords_per_datum;

			for (j = 0; j<dptr.wbuff[e]; ++j) // every pixel
			{
				for (k = 0; k<dptr.ndatum[e]; ++k)  // every datum
				{
					// Handle alternating-line chroma formats
					cur_datum = dptr.datum[e][k];
					yidx = yindex[k];
					if (dptr.alt_chroma[e] && k == 0)
					{
						if (yindex[k] & 1)
							cur_datum = dptr.alt_chroma[e];
						yidx = yindex[k] >> 1;
					}

					dlist[e].d[n] = cur_datum[yidx][xindex[k]];
					dlist[e].eol_flag[n++] = 0;
					if (dwords_per_datum == 2)
					{
						dlist[e].d[n] = cur_datum[yidx][xindex[k] + 1];
						dlist[e].eol_flag[n++] = 0;
					}
					xindex[k] += dptr.stride[e][k] * dwords_per_datum;
				}
			} // end of a row

			for (k = 0; k < 8; ++k)
				yindex[k]++;

			dlist[e].eol_flag[n-1] = 1;
		}  // end of image element	
		dlist[e].ndatum = n;
		dlist[e].bit_depth = f->ImageHeader.ImageElement[e].BitSize;
		if (f->ImageHeader.ImageElement[e].Encoding == 1)
		{
			if (f->ImageHeader.ImageElement[e].BitSize >= 32)
			{
				fprintf(stderr, "hdr_dpx_pic_to_datum_list() warning: RLE not supported with floating-point samples for image element %d\n", e);
				f->ImageHeader.ImageElement[e].Encoding = 0;
			}
			else
				hdr_dpx_rle_encode(&dlist[e], dptr.ndatum[e]);
		}
	}
	return(0);
}


datum_t hdr_dpx_get_datum(fifo_t *fifo, int bits, int dir)
{
	datum_t datum;

	if (dir == 0)
	{
		datum.d[0] = fifo_flip_get_bits(fifo, MIN(32, bits), 0);
		if (bits > 32)
			datum.d[1] = fifo_flip_get_bits(fifo, MIN(32, bits), 0);
	}
	else
	{
		datum.d[0] = fifo_get_bits(fifo, MIN(32, bits), 0);
		if (bits > 32)
			datum.d[1] = fifo_get_bits(fifo, MIN(32, bits), 0);
	}
	return(datum);
}

int hdr_dpx_get_pic_data(HDRDPXFILEFORMAT *f, FILE *fp, pic_t **p, int bswap)
{
	int xindex[8], yindex[8];
	int yidx;
	int i, j, k, e, err;
	datum_ptrs_t dptr;
	int **cur_datum;
	fifo_t fifo;
	uint32_t d;
	datum_t datum;
	datum_t rle_pix[8];
	int rle_state;  // 0=> flag, 1=>reading pixel value
	int rle_count = 0;
	int rle_flag = 0;
	uint32_t pad;
	int oflow = 0;

	fifo_init(&fifo, 64);

	if ((err = hdr_dpx_create_pic(f, p)) != 0)
		return (err);
	if ((err = hdr_dpx_map_datum_to_pic(f, *p, &dptr)) != 0)
		return(err);

	rle_state = 0;   // Reading flag
	for (e = 0; e < f->ImageHeader.NumberElements; e++) // Loop over image elements
	{
		fseek(fp, f->ImageHeader.ImageElement[e].DataOffset, SEEK_SET);
		// init indices
		for (k = 0; k < 8; ++k)
		{
			yindex[k] = 0;
		}

		for (i = 0; i < dptr.hbuff[e]; ++i) // every row
		{
			for (k = 0; k < 8; ++k)
				xindex[k] = dptr.offset[e][k];

			for (j = 0; j<dptr.wbuff[e]; ++j) // every pixel
			{
				for (k = 0; k < dptr.ndatum[e]; ++k)  // every datum
				{
					// Handle alternating-line chroma formats
					cur_datum = dptr.datum[e][k];
					yidx = yindex[k];
					if (dptr.alt_chroma[e] && k == 0)
					{
						if (yindex[k] & 1)
							cur_datum = dptr.alt_chroma[e];
						yidx = yindex[k] >> 1;
					}

					// Fill FIFO (need at least two words in queue)
					while (fifo.fullness <= 32)
					{
						if (fread(&d, 4, 1, fp) != 1)
						{
							oflow++;
							if(oflow > 2)	// We expect to read a little past EOF since we're reading ahead
								return(DPX_ERROR_CORRUPTED_FILE);
						}
						fifo_put_bits(&fifo, READ_DPX_32(d), 32);
					}
					switch (f->ImageHeader.ImageElement[e].Packing)
					{
					case 0:
						datum = hdr_dpx_get_datum(&fifo, f->ImageHeader.ImageElement[e].BitSize, f->FileHeader.DatumMappingDirection);
						break;

					case 1:   // Method A 
						if (f->FileHeader.DatumMappingDirection == 0) // ordered from lsbit
						{
							if (fifo.fullness == 48 || fifo.fullness == 64)   // start with padding bits
							{
								if (f->ImageHeader.ImageElement[e].BitSize == 10)
								{
									if (fifo_flip_get_bits(&fifo, 2, 0))
									{
										fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
										return(DPX_ERROR_CORRUPTED_FILE);
									}
								}
								else // 12-bit
								{ 
									if (fifo_flip_get_bits(&fifo, 4, 0))
									{
										fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
										return(DPX_ERROR_CORRUPTED_FILE);
									}
								}
							}
							datum = hdr_dpx_get_datum(&fifo, f->ImageHeader.ImageElement[e].BitSize, f->FileHeader.DatumMappingDirection);
						}
						else  // ordered from msbit
						{
							datum = hdr_dpx_get_datum(&fifo, f->ImageHeader.ImageElement[e].BitSize, f->FileHeader.DatumMappingDirection);
							if (fifo.fullness == 64 - 30)
							{ 
								if (fifo_get_bits(&fifo, 2, 0))
								{
									fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
									return(DPX_ERROR_CORRUPTED_FILE);
								}
							}
							else if (fifo.fullness == 64 - 12 || fifo.fullness == 64 - 28)
							{ 
								if (fifo_get_bits(&fifo, 4, 0))
								{
									fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
									return(DPX_ERROR_CORRUPTED_FILE);
								}
							}
						}
						break;

					case 2:   // Method B
						if (f->FileHeader.DatumMappingDirection == 0)  // Ordered from lsbit
						{
							datum = hdr_dpx_get_datum(&fifo, f->ImageHeader.ImageElement[e].BitSize, f->FileHeader.DatumMappingDirection);
							if (fifo.fullness == 64 - 30)
							{
								if (fifo_flip_get_bits(&fifo, 2, 0))
								{
									fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
									return(DPX_ERROR_CORRUPTED_FILE);
								}
							}
							else if (fifo.fullness == 64 - 12 || fifo.fullness == 64 - 28)
							{
								if (fifo_flip_get_bits(&fifo, 4, 0))
								{
									fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
									return(DPX_ERROR_CORRUPTED_FILE);
								}
							}
						}
						else  // ordered from msbit
						{
							if (fifo.fullness == 64 - 0 || fifo.fullness == 64 - 16)
							{
								if (f->ImageHeader.ImageElement[e].BitSize == 10)
								{
									if (fifo_get_bits(&fifo, 2, 0))
									{
										fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
										return(DPX_ERROR_CORRUPTED_FILE);
									}
								}
								else
								{
									if (fifo_get_bits(&fifo, 4, 0))
									{
										fprintf(stderr, "hdr_dpx_get_pic_data(): Unexpected non-zero bits\n");
										return(DPX_ERROR_CORRUPTED_FILE);
									}
								}
							}
							datum = hdr_dpx_get_datum(&fifo, f->ImageHeader.ImageElement[e].BitSize, f->FileHeader.DatumMappingDirection);
						}
						break;
					default:
						fprintf(stderr, "hdr_dpx_write() - unsupported packing format %d\n", f->ImageHeader.ImageElement[e].Packing);
						return(DPX_ERROR_BAD_PARAMETER);
					}

					if (f->ImageHeader.ImageElement[e].Encoding == 0)  // No RLE
					{
						if(f->ImageHeader.ImageElement[e].BitSize <= 32)
							cur_datum[yidx][xindex[k]] = datum.d[0];
						else
						{
							cur_datum[yidx][xindex[k]] = datum.d[0];
							cur_datum[yidx][xindex[k]+1] = datum.d[1];
						}
						xindex[k] += dptr.stride[e][k] << (f->ImageHeader.ImageElement[e].BitSize == 64);
					}
					else if (f->ImageHeader.ImageElement[e].Encoding == 1)  // RLE
					{
						if (rle_state == 0)
						{
							rle_flag = datum.d[0] & 0x1;
							rle_count = datum.d[0] >> 1;
							rle_state = 1;
							k = -1;
							//printf("%d: Run flag = %d, count = %d, xindex = %d\n", yidx, rle_flag, rle_count, xindex[0]);
						}
						else if (rle_flag == 1)
						{
							rle_pix[k] = datum;
							if (k == dptr.ndatum[e] - 1)
							{
								int l;
								for (l = 0; l < rle_count; ++l)
								{
									for (k = 0; k < dptr.ndatum[e]; ++k)
									{
										cur_datum = dptr.datum[e][k];
										yidx = yindex[k];
										if (dptr.alt_chroma[e] && k == 0)
										{
											if (yindex[k] & 1)
												cur_datum = dptr.alt_chroma[e];
											yidx = yindex[k] >> 1;
										}

										if (f->ImageHeader.ImageElement[e].BitSize <= 32)
											cur_datum[yidx][xindex[k]] = rle_pix[k].d[0];
										else
										{
											cur_datum[yidx][xindex[k]] = rle_pix[k].d[0];
											cur_datum[yidx][xindex[k]] = rle_pix[k].d[1];
										}
										xindex[k] += dptr.stride[e][k] << (f->ImageHeader.ImageElement[e].BitSize == 64);
										if ((dptr.stride[e][k] == 1 && xindex[k] > dptr.wbuff[e]) ||
											(dptr.stride[e][k] == 2 && xindex[k] > 2 * dptr.wbuff[e] + 1))
											printf("debug\n");
									}
									j++;
								}
								j--;
								rle_state = rle_count = rle_flag = 0;
							}
						}
						else  // rle_state == 1 && rle_flag == 0
						{
							if (f->ImageHeader.ImageElement[e].BitSize <= 32)
								cur_datum[yidx][xindex[k]] = datum.d[0];
							else
							{
								cur_datum[yidx][xindex[k]] = datum.d[0];
								cur_datum[yidx][xindex[k] + 1] = datum.d[1];
							}
							xindex[k] += dptr.stride[e][k] << (f->ImageHeader.ImageElement[e].BitSize == 64);
							if ((dptr.stride[e][k] == 1 && xindex[k] > dptr.wbuff[e]) ||
								(dptr.stride[e][k] == 2 && xindex[k] > 2 * dptr.wbuff[e] + 1))
								printf("debug\n");
							if (k == dptr.ndatum[e] - 1)
							{
								rle_count--;
								if (rle_count <= 0)
									rle_state = rle_count = rle_flag = 0;
							}
						}
					}
				}
			} // end of a row
			if (fifo.fullness % 32)
			{
				if (f->FileHeader.DatumMappingDirection == 0)
					fifo_flip_get_bits(&fifo, fifo.fullness % 32, 0);
				else
					fifo_get_bits(&fifo, fifo.fullness % 32, 0);
				for (pad = 0; pad < f->ImageHeader.ImageElement[e].EndOfLinePadding / 4; ++pad)
				{
					if(fread(&d, 4, 1, fp) != 1)
						return(DPX_ERROR_CORRUPTED_FILE);
					fifo_put_bits(&fifo, READ_DPX_32(d), 32);
					if (f->FileHeader.DatumMappingDirection == 0)
						fifo_flip_get_bits(&fifo, 32, 0);
					else
						fifo_get_bits(&fifo, 32, 0);
				}
			}
			for (k = 0; k < 8; ++k)
				yindex[k]++;
		}  // end of image element	
	}
	return(0);
}


void hdr_dpx_byte_swap(HDRDPXFILEFORMAT *f)
{
	int i;

	BS_32(*f, FileHeader.Magic);
	BS_32(*f, FileHeader.ImageOffset);
	BS_32(*f, FileHeader.FileSize);
	BS_32(*f, FileHeader.DittoKey);
	BS_32(*f, FileHeader.GenericSize);
	BS_32(*f, FileHeader.IndustrySize);
	BS_32(*f, FileHeader.UserSize);
	BS_32(*f, FileHeader.EncryptKey);
	BS_32(*f, FileHeader.StandardsBasedMetadataOffset);

	BS_16(*f, ImageHeader.Orientation);
	BS_16(*f, ImageHeader.NumberElements);
	BS_32(*f, ImageHeader.PixelsPerLine);
	BS_32(*f, ImageHeader.LinesPerElement);
	BS_32(*f, ImageHeader.ChromaSubsampling);

	for (i = 0; i < NUM_IMAGE_ELEMENTS; ++i)
	{
		BS_32(*f, ImageHeader.ImageElement[i].DataSign);
		BS_32(*f, ImageHeader.ImageElement[i].LowData.d);
		BS_32(*f, ImageHeader.ImageElement[i].LowQuantity);
		BS_32(*f, ImageHeader.ImageElement[i].HighData.d);
		BS_32(*f, ImageHeader.ImageElement[i].HighQuantity);
		BS_32(*f, ImageHeader.ImageElement[i].DataOffset);
		BS_16(*f, ImageHeader.ImageElement[i].Packing);
		BS_16(*f, ImageHeader.ImageElement[i].Encoding);
		BS_32(*f, ImageHeader.ImageElement[i].DataOffset);
		BS_32(*f, ImageHeader.ImageElement[i].EndOfLinePadding);
		BS_32(*f, ImageHeader.ImageElement[i].EndOfImagePadding);
	}

	/* Image source info header details */
	BS_32(*f, SourceInfoHeader.XOffset);
	BS_32(*f, SourceInfoHeader.YOffset);
	BS_32(*f, SourceInfoHeader.XCenter);
	BS_32(*f, SourceInfoHeader.YCenter);
	BS_32(*f, SourceInfoHeader.XOriginalSize);
	BS_32(*f, SourceInfoHeader.YOriginalSize);
	for (i = 0; i<4; ++i)
		BS_16(*f, SourceInfoHeader.Border[i]);
	BS_32(*f, SourceInfoHeader.AspectRatio[0]);
	BS_32(*f, SourceInfoHeader.AspectRatio[1]);

	/* Film header info details */
	BS_32(*f, FilmHeader.FramePosition);
	BS_32(*f, FilmHeader.SequenceLen);
	BS_32(*f, FilmHeader.HeldCount);
	BS_32(*f, FilmHeader.FrameRate);
	BS_32(*f, FilmHeader.ShutterAngle);

	/* TV header info details */
	BS_32(*f, TvHeader.TimeCode);
	BS_32(*f, TvHeader.UserBits);
	BS_32(*f, TvHeader.HorzSampleRate);
	BS_32(*f, TvHeader.VertSampleRate);
	BS_32(*f, TvHeader.FrameRate);
	BS_32(*f, TvHeader.TimeOffset);
	BS_32(*f, TvHeader.Gamma);
	BS_32(*f, TvHeader.BlackLevel);
	BS_32(*f, TvHeader.BlackGain);
	BS_32(*f, TvHeader.Breakpoint);
	BS_32(*f, TvHeader.WhiteLevel);
	BS_32(*f, TvHeader.IntegrationTimes);
}

int hdr_dpx_compute_offsets(HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm)
{
	uint32_t recommended_image_offset, min_image_offset, ud_length;

	ud_length = (dpxu == NULL) ? 0 : dpxh->FileHeader.UserSize;
	min_image_offset = ((2048 + ud_length + 3) >> 2) << 2;  // Required by spec: 2048 byte header + user data at 4-byte boundary
	recommended_image_offset = ((2048 + ud_length + 511) >> 9) << 9; // Not required by spec, but if offset is unspecified, chooses 512-byte boundary
	if (dpxsbm == NULL)
		dpxh->FileHeader.StandardsBasedMetadataOffset = UINT32_MAX;

	if (dpxh->FileHeader.ImageOffset == 0 || dpxh->FileHeader.ImageOffset == UINT32_MAX)
		dpxh->FileHeader.ImageOffset = recommended_image_offset;
	else
	{
		if (dpxh->FileHeader.ImageOffset < min_image_offset)
		{
			fprintf(stderr, "hdr_dpx_write() error: Image offset must be at least %d", min_image_offset);
			return(DPX_ERROR_BAD_PARAMETER);
		}
	}
	return(0);
}

int hdr_dpx_fill_core_fields(HDRDPXFILEFORMAT *dpxh, pic_t *p)
{
	int i, d;

	dpxh->FileHeader.Magic = 0x53445058;
	sprintf(dpxh->FileHeader.Version, "V2.0HDR");
	if (dpxh->FileHeader.DatumMappingDirection == 0xff)
		dpxh->FileHeader.DatumMappingDirection = 1;
	dpxh->FileHeader.GenericSize = 1684;
	dpxh->FileHeader.IndustrySize = 384;

	if (dpxh->ImageHeader.Orientation == UINT16_MAX)
		dpxh->ImageHeader.Orientation = 0;
	if (dpxh->ImageHeader.NumberElements == 0 || dpxh->ImageHeader.NumberElements == UINT16_MAX)
		dpxh->ImageHeader.NumberElements = 1;
	if (dpxh->ImageHeader.PixelsPerLine == UINT32_MAX)
		dpxh->ImageHeader.PixelsPerLine = p->w;
	if (dpxh->ImageHeader.LinesPerElement == UINT32_MAX)
		dpxh->ImageHeader.LinesPerElement = p->h;
	for (i = 0; i < dpxh->ImageHeader.NumberElements; ++i)
	{
		if (dpxh->ImageHeader.ImageElement[i].DataSign == UINT32_MAX)
			dpxh->ImageHeader.ImageElement[i].DataSign = 0;		// unsigned
		if (dpxh->ImageHeader.ImageElement[i].BitSize == UINT8_MAX)
			dpxh->ImageHeader.ImageElement[i].BitSize = p->bits;
		if (dpxh->ImageHeader.ImageElement[i].LowData.d == UINT32_MAX || dpxh->ImageHeader.ImageElement[i].HighData.d == UINT32_MAX)
		{
			if (p->limited_range && dpxh->ImageHeader.ImageElement[i].BitSize >= 8)
			{
				if (dpxh->ImageHeader.ImageElement[i].BitSize < 32)
				{
					dpxh->ImageHeader.ImageElement[i].LowData.d = 16 << (dpxh->ImageHeader.ImageElement[i].BitSize - 8); 
					dpxh->ImageHeader.ImageElement[i].HighData.d = 235 << (dpxh->ImageHeader.ImageElement[i].BitSize - 8);
				}
				else  // Floating point samples
				{
					dpxh->ImageHeader.ImageElement[i].LowData.f = (float)16.0;
					dpxh->ImageHeader.ImageElement[i].HighData.f = (float)240.0;
				}
			}
			else {
				if (dpxh->ImageHeader.ImageElement[i].BitSize < 32)
				{
					dpxh->ImageHeader.ImageElement[i].LowData.d = 0;
					dpxh->ImageHeader.ImageElement[i].HighData.d = (1 << dpxh->ImageHeader.ImageElement[i].BitSize) - 1;
				}
				else  // Floating point samples
				{
					dpxh->ImageHeader.ImageElement[i].LowData.f = (float)0.0;
					dpxh->ImageHeader.ImageElement[i].HighData.f = (float)256.0;
				}
			}
		}
		if (dpxh->ImageHeader.ImageElement[i].Descriptor == UINT8_MAX)
		{
			if (p->color == RGB)
				dpxh->ImageHeader.ImageElement[i].Descriptor = p->alpha ? HDRDPX_DESC_RGBA : HDRDPX_DESC_RGB;
			else if (p->color == YUV_HD || p->color == YUV_SD)
			{
				if (p->chroma == YUV_444 || p->chroma == YUV_4444)
					dpxh->ImageHeader.ImageElement[i].Descriptor = p->alpha ? HDRDPX_DESC_CbYCrA : HDRDPX_DESC_CbYCr;
				else if (p->chroma == YUV_422)
					dpxh->ImageHeader.ImageElement[i].Descriptor = p->alpha ? HDRDPX_DESC_CbYACrYA : HDRDPX_DESC_CbYCrY;
				else if (p->chroma == YUV_420)
					dpxh->ImageHeader.ImageElement[i].Descriptor = p->alpha ? HDRDPX_DESC_CYAYA : HDRDPX_DESC_CYY;
			}
			else if (p->color == G_1C)
				dpxh->ImageHeader.ImageElement[i].Descriptor = HDRDPX_DESC_USER;
			else if (p->color != UNDEFINED_COLOR)
				dpxh->ImageHeader.ImageElement[i].Descriptor = p->color - G_2C + HDRDPX_DESC_G2C;
			else
			{
				fprintf(stderr, "hdr_dpx_fill_core_fields() encountered unrecognized color/chroma type\n");
				return(DPX_ERROR_UNRECOGNIZED_CHROMA);
			}
		}
		if (dpxh->ImageHeader.ImageElement[i].Transfer == UINT8_MAX)
		{
			if (p->transfer == TF_USER)				dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_USER;
			else if (p->transfer == TF_DENSITY)		dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_DENSITY;
			else if (p->transfer == TF_LINEAR)		dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_LINEAR;
			else if (p->transfer == TF_LOG)			dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_LOGARITHMIC;
			else if (p->transfer == TF_UNSPEC_VID)	dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_UNSPECIFIED_VIDEO;
			else if (p->transfer == TF_BT_709)		dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_BT_709;
			else if (p->transfer == TF_XVYCC)		dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_IEC_61966_2_4;
			else if (p->transfer == TF_Z_LINEAR)	dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_Z_LINEAR;
			else if (p->transfer == TF_Z_HOMOG)		dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_Z_HOMOGENOUS;
			else if (p->transfer == TF_ADX)			dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_ST_2065_3;
			else if (p->transfer == TF_BT_2020_NCL) dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_BT_2020_NCL;
			else if (p->transfer == TF_BT_2020_CL)	dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_BT_2020_CL;
			else if (p->transfer == TF_BT_2100_PQ_NCL) dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_BT_2100_PQ_NCL;
			else if (p->transfer == TF_BT_2100_PQ_CI) dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_BT_2100_PQ_CI;
			else if (p->transfer == TF_BT_2100_HLG_NCL) dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_BT_2100_HLG_NCL;
			else if (p->transfer == TF_BT_2100_HLG_CI) dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_BT_2100_HLG_CI;
			else if (p->transfer == TF_P3_GAMMA)	dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_RP_231_2;
			else if (p->transfer == TF_SRGB)		dpxh->ImageHeader.ImageElement[i].Transfer = HDRDPX_TF_IEC_61966_2_1;
			else
			{
				fprintf(stderr, "hdr_dpx_fill_core_fields() encountered an unrecognized transfer curve %d", p->transfer);
				return(DPX_ERROR_BAD_PARAMETER);
			}
		}
		if (dpxh->ImageHeader.ImageElement[i].Colorimetric == UINT8_MAX)
		{
			if (p->colorimetry == C_USER)			dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_USER;
			else if (p->colorimetry == C_DENSITY)	dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_DENSITY;
			else if (p->colorimetry == C_UNSPEC_VID) dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_UNSPECIFIED_VIDEO;
			else if (p->colorimetry == C_REC_709)	dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_BT_709;
			else if (p->colorimetry == C_REC_601_525) dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_BT_601_525;
			else if (p->colorimetry == C_REC_601_625) dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_BT_601_625;
			else if (p->colorimetry == C_ADX)		dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_ST_2065_3;
			else if (p->colorimetry == C_REC_2020)	dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_BT_2020;
			else if (p->colorimetry == C_P3D65)		dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_ST_2113_P3D65;
			else if (p->colorimetry == C_P3DCI)		dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_ST_2113_P3DCI;
			else if (p->colorimetry == C_P3D60)		dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_ST_2113_P3D60;
			else if (p->colorimetry == C_ACES)		dpxh->ImageHeader.ImageElement[i].Colorimetric = HDRDPX_C_ST_2065_1_ACES;
			else
			{
				fprintf(stderr, "hdr_dpx_fill_core_fields() encountered an unrecognized colorimetry %d", p->colorimetry);
				return(DPX_ERROR_BAD_PARAMETER);
			}
		}
		if (dpxh->ImageHeader.ImageElement[i].BitSize == UINT8_MAX)
			dpxh->ImageHeader.ImageElement[i].BitSize = p->bits;
		if (dpxh->ImageHeader.ImageElement[i].Packing == UINT16_MAX)
		{
			if (p->bits == 10 || p->bits == 12)
				dpxh->ImageHeader.ImageElement[i].Packing = 1;		// Method A
			else
				dpxh->ImageHeader.ImageElement[i].Packing = 0;
		}
		if (dpxh->ImageHeader.ImageElement[i].Encoding == UINT16_MAX)
			dpxh->ImageHeader.ImageElement[i].Encoding = 0;		// No compression (default)
		if (dpxh->ImageHeader.ImageElement[i].EndOfLinePadding == UINT32_MAX)
			dpxh->ImageHeader.ImageElement[i].EndOfLinePadding = 0;
		if (dpxh->ImageHeader.ImageElement[i].EndOfImagePadding == UINT32_MAX)
			dpxh->ImageHeader.ImageElement[i].EndOfImagePadding = 0;
		d = dpxh->ImageHeader.ImageElement[i].Descriptor;
		if (((dpxh->ImageHeader.ChromaSubsampling >> (4 * i)) & 0xf) == 0xf) // Not core, but will fill in if available
		{
			if (d == 7 || d == 10 || d == 11 || d == 100 || d == 101 || d == 104 || d == 105)
			{
				dpxh->ImageHeader.ChromaSubsampling &= ~(0xf << (i * 4));
				dpxh->ImageHeader.ChromaSubsampling |= (p->chroma_siting << (i * 4));
			}
		}
	}
	// Rest is not core, but we'll fill in if available from struct
	if (dpxh->SourceInfoHeader.AspectRatio[0] == UINT32_MAX || dpxh->SourceInfoHeader.AspectRatio[1] == UINT32_MAX)
	{
		if (p->ar1 >= 0)  // -1 => unspecified
		{
			dpxh->SourceInfoHeader.AspectRatio[0] = p->ar1;
			dpxh->SourceInfoHeader.AspectRatio[1] = p->ar2;
		}
	}
	if (dpxh->FilmHeader.FramePosition == UINT32_MAX)
		dpxh->FilmHeader.FramePosition = p->frm_no;
	if (dpxh->FilmHeader.SequenceLen == UINT32_MAX)
		dpxh->FilmHeader.SequenceLen = p->seq_len;
	if (memcmp(&(dpxh->FilmHeader.FrameRate), &undefined_4bytes, 4) == 0 && p->framerate > 0)
		dpxh->FilmHeader.FrameRate = p->framerate;
	if (memcmp(&(dpxh->TvHeader.FrameRate), &undefined_4bytes, 4) == 0 && p->framerate > 0)
		dpxh->TvHeader.FrameRate = p->framerate;
	if (dpxh->TvHeader.Interlace == UINT32_MAX)
		dpxh->TvHeader.Interlace = p->interlaced;
	if (dpxh->TvHeader.FieldNumber == UINT32_MAX)
		dpxh->TvHeader.FieldNumber = (p->format == TOP) ? 1 : 2;
	return (0);
}


int hdr_dpx_write(char *fname, pic_t *p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm, int write_as_be, HDRDPXFILEFORMAT *dpxh_written)
{
	FILE *fp;
	int k, e;
	uint32_t i;
	int err;
	HDRDPXFILEFORMAT f, f_bs;
	int *alt_chroma[8];
	int ndatum[8];
	int hbuff[8];
	DWORD data32b = 0;
	int bswap;
	union
	{
		unsigned int ui;
		unsigned char c[4];
	} endiantest;
	fifo_t fifo;
	datum_list_t dlist[8];
	
	memset((void *)hbuff, 0, 8 * sizeof(int));
	memset((void *)ndatum, 0, 8 * sizeof(int));
	memset((void *)alt_chroma, 0, 8 * sizeof(int *));

	endiantest.ui = 0x12345678;
	if(endiantest.c[0] == 0x12)  // Big endian
		bswap = 0;
	else
		bswap = 1;  // By default, write file as big-endian

	if(!write_as_be) bswap = !bswap;  // Invert to output little-endian file

	ndatum[0] = 0; ndatum[1] = 0; ndatum[2] = 0; ndatum[3] = 0;


	/* File header details */
	if ((err = hdr_dpx_compute_offsets(dpxh, dpxu, dpxsbm)) != 0)
		return(err);
	/* Start by copying over ASCII & U8 fields */
	memcpy(&f, dpxh, sizeof(HDRDPXFILEFORMAT));
	/* Then fill in automatic and non-overridable fields and byte swap as needed */
	hdr_dpx_fill_core_fields(&f, p);

	if ((fp = fopen(fname, "wb")) == NULL)
	{
		fprintf(stderr, "Cannot open %s for output\n", fname);
		exit(1);
	}

	fseek(fp, f.FileHeader.ImageOffset, SEEK_SET);

	hdr_dpx_pic_to_datum_list(&f, dlist, p);

	fifo_init(&fifo, 64);
	for (e = 0; e < f.ImageHeader.NumberElements; e++) // Loop over image elements
	{
		if (f.ImageHeader.ImageElement[e].DataOffset == UINT32_MAX)
			f.ImageHeader.ImageElement[e].DataOffset = (unsigned int)ftell(fp);

		for (k=0; k<dlist[e].ndatum; ++k)  // every datum
		{
			switch (f.ImageHeader.ImageElement[e].Packing)
			{
			case 0:
				if (f.FileHeader.DatumMappingDirection == 0)  // ordered from lsbit
					fifo_flip_put_bits(&fifo, dlist[e].d[k], MIN(32, f.ImageHeader.ImageElement[e].BitSize));
				else  // ordered from msbit
					fifo_put_bits(&fifo, dlist[e].d[k], MIN(32, f.ImageHeader.ImageElement[e].BitSize));
				break;

			case 1:   // Method A 
				if (f.FileHeader.DatumMappingDirection == 0) // ordered from lsbit
				{
					if (fifo.fullness == 0 || fifo.fullness == 16)   // start with padding bits
					{
						if (f.ImageHeader.ImageElement[e].BitSize == 10)
							fifo_flip_put_bits(&fifo, 0, 2);
						else // 12-bit
							fifo_flip_put_bits(&fifo, 0, 4);
					}
					fifo_flip_put_bits(&fifo, dlist[e].d[k], f.ImageHeader.ImageElement[e].BitSize);
				}
				else  // ordered from msbit
				{
					fifo_put_bits(&fifo, dlist[e].d[k], f.ImageHeader.ImageElement[e].BitSize);
					if (fifo.fullness == 30)
						fifo_put_bits(&fifo, 0, 2);
					else if (fifo.fullness == 12 || fifo.fullness == 28)
						fifo_put_bits(&fifo, 0, 4);
				}
				break;

			case 2:   // Method B
				if (f.FileHeader.DatumMappingDirection == 0)  // Ordered from lsbit
				{
					fifo_flip_put_bits(&fifo, dlist[e].d[k], f.ImageHeader.ImageElement[e].BitSize);
					if (fifo.fullness == 30)
						fifo_flip_put_bits(&fifo, 0, 2);
					else if (fifo.fullness == 12 || fifo.fullness == 28)
						fifo_flip_put_bits(&fifo, 0, 4);
				}
				else  // ordered from msbit
				{
					if (fifo.fullness == 0 || fifo.fullness == 16)
					{
						if (f.ImageHeader.ImageElement[e].BitSize == 10)
							fifo_put_bits(&fifo, 0, 2);
						else
							fifo_put_bits(&fifo, 0, 4);
					}
					fifo_put_bits(&fifo, dlist[e].d[k], f.ImageHeader.ImageElement[e].BitSize);
				}
				break;
			default:
				fprintf(stderr, "hdr_dpx_write() - unsupported packing format %d\n", f.ImageHeader.ImageElement[e].Packing);
				return(DPX_ERROR_BAD_PARAMETER);
			}
					
			while (fifo.fullness >= 32)
			{
				data32b = fifo_get_bits(&fifo, 32, 0);
				data32b = READ_DPX_32(data32b);
				fwrite(&data32b, 4, 1, fp);
			}

			if (dlist[e].eol_flag[k]) // last datum of scan line
			{
				if (fifo.fullness > 0)  // pad to 32-bit word
				{
					if (f.FileHeader.DatumMappingDirection)
						fifo_put_bits(&fifo, 0, 32 - fifo.fullness);
					else
						fifo_flip_put_bits(&fifo, 0, 32 - fifo.fullness);

					data32b = fifo_get_bits(&fifo, 32, 0);
					data32b = READ_DPX_32(data32b);
					fwrite(&data32b, 4, 1, fp);
				}
				for (i = 0; i < (int)(f.ImageHeader.ImageElement[e].EndOfLinePadding / 4); ++i)
				{
					data32b = 0;
					fwrite(&data32b, 4, 1, fp);
				}
			}
		}  // end of image element
		for (i = 0; i < (f.ImageHeader.ImageElement[e].EndOfImagePadding / 4); ++i)
		{
			data32b = 0;
			fwrite(&data32b, 4, 1, fp);
		}
	}



	// Write standards-based metadata section
	if (dpxsbm->SbmLength)
	{
		int str_end = 0;
		uint32_t d;
		if (f.FileHeader.StandardsBasedMetadataOffset == UINT32_MAX)
			f.FileHeader.StandardsBasedMetadataOffset = (unsigned int)ftell(fp);
		else
			fseek(fp, f.FileHeader.StandardsBasedMetadataOffset, SEEK_SET);
		for (i = 0; i < 128; ++i)
		{
			if (str_end)
				fputc(0, fp);
			else
				fputc(dpxsbm->SbmFormatDescriptor[i], fp);
			str_end |= dpxsbm->SbmFormatDescriptor[i] == '\0';
		}
		d = READ_DPX_32(dpxsbm->SbmLength);
		fwrite(&d, 4, 1, fp);
		for (i = 0; i < dpxsbm->SbmLength; ++i)
			fputc(dpxsbm->SbmData[i], fp);
	}

	// Write file header
	fseek(fp, SEEK_END, 0);
	f.FileHeader.FileSize = (unsigned int)ftell(fp);

	fseek(fp, 0, SEEK_SET);
	memcpy(&f_bs, &f, sizeof(HDRDPXFILEFORMAT));
	if (bswap)
		hdr_dpx_byte_swap(&f_bs);
	fwrite(&f_bs, sizeof(HDRDPXFILEFORMAT), 1, fp);
	
	// Output header
	if(dpxh_written != NULL)
		memcpy(dpxh_written, &f, sizeof(HDRDPXFILEFORMAT)); // Pass back calculated header values if desired

	// Write user data section
	if (f.FileHeader.UserSize > 0)
	{
		int str_end = 0;
		fseek(fp, 2048, SEEK_SET);
		for (i = 0; i < 32; ++i)
		{
			if (str_end)
				fputc(0, fp);
			else
				fputc(dpxu->UserIdentification[i], fp);
			str_end |= dpxu->UserIdentification[i] == '\0';
		}
		for (i = 0; i < f.FileHeader.UserSize - 32; ++i)
			fputc(dpxu->UserData[i], fp);
	}


	fifo_free(&fifo);
	for (e = 0; e < f.ImageHeader.NumberElements; e++) 
	{
		free(dlist[e].d);
		free(dlist[e].eol_flag);
	}

	fclose(fp);
	return(0);
}


int hdr_dpx_read(char *fname, pic_t **p, HDRDPXFILEFORMAT *dpxh, HDRDPXUSERDATA *dpxu, HDRDPXSBMDATA *dpxsbm)
{
	FILE *fp;
	DWORD magic;
	int bswap = 0;
	HDRDPXFILEFORMAT f;
	int ecode;
	uint32_t i, d;

	if ((fp = fopen(fname, "rb")) == NULL)
	{
		fprintf(stderr, "Error: Cannot read HDR DPX file %s\n", fname); 
		perror("Cannot read HDR DPX file");
		return(DPX_ERROR_BAD_FILENAME);
	}
	if (fread(&magic, 1, 4, fp) != 4)
	{
		return(DPX_ERROR_CORRUPTED_FILE);
	}
	fseek(fp, 0, SEEK_SET);

	if (magic == 0x53445058)
		bswap = 0;
	else if (magic == 0x58504453)
		bswap = 1;
	else
		return(DPX_ERROR_CORRUPTED_FILE);

	memset(&f, 0, sizeof(HDRDPXFILEFORMAT));
	if (fread(&f, sizeof(HDRDPXFILEFORMAT), 1, fp) == 0)
	{
		fprintf(stderr, "Error: Tried to HDR read corrupted DPX file %s\n", fname);
		return(DPX_ERROR_CORRUPTED_FILE);
	}

	if(bswap)
		hdr_dpx_byte_swap(&f);

	ecode = hdr_dpx_get_pic_data(&f, fp, p, bswap);
	if (ecode)
		return(ecode);
	if((*p)==NULL)
		return(DPX_ERROR_MALLOC_FAIL);
	if (dpxh != NULL)
		memcpy(dpxh, &f, sizeof(HDRDPXFILEFORMAT));

	// Read standards-based metadata if present
	if (dpxsbm != NULL && f.FileHeader.StandardsBasedMetadataOffset != UINT32_MAX)
	{
		fseek(fp, f.FileHeader.StandardsBasedMetadataOffset, SEEK_SET);
		for (i = 0; i < 128; ++i)
			dpxsbm->SbmFormatDescriptor[i] = fgetc(fp);
		if (fread(&d, 4, 1, fp) == 0)
		{
			fprintf(stderr, "Error: Tried to HDR read corrupted DPX file %s\n", fname);
			return(DPX_ERROR_CORRUPTED_FILE);
		}
		dpxsbm->SbmLength = READ_DPX_32(d);
		if (dpxsbm->SbmLength > 0)
		{
			dpxsbm->SbmData = (uint8_t *)malloc(dpxsbm->SbmLength);
			for (i = 0; i < dpxsbm->SbmLength; ++i)
				dpxsbm->SbmData[i] = fgetc(fp);
		}
	}

	// Read user data if present
	if (dpxu != NULL && f.FileHeader.UserSize > 0)
	{
		fseek(fp, 2048, SEEK_SET);
		for (i = 0; i < 32; ++i)
			dpxu->UserIdentification[i] = fgetc(fp);
		if (f.FileHeader.UserSize > 32)
		{
			dpxu->UserData = (uint8_t *)malloc(f.FileHeader.UserSize - 32);
			for (i = 0; i < f.FileHeader.UserSize - 32; ++i)
				dpxu->UserData[i] = fgetc(fp);
		}
	}

	fclose(fp);

	return(0);
}

int	 hdr_dpx_determine_file_type(char *fname)
{
	HDRDPXFILEFORMAT f;
	FILE *fp;
	int type;

	if ((fp = fopen(fname, "rb")) == NULL)
	{
		fprintf(stderr, "hdr_dpx_determine_file_type(): can't open input file %s", fname);
		return(-1);
	}

	if(fread(&f, sizeof(HDRDPXFILEFORMAT), 1, fp) != 1)
		return(DPX_ERROR_CORRUPTED_FILE);

	if (!strcmp(f.FileHeader.Version, "V1.0"))
		type = 1;
	else if (!strcmp(f.FileHeader.Version, "V2.0"))
		type = 2;
	else if (!strcmp(f.FileHeader.Version, "V2.0HDR"))
		type = 3;
	else 
	{
		fprintf(stderr, "hdr_dpx_determine_file_type(): Unrecognized version field\n");
		type = -1;
	}

	fclose(fp);
	return(type);
}

