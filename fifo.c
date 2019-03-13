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

/*! \file fifo.c
 *    Generic bit FIFO functions */

#include <stdio.h>
#include <stdlib.h>
#include "fifo.h"

//! Initialize a FIFO object
/*! \param fifo		 Pointer to FIFO data structure
    \param size		 Specifies FIFO size in bytes */
void fifo_init(fifo_t *fifo, int size)
{
	fifo->data = (unsigned char *)malloc(sizeof(unsigned char) * size);
	fifo->size = size*8;
	fifo->fullness = 0;
	fifo->read_ptr = fifo->write_ptr = 0;
	fifo->max_fullness = 0;
	fifo->byte_ctr = 0;
}


//! Free a FIFO object
/*! \param fifo		Pointer to FIFO data structure */
void fifo_free(fifo_t *fifo)
{
	free(fifo->data);
}


//! Get bits from a FIFO
/*! \param fifo		Pointer to FIFO data structure 
    \param n		Number of bits to retrieve
	\param sign_extend Flag indicating to extend sign bit for return value
	\return			Value from FIFO */
int fifo_get_bits(fifo_t *fifo, int n, int sign_extend)
{
	unsigned int d = 0;
	int i;
	unsigned char b;
	int sign = 0;

	if (fifo->fullness < n)
	{
		printf("FIFO underflow!\n");
		exit(1);
	}

	for (i=0; i<n; ++i)
	{
		b = (fifo->data[fifo->read_ptr/8] >> (7-(fifo->read_ptr%8))) & 1;
		if (i==0) sign=b;
		d = (d<<1) + b;
		fifo->fullness --;
		fifo->read_ptr ++;
		if (fifo->read_ptr >= fifo->size)
			fifo->read_ptr = 0;
	}

	if (sign_extend && sign)
	{
		int mask;
		mask = (1<<n)-1;
		d |= (~mask);
	}
	return (d);
}


//! Put bits into a FIFO
/*! \param fifo		Pointer to FIFO data structure
	\param d		Value to add to FIFO
    \param n		Number of bits to add to FIFO */
void fifo_put_bits(fifo_t *fifo, unsigned int d, int nbits)
{
	int i;
	unsigned char b;

	if (fifo->fullness + nbits > fifo->size)
	{
		printf("FIFO overflow!\n");
		exit(1);
	}

	fifo->fullness += nbits;
	for (i=0; i<nbits; ++i)
	{
		b = (d >> (nbits-i-1)) & 1;
		if (!b)
			fifo->data[fifo->write_ptr/8] &= ~(1<<(7-(fifo->write_ptr%8)));
		else
			fifo->data[fifo->write_ptr/8] |= (1<<(7-(fifo->write_ptr%8)));
		fifo->write_ptr++;
		if (fifo->write_ptr >= fifo->size)
			fifo->write_ptr = 0;
	}
	if (fifo->fullness > fifo->max_fullness)
		fifo->max_fullness = fifo->fullness;
}


//! Clones a FIFO
/*! \param dst      Pointer to new FIFO created by function 
	\param src      Pointer to original FIFO */
void fifo_clone(fifo_t *dst, fifo_t *src)
{
	int d, s;
	fifo_t tmp;

	fifo_init(dst, src->size/8);
	fifo_init(&tmp, src->size/8);

	while(src->fullness>0)
	{
		s = (src->fullness>=8) ? 8 : src->fullness;
		fifo_put_bits(&tmp, fifo_get_bits(src, s, 0), s);
	}
	while(tmp.fullness>0)
	{
		s = (tmp.fullness>=8) ? 8 : tmp.fullness;
		d = fifo_get_bits(&tmp, s, 0);
		fifo_put_bits(src, d, s);
		fifo_put_bits(dst, d, s);
	}
	fifo_free(&tmp);
}

//! Get bits from a FIFO in a 32-bit reverse order
/*! \param fifo		Pointer to FIFO data structure 
    \param n		Number of bits to retrieve
	\param sign_extend Flag indicating to extend sign bit for return value
	\return			Value from FIFO */
int fifo_flip_get_bits(fifo_t *fifo, int n, int sign_extend)
{
	unsigned int d = 0;
	int i;
	unsigned char b;
	int sign = 0;

	if (fifo->fullness < n)
	{
		printf("FIFO underflow!\n");
		exit(1);
	}

	// Note, you need to allocate 32 bits more than you plan to use so the reordering doesn't get overwritten
	// Also, you need to allocate a 32-bit multiple size
	for (i=0; i<n; ++i)
	{
		int idx1, idx2;
		idx1 = (fifo->read_ptr/32) * 4;
		idx2 = (31 - (fifo->read_ptr%32))/8;
		b = (fifo->data[idx1+idx2] >> ((fifo->read_ptr%8))) & 1;
		sign = b;  // MSbit
		d |= (b<<i);
		fifo->fullness --;
		fifo->read_ptr ++;
		if (fifo->read_ptr >= fifo->size)
			fifo->read_ptr = 0;
	}

	if (sign_extend && sign)
	{
		int mask;
		mask = (1<<n)-1;
		d |= (~mask);
	}
	return (d);
}


//! Put bits into a FIFO in 32-bit reverse order
/*! \param fifo		Pointer to FIFO data structure
	\param d		Value to add to FIFO
    \param n		Number of bits to add to FIFO */
void fifo_flip_put_bits(fifo_t *fifo, unsigned int d, int nbits)
{
	int i;
	unsigned char b;
	int word, bitpos, byte;

	if (fifo->fullness + nbits > fifo->size)
	{
		printf("FIFO overflow!\n");
		exit(1);
	}

	fifo->fullness += nbits;
	for (i=0; i<nbits; ++i)
	{
		b = d & 1;
		word = fifo->write_ptr/32;
		byte = (fifo->write_ptr/8) % 4;
		bitpos = fifo->write_ptr%8;
		if (!b)
			fifo->data[word*4 + 3-byte] &= ~(1<<(bitpos));
		else
			fifo->data[word*4 + 3-byte] |= (1<<(bitpos));
		fifo->write_ptr++;
		if (fifo->write_ptr >= fifo->size)
			fifo->write_ptr = 0;
		d>>=1;  // Get next LSB
	}
	if (fifo->fullness > fifo->max_fullness)
		fifo->max_fullness = fifo->fullness;
}
