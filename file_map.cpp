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
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>

#include "file_map.h"
using namespace Dpx;


void FileMap::AddRegion(uint32_t region_start, uint32_t region_end, int tag)
{
	FileRegion region;
	if (region_start == region_end)
		return;
	region.start = region_start;
	region.end = region_end;
	region.tag = tag;
	m_r.push_back(region);
	std::sort(m_r.begin(), m_r.end());
}

uint32_t FileMap::FindEmptySpace(uint32_t region_size, int tag)
{
	unsigned int i;
	if (m_r.size() == 0)
		return UINT32_MAX;
	for (i = 0; i < m_r.size() - 1; ++i)
	{
		if (m_r[i + 1].start - m_r[i].end >= region_size)
		{
			AddRegion(m_r[i].end, m_r[i].end + region_size, tag);
			return m_r[i].end;
		}
	}
	AddRegion(m_r[i].end, m_r[i].end + region_size, tag);
	return(m_r[i].end);
}

bool FileMap::CheckCollisions()
{
	unsigned int i;
	if (m_r.size() == 0)
		return false;
	for (i = 0; i < m_r.size() - 1; ++i)
	{
		if (m_r[i + 1].start < m_r[i].end)
			return true;
	}
	return false;
}

void FileMap::EditRegionEnd(int tag, uint32_t region_end)
{
	for (unsigned int i = 0; i < m_r.size(); ++i)
	{
		if (m_r[i].tag == tag)
			m_r[i].end = region_end;
	}
}

void FileMap::AdvanceRLEIE(void)
{
	m_rle_ie_idx++;
	if (m_rle_ie_idx >= m_rle_ie.size())		// No more IEs to process
		return;
	uint32_t data_offset = m_rle_ie[m_rle_ie_idx].data_offset;
	if (data_offset == UINT32_MAX)
	{
		m_rle_ie[m_rle_ie_idx].data_offset = FindEmptySpace(m_rle_ie[m_rle_ie_idx].est_size, m_rle_ie[m_rle_ie_idx].ie_index);
	}
}

void FileMap::AddRLEIE(uint8_t ie_index, uint32_t data_offset, uint32_t est_size)
{
	RLEImageElement ie;
	ie.ie_index = ie_index;
	ie.data_offset = data_offset;
	ie.est_size = est_size;
	m_rle_ie.push_back(ie);
}

uint8_t FileMap::GetActiveRLEIndex()
{
	if (m_rle_ie_idx >= m_rle_ie.size())
		return 255;
	else
		return m_rle_ie[m_rle_ie_idx].ie_index;
}

std::vector<uint32_t> FileMap::GetRLEIEDataOffsets()
{
	std::vector<uint32_t> offsets;
	offsets.resize(8, UINT32_MAX);
	for (auto r : m_rle_ie)
		offsets[r.ie_index] = r.data_offset;
	return offsets;
}

void FileMap::Reset()
{
	m_rle_ie_idx = 0;
	m_r.clear();
	m_rle_ie.clear();
}

void FileMap::SortRegions(void)
{
	std::sort(m_r.begin(), m_r.end());
}
