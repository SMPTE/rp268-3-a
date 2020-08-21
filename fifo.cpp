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
#include <iostream>
#include "fifo.h"

#ifdef NDEBUG
#define ASSERT_MSG(condition, msg) 0
#else
#define ASSERT_MSG(condition, msg) \
   (!(condition)) ? \
   ( std::cerr << "Assertion failed: (" << #condition << "), " \
               << "function " << __FUNCTION__ \
               << ", file " << __FILE__ \
               << ", line " << __LINE__ << "." \
               << std::endl << msg << std::endl, abort(), 0) : 1
#endif


//! Initialize a FIFO object
/*! \param fifo		 Pointer to FIFO data structure
\param size		 Specifies FIFO size in bytes */
Fifo::Fifo(int size)
{
	m_data.resize(size);
	m_size = size * 8;
	m_fullness = 0;
	m_read_ptr = m_write_ptr = 0;
	m_max_fullness = 0;
	m_byte_ctr = 0;
}


Fifo::Fifo(const Fifo &f)
{
	m_data = f.m_data;
	m_size = f.m_size;
	m_fullness = f.m_fullness;
	m_read_ptr = f.m_read_ptr;
	m_write_ptr = f.m_write_ptr;
	m_max_fullness = f.m_max_fullness;
	m_byte_ctr = f.m_byte_ctr;
}


//! Free a FIFO object
/*! \param fifo		Pointer to FIFO data structure */
Fifo::~Fifo()
{
	;
}


//! Get bits from a FIFO
/*! \param fifo		Pointer to FIFO data structure
\param n		Number of bits to retrieve
\param sign_extend Flag indicating to extend sign bit for return value
\return			Value from FIFO */
uint32_t Fifo::GetBitsUi(int n)
{
	unsigned int d = 0;
	int i;
	unsigned char b;

	ASSERT_MSG(m_fullness >= n, "FIFO underflow has occured");

	for (i = 0; i<n; ++i)
	{
		b = (m_data[m_read_ptr / 8] >> (7 - (m_read_ptr % 8))) & 1;
		d = (d << 1) + b;
		m_fullness--;
		m_read_ptr++;
		if (m_read_ptr >= m_size)
			m_read_ptr = 0;
	}

	return (d);
}

int32_t Fifo::GetBitsI(int n)
{
	unsigned int d = 0;
	int i;
	unsigned char b;
	int sign = 0;

	ASSERT_MSG(m_fullness >= n, "FIFO underflow has occured");

	for (i = 0; i<n; ++i)
	{
		b = (m_data[m_read_ptr / 8] >> (7 - (m_read_ptr % 8))) & 1;
		if (i == 0) sign = b;
		d = (d << 1) + b;
		m_fullness--;
		m_read_ptr++;
		if (m_read_ptr >= m_size)
			m_read_ptr = 0;
	}

	if (sign)
	{
		int mask = INT32_MAX;
		d |= (~mask);
	}
	return (d);
}


//! Put bits into a FIFO
/*! \param fifo		Pointer to FIFO data structure
\param d		Value to add to FIFO
\param n		Number of bits to add to FIFO */
void Fifo::PutBits(uint32_t d, int nbits)
{
	int i;
	unsigned char b;

	ASSERT_MSG(m_fullness + nbits <= m_size, "FIFO has overflowed");

	m_fullness += nbits;
	for (i = 0; i<nbits; ++i)
	{
		b = (d >> (nbits - i - 1)) & 1;
		if (!b)
			m_data[m_write_ptr / 8] &= ~(1 << (7 - (m_write_ptr % 8)));
		else
			m_data[m_write_ptr / 8] |= (1 << (7 - (m_write_ptr % 8)));
		m_write_ptr++;
		if (m_write_ptr >= m_size)
			m_write_ptr = 0;
	}
	if (m_fullness > m_max_fullness)
		m_max_fullness = m_fullness;
}

void Fifo::PutBits(int32_t d, int nbits)
{
	int i;
	unsigned char b;

	ASSERT_MSG(m_fullness + nbits <= m_size, "FIFO has overflowed");

	m_fullness += nbits;
	for (i = 0; i<nbits; ++i)
	{
		b = (d >> (nbits - i - 1)) & 1;
		if (!b)
			m_data[m_write_ptr / 8] &= ~(1 << (7 - (m_write_ptr % 8)));
		else
			m_data[m_write_ptr / 8] |= (1 << (7 - (m_write_ptr % 8)));
		m_write_ptr++;
		if (m_write_ptr >= m_size)
			m_write_ptr = 0;
	}
	if (m_fullness > m_max_fullness)
		m_max_fullness = m_fullness;
}


//! Get bits from a FIFO in a 32-bit reverse order
/*! \param fifo		Pointer to FIFO data structure
\param n		Number of bits to retrieve
\param sign_extend Flag indicating to extend sign bit for return value
\return			Value from FIFO */
uint32_t Fifo::FlipGetBitsUi(int n)
{
	unsigned int d = 0;
	int i;
	unsigned char b;
	//int sign = 0;

	ASSERT_MSG(m_fullness >= n, "FIFO has underflowed");

	// Note, you need to allocate 32 bits more than you plan to use so the reordering doesn't get overwritten
	// Also, you need to allocate a 32-bit multiple size
	for (i = 0; i<n; ++i)
	{
		int idx1, idx2;
		idx1 = (m_read_ptr / 32) * 4;
		idx2 = (31 - (m_read_ptr % 32)) / 8;
		b = (m_data[idx1 + idx2] >> ((m_read_ptr % 8))) & 1;
		d |= (b << i);
		m_fullness--;
		m_read_ptr++;
		if (m_read_ptr >= m_size)
			m_read_ptr = 0;
	}

	return (d);
}

int32_t Fifo::FlipGetBitsI(int n)
{
	unsigned int d = 0;
	int i;
	unsigned char b;
	int sign = 0;

	ASSERT_MSG(m_fullness >= n, "FIFO has underflowed");

	// Note, you need to allocate 32 bits more than you plan to use so the reordering doesn't get overwritten
	// Also, you need to allocate a 32-bit multiple size
	for (i = 0; i<n; ++i)
	{
		int idx1, idx2;
		idx1 = (m_read_ptr / 32) * 4;
		idx2 = (31 - (m_read_ptr % 32)) / 8;
		b = (m_data[idx1 + idx2] >> ((m_read_ptr % 8))) & 1;
		sign = b;  // MSbit
		d |= (b << i);
		m_fullness--;
		m_read_ptr++;
		if (m_read_ptr >= m_size)
			m_read_ptr = 0;
	}

	if (sign)
	{
		int mask = INT32_MAX;
		d |= (~mask);
	}
	return (d);
}


//! Put bits into a FIFO in 32-bit reverse order
/*! \param fifo		Pointer to FIFO data structure
\param d		Value to add to FIFO
\param n		Number of bits to add to FIFO */
void Fifo::FlipPutBits(uint32_t d, int nbits)
{
	int i;
	unsigned char b;
	int word, bitpos, byte;

	ASSERT_MSG(m_fullness + nbits <= m_size, "FIFO has overflowed");

	m_fullness += nbits;
	for (i = 0; i<nbits; ++i)
	{
		b = d & 1;
		word = m_write_ptr / 32;
		byte = (m_write_ptr / 8) % 4;
		bitpos = m_write_ptr % 8;
		if (!b)
			m_data[word * 4 + 3 - byte] &= ~(1 << (bitpos));
		else
			m_data[word * 4 + 3 - byte] |= (1 << (bitpos));
		m_write_ptr++;
		if (m_write_ptr >= m_size)
			m_write_ptr = 0;
		d >>= 1;  // Get next LSB
	}
	if (m_fullness > m_max_fullness)
		m_max_fullness = m_fullness;
}

void Fifo::FlipPutBits(int32_t d, int nbits)
{
	int i;
	unsigned char b;
	int word, bitpos, byte;

	ASSERT_MSG(m_fullness + nbits <= m_size, "FIFO has overflowed");

	m_fullness += nbits;
	for (i = 0; i<nbits; ++i)
	{
		b = d & 1;
		word = m_write_ptr / 32;
		byte = (m_write_ptr / 8) % 4;
		bitpos = m_write_ptr % 8;
		if (!b)
			m_data[word * 4 + 3 - byte] &= ~(1 << (bitpos));
		else
			m_data[word * 4 + 3 - byte] |= (1 << (bitpos));
		m_write_ptr++;
		if (m_write_ptr >= m_size)
			m_write_ptr = 0;
		d >>= 1;  // Get next LSB
	}
	if (m_fullness > m_max_fullness)
		m_max_fullness = m_fullness;
}

int32_t Fifo::GetDatum(int nbits, bool is_signed, bool direction_r2l)
{
	if (is_signed)
	{
		if (direction_r2l)
			return FlipGetBitsI(nbits);
		else
			return GetBitsI(nbits);
	}
	else
	{
		if (direction_r2l)
			return static_cast<int32_t>(FlipGetBitsUi(nbits));
		else
			return static_cast<int32_t>(GetBitsUi(nbits));
	}
}

void Fifo::PutDatum(int32_t datum, int nbits, bool direction_r2l)
{
	if (direction_r2l)
		FlipPutBits(datum, nbits);
	else
		PutBits(datum, nbits);
}
