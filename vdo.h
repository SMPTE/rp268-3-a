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
#ifndef VDO_H
#define VDO_H

#include <math.h> /* for fmod(3) */

#ifdef _MSC_VER
#ifndef _DEPRECATION_DISABLE
#define _DEPRECATION_DISABLE
#if (_MSC_VER >= 1400)
#pragma warning(disable: 4996)
#endif
#endif
//#define _CRT_SECURE_NO_WARNINGS
#endif

#define CLAMP(X, MIN, MAX) ((X)>(MAX) ? (MAX) : ((X)<(MIN) ? (MIN) : (X)))
#define MAX(X, Y) ((X)>(Y) ? (X) : (Y))
#define MIN(X, Y) ((X)<(Y) ? (X) : (Y))
#define ABS(X) ((X)<0 ? (-1*(X)) : (X))
#define FMOD(X,Y) fmod(fmod(X,Y)+Y,Y)

typedef enum format_e {FRAME, TOP, BOTTOM,                     UNDEFINED_FORMAT} format_t;
typedef enum color_e  {RGB, YUV_SD, YUV_HD, G_1C, G_2C, G_3C, G_4C, G_5C, G_6C, G_7C, G_8C,                   UNDEFINED_COLOR}  color_t;
typedef enum chroma_e {YUV_420, YUV_422, YUV_444, YUV_4444,    UNDEFINED_CHROMA} chroma_t;
typedef enum transfer_e {
	TF_USER, TF_DENSITY, TF_LINEAR, TF_LOG, TF_UNSPEC_VID, TF_BT_709 /* use for ST 274, REC 601 */, TF_XVYCC,
	TF_Z_LINEAR, TF_Z_HOMOG, TF_ADX, TF_BT_2020_NCL, TF_BT_2020_CL,
	TF_BT_2100_PQ_NCL, TF_BT_2100_PQ_CI, TF_BT_2100_HLG_NCL, TF_BT_2100_HLG_CI,
	TF_P3_GAMMA, TF_SRGB, UNDEFINED_TRANSFER } transfer_t;
typedef enum colorimetry_e  { 
	C_USER, C_DENSITY, C_UNSPEC_VID, C_REC_709 /* use for ST 274, xvYCC, sRGB */, C_REC_601_525, C_REC_601_625,
	C_ADX, C_REC_2020, C_P3D65, C_P3DCI, C_P3D60, C_ACES,
	UNDEFINED_GAMUT } colorimetry_t;

typedef struct yuv_s {
    int **y;
	int **u;
	int **v;
	int **a;
} yuv_t;

typedef struct rgb_s {
	int **r;
	int **g;
	int **b;
	int **a;
} rgb_t;

typedef struct yuv_s_s {
	float **y;
	float **u;
	float **v;
	float **a;
} yuv_s_t;

typedef struct rgb_s_s {
	float **r;
	float **g;
	float **b;
	float **a;
} rgb_s_t;

typedef struct yuv_d_s {
	double **y;
	double **u;
	double **v;
	double **a;
} yuv_d_t;

typedef struct rgb_d_s {
	double **r;
	double **g;
	double **b;
	double **a;
} rgb_d_t;

typedef struct pic_s {
    format_t format;
    color_t  color;
    chroma_t chroma;
	transfer_t transfer;
	colorimetry_t colorimetry;
    int      alpha; // 0: not used, 1: used (same resolution as Y)
    int      w;
    int      h;
    int      bits;
    int      ar1;     // aspect ratio (h)
    int      ar2;     // aspect ratio (w)
    int      frm_no;  // frame number in seq
    int      seq_len; // num images in sequence
    float    framerate;
    int      interlaced; // prog(0) or int(1) content
	int      limited_range; // 1=limited, 0=full range
	int		 chroma_siting;  // applicable for 4:2:2/4:2:0 only
							// 0=>NA/Unknown; 1=> cosited; 2=>H interstital, V cosited; 3=> H cosited, V interstital; 4=> interstitial

    union data_u {
        rgb_t rgb;
        yuv_t yuv;
		rgb_s_t rgb_s;
		yuv_s_t yuv_s;
		rgb_d_t rgb_d;
		yuv_d_t yuv_d;
		int **gc[8];
		float **gc_s[8];
		double **gc_d[8];
    } data;

} pic_t;

typedef struct pix_s {
    int r; // R or V
    int g; // G or Y
    int b; // B or U
    int a; // alpha
} pix_t;

//
// Data type for a polyphase filter. In addition, it can be used as a 2D
// filter by using "tap" as the vertical dimension, and "sub" as the
// horizontal dimension.
//
typedef struct fir_s {
    int   tap;
    int   sub;
    float coeff[256];
} fir_t;


#endif

