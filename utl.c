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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "vdo.h"
#include "utl.h"
#include "logging.h"


// Simple anti-aliasing filters
fir_t chm_444_422_03 = { 1, 3,
{ 0.25, 0.50, 0.25 }
};

fir_t chm_422_420_03 = { 3, 1,
{ 0.25, 0.50, 0.25 }
};

void *palloc(int w, int h)
{
    int **p;
    int   i;

    p = (int **) calloc(h, sizeof(short *));
    if (p == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory.\n");
        exit(1);
    }
    else
        for (i = 0; i < h; i++)
        {
            p[i] = calloc(w, sizeof(int));
            if (p[i] == NULL)
            {
                fprintf(stderr, "ERROR: Failed to allocate memory.\n");
                exit(1);
            }
        }
    return p;
}


pic_t *pcreate(int format, int color, int chroma, int w, int h)
{
    pic_t *p;    

    p = malloc(sizeof(pic_t));

    if (p == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory.\n");
        exit(1);
    }

    p->format = format;
    p->color  = color;
    p->chroma = chroma;

    p->w = w;
    p->h = h;

    if (color == RGB)
    {
        p->data.rgb.r = (int **) palloc(w, h);
        p->data.rgb.g = (int **) palloc(w, h);
        p->data.rgb.b = (int **) palloc(w, h);
        p->data.rgb.a = (int **) palloc(w, h);
    }
    else if (chroma == YUV_420)
    {
        p->data.yuv.y = (int **) palloc(w, h);
        p->data.yuv.u = (int **) palloc(w / 2, h / 2);
        p->data.yuv.v = (int **) palloc(w / 2, h / 2);
        p->data.yuv.a = (int **) palloc(w, h);
    }
    else if (chroma == YUV_422)
    {

        p->data.yuv.y = (int **) palloc(w, h);
        p->data.yuv.u = (int **) palloc(w / 2, h);
        p->data.yuv.v = (int **) palloc(w / 2, h);
        p->data.yuv.a = (int **) palloc(w, h);
    }
    else
    {
        p->data.yuv.y = (int **) palloc(w, h);
        p->data.yuv.u = (int **) palloc(w, h);
        p->data.yuv.v = (int **) palloc(w, h);
        p->data.yuv.a = (int **) palloc(w, h);
    }

	// Set defaults for deterministic DPX output
	p->alpha = 0;
	p->ar1 = 16;
	p->ar2 = 9;
	p->framerate = 60.0;
	p->frm_no = 0;
	p->interlaced = 0;
	p->seq_len = 1;

    return p;
}

pic_t *pcreate_ext(format_t format, color_t color, chroma_t chroma, int w, int h, int bits)
{
	pic_t *p;
	int   dwords_per_sample;
	int i;

	dwords_per_sample = ((bits > 32) ? 2 : 1);

	p = malloc(sizeof(pic_t));

	if (p == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory.\n");
		exit(1);
	}

	p->format = format;
	p->color = color;
	p->chroma = chroma;

	p->w = w;
	p->h = h;

	if (color == RGB)
	{
		p->data.rgb.r = (int **)palloc(w * dwords_per_sample, h);
		p->data.rgb.g = (int **)palloc(w * dwords_per_sample, h);
		p->data.rgb.b = (int **)palloc(w * dwords_per_sample, h);
		p->data.rgb.a = (int **)palloc(w * dwords_per_sample, h);
	}
	else if (color >= G_1C && color <= G_8C)
	{
		for (i = 0; i <= color - G_1C; ++i)
		{
			p->data.gc[i] = (int **)palloc(w * dwords_per_sample, h);
		}
	}
	else if (chroma == YUV_420)
	{
		p->data.yuv.y = (int **)palloc(w * dwords_per_sample, h);
		p->data.yuv.u = (int **)palloc(w * dwords_per_sample / 2, h / 2);
		p->data.yuv.v = (int **)palloc(w * dwords_per_sample / 2, h / 2);
		p->data.yuv.a = (int **)palloc(w * dwords_per_sample, h);
	}
	else if (chroma == YUV_422)
	{

		p->data.yuv.y = (int **)palloc(w * dwords_per_sample, h);
		p->data.yuv.u = (int **)palloc(w * dwords_per_sample / 2, h);
		p->data.yuv.v = (int **)palloc(w * dwords_per_sample / 2, h);
		p->data.yuv.a = (int **)palloc(w * dwords_per_sample, h);
	}
	else if (chroma == YUV_444 || chroma == YUV_4444)
	{
		p->data.yuv.y = (int **)palloc(w * dwords_per_sample, h);
		p->data.yuv.u = (int **)palloc(w * dwords_per_sample, h);
		p->data.yuv.v = (int **)palloc(w * dwords_per_sample, h);
		p->data.yuv.a = (int **)palloc(w * dwords_per_sample, h);
	}
	else {
		fprintf(stderr, "pcreate_ext() failed, unrecognized chroma format\n");
		return NULL;
	}

	// Set defaults for deterministic DPX output
	p->alpha = 0;
	p->ar1 = UINT32_MAX;
	p->ar2 = UINT32_MAX;
	p->bits = bits;
	p->framerate = 0.0;
	p->frm_no = UINT32_MAX;
	p->interlaced = !(format == FRAME);
	p->seq_len = UINT32_MAX;
	p->limited_range = 1; 
	p->chroma_siting = 0;
	p->transfer = TF_BT_709;
	p->colorimetry = C_REC_709;

	return p;
}


void *pdestroy(pic_t *p)
{
    int i, c;

    if (p->color == RGB)
    {
        for (i = 0; i < p->h; i++)
        {
            free(p->data.rgb.r[i]);
            free(p->data.rgb.g[i]);
            free(p->data.rgb.b[i]);
            free(p->data.rgb.a[i]);
        }
        free(p->data.rgb.r);
        free(p->data.rgb.g);
        free(p->data.rgb.b);
        free(p->data.rgb.a);
    }
	else if (p->color >= G_1C && p->color <= G_8C)
	{
		for (c = 0; c <= p->color - G_1C; ++c)
		{
			for (i = 0; i < p->h; ++i)
				free(p->data.gc[c][i]);
			free(p->data.gc[c]);
		}
	}
	else if (p->chroma == YUV_420)
    {
        for (i = 0; i < p->h; i++)
        {
            free(p->data.yuv.y[i]);
            free(p->data.yuv.a[i]);
        }

        for (i = 0; i < p->h / 2; i++)
        {
            free(p->data.yuv.u[i]);
            free(p->data.yuv.v[i]);
        }
        free(p->data.yuv.y);
        free(p->data.yuv.u);
        free(p->data.yuv.v);
        free(p->data.yuv.a);
    }
    else
    {
        for (i = 0; i < p->h; i++)
        {
            free(p->data.yuv.y[i]);
            free(p->data.yuv.u[i]);
            free(p->data.yuv.v[i]);
            free(p->data.yuv.a[i]);
        }
        free(p->data.yuv.y);
        free(p->data.yuv.u);
        free(p->data.yuv.v);
        free(p->data.yuv.a);
    }
    free(p);
	return(NULL);
}


void rgb2yuv(pic_t *ip, pic_t *op)
{
    int i, j;
    float r, g, b;
    float y, u, v;
	 int black, half, max;

    if (ip->chroma != YUV_444)
    {
        fprintf(stderr, "ERROR: rgb2yuv() Incorrect input chroma type.\n");
        exit(1);
    }

    if (ip->color != RGB)
    {
        fprintf(stderr, "ERROR: rgb2yuv() Incorrect input color type.\n");
        exit(1);
    }

    if (ip->w != op->w || ip->h != op->h)
    {
        fprintf(stderr, "ERROR: rgb2yuv() Incorrect picture size.\n");
        exit(1);
    }

    if (op->chroma != YUV_444)
    {
        fprintf(stderr, "ERROR: rgb2yuv() Incorrect output chroma type.\n");
        exit(1);
    }

    if (op->color != YUV_SD && op->color != YUV_HD)
    {
        fprintf(stderr, "ERROR: rgb2yuv() Incorrect output color type.\n");
        exit(1);
    }
	if((ip->bits < 8) || (ip->bits > 16))
	{
		fprintf(stderr, "ERROR: rgb2yuv() Unsupported bit resolution (bits=%d).\n", ip->bits);
        exit(1);
	}

	black = 16 << (ip->bits - 8);
	half = 128 << (ip->bits - 8);
	max = (256 << (ip->bits - 8)) - 1;   

    for (i = 0; i < op->h; i++)
        for (j = 0; j < op->w; j++)
        {
            r = (float)ip->data.rgb.r[i][j];
            g = (float)ip->data.rgb.g[i][j];
            b = (float)ip->data.rgb.b[i][j];

            if (op->color == YUV_SD) // 601
            {
                y =  (float)(0.257 * r + 0.504 * g + 0.098 * b + black);
                u = (float)(-0.148 * r - 0.291 * g + 0.439 * b + half);
                v =  (float)(0.439 * r - 0.368 * g - 0.071 * b + half);
            }
            else if (op->color == YUV_HD) // 709
            {
                y =  (float)(0.183 * r + 0.614 * g + 0.062 * b + black);
                u = (float)(-0.101 * r - 0.338 * g + 0.439 * b + half);
                v =  (float)(0.439 * r - 0.399 * g - 0.040 * b + half);
            }
            else
            {
                fprintf(stderr, "ERROR: rgb2yuv() Incorrect output color type.\n");
                exit(1);
            }

            // rounding
            y += 0.5;
            u += 0.5;
            v += 0.5;

            op->data.yuv.y[i][j] = (int)CLAMP(y, 0, max);
            op->data.yuv.u[i][j] = (int)CLAMP(u, 0, max);
            op->data.yuv.v[i][j] = (int)CLAMP(v, 0, max);
        }
}


void yuv2rgb(pic_t *ip, pic_t *op)
{
    int i, j;

    double r, g, b;
    float y, u, v;
	int black, half, max;

    if (ip->chroma != YUV_444)
    {
        fprintf(stderr, "ERROR: Incorrect input chroma type.\n");
        exit(1);
    }

    if (ip->color != YUV_SD && ip->color != YUV_HD)
    {
        fprintf(stderr, "ERROR: Incorrect input color type.\n");
        exit(1);
    }

    if (ip->w != op->w || ip->h != op->h)
    {
        fprintf(stderr, "ERROR: Incorrect picture size.\n");
        exit(1);
    }

    if (op->chroma != YUV_444)
    {
        fprintf(stderr, "ERROR: Incorrect output chroma type.\n");
        exit(1);
    }

    if (op->color != RGB)
    {
        fprintf(stderr, "ERROR: Incorrect output color type.\n");
        exit(1);
    }

	black = 16<<(ip->bits-8);
	half = 128<<(ip->bits-8);
	max = (256<<(ip->bits-8))-1;

    for (i = 0; i < op->h; i++)
        for (j = 0; j < op->w; j++)
        {
            y = (float)ip->data.yuv.y[i][j];
            u = (float)ip->data.yuv.u[i][j];
            v = (float)ip->data.yuv.v[i][j];

            if (ip->color == YUV_SD) // 601
            {
                r = 1.164 * (y - black) + 1.596 * (v - half);
                g = 1.164 * (y - black) - 0.813 * (v - half) - 0.391 * (u - half);
                b = 1.164 * (y - black)                     + 2.018 * (u - half);
            }
            else if (ip->color == YUV_HD) // 709
            {
                r = 1.164 * (y - black) + 1.793 * (v - half);
                g = 1.164 * (y - black) - 0.534 * (v - half) - 0.213 * (u - half);
                b = 1.164 * (y - black)                     + 2.115 * (u - half);
            }
            else
            {
                fprintf(stderr, "ERROR: Incorrect output color type.\n");
                exit(1);
            }

            r += 0.5;
            g += 0.5;
            b += 0.5;

            op->data.rgb.r[i][j] = (int)CLAMP(r, 0, max);
            op->data.rgb.g[i][j] = (int)CLAMP(g, 0, max);
            op->data.rgb.b[i][j] = (int)CLAMP(b, 0, max);
        }
}

//
// 2D convolution.
//
float conv(int **p, fir_t fir, int w, int h, int x, int y)
{
    int   i, j, k, ii, jj, kk;
    float sum;

    sum = 0.0;

    for (i = 0; i < fir.tap; i++)
    {
        ii = y - fir.tap / 2 + i;
        ii = CLAMP(ii, 0, h - 1);

        k = i * fir.sub;

        for (j = 0; j < fir.sub; j++)
        {
            jj = x - fir.sub / 2 + j;
            jj = CLAMP(jj, 0, w - 1);

            kk = k + j;

            sum += p[ii][jj] * fir.coeff[kk];
        }
    }
    return sum;
}




//
// Chroma 444 to 422 conversion. (Catmull-Rom)
//
void yuv_422_444(pic_t *ip, pic_t *op)
{
	int i, j, max;
	int m1, z, p1, p2;

	max = (1<<ip->bits)-1;

	for(i=0; i<ip->h; ++i)
	{
		for(j=0; j<ip->w/2; ++j)
		{
			op->data.yuv.y[i][2*j] = ip->data.yuv.y[i][2*j];
			op->data.yuv.y[i][2*j+1] = ip->data.yuv.y[i][2*j+1];
			op->data.yuv.u[i][2*j] = ip->data.yuv.u[i][j];
			op->data.yuv.v[i][2*j] = ip->data.yuv.v[i][j];
			m1 = ip->data.yuv.u[i][CLAMP(j-1,0,ip->w/2-1)];
			z = ip->data.yuv.u[i][j];
			p1 = ip->data.yuv.u[i][CLAMP(j+1,0,ip->w/2-1)];
			p2 = ip->data.yuv.u[i][CLAMP(j+2,0,ip->w/2-1)];
			op->data.yuv.u[i][2*j+1] = CLAMP((9 * (z + p1) - (m1 + p2) + 8) >> 4, 0, max);
			m1 = ip->data.yuv.v[i][CLAMP(j-1,0,ip->w/2-1)];
			z = ip->data.yuv.v[i][j];
			p1 = ip->data.yuv.v[i][CLAMP(j+1,0,ip->w/2-1)];
			p2 = ip->data.yuv.v[i][CLAMP(j+2,0,ip->w/2-1)];
			op->data.yuv.v[i][2*j+1] = CLAMP((9 * (z + p1) - (m1 + p2) + 8) >> 4, 0, max);
		}
	}
}

void yuv_444_422(pic_t *ip, pic_t *op)
{
    int   i, j, max;
    float u, v;
	fir_t fir = chm_444_422_03;

    if (ip->chroma != YUV_444)
    {
		if (ip->chroma != YUV_422)
		{
			printf("ERROR: Incorrect input chroma type.\n");
			exit(1);
		}
		else
		{
		  printf("4:2:2 input chroma type.\n");
		  for (i = 0; i < op->h; i++)
			  for (j = 0; j < op->w; j++)
				op->data.yuv.y[i][j] = ip->data.yuv.y[i][j];

		  for (i = 0; i < op->h; i++)
			  for (j = 0; j < op->w / 2; j++)
			  {
				//u = conv(ip->data.yuv.u, fir, op->w, op->h, 2 * j, i);
				op->data.yuv.u[i][j] = ip->data.yuv.u[i][j];
			  }

		  for (i = 0; i < op->h; i++)
			  for (j = 0; j < op->w / 2; j++)
			  {
				//u = conv(ip->data.yuv.u, fir, op->w, op->h, 2 * j, i);
				op->data.yuv.v[i][j] = ip->data.yuv.v[i][j];
			  }
		}  
    }
    else
    {
	    if (ip->w != op->w || ip->h != op->h)
	    {
			printf("ERROR: Incorrect picture size.\n");
			exit(1);
		}

	    op->chroma = YUV_422; // force to 4:2:2

		max = (1<<ip->bits)-1;

	//
	// Y
	//
		for (i = 0; i < op->h; i++)
			for (j = 0; j < op->w; j++)
				op->data.yuv.y[i][j] = ip->data.yuv.y[i][j];

	//
	// Cb
	//
		for (i = 0; i < op->h; i++)
			for (j = 0; j < op->w / 2; j++)
			{
				u = conv(ip->data.yuv.u, fir, op->w, op->h, 2 * j, i);
				op->data.yuv.u[i][j] = (int)CLAMP(u, 0, max);
			}

	//
	// Cr
	//
		for (i = 0; i < op->h; i++)
			for (j = 0; j < op->w / 2; j++)
			{
				v = conv(ip->data.yuv.v, fir, op->w, op->h, 2 * j, i);
				op->data.yuv.v[i][j] = (int)CLAMP(v, 0, max);
			}
	}
}

//
// Chroma 422 to 420 conversion.
// (simple 3-tap filter to cosited siting)
//
void yuv_422_420(pic_t *ip, pic_t *op)
{
    int   i, j, max;
    float u, v;
	fir_t fir = chm_422_420_03;

    if (ip->chroma != YUV_422)
    {
		printf("ERROR: Incorrect input chroma type.\n");
		exit(1);
    }
    else
    {
	    if (ip->w != op->w || ip->h != op->h)
	    {
			printf("ERROR: Incorrect picture size.\n");
			exit(1);
		}

	    op->chroma = YUV_420; // force to 4:2:0

		max = (1<<ip->bits)-1;

	//
	// Y
	//
		for (i = 0; i < op->h; i++)
			for (j = 0; j < op->w; j++)
				op->data.yuv.y[i][j] = ip->data.yuv.y[i][j];

	//
	// Cb
	//
		for (i = 0; i < op->h / 2; i++)
			for (j = 0; j < op->w / 2; j++)
			{
				u = conv(ip->data.yuv.u, fir, op->w/2, op->h, j, 2*i);
				op->data.yuv.u[i][j] = (int)CLAMP(u, 0, max);
			}

	//
	// Cr
	//
		for (i = 0; i < op->h/2; i++)
			for (j = 0; j < op->w / 2; j++)
			{
				v = conv(ip->data.yuv.v, fir, op->w/2, op->h, j, 2*i);
				op->data.yuv.v[i][j] = (int)CLAMP(v, 0, max);
			}
	}
}


// Catmull-Rom (cosited chroma upsampling)
void yuv_420_422(pic_t *ip, pic_t *op)
{
	int i, j, max;
	int m1, z, p1, p2;

	max = (1<<ip->bits)-1;

	for(i=0; i<ip->h/2; ++i)
	{
		for(j=0; j<ip->w/2; ++j)
		{
			op->data.yuv.y[2*i][2*j] = ip->data.yuv.y[2*i][2*j];
			op->data.yuv.y[2*i][2*j+1] = ip->data.yuv.y[2*i][2*j+1];
			op->data.yuv.y[2*i+1][2*j] = ip->data.yuv.y[2*i+1][2*j];
			op->data.yuv.y[2*i+1][2*j+1] = ip->data.yuv.y[2*i+1][2*j+1];
			op->data.yuv.u[2*i][j] = ip->data.yuv.u[i][j];
			op->data.yuv.v[2*i][j] = ip->data.yuv.v[i][j];
			m1 = ip->data.yuv.u[CLAMP(i-1,0,ip->h/2-1)][j];
			z = ip->data.yuv.u[i][j];
			p1 = ip->data.yuv.u[CLAMP(i+1,0,ip->h/2-1)][j];
			p2 = ip->data.yuv.u[CLAMP(i+2,0,ip->h/2-1)][j];
			op->data.yuv.u[2*i+1][j] = CLAMP((9 * (z + p1) - (m1 + p2) + 8) >> 4, 0, max);
			m1 = ip->data.yuv.v[CLAMP(i-1,0,ip->h/2-1)][j];
			z = ip->data.yuv.v[i][j];
			p1 = ip->data.yuv.v[CLAMP(i+1,0,ip->h/2-1)][j];
			p2 = ip->data.yuv.v[CLAMP(i+2,0,ip->h/2-1)][j];
			op->data.yuv.v[2*i+1][j] = CLAMP((9 * (z + p1) - (m1 + p2) + 8) >> 4, 0, max);
		}
	}
}



pic_t *convertbits(pic_t *p, int newbits)
{
	int ii, jj, kk;
	int error = 0;
	double s[8];
	pic_t *new_p;
	double scale;

	new_p = pcreate_ext(p->format, p->color, p->chroma, p->w, p->h, newbits);
	new_p->alpha = p->alpha;
	new_p->ar1 = p->ar1;
	new_p->ar2 = p->ar2;
	new_p->chroma_siting = p->chroma_siting;
	new_p->colorimetry = p->colorimetry;
	new_p->framerate = p->framerate;
	new_p->frm_no = p->frm_no;
	new_p->interlaced = p->interlaced;
	new_p->limited_range = p->limited_range;
	new_p->seq_len = p->seq_len;
	new_p->transfer = p->transfer;
	// This scale factor computaiton is rudimentary & should be redone to take into account the range, data sign, etc.
	if (p->bits < 32)
		scale = 1.0 / ((1 << p->bits) - 1);
	else
		scale = 1.0;		// Assume input is scale 0 - 1.0;
	if (newbits < 32)
		scale *= (double)((1 << newbits) - 1);

	if (p->color == RGB)
	{
		if (p->chroma == YUV_444 || p->chroma == YUV_4444)
		{
			// RGB 4:4:4
			for (ii = 0; ii < p->h; ++ii)
			{
				for (jj = 0; jj < p->w; ++jj)
				{
					if (p->bits < 32)
					{
						s[0] = (double)p->data.rgb.r[ii][jj];
						s[1] = (double)p->data.rgb.g[ii][jj];
						s[2] = (double)p->data.rgb.b[ii][jj];
						s[3] = (double)p->data.rgb.a[ii][jj];
					}
					else if (p->bits == 32) 
					{
						s[0] = (double)p->data.rgb_s.r[ii][jj];
						s[1] = (double)p->data.rgb_s.g[ii][jj];
						s[2] = (double)p->data.rgb_s.b[ii][jj];
						s[3] = (double)p->data.rgb_s.a[ii][jj];
					}
					else 
					{
						s[0] = p->data.rgb_d.r[ii][jj];
						s[1] = p->data.rgb_d.g[ii][jj];
						s[2] = p->data.rgb_d.b[ii][jj];
						s[3] = p->data.rgb_d.a[ii][jj];
					}
					if (newbits < 32)
					{
						new_p->data.rgb.r[ii][jj] = (int)(s[0] * scale + 0.5);
						new_p->data.rgb.g[ii][jj] = (int)(s[1] * scale + 0.5);
						new_p->data.rgb.b[ii][jj] = (int)(s[2] * scale + 0.5);
						new_p->data.rgb.a[ii][jj] = (int)(s[3] * scale + 0.5);
					}
					else if (newbits == 32) {
						new_p->data.rgb_s.r[ii][jj] = (float)(s[0] * scale);
						new_p->data.rgb_s.g[ii][jj] = (float)(s[1] * scale);
						new_p->data.rgb_s.b[ii][jj] = (float)(s[2] * scale);
						new_p->data.rgb_s.a[ii][jj] = (float)(s[3] * scale);
					}
					else 
					{
						new_p->data.rgb_d.r[ii][jj] = s[0] * scale;
						new_p->data.rgb_d.g[ii][jj] = s[1] * scale;
						new_p->data.rgb_d.b[ii][jj] = s[2] * scale;
						new_p->data.rgb_d.a[ii][jj] = s[3] * scale;
					}
				}
			}
		}
		else
		{
			// we don't handle any RGB that's not 4:4:4 at this point
			printf(" RGB 422 not supported.\n");
			error = 1;
		}
	}
	else {
		// YUV format (either SD or HD)
		if (p->chroma == YUV_444 || p->chroma == YUV_4444)
		{
			// YUV 4:4:4
			for (ii = 0; ii < p->h; ++ii)
			{
				for (jj = 0; jj < p->w; ++jj)
				{
					if (p->bits < 32)
					{
						s[0] = (double)p->data.yuv.y[ii][jj];
						s[1] = (double)p->data.yuv.u[ii][jj];
						s[2] = (double)p->data.yuv.v[ii][jj];
						s[3] = (double)p->data.yuv.a[ii][jj];
					}
					else if (p->bits == 32)
					{
						s[0] = (double)p->data.yuv_s.y[ii][jj];
						s[1] = (double)p->data.yuv_s.u[ii][jj];
						s[2] = (double)p->data.yuv_s.v[ii][jj];
						s[3] = (double)p->data.yuv_s.a[ii][jj];
					}
					else
					{
						s[0] = p->data.yuv_d.y[ii][jj];
						s[1] = p->data.yuv_d.u[ii][jj];
						s[2] = p->data.yuv_d.v[ii][jj];
						s[3] = p->data.yuv_d.a[ii][jj];
					}
					if (newbits < 32)
					{
						new_p->data.yuv.y[ii][jj] = (int)(s[0] * scale + 0.5);
						new_p->data.yuv.u[ii][jj] = (int)(s[1] * scale + 0.5);
						new_p->data.yuv.v[ii][jj] = (int)(s[2] * scale + 0.5);
						new_p->data.yuv.a[ii][jj] = (int)(s[3] * scale + 0.5);
					}
					else if (newbits == 32)
					{
						new_p->data.yuv_s.y[ii][jj] = (float)(s[0] * scale);
						new_p->data.yuv_s.u[ii][jj] = (float)(s[1] * scale);
						new_p->data.yuv_s.v[ii][jj] = (float)(s[2] * scale);
						new_p->data.yuv_s.a[ii][jj] = (float)(s[3] * scale);
					}
					else
					{
						new_p->data.yuv_d.y[ii][jj] = s[0] * scale;
						new_p->data.yuv_d.u[ii][jj] = s[1] * scale;
						new_p->data.yuv_d.v[ii][jj] = s[2] * scale;
						new_p->data.yuv_d.a[ii][jj] = s[3] * scale;
					}
				}
			}
		}
		else
		{
			if (p->chroma == YUV_422)
			{
				for (ii = 0; ii < p->h; ++ii)
				{
					for (jj = 0; jj < (p->w / 2); ++jj)
					{
						if (p->bits < 32)
						{
							s[0] = (double)p->data.yuv.u[ii][jj];
							s[1] = (double)p->data.yuv.y[ii][jj * 2];
							s[2] = (double)p->data.yuv.v[ii][jj];
							s[3] = (double)p->data.yuv.y[ii][jj * 2 + 1];
							s[4] = (double)p->data.yuv.a[ii][jj * 2];
							s[5] = (double)p->data.yuv.a[ii][jj * 2 + 1];
						}
						else if (p->bits == 32)
						{
							s[0] = (double)p->data.yuv_s.u[ii][jj];
							s[1] = (double)p->data.yuv_s.y[ii][jj * 2];
							s[2] = (double)p->data.yuv_s.v[ii][jj];
							s[3] = (double)p->data.yuv_s.y[ii][jj * 2 + 1];
							s[4] = (double)p->data.yuv_s.a[ii][jj * 2];
							s[5] = (double)p->data.yuv_s.a[ii][jj * 2 + 1];
						}
						else
						{
							s[0] = p->data.yuv.u[ii][jj];
							s[1] = p->data.yuv.y[ii][jj * 2];
							s[2] = p->data.yuv.v[ii][jj];
							s[3] = p->data.yuv.y[ii][jj * 2 + 1];
							s[4] = p->data.yuv.a[ii][jj * 2];
							s[5] = p->data.yuv.a[ii][jj * 2 + 1];
						}
						if (newbits < 32)
						{
							new_p->data.yuv.u[ii][jj] = (int)(s[0] * scale + 0.5);
							new_p->data.yuv.y[ii][jj * 2] = (int)(s[1] * scale + 0.5);
							new_p->data.yuv.v[ii][jj] = (int)(s[2] * scale + 0.5);
							new_p->data.yuv.y[ii][jj * 2 + 1] = (int)(s[3] * scale + 0.5);
							new_p->data.yuv.a[ii][jj * 2] = (int)(s[4] * scale + 0.5);
							new_p->data.yuv.a[ii][jj * 2 + 1] = (int)(s[5] * scale + 0.5);
						}
						else if (newbits == 32)
						{
							new_p->data.yuv_s.u[ii][jj] = (float)(s[0] * scale);
							new_p->data.yuv_s.y[ii][jj * 2] = (float)(s[1] * scale);
							new_p->data.yuv_s.v[ii][jj] = (float)(s[2] * scale);
							new_p->data.yuv_s.y[ii][jj * 2 + 1] = (float)(s[3] * scale);
							new_p->data.yuv_s.a[ii][jj * 2] = (float)(s[4] * scale);
							new_p->data.yuv_s.a[ii][jj * 2 + 1] = (float)(s[5] * scale);
						}
						else {
							new_p->data.yuv_d.u[ii][jj] = s[0] * scale;
							new_p->data.yuv_d.y[ii][jj * 2] = s[1] * scale;
							new_p->data.yuv_d.v[ii][jj] = s[2] * scale;
							new_p->data.yuv_d.y[ii][jj * 2 + 1] = s[3] * scale;
							new_p->data.yuv_d.a[ii][jj * 2] = s[4] * scale;
							new_p->data.yuv_d.a[ii][jj * 2 + 1] = s[5] * scale;
						}
					}
				}
			}
			else if (p->chroma == YUV_420)
			{
				for (ii = 0; ii < p->h / 2; ++ii)
				{
					for (jj = 0; jj < (p->w / 2); ++jj)
					{
						if (p->bits < 32)
						{
							s[0] = (double)p->data.yuv.u[ii][jj];
							s[1] = (double)p->data.yuv.v[ii][jj];
							s[2] = (double)p->data.yuv.y[ii * 2][jj * 2];
							s[3] = (double)p->data.yuv.y[ii * 2][jj * 2 + 1];
							s[4] = (double)p->data.yuv.y[ii * 2 + 1][jj * 2];
							s[5] = (double)p->data.yuv.y[ii * 2 + 1][jj * 2 + 1];
							s[6] = (double)p->data.yuv.a[ii * 2][jj * 2];
							s[7] = (double)p->data.yuv.a[ii * 2][jj * 2 + 1];
							s[8] = (double)p->data.yuv.a[ii * 2 + 1][jj * 2];
							s[9] = (double)p->data.yuv.a[ii * 2 + 1][jj * 2 + 1];
						}
						else if (p->bits == 32)
						{
							s[0] = (double)p->data.yuv_s.u[ii][jj];
							s[1] = (double)p->data.yuv_s.v[ii][jj];
							s[2] = (double)p->data.yuv_s.y[ii * 2][jj * 2];
							s[3] = (double)p->data.yuv_s.y[ii * 2][jj * 2 + 1];
							s[4] = (double)p->data.yuv_s.y[ii * 2 + 1][jj * 2];
							s[5] = (double)p->data.yuv_s.y[ii * 2 + 1][jj * 2 + 1];
							s[6] = (double)p->data.yuv_s.a[ii * 2][jj * 2];
							s[7] = (double)p->data.yuv_s.a[ii * 2][jj * 2 + 1];
							s[8] = (double)p->data.yuv_s.a[ii * 2 + 1][jj * 2];
							s[9] = (double)p->data.yuv_s.a[ii * 2 + 1][jj * 2 + 1];
						}
						else
						{
							s[0] = p->data.yuv.u[ii][jj];
							s[1] = p->data.yuv.v[ii][jj];
							s[2] = p->data.yuv.y[ii * 2][jj * 2];
							s[3] = p->data.yuv.y[ii * 2][jj * 2 + 1];
							s[4] = p->data.yuv.y[ii * 2 + 1][jj * 2];
							s[5] = p->data.yuv.y[ii * 2 + 1][jj * 2 + 1];
							s[6] = p->data.yuv.a[ii * 2][jj * 2];
							s[7] = p->data.yuv.a[ii * 2][jj * 2 + 1];
							s[8] = p->data.yuv.a[ii * 2 + 1][jj * 2];
							s[9] = p->data.yuv.a[ii * 2 + 1][jj * 2 + 1];
						}
						if (newbits < 32)
						{
							new_p->data.yuv.u[ii][jj] = (int)(s[0] * scale + 0.5);
							new_p->data.yuv.v[ii][jj] = (int)(s[1] * scale + 0.5);
							new_p->data.yuv.y[ii * 2][jj * 2] = (int)(s[2] * scale + 0.5);
							new_p->data.yuv.y[ii * 2][jj * 2 + 1] = (int)(s[3] * scale + 0.5);
							new_p->data.yuv.y[ii * 2 + 1][jj * 2] = (int)(s[4] * scale + 0.5);
							new_p->data.yuv.y[ii * 2 + 1][jj * 2 + 1] = (int)(s[5] * scale + 0.5);
							new_p->data.yuv.a[ii * 2][jj * 2] = (int)(s[6] * scale + 0.5);
							new_p->data.yuv.a[ii * 2][jj * 2 + 1] = (int)(s[7] * scale + 0.5);
							new_p->data.yuv.a[ii * 2 + 1][jj * 2] = (int)(s[8] * scale + 0.5);
							new_p->data.yuv.a[ii * 2 + 1][jj * 2 + 1] = (int)(s[9] * scale + 0.5);
						}
						else if (newbits == 32)
						{
							new_p->data.yuv_s.u[ii][jj] = (float)(s[0] * scale);
							new_p->data.yuv_s.v[ii][jj] = (float)(s[1] * scale);
							new_p->data.yuv_s.y[ii * 2][jj * 2] = (float)(s[2] * scale);
							new_p->data.yuv_s.y[ii * 2][jj * 2 + 1] = (float)(s[3] * scale);
							new_p->data.yuv_s.y[ii * 2 + 1][jj * 2] = (float)(s[4] * scale);
							new_p->data.yuv_s.y[ii * 2 + 1][jj * 2 + 1] = (float)(s[5] * scale);
							new_p->data.yuv_s.a[ii * 2][jj * 2] = (float)(s[6] * scale);
							new_p->data.yuv_s.a[ii * 2][jj * 2 + 1] = (float)(s[7] * scale);
							new_p->data.yuv_s.a[ii * 2 + 1][jj * 2] = (float)(s[8] * scale);
							new_p->data.yuv_s.a[ii * 2 + 1][jj * 2 + 1] = (float)(s[9] * scale);
						}
						else
						{
							new_p->data.yuv_d.u[ii][jj] = s[0] * scale;
							new_p->data.yuv_d.v[ii][jj] = s[1] * scale;
							new_p->data.yuv_d.y[ii * 2][jj * 2] = s[2] * scale;
							new_p->data.yuv_d.y[ii * 2][jj * 2 + 1] = s[3] * scale;
							new_p->data.yuv_d.y[ii * 2 + 1][jj * 2] = s[4] * scale;
							new_p->data.yuv_d.y[ii * 2 + 1][jj * 2 + 1] = s[5] * scale;
							new_p->data.yuv_d.a[ii * 2][jj * 2] = s[6] * scale;
							new_p->data.yuv_d.a[ii * 2][jj * 2 + 1] = s[7] * scale;
							new_p->data.yuv_d.a[ii * 2 + 1][jj * 2] = s[8] * scale;
							new_p->data.yuv_d.a[ii * 2 + 1][jj * 2 + 1] = s[9] * scale;
						}
					}
				}
			}
			else if (p->color >= G_1C && p->color <= G_8C)
			{
				for (ii = 0; ii < p->h / 2; ++ii)
				{
					for (jj = 0; jj < (p->w / 2); ++jj)
					{
						for (kk = 0; kk <= p->color - G_1C; ++kk)
						{
							if (p->bits < 32)
								s[kk] = (double)p->data.gc[kk][ii][jj];
							else if (p->bits == 32)
								s[kk] = (double)p->data.gc_s[kk][ii][jj];
							else
								s[kk] = p->data.gc_d[kk][ii][jj];
							if (newbits < 32)
								new_p->data.gc[kk][ii][jj] = (int)(s[kk] * scale + 0.5);
							else if (newbits == 32)
								new_p->data.gc_s[kk][ii][jj] = (float)(s[kk] * scale);
							else
								new_p->data.gc_d[kk][ii][jj] = s[kk] * scale;
						}
					}
				}
			}
			else
			{  // TBD: add GC support
				error = 1;
			}
		}
	}

	if (error) {
		fprintf(stderr, "ERROR: Calling convert8to10() with incompatible format.  Only handle RGB444 or YUV422 or YUV444.\n");
		exit(1);
	}
	return(new_p);
}

void gettoken(FILE *fp, char *token, char *line, int *pos)
{
	char c;
	int count = 0;

	// Get whitespace
	c = line[(*pos)++];
	while ((c == '\0') || (c == ' ') || (c == '\t') || (c == 10) || (c == 13) || (c == '#'))
	{
		if (c == '\0' || c == '\n' || c == '#')
		{
			if (fgets(line, 1000, fp) == NULL)
			{
				token[0] = '\0';
				return;
			}
			*pos = 0;
		}
		c = line[(*pos)++];
	}

	// Get token
	while (count<999 && !((c == '\0') || (c == ' ') || (c == '\t') || (c == 10) || (c == 13) || (c == '#')))
	{
		token[count++] = c;
		c = line[(*pos)++];
	}
	token[count] = '\0';
}

pic_t *readppm(FILE *fp)
{
	pic_t *p;

	char magicnum[128];
	char line[1000];
	char token[1000];

	int w, h;
	int i, j;
	int g = 0;
	int maxval;
	int pos = 0;
	int bits;

	if (fgets(line, 1000, fp) == NULL)
		Err("Error reading PPM file\n");
	gettoken(fp, token, line, &pos);

	if (token[0] != 'P')
	{
		Err("Incorrect file type.");
	}
	strcpy(magicnum, token);

	gettoken(fp, token, line, &pos);
	w = atoi(token);
	gettoken(fp, token, line, &pos);
	h = atoi(token);
	gettoken(fp, token, line, &pos);
	maxval = atoi(token);

	if (maxval <= 255)
		bits = 8;
	else if (maxval <= 1023)
		bits = 10;
	else if (maxval <= 4095)
		bits = 12;
	else if (maxval <= 16383)
		bits = 14;
	else if (maxval <= 65535)
		bits = 16;
	else
	{
		printf("PPM read error, maxval = %d\n", maxval);
		return(NULL);
	}
	p = pcreate_ext(FRAME, RGB, YUV_444, w, h, bits);

	if (magicnum[1] == '2')
		for (i = 0; i < h; i++)
			for (j = 0; j < w; j++)
			{
				if (fscanf(fp, "%d", &g) != 1)  // Gray value in PGM
					Err("Error reading PPM (PGM) file\n");
				p->data.rgb.r[i][j] = g;
				p->data.rgb.g[i][j] = g;
				p->data.rgb.b[i][j] = g;
			}
	else if (magicnum[1] == '3')
	{
		int c, v;
		i = 0; j = 0; c = 0;
		while (i < h)
		{
			char *rest;
			do
			{
				int sz = -1;
				do
				{
					sz++;
					line[sz] = fgetc(fp);
					if (feof(fp))
						line[++sz] = '\n';
				} while ((line[sz] != '\n') && ((sz < 900) || ((line[sz] >= '0') && (line[sz] <= '9'))));
				line[sz + 1] = '\0';
			} while (line[0] == '#');
			rest = line;
			while ((rest[0] != '\0') && ((rest[0] < '0') || (rest[0] > '9'))) rest++;
			while (rest[0] != '\0')
			{
				v = 0;
				while ((rest[0] >= '0') && (rest[0] <= '9'))
				{
					v = 10 * v + (rest[0] - '0');
					rest++;
				}
				if (c == 0)      p->data.rgb.r[i][j] = v;
				else if (c == 1) p->data.rgb.g[i][j] = v;
				else if (c == 2) p->data.rgb.b[i][j] = v;
				c++;
				if (c>2)
				{
					c = 0; j++;
					if (j >= w)
					{
						j = 0; i++;
					}
				}
				while ((rest[0] != '\0') && ((rest[0] < '0') || (rest[0] > '9'))) rest++;
			}
		}
	}
	else if (magicnum[1] == '5') // PGM binary
		for (i = 0; i < h; i++)
			for (j = 0; j < w; j++)
			{
				g = (unsigned char)fgetc(fp);  // Gray value
				if (maxval > 255)
					g = (g << 8) + (unsigned char)fgetc(fp);
				p->data.rgb.r[i][j] = g;
				p->data.rgb.g[i][j] = g;
				p->data.rgb.b[i][j] = g;
			}
	else // P6
	{
		for (i = 0; i < h; i++)
		{
			for (j = 0; j < w; j++)
			{
				p->data.rgb.r[i][j] = (unsigned char)fgetc(fp);
				if (maxval > 255)
					p->data.rgb.r[i][j] = (p->data.rgb.r[i][j] << 8) + (unsigned char)fgetc(fp);
				p->data.rgb.g[i][j] = (unsigned char)fgetc(fp);
				if (maxval > 255)
					p->data.rgb.g[i][j] = (p->data.rgb.g[i][j] << 8) + (unsigned char)fgetc(fp);
				p->data.rgb.b[i][j] = (unsigned char)fgetc(fp);
				if (maxval > 255)
					p->data.rgb.b[i][j] = (p->data.rgb.b[i][j] << 8) + (unsigned char)fgetc(fp);
			}
		}
	}

	return p;
}

int ppm_read(char *fname, pic_t **pic)
{
	FILE *fp;

	fp = fopen(fname, "rb");

	if (fp == NULL)
		return(-1);  // Error condition

	*pic = readppm(fp);

	fclose(fp);
	return(0);
}


void writeppm(FILE *fp, pic_t *p)
{
	int i, j;

	fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", p->w, p->h);
	fprintf(fp, "%d\n", (1 << p->bits) - 1);

	for (i = 0; i < p->h; i++)
		for (j = 0; j < p->w; j++)
		{
			if (p->bits>8)
				fputc((unsigned char)(p->data.rgb.r[i][j] >> 8), fp);
			fputc((unsigned char)p->data.rgb.r[i][j] & 0xff, fp);
			if (p->bits>8)
				fputc((unsigned char)(p->data.rgb.g[i][j] >> 8), fp);
			fputc((unsigned char)p->data.rgb.g[i][j] & 0xff, fp);
			if (p->bits>8)
				fputc((unsigned char)(p->data.rgb.b[i][j] >> 8), fp);
			fputc((unsigned char)p->data.rgb.b[i][j] & 0xff, fp);
		}
}


int ppm_write(char *fname, pic_t *pic_in)
{
	FILE *fp;
	pic_t *pic, *temp_pic, *temp_pic2;

	fp = fopen(fname, "wb");

	if (fp == NULL)
		return(-1);  // Error condition

	if (pic_in->bits == 32 || pic_in->bits == 64)  // Create a 16-bit fixed point pic from a floating point one
	{
		temp_pic = convertbits(pic_in, 16);
		pic = temp_pic;
	}
	else
		pic = pic_in;

	if (pic->chroma == YUV_420)
	{
		temp_pic = pcreate_ext(0, pic->color, YUV_422, pic->w, pic->h, pic->bits);
		yuv_420_422(pic, temp_pic);
		temp_pic2 = pcreate_ext(0, pic->color, YUV_444, pic->w, pic->h, pic->bits);
		yuv_422_444(temp_pic, temp_pic2);
		pdestroy(temp_pic);
		temp_pic = pcreate_ext(0, RGB, YUV_444, pic->w, pic->h, pic->bits);
		yuv2rgb(temp_pic2, temp_pic);
		writeppm(fp, temp_pic);
		pdestroy(temp_pic);
		pdestroy(temp_pic2);
	}
	else if (pic->chroma == YUV_422)
	{
		temp_pic2 = pcreate_ext(0, pic->color, YUV_444, pic->w, pic->h, pic->bits);
		yuv_422_444(pic, temp_pic2);
		temp_pic = pcreate_ext(0, RGB, YUV_444, pic->w, pic->h, pic->bits);
		temp_pic->bits = pic->bits;
		yuv2rgb(temp_pic2, temp_pic);
		writeppm(fp, temp_pic);
		pdestroy(temp_pic);
		pdestroy(temp_pic2);
	}
	else if (pic->color != RGB)
	{
		temp_pic = pcreate_ext(0, RGB, YUV_444, pic->w, pic->h, pic->bits);
		yuv2rgb(pic, temp_pic);
		writeppm(fp, temp_pic);
		pdestroy(temp_pic);
	}
	else
		writeppm(fp, pic);

	if (pic_in->bits == 32 || pic_in->bits == 64)
		pdestroy(pic);

	fclose(fp);
	return(0);
}


int uyvy_read(pic_t *ip, FILE *fp)
{
	int i, j;
	int mode16bit;
	int shift;

	mode16bit = ip->bits > 8;
	shift = mode16bit ? (16 - ip->bits) : 0;
	for (i = 0; i<ip->h; ++i)
		for (j = 0; j<ip->w / 2; ++j)
		{
			ip->data.yuv.u[i][j] = fgetc(fp);
			if (mode16bit) ip->data.yuv.u[i][j] |= fgetc(fp) << 8;
			ip->data.yuv.y[i][2 * j] = fgetc(fp);
			if (mode16bit) ip->data.yuv.y[i][2 * j] |= fgetc(fp) << 8;
			ip->data.yuv.v[i][j] = fgetc(fp);
			if (mode16bit) ip->data.yuv.v[i][j] |= fgetc(fp) << 8;
			ip->data.yuv.y[i][2 * j + 1] = fgetc(fp);
			if (mode16bit) ip->data.yuv.y[i][2 * j + 1] |= fgetc(fp) << 8;
			ip->data.yuv.u[i][j] >>= shift;
			ip->data.yuv.y[i][2 * j] >>= shift;
			ip->data.yuv.v[i][j] >>= shift;
			ip->data.yuv.y[i][2 * j + 1] >>= shift;
		}
	fclose(fp);
	return(0);
}


int yuv_read(char *fname, pic_t **ip, int width, int height, int framenum, int bpc, int *numframes, int filetype)
{
	FILE *fp;
	int i, j;
	int framesize;
	int mode16bit;
	int maxval;

	mode16bit = (bpc>8);
	maxval = (1 << bpc) - 1;

	if ((fp = fopen(fname, "rb")) == NULL)
	{
		printf("Could not read YUV file %s\n", fname);
		return(1);
	}

	*ip = pcreate_ext(0, YUV_HD, (filetype == 1) ? YUV_422 : YUV_420, width, height, bpc);

	if (filetype == 1)
	{
		*numframes = 1;
		return(uyvy_read(*ip, fp));
	}

	framesize = width*height * 3 / (mode16bit ? 1 : 2);  // number of bytes
#ifdef WIN32
	_fseeki64(fp, 0, SEEK_END);
	*numframes = (int)((_ftelli64(fp) + 1) / (long long)framesize);
	if (*numframes == 0)
	{
		// for some reason the _ftelli64 fails sometimes
	}
	_fseeki64(fp, (__int64)framesize*framenum, SEEK_SET);
#else
	fseeko(fp, 0, SEEK_END);
	{
		//struct stat st;
		//*numframes = (int)((st.st_size+1) / framesize);
		*numframes = (int)((ftello(fp) + 1) / framesize);
		fseeko(fp, (off_t)framesize*framenum, SEEK_SET);
	}
#endif

	for (i = 0; i<height; ++i)
		for (j = 0; j<width; ++j)
		{
			(*ip)->data.yuv.y[i][j] = fgetc(fp);
			if (mode16bit)
				(*ip)->data.yuv.y[i][j] |= (fgetc(fp) << 8);
			if ((*ip)->data.yuv.y[i][j] < 0 || (*ip)->data.yuv.y[i][j] > maxval)
			{
				printf("YUV read error: sample value did not match expected bit depth\n");
				return (1);
			}
			(*ip)->data.yuv.y[i][j] = fgetc(fp);
			if (mode16bit)
				(*ip)->data.yuv.y[i][j] |= (fgetc(fp) << 8);
			if ((*ip)->data.yuv.y[i][j] < 0 || (*ip)->data.yuv.y[i][j] > maxval)
			{
				printf("YUV read error: sample value did not match expected bit depth\n");
				return (1);
			}
		}
	for (i = 0; i<height / 2; ++i)
		for (j = 0; j<width / 2; ++j)
		{
			(*ip)->data.yuv.u[i][j] = fgetc(fp);
			if (mode16bit)
				(*ip)->data.yuv.u[i][j] |= (fgetc(fp) << 8);
		}
	for (i = 0; i<height / 2; ++i)
		for (j = 0; j<width / 2; ++j)
		{
			(*ip)->data.yuv.v[i][j] = fgetc(fp);
			if (mode16bit)
				(*ip)->data.yuv.v[i][j] |= (fgetc(fp) << 8);
		}

	fclose(fp);
	return(0);
}


int uyvy_write(pic_t *op, FILE *fp)
{
	int i, j;
	int mode16bit;
	int shift, c;

	mode16bit = op->bits > 8;
	shift = mode16bit ? (16 - op->bits) : 0;
	for (i = 0; i<op->h; ++i)
		for (j = 0; j<op->w / 2; ++j)
		{
			c = op->data.yuv.u[i][j] << shift;
			fputc(c & 0xff, fp);
			if (mode16bit) fputc((c >> 8) & 0xff, fp);
			c = op->data.yuv.y[i][2 * j] << shift;
			fputc(c & 0xff, fp);
			if (mode16bit) fputc((c >> 8) & 0xff, fp);
			c = op->data.yuv.v[i][j] << shift;
			fputc(c & 0xff, fp);
			if (mode16bit) fputc((c >> 8) & 0xff, fp);
			c = op->data.yuv.y[i][2 * j + 1] << shift;
			fputc(c & 0xff, fp);
			if (mode16bit) fputc((c >> 8) & 0xff, fp);
		}

	fclose(fp);
	return(0);
}


int yuv_write(char *fname, pic_t *op, int framenum, int filetype)
{
	FILE *fp;
	int i, j;
	int framesize;
	int width, height;
	int mode16bit;

	mode16bit = (op->bits>8);

	if (framenum == 0)
	{
		if ((fp = fopen(fname, "wb")) == NULL)
		{
			printf("Could not write YUV file %s\n", fname);
			return(1);
		}
	}
	else {
		if ((fp = fopen(fname, "ab")) == NULL)
		{
			printf("Could not write YUV file %s\n", fname);
			return(1);
		}
	}

	if (filetype == 1)
		return(uyvy_write(op, fp));

	width = op->w;  height = op->h;
	framesize = width*height * 3 / (mode16bit ? 1 : 2);  // number of bytes
	fseek(fp, framesize*framenum, SEEK_SET);

	for (i = 0; i<height; ++i)
		for (j = 0; j<width; ++j)
		{
			if (mode16bit)
			{
				fputc(op->data.yuv.y[i][j] & 0xff, fp);
				fputc(op->data.yuv.y[i][j] >> 8, fp);
			}
			else
				fputc(op->data.yuv.y[i][j], fp);
		}
	for (i = 0; i<height / 2; ++i)
		for (j = 0; j<width / 2; ++j)
		{
			if (mode16bit)
			{
				fputc(op->data.yuv.u[i][j] & 0xff, fp);
				fputc(op->data.yuv.u[i][j] >> 8, fp);
			}
			else
				fputc(op->data.yuv.u[i][j], fp);
		}
	for (i = 0; i<height / 2; ++i)
		for (j = 0; j<width / 2; ++j)
		{
			if (mode16bit)
			{
				fputc(op->data.yuv.v[i][j] & 0xff, fp);
				fputc(op->data.yuv.v[i][j] >> 8, fp);
			}
			else
				fputc(op->data.yuv.v[i][j], fp);
		}
	fclose(fp);
	return(0);
}

pic_t *pcopy(pic_t *ip)
{
	int i, j, c;
	int w;
	pic_t *op;

	op = pcreate_ext(ip->format, ip->color, ip->chroma, ip->w, ip->h, ip->bits);

	//if (ip->chroma != YUV_422 || op->chroma != YUV_422)
	//	UErr("Only 4:2:2 implemented so far...\n");
	op->alpha = ip->alpha;
	op->transfer = ip->transfer;
	op->colorimetry = ip->colorimetry;
	op->ar1 = ip->ar1;
	op->ar2 = ip->ar2;
	op->frm_no = ip->frm_no;
	op->seq_len = ip->seq_len;
	op->framerate = ip->framerate;
	op->interlaced = ip->interlaced;
	op->limited_range = ip->limited_range;
	op->chroma_siting = ip->chroma_siting;

	w = (ip->bits > 32) ? (ip->w * 2) : ip->w;

	if (ip->color == RGB)
	{
		for (i = 0; i < ip->h; ++i)
			for (j = 0; j < w; ++j)
			{
				op->data.rgb.r[i][j] = ip->data.rgb.r[i][j];
				op->data.rgb.g[i][j] = ip->data.rgb.g[i][j];
				op->data.rgb.b[i][j] = ip->data.rgb.b[i][j];
				op->data.rgb.a[i][j] = ip->data.rgb.a[i][j];
			}
	}
	else if (ip->color >= G_1C && ip->color <= G_8C)
	{
		for (i = 0; i < ip->h; ++i)
			for (j = 0; j < w; ++j)
				for (c = 0; c <= ip->color - G_1C; ++c)
					op->data.gc[c][i][j] = ip->data.gc[c][i][j];
	}
	else if (ip->chroma == YUV_444)
	{
		for (i = 0; i < ip->h; ++i)
			for (j = 0; j < w; ++j)
			{
				op->data.yuv.y[i][j] = ip->data.yuv.y[i][j];
				op->data.yuv.u[i][j] = ip->data.yuv.u[i][j];
				op->data.yuv.v[i][j] = ip->data.yuv.v[i][j];
				op->data.yuv.a[i][j] = ip->data.yuv.a[i][j];
			}
	}
	else if (ip->chroma == YUV_422)
	{
		for (i = 0; i < ip->h; ++i)
			for (j = 0; j < w / 2; ++j)
			{
				op->data.yuv.y[i][j * 2] = ip->data.yuv.y[i][j * 2];
				op->data.yuv.y[i][j * 2 + 1] = ip->data.yuv.y[i][j * 2 + 1];
				op->data.yuv.u[i][j] = ip->data.yuv.u[i][j];
				op->data.yuv.v[i][j] = ip->data.yuv.v[i][j];
			}
	}
	else  // 4:2:0
	{
		for (i = 0; i < ip->h; ++i)
			for (j = 0; j < w; ++j)
			{
				op->data.yuv.y[i][j] = ip->data.yuv.y[i][j];
				op->data.yuv.a[i][j] = ip->data.yuv.a[i][j];
				if (i < ip->h / 2 && j < w / 2)
				{
					op->data.yuv.u[i][j] = ip->data.yuv.u[i][j];
					op->data.yuv.v[i][j] = ip->data.yuv.v[i][j];
				}
			}
	}
	return(op);
}


int rgba_read(char *fname, pic_t **ip, int width, int height, int bpc, int alpha_present)
{
	FILE *fp;
	int i, j;
	int mode16bit;
	int maxval;

	mode16bit = (bpc>8);
	maxval = (1 << bpc) - 1;

	if ((fp = fopen(fname, "rb")) == NULL)
	{
		printf("Could not read YUV file %s\n", fname);
		return(1);
	}

	*ip = pcreate_ext(0, RGB, YUV_444, width, height, bpc);
	(*ip)->alpha = alpha_present;

	mode16bit = (*ip)->bits > 8;
	for (i = 0; i<(*ip)->h; ++i)
		for (j = 0; j<(*ip)->w; ++j)
		{
			(*ip)->data.rgb.r[i][j] = fgetc(fp);
			if (mode16bit) (*ip)->data.rgb.r[i][j] |= fgetc(fp) << 8;
			(*ip)->data.rgb.g[i][j] = fgetc(fp);
			if (mode16bit) (*ip)->data.rgb.g[i][j] |= fgetc(fp) << 8;
			(*ip)->data.rgb.b[i][j] = fgetc(fp);
			if (mode16bit) (*ip)->data.rgb.b[i][j] |= fgetc(fp) << 8;
			if ((*ip)->data.rgb.r[i][j] > maxval) UErr("rgba_read() unexpected pixel value\n");
			if ((*ip)->data.rgb.g[i][j] > maxval) UErr("rgba_read() unexpected pixel value\n");
			if ((*ip)->data.rgb.b[i][j] > maxval) UErr("rgba_read() unexpected pixel value\n");
			if (alpha_present)
			{
				(*ip)->data.rgb.a[i][j] = fgetc(fp);
				if (mode16bit) (*ip)->data.rgb.a[i][j] |= fgetc(fp) << 8;
				if ((*ip)->data.rgb.a[i][j] > maxval) UErr("rgba_read() unexpected pixel value\n");
			}
		}
	fclose(fp);
	return(0);

}


double rgb_2020_709[12] = {
	1.660491,  -0.587641,   -0.072850,    0.0,
	-0.124550, 1.132900,    -0.008349,    0.0 ,
	-0.018151, -0.100579,   1.118730,    0.0
};

void convert_rgb_2020_to_709(pic_t *ip, pic_t *op)
{
	int i, j;
	int k;
	double mtx[12];
	int maxval;

	if (ip->bits >= 32)
	{
		printf("ERROR: convert_rgb_2020_to_709() does not support floating point\n");
		exit(1);
	}

	for (k = 0; k < 12; ++k)
		mtx[k] = rgb_2020_709[k];

	maxval = (1 << ip->bits) - 1;

	if (ip->color != RGB || ip->chroma != YUV_444)
		UErr("Bad input picture to convert_rgb_2020_to_709()");
	if (op->color != RGB || op->chroma != YUV_444)
		UErr("Bad output picture to convert_rgb_2020_to_709()");

	for (i = 0; i<ip->h; ++i)
		for (j = 0; j < ip->w; ++j)
		{
			op->data.rgb.r[i][j] = (int)CLAMP(mtx[0] * ip->data.rgb.r[i][j] + mtx[1] * ip->data.rgb.g[i][j] +
				mtx[2] * ip->data.rgb.b[i][j] + mtx[3] + 0.5, 0, maxval);
			op->data.rgb.g[i][j] = (int)CLAMP(mtx[4] * ip->data.rgb.r[i][j] + mtx[5] * ip->data.rgb.g[i][j] +
				mtx[6] * ip->data.rgb.b[i][j] + mtx[7] + 0.5, 0, maxval);
			op->data.rgb.b[i][j] = (int)CLAMP(mtx[8] * ip->data.rgb.r[i][j] + mtx[9] * ip->data.rgb.g[i][j] +
				mtx[10] * ip->data.rgb.b[i][j] + mtx[11] + 0.5, 0, maxval);
		}
}

