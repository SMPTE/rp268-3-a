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

/*! \file fifo.h
 *    Generic bit FIFO functions */


#ifndef FIFO_H
#define FIFO_H
#include <cstdint>
#include <vector>

class Fifo
{
public:
	Fifo(int size);
	~Fifo();
	Fifo(const Fifo &f);
	uint32_t GetBitsUi(int nbits);
	int32_t GetBitsI(int nbits);
	void PutBits(uint32_t d, int nbits);
	void PutBits(int32_t d, int nbits);
	uint32_t FlipGetBitsUi(int nbits);
	int32_t FlipGetBitsI(int nbits);
	int32_t GetDatum(int nbits, bool is_signed, bool direction_r2l);
	void FlipPutBits(uint32_t d, int nbits);
	void FlipPutBits(int32_t d, int nbits);
	void PutDatum(int32_t datum, int nbits, bool direction_r2l);
	int m_fullness;
private:
	std::vector<uint8_t> m_data;
	int m_size;
	int m_read_ptr;
	int m_write_ptr;
	int m_max_fullness;
	int m_byte_ctr;
};

#endif

