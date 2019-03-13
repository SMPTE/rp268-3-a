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
#ifndef _UTL_H
#define _UTL_H

#include <stdio.h>
#include "vdo.h"

void *palloc(int w, int h);
pic_t *pcreate(int format, int color, int chroma, int w, int h);
pic_t *pcreate_ext(format_t format, color_t color, chroma_t chroma, int w, int h, int bits);
void *pdestroy(pic_t *p);
pic_t *pcopy(pic_t *ip);


void yuv_444_422(pic_t *ip, pic_t *op);
void yuv_422_420(pic_t *ip, pic_t *op);
void yuv_422_444(pic_t *ip, pic_t *op);
void yuv_420_422(pic_t *ip, pic_t *op);
void rgb2yuv(pic_t *ip, pic_t *op);
void yuv2rgb(pic_t *ip, pic_t *op);
void convert_rgb_2020_to_709(pic_t *ip, pic_t *op);

pic_t *convertbits(pic_t *p, int newbits);
int ppm_read(char *fname, pic_t **pic);
int ppm_write(char *fname, pic_t *pic);
float conv(int **p, fir_t fir, int w, int h, int x, int y);

int yuv_read(char *fname, pic_t **ip, int width, int height, int framenum, int bpc, int *numframes, int filetype);
int yuv_write(char *fname, pic_t *op, int framenum, int filetype);
int rgba_read(char *fname, pic_t **ip, int width, int height, int bpc, int alpha_present);

#endif
