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
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <cmath>
#include "hdr_dpx.h"
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
#define LOG_ERROR(number, severity, msg) \
	m_err.LogError(number, severity, ((severity == eInformational) ? "INFO #" : ((severity == eWarning) ? "WARNING #" : "ERROR #")) \
			   + std::to_string(static_cast<int>(number)) + " in " \
               + "function " + __FUNCTION__ \
               + ", file " + __FILE__ \
               + ", line " + std::to_string(__LINE__) + ":\n" \
               + msg + "\n");


using namespace Dpx;


std::vector<DatumLabel> Dpx::DescriptorToDatumList(uint8_t desc)
{
	std::vector<DatumLabel> dl;
	dl.clear();
	switch (desc)
	{
	case eDescUser:
	case eDescUndefined:
		dl.push_back(DATUM_UNSPEC);
		break;
	case eDescR:
		dl.push_back(DATUM_R);
		break;
	case eDescG:
		dl.push_back(DATUM_G);
		break;
	case eDescB:
		dl.push_back(DATUM_B);
		break;
	case eDescA:
		dl.push_back(DATUM_A);
		break;
	case eDescY:
		dl.push_back(DATUM_Y);
		break;
	case eDescCbCr:
		dl.push_back(DATUM_CB);
		dl.push_back(DATUM_CR);
		break;
	case eDescZ:
		dl.push_back(DATUM_Z);
		break;
	case eDescComposite:
		dl.push_back(DATUM_COMPOSITE);
		break;
	case eDescCb:
		dl.push_back(DATUM_CB);
		break;
	case eDescCr:
		dl.push_back(DATUM_CR);
		break;
	case eDescRGB_268_1:
		dl.push_back(DATUM_R);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_B);
		break;
	case eDescRGBA_268_1:
		dl.push_back(DATUM_R);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_B);
		dl.push_back(DATUM_A);
		break;
	case eDescABGR_268_1:
		dl.push_back(DATUM_A);
		dl.push_back(DATUM_B);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_R);
		break;
	case eDescBGR:
		dl.push_back(DATUM_B);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_R);
		break;
	case eDescBGRA:
		dl.push_back(DATUM_B);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_R);
		dl.push_back(DATUM_A);
		break;
	case eDescARGB:
		dl.push_back(DATUM_A);
		dl.push_back(DATUM_R);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_B);
		break;
	case eDescRGB:
		dl.push_back(DATUM_R);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_B);
		break;
	case eDescRGBA:
		dl.push_back(DATUM_R);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_B);
		dl.push_back(DATUM_A);
		break;
	case eDescABGR:
		dl.push_back(DATUM_A);
		dl.push_back(DATUM_B);
		dl.push_back(DATUM_G);
		dl.push_back(DATUM_R);
		break;
	case eDescCbYCrY:
		dl.push_back(DATUM_CB);
		dl.push_back(DATUM_Y);
		dl.push_back(DATUM_CR);
		dl.push_back(DATUM_Y2);
		break;
	case eDescCbYACrYA:
		dl.push_back(DATUM_CB);
		dl.push_back(DATUM_Y);
		dl.push_back(DATUM_A);
		dl.push_back(DATUM_CR);
		dl.push_back(DATUM_Y2);
		dl.push_back(DATUM_A2);
		break;
	case eDescCbYCr:
		dl.push_back(DATUM_CB);
		dl.push_back(DATUM_Y);
		dl.push_back(DATUM_CR);
		break;
	case eDescCbYCrA:
		dl.push_back(DATUM_CB);
		dl.push_back(DATUM_Y);
		dl.push_back(DATUM_CR);
		dl.push_back(DATUM_A);
		break;
	case eDescCYY:
		dl.push_back(DATUM_C);
		dl.push_back(DATUM_Y);
		dl.push_back(DATUM_Y2);
		break;
	case eDescCYAYA:
		dl.push_back(DATUM_C);
		dl.push_back(DATUM_Y);
		dl.push_back(DATUM_A);
		dl.push_back(DATUM_Y2);
		dl.push_back(DATUM_A2);
		break;
	case eDescGeneric2:
		dl.push_back(DATUM_UNSPEC);
		dl.push_back(DATUM_UNSPEC2);
		break;
	case eDescGeneric3:
		dl.push_back(DATUM_UNSPEC);
		dl.push_back(DATUM_UNSPEC2);
		dl.push_back(DATUM_UNSPEC3);
		break;
	case eDescGeneric4:
		dl.push_back(DATUM_UNSPEC);
		dl.push_back(DATUM_UNSPEC2);
		dl.push_back(DATUM_UNSPEC3);
		dl.push_back(DATUM_UNSPEC4);
		break;
	case eDescGeneric5:
		dl.push_back(DATUM_UNSPEC);
		dl.push_back(DATUM_UNSPEC2);
		dl.push_back(DATUM_UNSPEC3);
		dl.push_back(DATUM_UNSPEC4);
		dl.push_back(DATUM_UNSPEC5);
		break;
	case eDescGeneric6:
		dl.push_back(DATUM_UNSPEC);
		dl.push_back(DATUM_UNSPEC2);
		dl.push_back(DATUM_UNSPEC3);
		dl.push_back(DATUM_UNSPEC4);
		dl.push_back(DATUM_UNSPEC5);
		dl.push_back(DATUM_UNSPEC6);
		break;
	case eDescGeneric7:
		dl.push_back(DATUM_UNSPEC);
		dl.push_back(DATUM_UNSPEC2);
		dl.push_back(DATUM_UNSPEC3);
		dl.push_back(DATUM_UNSPEC4);
		dl.push_back(DATUM_UNSPEC5);
		dl.push_back(DATUM_UNSPEC6);
		dl.push_back(DATUM_UNSPEC7);
		break;
	case eDescGeneric8:
		dl.push_back(DATUM_UNSPEC);
		dl.push_back(DATUM_UNSPEC2);
		dl.push_back(DATUM_UNSPEC3);
		dl.push_back(DATUM_UNSPEC4);
		dl.push_back(DATUM_UNSPEC5);
		dl.push_back(DATUM_UNSPEC6);
		dl.push_back(DATUM_UNSPEC7);
		dl.push_back(DATUM_UNSPEC8);
		break;
	}
	return dl;
}

uint8_t Dpx::DatumListToDescriptor(std::vector<DatumLabel> dl)
{
	if (dl.size() == 0)
		return 255;
	if (dl.size() == 1)
	{
		if (dl[0] == DATUM_UNSPEC)
			return eDescUser;
		if (dl[0] == DATUM_R)
			return eDescR;
		if (dl[0] == DATUM_G)
			return eDescG;
		if (dl[0] == DATUM_B)
			return eDescB;
		if (dl[0] == DATUM_A)
			return eDescA;
		if (dl[0] == DATUM_Y)
			return eDescY;
		if (dl[0] == DATUM_Z)
			return eDescZ;
		if (dl[0] == DATUM_COMPOSITE)
			return eDescComposite;
		if (dl[0] == DATUM_CB)
			return eDescCb;
		if (dl[0] == DATUM_CR)
			return eDescCr;
		return 255;
	}
	if (dl.size() == 2)
	{
		if (dl[0] == DATUM_CB && dl[1] == DATUM_CR)
			return eDescCbCr;
		if (dl[0] == DATUM_UNSPEC && dl[1] == DATUM_UNSPEC2)
			return eDescGeneric2;
		return 255;
	}
	if (dl.size() == 3)
	{
		if (dl[0] == DATUM_B && dl[1] == DATUM_G && dl[2] == DATUM_R)
			return eDescBGR;
		if (dl[0] == DATUM_R && dl[1] == DATUM_G && dl[2] == DATUM_B)
			return eDescRGB;
		if (dl[0] == DATUM_CB && dl[1] == DATUM_Y && dl[2] == DATUM_CR)
			return eDescCbYCr;
		if (dl[0] == DATUM_C && dl[1] == DATUM_Y && dl[2] == DATUM_Y2)
			return eDescCYY;
		if (dl[0] == DATUM_UNSPEC && dl[1] == DATUM_UNSPEC2 && dl[2] == DATUM_UNSPEC3)
			return eDescGeneric3;
		return 255;
	}
	if (dl.size() == 4)
	{
		if (dl[0] == DATUM_B && dl[1] == DATUM_G && dl[2] == DATUM_R && dl[3] == DATUM_A)
			return eDescBGRA;
		if (dl[0] == DATUM_A && dl[1] == DATUM_R && dl[2] == DATUM_G && dl[3] == DATUM_B)
			return eDescARGB;
		if (dl[0] == DATUM_R && dl[1] == DATUM_G && dl[2] == DATUM_B && dl[3] == DATUM_A)
			return eDescRGBA;
		if (dl[0] == DATUM_A && dl[1] == DATUM_B && dl[2] == DATUM_G && dl[3] == DATUM_R)
			return eDescARGB;
		if (dl[0] == DATUM_CB && dl[1] == DATUM_Y && dl[2] == DATUM_CR && dl[3] == DATUM_Y2)
			return eDescCbYCrY;
		if (dl[0] == DATUM_CB && dl[1] == DATUM_Y && dl[2] == DATUM_CR && dl[3] == DATUM_A)
			return eDescCbYCrA;
		if (dl[0] == DATUM_UNSPEC && dl[1] == DATUM_UNSPEC2 && dl[2] == DATUM_UNSPEC3 && dl[3] == DATUM_UNSPEC4)
			return eDescGeneric4;
		return 255;
	}
	if (dl.size() == 5)
	{
		if (dl[0] == DATUM_C && dl[1] == DATUM_Y && dl[2] == DATUM_A && dl[3] == DATUM_Y2 && dl[4] == DATUM_A2)
			return eDescCYAYA;
		if (dl[0] == DATUM_UNSPEC && dl[1] == DATUM_UNSPEC2 && dl[2] == DATUM_UNSPEC3 && dl[3] == DATUM_UNSPEC4 &&
			dl[4] == DATUM_UNSPEC5)
			return eDescGeneric5;
		return 255;
	}
	if (dl.size() == 6)
	{
		if (dl[0] == DATUM_CB && dl[1] == DATUM_Y && dl[2] == DATUM_A && dl[3] == DATUM_CR && dl[4] == DATUM_Y2 && dl[5] == DATUM_A2)
			return eDescCbYACrYA;
		if (dl[0] == DATUM_UNSPEC && dl[1] == DATUM_UNSPEC2 && dl[2] == DATUM_UNSPEC3 && dl[3] == DATUM_UNSPEC4 &&
			dl[4] == DATUM_UNSPEC5 && dl[5] == DATUM_UNSPEC6)
			return eDescGeneric6;
		return 255;
	}
	if (dl.size() == 7)
	{
		if (dl[0] == DATUM_UNSPEC && dl[1] == DATUM_UNSPEC2 && dl[2] == DATUM_UNSPEC3 && dl[3] == DATUM_UNSPEC4 &&
			dl[4] == DATUM_UNSPEC5 && dl[5] == DATUM_UNSPEC6 && dl[6] == DATUM_UNSPEC7)
			return eDescGeneric7;
		return 255;
	}
	if (dl.size() == 8)
	{
		if (dl[0] == DATUM_UNSPEC && dl[1] == DATUM_UNSPEC2 && dl[2] == DATUM_UNSPEC3 && dl[3] == DATUM_UNSPEC4 &&
			dl[4] == DATUM_UNSPEC5 && dl[5] == DATUM_UNSPEC6 && dl[6] == DATUM_UNSPEC7 && dl[7] == DATUM_UNSPEC8)
			return eDescGeneric8;
		return 255;
	}
	return 255;
}

std::string HdrDpxImageElement::DatumLabelToName(Dpx::DatumLabel dl) const
{
	switch (dl)
	{
	case DATUM_UNSPEC:
		return "Unspec";
	case DATUM_R:
		return "R";
	case DATUM_G:
		return "G";
	case DATUM_B:
		return "B";
	case DATUM_A:
		return "A";
	case DATUM_Y:
		return "Y";
	case DATUM_CB:
		return "Cb";
	case DATUM_CR:
		return "Cr";
	case DATUM_Z:
		return "Z";
	case DATUM_COMPOSITE:
		return "Composite";
	case DATUM_A2:
		return "A2";
	case DATUM_Y2:
		return "Y2";
	case DATUM_C:
		return "C";
	case DATUM_UNSPEC2:
		return "Unspec2";
	case DATUM_UNSPEC3:
		return "Unspec3";
	case DATUM_UNSPEC4:
		return "Unspec4";
	case DATUM_UNSPEC5:
		return "Unspec5";
	case DATUM_UNSPEC6:
		return "Unspec6";
	case DATUM_UNSPEC7:
		return "Unspec7";
	case DATUM_UNSPEC8:
		return "Unspec8";
	}
	return "Unrecognized";
}

HdrDpxImageElement::HdrDpxImageElement() : m_fifo(16)
{
	m_isinitialized = false;
}

HdrDpxImageElement::HdrDpxImageElement(uint8_t ie_index, std::fstream *fstream_ptr, HDRDPXFILEFORMAT *dpxf_ptr, FileMap *file_map_ptr) : m_fifo(16)
{
	m_is_header_locked = false;
	Initialize(ie_index, fstream_ptr, dpxf_ptr, file_map_ptr);
}

void HdrDpxImageElement::Initialize(uint8_t ie_index, std::fstream *fstream_ptr, HDRDPXFILEFORMAT *dpxf_ptr, FileMap *file_map_ptr)
{
	m_ie_index = ie_index;
	m_isinitialized = true;
	m_dpx_ie_ptr = &(dpxf_ptr->ImageHeader.ImageElement[ie_index]);
	m_dpx_hdr_ptr = dpxf_ptr;
	m_filestream_ptr = fstream_ptr;
	m_file_map_ptr = file_map_ptr;
	m_is_open_for_read = false;
	m_is_open_for_write = false;
	m_is_header_locked = false;
	m_err.Clear();
	m_warnings.clear();
}


HdrDpxImageElement::~HdrDpxImageElement()
{
	;
}

void HdrDpxImageElement::Deinitialize()
{
	m_isinitialized = false;
}


void HdrDpxImageElement::ComputeWidthAndHeight()
{
	m_is_h_subsampled = m_is_v_subsampled = false;
	if (m_dpx_ie_ptr->Descriptor == eDescCb || m_dpx_ie_ptr->Descriptor == eDescCr)
		m_is_h_subsampled = m_is_v_subsampled = true;
	else if (m_dpx_ie_ptr->Descriptor == eDescCbCr || m_dpx_ie_ptr->Descriptor == eDescCbYCrY ||
		m_dpx_ie_ptr->Descriptor == eDescCbYACrYA || m_dpx_ie_ptr->Descriptor == eDescCYY ||
		m_dpx_ie_ptr->Descriptor == eDescCYAYA)
		m_is_h_subsampled = true;
	m_width = m_dpx_hdr_ptr->ImageHeader.PixelsPerLine / (m_is_h_subsampled ? 2 : 1);
	m_height = m_dpx_hdr_ptr->ImageHeader.LinesPerElement / (m_is_v_subsampled ? 2 : 1);
}

void HdrDpxImageElement::OpenForReading(bool bswap)
{
	ComputeWidthAndHeight();
	m_byte_swap = bswap;
	m_direction_r2l = (m_dpx_hdr_ptr->FileHeader.DatumMappingDirection == 0);
	m_is_open_for_read = true;
	m_is_open_for_write = false;
	m_is_header_locked = true;
}

void HdrDpxImageElement::OpenForWriting(bool bswap)
{
	if (m_dpx_ie_ptr->Descriptor == eDescUndefined)
	{
		LOG_ERROR(eFileWriteError, eFatal, "Cannot write image element " + std::to_string(m_ie_index + 1) + " without descriptor field");
		return;
	}
	if (m_dpx_ie_ptr->BitSize == eBitDepthUndefined)
	{
		LOG_ERROR(eFileWriteError, eFatal, "Cannot write image element " + std::to_string(m_ie_index + 1) + " without bit depth field");
		return;
	}
	ComputeWidthAndHeight();

	m_byte_swap = bswap;
	m_direction_r2l = (m_dpx_hdr_ptr->FileHeader.DatumMappingDirection == 0);
	m_is_open_for_write = true;
	m_is_open_for_read = false;
	m_is_header_locked = true;
}


void HdrDpxImageElement::LockHeader(void)
{
	m_is_header_locked = true;
}

void HdrDpxImageElement::UnlockHeader(void)
{
	m_is_header_locked = false;
}

std::vector<DatumLabel> HdrDpxImageElement::GetDatumLabels(void) const
{
	return DescriptorToDatumList(m_dpx_ie_ptr->Descriptor);
}

uint8_t HdrDpxImageElement::GetNumberOfComponents(void) const
{
	return static_cast<int>(GetDatumLabels().size());
}

uint8_t HdrDpxImageElement::GetDatumLabelIndex(DatumLabel dl) const
{
	uint8_t i;
	std::vector<DatumLabel> dl_list = GetDatumLabels();
	for (i = 0; i < GetNumberOfComponents(); ++i)
		if (dl == dl_list[i])
			return i;
	return 0xff;
}

uint32_t HdrDpxImageElement::GetRowSizeInBytes(bool include_padding) const
{
	uint8_t num_c = GetNumberOfComponents();
	uint32_t idw_per_line;

	if (m_dpx_ie_ptr->BitSize == 64)
		idw_per_line = static_cast<uint32_t>(2 * num_c * m_width);
	else if (m_dpx_ie_ptr->BitSize == 32)
		idw_per_line = static_cast<uint32_t>(num_c * m_width);
	else if (m_dpx_ie_ptr->BitSize == 16)
		idw_per_line = static_cast<uint32_t>(std::ceil(num_c * m_width / 2.0));
	else if (m_dpx_ie_ptr->Packing == 0 || m_dpx_ie_ptr->BitSize == 8)
		idw_per_line = static_cast<uint32_t>(std::ceil(num_c * m_width * m_dpx_ie_ptr->BitSize / 8.0 / 4.0));
	else
	{
		if (m_dpx_ie_ptr->BitSize == 10)   // 3 datums per IDW
			idw_per_line = static_cast<uint32_t>(std::ceil(num_c * m_width / 3.0));
		else  // 12 bits, 2 datums per IDW
			idw_per_line = static_cast<uint32_t>(std::ceil(num_c * m_width / 2.0));
	}
	if(include_padding)
		idw_per_line += m_dpx_ie_ptr->EndOfLinePadding / 4;
	return idw_per_line * 4;
}

uint32_t HdrDpxImageElement::GetRowSizeInDatums() const
{
	return GetWidth() * GetNumberOfComponents();
}

uint32_t HdrDpxImageElement::GetOffsetForRow(uint32_t row) const
{

	return m_dpx_ie_ptr->DataOffset + GetRowSizeInBytes(true) * row;
}

void HdrDpxImageElement::Dpx2AppPixels(uint32_t row, int32_t *datum_ptr)
{

	if (!m_isinitialized)
	{
		LOG_ERROR(eBadParameter, eFatal, "Tried to read pixels from uninitialized image element");
		return;
	}
	if (!m_is_open_for_read || !m_filestream_ptr->good())
	{
		LOG_ERROR(eFileReadError, eFatal, "File read error");
		return;
	}
	if (m_dpx_ie_ptr->BitSize >= 32)
	{
		LOG_ERROR(eBadParameter, eFatal, "Failed attempt reading integer pixels from floating point file");
		return;
	}
	
	uint8_t num_components = GetNumberOfComponents();

	m_int_row = datum_ptr;
	ReadRow(row);
}

void HdrDpxImageElement::Dpx2AppPixels(uint32_t row, float *datum_ptr)
{

	if (!m_isinitialized)
	{
		LOG_ERROR(eBadParameter, eFatal, "Tried to read pixels from uninitialized image element");
		return;
	}
	if (!m_is_open_for_read || !m_filestream_ptr->good())
	{
		LOG_ERROR(eFileReadError, eFatal, "File read error");
		return;
	}
	if (m_dpx_ie_ptr->BitSize != 32)
	{
		LOG_ERROR(eBadParameter, eFatal, "Failed attempt reading single-precision pixels from file");
		return;
	}

	uint8_t num_components = GetNumberOfComponents();

	m_float_row = datum_ptr;
	ReadRow(row);
}

void HdrDpxImageElement::Dpx2AppPixels(uint32_t row, double *datum_ptr)
{

	if (!m_isinitialized)
	{
		LOG_ERROR(eBadParameter, eFatal, "Tried to read pixels from uninitialized image element");
		return;
	}
	if (!m_is_open_for_read || !m_filestream_ptr->good())
	{
		LOG_ERROR(eFileReadError, eFatal, "File read error");
		return;
	}
	if (m_dpx_ie_ptr->BitSize != 32)
	{
		LOG_ERROR(eBadParameter, eFatal, "Failed attempt reading double-precision pixels from file");
		return;
	}

	uint8_t num_components = GetNumberOfComponents();

	m_double_row = datum_ptr;
	ReadRow(row);
}

#define READ_DPX_32(x)   (m_byte_swap ? (((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | ((x) >> 24))) : (x))
void HdrDpxImageElement::ReadRow(uint32_t row)
{
	int component;
	int num_components;
	uint32_t xpos;
	uint32_t image_data_word;
	Fifo fifo(16);
	int32_t int_datum;
	uint32_t row_wr_idx = 0;
	uint32_t expected_zero;
	union {
		double r64;
		uint32_t d[2];
	} c_r64;
	union {
		float r32;
		uint32_t d;
	} c_r32;
	int rle_state = 0;   // 0 = flag, 1-n = component value
	int32_t run_length = 0;
	int rle_count = 0;
	int32_t rle_pixel[8];
	bool rle_is_same;
	const bool is_signed = (m_dpx_ie_ptr->DataSign == 1);
	const uint8_t bpc = m_dpx_ie_ptr->BitSize;

	if (m_dpx_ie_ptr->Encoding == 1)  // RLE
	{
		if (row == 0)
		{
			m_filestream_ptr->seekg(m_dpx_ie_ptr->DataOffset);
		}
		else if (row != m_previous_row + 1)
		{
			LOG_ERROR(eBadParameter, eFatal, "When RLE is enabled, rows must be read in sequential order");
			return;
		}
		else
			m_filestream_ptr->seekg(m_previous_file_offset);
		m_previous_row = row;
	}
	else
		m_filestream_ptr->seekg(GetOffsetForRow(row));

	num_components = GetNumberOfComponents();

	xpos = 0;
	component = 0;
	while (xpos < m_width && component < num_components)
	{
		while (fifo.m_fullness <= 32)
		{
			// Read 32-bit word & byte swap if needed
			m_filestream_ptr->read((char *)&image_data_word, 4);
			image_data_word = READ_DPX_32(image_data_word);
			fifo.PutBits(image_data_word, 32);
		}
		switch (bpc)
		{
		case 1:
		case 8:
		case 16:
			int_datum = fifo.GetDatum(bpc, is_signed, m_direction_r2l);
			break;
		case 10:
		case 12:
			if (m_dpx_ie_ptr->Packing == 1) // Method A
			{
				if (m_direction_r2l)
				{
					if (fifo.m_fullness == 48 || fifo.m_fullness == 64)   // start with padding bits
					{
						if (bpc == 10)
						{
							expected_zero = fifo.FlipGetBitsUi(2);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
						else // 12-bit
						{
							expected_zero = fifo.FlipGetBitsUi(4);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
					}
				}

				int_datum = fifo.GetDatum(bpc, is_signed, m_direction_r2l);
				if (!m_direction_r2l)
				{
					if (fifo.m_fullness == 64 - 30)
					{
						expected_zero = fifo.GetBitsUi(2);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
					else if (fifo.m_fullness == 64 - 12 || fifo.m_fullness == 64 - 28)
					{
						expected_zero = fifo.GetBitsUi(4);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
				}
			}
			else if (m_dpx_ie_ptr->Packing == 2) // Method B
			{
				if (!m_direction_r2l)
				{
					if (fifo.m_fullness == 64 - 0 || fifo.m_fullness == 64 - 16)
					{
						if (bpc == 10)
						{
							expected_zero = fifo.GetBitsUi(2);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
						else
						{
							expected_zero = fifo.GetBitsUi(4);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
					}
				}
				int_datum = fifo.GetDatum(bpc, is_signed, m_direction_r2l);
				if (m_direction_r2l)
				{
					if (fifo.m_fullness == 64 - 30)
					{
						expected_zero = fifo.FlipGetBitsUi(2);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
					else if (fifo.m_fullness == 64 - 12 || fifo.m_fullness == 64 - 28)
					{
						expected_zero = fifo.FlipGetBitsUi(4);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
				}
			}
			else   // Packed
			{
				int_datum = fifo.GetDatum(bpc, is_signed, m_direction_r2l);
			}
			break;
		case 32:
			c_r32.d = fifo.GetBitsUi(32);
			m_float_row[row_wr_idx++] = c_r32.r32;
			break;
		case 64:
			c_r64.d[0] = fifo.GetBitsUi(32);
			c_r64.d[1] = fifo.GetBitsUi(32);
			m_double_row[row_wr_idx++] = c_r64.r64;
			break;
		}
		if (m_dpx_ie_ptr->Encoding == 1) // RLE
		{
			if (rle_state == 0)
			{
				rle_state++;
				run_length = (int_datum >> 1) & INT16_MAX;
				if (run_length == 0 && !m_warn_zero_run_length)
				{
					m_warn_zero_run_length = true;
					m_warnings.push_back("Encountered a run-length of zero, first occurred at " + std::to_string(xpos) + ", " + std::to_string(row));
				}
				rle_count = 0;
				rle_is_same = (int_datum & 1);
			}
			else if (component == num_components - 1)
			{
				m_int_row[row_wr_idx++] = int_datum;
				rle_pixel[component] = int_datum;
				if (rle_is_same)
				{
					if (xpos + run_length > m_width && !m_warn_rle_same_past_eol)
					{
						m_warn_rle_same_past_eol = true;
						m_warnings.push_back("RLE same-pixel run went past the end of a line, first occurred at " + std::to_string(xpos) + ", " + std::to_string(row));
					}
					for (int i = 1; i < run_length; ++i)
					{
						for (int c = 0; c < num_components; ++c)
						{
							m_int_row[row_wr_idx++] = rle_pixel[c];
						}
					}
					component = 0;
					xpos += MAX(1, run_length);
					rle_state = 0;
				}
				else
				{
					rle_count++;
					xpos++;
					component = 0;
					if (rle_count >= run_length)
						rle_state = 0;
					else
					{
						if (xpos >= m_width && !m_warn_rle_diff_past_eol)
						{
							m_warn_rle_diff_past_eol = true;
							m_warnings.push_back("RLE different-pixel run went past the end of a line, first occurred at " + std::to_string(xpos) + ", " + std::to_string(row));
						}
					}
				}
			}
			else
			{
				m_int_row[row_wr_idx++] = int_datum;
				rle_pixel[component] = int_datum;
				component++;
			}
		}
		else			// No RLE
		{
			m_int_row[row_wr_idx++] = int_datum;
			component++;
			if (component == num_components)
			{
				component = 0;
				xpos++;
			}
		}
	}

	m_previous_file_offset = static_cast<uint32_t>(m_filestream_ptr->tellg());
}



void HdrDpxImageElement::WriteFlush()
{
	uint32_t image_data_word;

	while (m_fifo.m_fullness >= 32)
	{
		image_data_word = m_fifo.GetBitsUi(32);
		if (m_byte_swap)
			ByteSwap32((void *)(&image_data_word));
		m_filestream_ptr->write((char *)(&image_data_word), 4);
	}
}


void HdrDpxImageElement::WriteDatum(int32_t datum)
{
	const uint8_t bpc = m_dpx_ie_ptr->BitSize;
	switch (m_dpx_ie_ptr->Packing)
	{
	case 0:
		m_fifo.PutDatum(datum, bpc, m_direction_r2l);
		break;
	case 1:  /* Method A */
		if (m_direction_r2l && (m_fifo.m_fullness == 0 || m_fifo.m_fullness == 16))
			m_fifo.FlipPutBits(0, (bpc == 10) ? 2 : 4);
		m_fifo.PutDatum(datum, bpc, m_direction_r2l);
		if (!m_direction_r2l && (m_fifo.m_fullness == 12 || m_fifo.m_fullness == 28 || m_fifo.m_fullness == 30))
			m_fifo.PutBits(0, (bpc == 10) ? 2 : 4);
		break;  /* Method B */
	case 2:
		if (!m_direction_r2l && (m_fifo.m_fullness == 0 || m_fifo.m_fullness == 16))
			m_fifo.PutBits(0, (bpc == 10) ? 2 : 4);
		m_fifo.PutDatum(datum, bpc, m_direction_r2l);
		if (m_direction_r2l && (m_fifo.m_fullness == 12 || m_fifo.m_fullness == 28 || m_fifo.m_fullness == 30))
			m_fifo.FlipPutBits(0, (bpc == 10) ? 2 : 4);
		break;
	}
	WriteFlush();
}

void HdrDpxImageElement::WritePixel(uint32_t xpos)
{
	int component;
	int num_components;
	union {
		double r64;
		uint32_t d[2];
	} c_r64;
	union {
		float r32;
		uint32_t d;
	} c_r32;
	int32_t int_datum;

	num_components = GetNumberOfComponents();

	for (component = 0; component < num_components; ++component)
	{
		switch (m_dpx_ie_ptr->BitSize)
		{
		case 1:
		case 8:
		case 10:
		case 12:
		case 16:
			int_datum = m_int_row[xpos * num_components + component];
			WriteDatum(int_datum);
			break;
		case 32:
			c_r32.r32 = m_float_row[xpos * num_components + component];
			m_fifo.PutBits(c_r32.d, 32);
			WriteFlush();
			break;
		case 64:
			c_r64.r64 = m_double_row[xpos * num_components + component];
			m_fifo.PutBits(c_r64.d[0], 32);
			m_fifo.PutBits(c_r64.d[1], 32);
			WriteFlush();
			break;
		}

	}
}

bool HdrDpxImageElement::IsNextSame(uint32_t xpos, int32_t pixel[])
{
	int num_components;

	if (xpos >= m_width - 1)
		return false;
	num_components = GetNumberOfComponents();

	for (int c = 0; c < num_components; ++c)
	{
		if (pixel[c] != m_int_row[(xpos + 1) * num_components + c])
			return false;
	}
	return true;
}

void HdrDpxImageElement::WriteLineEnd()
{
	if (m_fifo.m_fullness & 0x1f)   // not an even multiple of 32 
		m_fifo.PutDatum(0, 32 - (m_fifo.m_fullness & 0x1f), m_direction_r2l);
	WriteFlush();
}


void HdrDpxImageElement::App2DpxPixels(uint32_t row, int32_t *datum_ptr)
{
	// There is no check on whether the d pointer is valid or the size of d, that is the responsiblility of the caller
	if (!m_isinitialized)
	{
		LOG_ERROR(eBadParameter, eFatal, "Tried to write pixels to uninitialized image element");
		return;
	}
	if (!m_is_open_for_write || !m_filestream_ptr->good())
	{
		LOG_ERROR(eFileWriteError, eFatal, "File write error");
		return;
	}
	if (m_dpx_ie_ptr->BitSize >= 32)
	{
		LOG_ERROR(eBadParameter, eFatal, "Failed attempt writing integer pixels to floating point file");
		return;
	}

	uint8_t num_components = GetNumberOfComponents();

	m_int_row = datum_ptr;
	WriteRow(row);
}

void HdrDpxImageElement::App2DpxPixels(uint32_t row, float *datum_ptr)
{
	// There is no check on whether the d pointer is valid or the size of d, that is the responsiblility of the caller
	if (!m_isinitialized)
	{
		LOG_ERROR(eBadParameter, eFatal, "Tried to write pixels from uninitialized image element");
		return;
	}
	if (!m_is_open_for_write || !m_filestream_ptr->good())
	{
		LOG_ERROR(eFileWriteError, eFatal, "File write error");
		return;
	}
	if (m_dpx_ie_ptr->BitSize != 32)
	{
		LOG_ERROR(eBadParameter, eFatal, "Failed attempt writing single-precision pixels to file");
		return;
	}

	uint8_t num_components = GetNumberOfComponents();

	m_float_row = datum_ptr;
	WriteRow(row);
}

void HdrDpxImageElement::App2DpxPixels(uint32_t row, double *datum_ptr)
{
	// There is no check on whether the d pointer is valid or the size of d, that is the responsiblility of the caller
	if (!m_isinitialized)
	{
		LOG_ERROR(eBadParameter, eFatal, "Tried to write pixels from uninitialized image element");
		return;
	}
	if (!m_is_open_for_write || !m_filestream_ptr->good())
	{
		LOG_ERROR(eFileWriteError, eFatal, "File write error");
		return;
	}
	if (m_dpx_ie_ptr->BitSize != 32)
	{
		LOG_ERROR(eBadParameter, eFatal, "Failed attempt writing double-precision pixels to file");
		return;
	}

	uint8_t num_components = GetNumberOfComponents();

	m_double_row = datum_ptr;
	WriteRow(row);
}

void HdrDpxImageElement::WriteRow(uint32_t row)
{
	uint32_t xpos;
	int component;
	int num_components;
	bool is_signed = (m_dpx_ie_ptr->DataSign == 1);
	uint32_t run_length = 0;
	bool freeze_increment = false;
	unsigned int max_run;
	bool run_type;
	int32_t rle_pixel[8];
	uint32_t row_wr_idx;
	const uint8_t bpc = m_dpx_ie_ptr->BitSize;

	m_fifo.Clear();

	if (m_dpx_ie_ptr->Encoding == 1)
	{
		if (m_file_map_ptr->GetActiveRLEIndex() != m_ie_index)
		{
			LOG_ERROR(eFileWriteError, eWarning, "Write row failed because RLE files have to be written sequentially");
		}
	}

	num_components = GetNumberOfComponents();

	if (m_dpx_ie_ptr->Encoding == 1)  // RLE
	{
		if (row == 0)
		{
			uint32_t data_offset = m_dpx_ie_ptr->DataOffset;
			if (data_offset == UNDEFINED_U32)
			{
				std::vector<uint32_t> rle_ie_offsets = m_file_map_ptr->GetRLEIEDataOffsets();
				data_offset = rle_ie_offsets[m_ie_index];
				if (data_offset == UNDEFINED_U32)
				{
					LOG_ERROR(eBadParameter, eFatal, "Could not find valid image data offset");
					return;
				}
				m_dpx_ie_ptr->DataOffset = data_offset;
			}
			m_filestream_ptr->seekp(data_offset);
		}
		else if (row != m_previous_row + 1)
		{
			LOG_ERROR(eBadParameter, eFatal, "When RLE is enabled, rows must be written in sequential order");
			return;
		}
		else
			m_filestream_ptr->seekp(m_previous_file_offset);
		m_previous_row = row;
	}
	else
		m_filestream_ptr->seekp(GetOffsetForRow(row));

	max_run = (1 << (bpc - 1)) - 1;

	xpos = 0;

	// write
	row_wr_idx = 0;
	m_row_rd_idx = 0;
	xpos = 0;
	while (xpos < m_width)
	{
		if (m_dpx_ie_ptr->Encoding == 1 && bpc <= 16) // RLE
		{
			if (xpos == m_width - 1)  // Only one pixel left on line
			{
				WriteDatum(2);  // Indicates run of 1 pixel
				WritePixel(xpos);
				xpos++;
			}
			else {
				for (component = 0; component < num_components; ++component)
				{
					rle_pixel[component] = m_int_row[xpos * num_components + component];
				}
				if (num_components > 1)
					run_type = IsNextSame(xpos, rle_pixel);
				else  // For 1-component IEs, it doesn't make sense to declare a run unleses it lasts more than 2 pixels
					run_type = IsNextSame(xpos, rle_pixel) && IsNextSame(xpos + 1, rle_pixel); 
				if (run_type)  //  Same run
				{
					for (run_length = 1; run_length < m_width - xpos && run_length < max_run - 1; ++run_length)
					{
						if (IsNextSame(xpos + run_length, rle_pixel) != run_type)
							break;
					}
				}
				else   // Different run
				{
					for (run_length = 1; run_length < m_width - xpos - 1 && run_length < max_run - 1; ++run_length)
					{
						for (component = 0; component < num_components; ++component)
						{
							rle_pixel[component] = m_int_row[(xpos + run_length) * num_components + component];
						}
						if (IsNextSame(xpos + run_length, rle_pixel) != run_type)
						{
							run_length--;
							break;
						}
					}
				}
				run_length++;
				if (run_type)
				{
					WriteDatum(1 | (run_length << 1));
					WritePixel(xpos);
					xpos += run_length;
					if (xpos > m_width)
						std::cout << "Something went wrong";
				}
				else
				{
					WriteDatum(0 | (run_length << 1));
					while (run_length--)
					{
						WritePixel(xpos);
						xpos++;
						if (xpos > m_width)
							std::cout << "Something went wrong";
					}
				}
			}
		}
		else  // non-RLE (uncompressed)
		{
			for (run_length = 0; run_length < m_width; ++run_length)
			{
				WritePixel(xpos);
				xpos++;
				if (xpos > m_width)
					std::cout << "Something went wrong";
			}
		}

		if (xpos >= m_width)
		{
			// pad last line
			WriteLineEnd();
		}
	}
	m_previous_file_offset = static_cast<uint32_t>(m_filestream_ptr->tellp());
	if (row == m_height - 1)
	{
		uint32_t padding = 0;
		for (uint32_t b = 0; b < m_dpx_ie_ptr->EndOfImagePadding; ++b)
			m_filestream_ptr->write((char *)(&padding), 4);
		if (m_dpx_ie_ptr->Encoding == 1)
		{
			m_file_map_ptr->EditRegionEnd(m_ie_index, static_cast<uint32_t>(m_filestream_ptr->tellp()));

			m_file_map_ptr->AdvanceRLEIE();
		}
	}
}


void HdrDpxImageElement::ResetWarnings(void)
{
	m_warn_unexpected_nonzero_data_bits = false;
	m_warn_image_data_word_mask = 0;
	m_warn_rle_same_past_eol = false;
	m_warn_rle_diff_past_eol = false;
	m_warn_zero_run_length = false;
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsDataSign field, HdrDpxDataSign value)
{
	if (m_is_header_locked)
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
	m_dpx_ie_ptr->DataSign = static_cast<uint8_t>(value);
}

HdrDpxDataSign HdrDpxImageElement::GetHeader(HdrDpxFieldsDataSign field) const
{
	return static_cast<HdrDpxDataSign>(m_dpx_ie_ptr->DataSign);
}

void HdrDpxImageElement::SetHeader(HdrDpxIEFieldsR32 field, float value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eReferenceLowDataCode:
		if (m_dpx_ie_ptr->BitSize >= 32)
			m_dpx_ie_ptr->LowData.f = value;
		else
			m_dpx_ie_ptr->LowData.d = (uint32_t)(value + 0.5); // Could add bounds checking
		break;
	case eReferenceLowQuantity:
		m_dpx_ie_ptr->LowQuantity = value;
		break;
	case eReferenceHighDataCode:
		if (m_dpx_ie_ptr->BitSize >= 32)
			m_dpx_ie_ptr->HighData.f = value;
		else
			m_dpx_ie_ptr->HighData.d = (uint32_t)(value + 0.5);
		break;
	case eReferenceHighQuantity:
		m_dpx_ie_ptr->HighQuantity = value;
	}
}

float HdrDpxImageElement::GetHeader(HdrDpxIEFieldsR32 field) const
{
	switch (field)
	{
	case eReferenceLowDataCode:
		if (m_dpx_ie_ptr->BitSize >= 32)
			return m_dpx_ie_ptr->LowData.f;
		else
			return (float)m_dpx_ie_ptr->LowData.d;
	case eReferenceLowQuantity:
		return m_dpx_ie_ptr->LowQuantity;
	case eReferenceHighDataCode:
		if (m_dpx_ie_ptr->BitSize >= 32)
			return m_dpx_ie_ptr->HighData.f;
		else
			return (float)(m_dpx_ie_ptr->HighData.d);
	case eReferenceHighQuantity:
		return m_dpx_ie_ptr->HighQuantity;
	}
	return 0;
}


void HdrDpxImageElement::SetHeader(HdrDpxFieldsDescriptor field, HdrDpxDescriptor value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_ie_ptr->Descriptor = static_cast<uint8_t>(value);
}

HdrDpxDescriptor HdrDpxImageElement::GetHeader(HdrDpxFieldsDescriptor field) const
{
	return static_cast<HdrDpxDescriptor>(m_dpx_ie_ptr->Descriptor);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsTransfer field, HdrDpxTransfer value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_ie_ptr->Transfer = static_cast<uint8_t>(value);
}

HdrDpxTransfer HdrDpxImageElement::GetHeader(HdrDpxFieldsTransfer field) const
{
	return static_cast<HdrDpxTransfer>(m_dpx_ie_ptr->Transfer);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsColorimetric field, HdrDpxColorimetric value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_ie_ptr->Colorimetric = static_cast<uint8_t>(value);
}

HdrDpxColorimetric HdrDpxImageElement::GetHeader(HdrDpxFieldsColorimetric field) const
{
	return static_cast<HdrDpxColorimetric>(m_dpx_ie_ptr->Colorimetric);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsBitDepth field, HdrDpxBitDepth value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	if ((m_dpx_ie_ptr->BitSize <= 16 && (value == 32 || value == 64)) ||
		((m_dpx_ie_ptr->BitSize == 32 || m_dpx_ie_ptr->BitSize==64) && value<=16))
	{
		LOG_ERROR(eNoError, eInformational, "Changing bit depth invalidates previous low/high code values");
		m_dpx_ie_ptr->HighData.d = UNDEFINED_U32;
		m_dpx_ie_ptr->LowData.d = UNDEFINED_U32;
	}
	m_dpx_ie_ptr->BitSize = static_cast<uint8_t>(value);
	
}

HdrDpxBitDepth HdrDpxImageElement::GetHeader(HdrDpxFieldsBitDepth field) const
{
	return static_cast<HdrDpxBitDepth>(m_dpx_ie_ptr->BitSize);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsPacking field, HdrDpxPacking value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_ie_ptr->Packing = static_cast<uint16_t>(value);
}

HdrDpxPacking HdrDpxImageElement::GetHeader(HdrDpxFieldsPacking field) const
{
	return static_cast<HdrDpxPacking>(m_dpx_ie_ptr->Packing);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsEncoding field, HdrDpxEncoding value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_ie_ptr->Encoding = static_cast<uint16_t>(value);
}

HdrDpxEncoding HdrDpxImageElement::GetHeader(HdrDpxFieldsEncoding field) const
{
	return static_cast<HdrDpxEncoding>(m_dpx_ie_ptr->Encoding);
}

void HdrDpxImageElement::SetHeader(HdrDpxIEFieldsU32 field, uint32_t value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eOffsetToData:
		m_dpx_ie_ptr->DataOffset = value;
		break;
	case eEndOfLinePadding:
		m_dpx_ie_ptr->EndOfLinePadding = value;
		break;
	case eEndOfImagePadding:
		m_dpx_ie_ptr->EndOfImagePadding = value;
	}
}

uint32_t HdrDpxImageElement::GetHeader(HdrDpxIEFieldsU32 field) const
{
	switch (field)
	{
	case eOffsetToData:
		return m_dpx_ie_ptr->DataOffset;
	case eEndOfLinePadding:
		return m_dpx_ie_ptr->EndOfLinePadding;
	case eEndOfImagePadding:
		return m_dpx_ie_ptr->EndOfImagePadding;
	}
	return 0;
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsColorDifferenceSiting field, HdrDpxColorDifferenceSiting value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	uint32_t bitmask = ~(0xf << (4 * m_ie_index));
	m_dpx_hdr_ptr->ImageHeader.ChromaSubsampling &= bitmask;
	m_dpx_hdr_ptr->ImageHeader.ChromaSubsampling |= static_cast<uint32_t>(value) << (4 * m_ie_index);
}

HdrDpxColorDifferenceSiting HdrDpxImageElement::GetHeader(HdrDpxFieldsColorDifferenceSiting field) const
{
	return static_cast<HdrDpxColorDifferenceSiting>(m_dpx_hdr_ptr->ImageHeader.ChromaSubsampling >> (m_ie_index * 4) & 0xf);
}

void HdrDpxImageElement::SetHeader(HdrDpxIEFieldsString field, std::string value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eDescriptionOfImageElement:
		if (CopyStringN(m_dpx_ie_ptr->Description, value, DESCRIPTION_SIZE))
			m_warnings.push_back("SetHeader(): Specified description of IE (" + value + ") exceeds header field size\n");
		break;
	}
}

std::string HdrDpxImageElement::GetHeader(HdrDpxIEFieldsString field) const
{
	switch (field)
	{
	case eDescriptionOfImageElement:
		return CopyToStringN(m_dpx_ie_ptr->Description, FILE_NAME_SIZE);
	}
	return "";
}

uint32_t HdrDpxImageElement::GetWidth(void) const
{
	return m_width;
}

uint32_t HdrDpxImageElement::GetHeight(void) const
{
	return m_height;
}

uint32_t HdrDpxImageElement::BytesUsed(void)
{
	if (m_dpx_ie_ptr->Encoding == 1)
		return (((static_cast<uint32_t>(m_filestream_ptr->tellp()) - m_dpx_ie_ptr->DataOffset + 3) >> 2) << 2) + m_dpx_ie_ptr->EndOfImagePadding;
	else
		return(GetRowSizeInBytes(true) * m_height + m_dpx_ie_ptr->EndOfImagePadding);
}

uint32_t HdrDpxImageElement::GetImageDataSizeInBytes(void) const
{
	return(GetRowSizeInBytes(true) * m_height + m_dpx_ie_ptr->EndOfImagePadding);
}

void HdrDpxImageElement::CopyHeaderFrom(HdrDpxImageElement *ie)
{
	*m_dpx_ie_ptr = *(ie->m_dpx_ie_ptr);
	SetHeader(eColorDifferenceSiting, ie->GetHeader(eColorDifferenceSiting));
}


/*
bool HdrDpxImageElement::IsOk(void)
{
	return m_err.GetWorstSeverity() != eFatal;
}

int HdrDpxImageElement::GetNumErrors(void)
{
	return m_err.GetNumErrors();
}

void HdrDpxImageElement::GetError(int index, ErrorCode &errcode, ErrorSeverity &severity, std::string &errmsg)
{
	return m_err.GetError(index, errcode, severity, errmsg);
}

void HdrDpxImageElement::ClearErrors()
{
	m_err.Clear();
} */

