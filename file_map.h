/***************************************************************************
*    Copyright (c) 2020, Broadcom Inc.
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
#pragma once


namespace Dpx
{

	class FileRegion
	{
	public:
		bool operator < (FileRegion const &obj)
		{
			return start < obj.start;
		}
		uint32_t start;
		uint32_t end;
		int tag;
	};
	struct RLEImageElement
	{
		uint8_t ie_index;
		uint32_t data_offset;
		uint32_t est_size;
	};
	class FileMap
	{
	public:
		void AddRegion(uint32_t region_start, uint32_t region_end, int tag);
		uint32_t FindEmptySpace(uint32_t region_size, int tag);
		bool CheckCollisions();
		void EditRegionEnd(int tag, uint32_t region_end);
		void AdvanceRLEIE(void);
		void AddRLEIE(uint8_t ie_index, uint32_t data_offset, uint32_t est_size);
		uint8_t GetActiveRLEIndex();
		std::vector<uint32_t> GetRLEIEDataOffsets();
		void Reset();

	private:
		void SortRegions(void);
		std::vector<FileRegion> m_r;
		std::vector<RLEImageElement> m_rle_ie;
		uint8_t m_rle_ie_idx;
	};

}