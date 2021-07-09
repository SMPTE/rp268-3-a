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
/** @file file_map.h
	@brief Maps out different regions of DPX files. */


namespace Dpx
{

	/** Defines a file region */
	class FileRegion
	{
	public:
		/** Compare whether a region is before another region */
		bool operator < (FileRegion const &obj)
		{
			return start < obj.start;
		}
		uint32_t start;  ///< Region start
		uint32_t end;   ///< Region end
		int tag;    ///< Region tag
	};
	/** RLE image element descriptor */
	struct RLEImageElement
	{
		uint8_t ie_index;   ///< Image element index
		uint32_t data_offset;   ///< Data offset
		uint32_t est_size;   ///< Estimated size
	};
	/** File map object */
	class FileMap
	{
	public:
		/** Add a region to the map
			@param region_start			starting offset of region (in bytes)
			@param region_end			ending offset of region (in bytes)
			@param tag					unique tag describing what region contains */
		void AddRegion(uint32_t region_start, uint32_t region_end, int tag);
		/** Find an empty space within the map (or append if insufficient empty space is available)
			@param region_size			size of new region
			@param tag					unique tag describing what new region contains */
		uint32_t FindEmptySpace(uint32_t region_size, int tag);
		/** Check the map for any collisions and return true if there are any */
		bool CheckCollisions();
		/** Change the end offset of the region corresponding to the specified tag 
			@param tag					tag of the region
			@param region_end			new ending offset of region */
		void EditRegionEnd(int tag, uint32_t region_end);
		/** Advance the image element currently being run-length encoded (only one RLE can take place at a time) */
		void AdvanceRLEIE(void);
		/** Add an RLE image element
			@param ie_index				IE index of RLE IE
			@param data_offset			starting offset of RLE data
			@param est_size				estimated size of RLE IE */
		void AddRLEIE(uint8_t ie_index, uint32_t data_offset, uint32_t est_size);
		/** Get the index of the active RLE IE */
		uint8_t GetActiveRLEIndex();
		/** Get the data offsets for any RLE-coded IEs (return vector can be accessed using the ie_index) */
		std::vector<uint32_t> GetRLEIEDataOffsets();
		/** Reset file map */
		void Reset();

	private:
		/** Sort regions by starting offset */
		void SortRegions(void);
		std::vector<FileRegion> m_r; ///< region list
		std::vector<RLEImageElement> m_rle_ie;  ///< RLE IE list
		uint8_t m_rle_ie_idx;   ///< Current RLE IE index
	};

}