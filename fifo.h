/***************************************************************************
*    Copyright (c) 2013-2021, Broadcom Inc.
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

/** @file fifo.h
	@brief Generic bit FIFO functions. */

#ifndef FIFO_H
#define FIFO_H
#include <cstdint>
#include <vector>

/** FIFO object */
class Fifo
{
public:
	Fifo(int size); ///< Initialize FIFO using the specified size (in bytes)
	Fifo(const Fifo &f);  ///< Copy constructor
	uint32_t GetBitsUi(int nbits);   ///< Get an unsigned integer of the specified number of bits (from MSb to LSb)
	int32_t GetBitsI(int nbits);   ///< Get a signed integer of the specified number of bits (from MSb to LSb)
	void PutBits(uint32_t d, int nbits);   ///< Put an unsigned integer of the specified number of bits into the FIFO (from MSb to LSb)
	void PutBits(int32_t d, int nbits);   ///< Put a signed integer of the specified number of bits into the FIFO (from MSb to LSb)
	uint32_t FlipGetBitsUi(int nbits);   ///< Get an unsigned integer of the specified number of bits (from LSb to MSb)
	int32_t FlipGetBitsI(int nbits);   ///< Get an integer number of the specified number of bits (from LSb to MSb)
	/** Get a datum value from the FIFO
		@param nbits				number of bits in datum
		@param is_signed			flag indicating if data value is signed
		@param direction_r2l		true => right-to-left order, false => left-to-right order
		@return						datum value */
	int32_t GetDatum(int nbits, bool is_signed, bool direction_r2l); 
	void FlipPutBits(uint32_t d, int nbits);  ///< Put an unsigned integer of the specified number of bits into the FIFO (from LSb to MSb)
	void FlipPutBits(int32_t d, int nbits);  ///< Put a signed integer of the specified number of bits into the FIFO (from LSb to MSb)
	/** Put a datum value into the FIFO
		@param datum				value to add
		@param nbits				number of bits in datum
		@param direction_r2l		true => right-to-left order, false => left-to-right order */
	void PutDatum(int32_t datum, int nbits, bool direction_r2l);
	void Clear();  ///< Clear the FIFO
	int m_fullness;   ///< FIFO fullness (in bits)
private:
	std::vector<uint8_t> m_data;  ///< data storage
	int m_size;    ///< Size of FIFO
	int m_read_ptr;   ///< FIFO read pointer
	int m_write_ptr;   ///< FIFO write pointer
	int m_max_fullness;   ///< FIFO maximum fullnesss
	int m_byte_ctr;   ///< FIFO byte counter
};

#endif

