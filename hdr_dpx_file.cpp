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
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cmath>
#include "hdr_dpx.h"

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

#if 0
static int g_HdrDpxStringLengths[] =
{
	FILE_NAME_SIZE,
	TIMEDATE_SIZE,
	CREATOR_SIZE,
	PROJECT_SIZE,
	COPYRIGHT_SIZE,
	FILE_NAME_SIZE,
	TIMEDATE_SIZE,
	INPUTNAME_SIZE,
	2,
	2,
	4,
	6,
	4,
	32,
	32,
	100
};
#endif

HdrDpxFile::HdrDpxFile(std::string filename)
{
	union
	{
		uint8_t byte[4];
		uint32_t u32;
	} u;
	u.u32 = 0x12345678;
	m_machine_is_msbf = (u.u32 == 0x12);
	this->OpenForReading(filename);
}

HdrDpxFile::HdrDpxFile()
{
	union
	{
		uint8_t byte[4];
		uint32_t u32;
	} u;
	u.u32 = 0x12345678;
	m_machine_is_msbf = (u.u32 == 0x12);
	memset(&m_dpx_header, 0xff, sizeof(HDRDPXFILEFORMAT));
}

HdrDpxFile::~HdrDpxFile()
{
	if(m_open_for_read || m_open_for_write)
		m_file_stream.close();
}

void HdrDpxFile::OpenForReading(std::string filename)
{
	ErrorObject err;

	m_file_stream.open(filename, std::ios::binary | std::ios::in);

	m_err.Clear();
	m_warn_messages.clear();
	m_file_name = filename;

	if (!m_file_stream)
	{
		LOG_ERROR(eFileOpenError, eFatal, "Unable to open file " + filename + "\n");
		return;
	}
	m_file_stream.read((char *)&m_dpx_header, sizeof(HDRDPXFILEFORMAT));
	bool swapped = ByteSwapToMachine();
	if ((m_machine_is_msbf && swapped) || (!m_machine_is_msbf && !swapped))
		m_byteorder = eLSBF;
	else
		m_byteorder = eMSBF;
	if (m_file_stream.bad())
	{
		LOG_ERROR(eFileReadError, eFatal, "Error attempting to read file " + filename + "\n");
		m_file_stream.close();
		return;
	}

	// Read any present image elements
	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_dpx_header.ImageHeader.ImageElement[ie_idx].DataOffset != UINT32_MAX)
		{
			m_IE[ie_idx].Initialize(ie_idx, &m_file_stream, &m_dpx_header, &m_filemap);
			m_IE[ie_idx].OpenForReading(swapped);
		}
		else
			m_IE[ie_idx].m_isinitialized = false;
	}
	m_file_is_hdr_version = (static_cast<bool>(!strcmp(m_dpx_header.FileHeader.Version, "DPX2.0HDR")));

	m_open_for_write = false;
	m_open_for_read = true;
}

HdrDpxImageElement *HdrDpxFile::GetImageElement(uint8_t ie_idx)
{
	if (ie_idx < 0 || ie_idx > 7)
	{
		LOG_ERROR(eBadParameter, eFatal, "Image element number out of bounds\n");
		return NULL;
	}
	if (!m_IE[ie_idx].m_isinitialized)
	{
		m_IE[ie_idx].Initialize(ie_idx, &m_file_stream, &m_dpx_header, &m_filemap);
	}
	return &(m_IE[ie_idx]);
}


void HdrDpxFile::ByteSwapHeader(void)
{
	ByteSwap32(&(m_dpx_header.FileHeader.Magic));
	ByteSwap32(&(m_dpx_header.FileHeader.ImageOffset));
	ByteSwap32(&(m_dpx_header.FileHeader.FileSize));
	ByteSwap32(&(m_dpx_header.FileHeader.DittoKey));
	ByteSwap32(&(m_dpx_header.FileHeader.GenericSize));
	ByteSwap32(&(m_dpx_header.FileHeader.IndustrySize));
	ByteSwap32(&(m_dpx_header.FileHeader.UserSize));
	ByteSwap32(&(m_dpx_header.FileHeader.EncryptKey));
	ByteSwap32(&(m_dpx_header.FileHeader.StandardsBasedMetadataOffset));

	ByteSwap16(&(m_dpx_header.ImageHeader.Orientation));
	ByteSwap16(&(m_dpx_header.ImageHeader.NumberElements));
	ByteSwap32(&(m_dpx_header.ImageHeader.PixelsPerLine));
	ByteSwap32(&(m_dpx_header.ImageHeader.LinesPerElement));
	ByteSwap32(&(m_dpx_header.ImageHeader.ChromaSubsampling));

	for (int ie_idx = 0; ie_idx < NUM_IMAGE_ELEMENTS; ++ie_idx)
	{
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].DataSign));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].LowData.d));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].LowQuantity));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].HighData.d));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].HighQuantity));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].DataOffset));
		ByteSwap16(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].Packing));
		ByteSwap16(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].Encoding));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].DataOffset));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].EndOfLinePadding));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[ie_idx].EndOfImagePadding));
	}

	/* Image source info header details */
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.XOffset));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.YOffset));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.XCenter));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.YCenter));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.XOriginalSize));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.YOriginalSize));
	for (int border_idx = 0; border_idx<4; ++border_idx)
		ByteSwap16(&(m_dpx_header.SourceInfoHeader.Border[border_idx]));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.AspectRatio[0]));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.AspectRatio[1]));

	/* Film header info details */
	ByteSwap32(&(m_dpx_header.FilmHeader.FramePosition));
	ByteSwap32(&(m_dpx_header.FilmHeader.SequenceLen));
	ByteSwap32(&(m_dpx_header.FilmHeader.HeldCount));
	ByteSwap32(&(m_dpx_header.FilmHeader.FrameRate));
	ByteSwap32(&(m_dpx_header.FilmHeader.ShutterAngle));

	/* TV header info details */
	ByteSwap32(&(m_dpx_header.TvHeader.TimeCode));
	ByteSwap32(&(m_dpx_header.TvHeader.UserBits));
	ByteSwap32(&(m_dpx_header.TvHeader.HorzSampleRate));
	ByteSwap32(&(m_dpx_header.TvHeader.VertSampleRate));
	ByteSwap32(&(m_dpx_header.TvHeader.FrameRate));
	ByteSwap32(&(m_dpx_header.TvHeader.TimeOffset));
	ByteSwap32(&(m_dpx_header.TvHeader.Gamma));
	ByteSwap32(&(m_dpx_header.TvHeader.BlackLevel));
	ByteSwap32(&(m_dpx_header.TvHeader.BlackGain));
	ByteSwap32(&(m_dpx_header.TvHeader.Breakpoint));
	ByteSwap32(&(m_dpx_header.TvHeader.WhiteLevel));
	ByteSwap32(&(m_dpx_header.TvHeader.IntegrationTimes));

}

void HdrDpxFile::ByteSwapSbmHeader(void)
{
	/* Standards-based metadata (if applicable) */
	ByteSwap32(&(m_dpx_sbmdata.SbmLength));
}

bool HdrDpxFile::ByteSwapToMachine(void)
{

	if (m_dpx_header.FileHeader.Magic == 0x58504453)  // XPDS
	{
		ByteSwapHeader();
		ByteSwapSbmHeader();
		return(true);
	}
	return(false);
}


bool HdrDpxFile::IsHdr(void) const
{
	return (strcmp(m_dpx_header.FileHeader.Version, "V2.0HDR") == 0);
}

void HdrDpxFile::ComputeOffsets()
{
	uint32_t data_offset;
	uint32_t min_offset = UINT32_MAX;

	m_filemap.Reset();

	// Fill in what is specified
	m_filemap.AddRegion(0, sizeof(HDRDPXFILEFORMAT), 100);

	if (m_dpx_header.FileHeader.UserSize == UINT32_MAX)   // Undefined value is not permitted in 268-2
		m_dpx_header.FileHeader.UserSize = 0;

	if (m_dpx_header.FileHeader.UserSize != 0)
		m_filemap.AddRegion(sizeof(HDRDPXFILEFORMAT), ((sizeof(HDRDPXFILEFORMAT) + m_dpx_header.FileHeader.UserSize + 3) >> 2) << 2, 101);

	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UINT32_MAX && m_dpx_header.FileHeader.StandardsBasedMetadataOffset != eSBMAutoLocate)
		m_filemap.AddRegion(m_dpx_header.FileHeader.StandardsBasedMetadataOffset, m_dpx_header.FileHeader.StandardsBasedMetadataOffset + m_dpx_sbmdata.SbmLength + 132, 102);

	// Add IEs with data offsets
	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			m_IE[ie_idx].ComputeWidthAndHeight();
			data_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
			if (data_offset != UINT32_MAX)
			{
				if (m_IE[ie_idx].GetHeader(eEncoding) == eEncodingRLE)   // RLE can theoretically use more bits than compressed
				{
					uint32_t est_size = static_cast<uint32_t>((1.0 + RLE_MARGIN) * m_IE[ie_idx].GetImageDataSizeInBytes());
					m_filemap.AddRegion(data_offset, data_offset + est_size, ie_idx);
					m_filemap.AddRLEIE(ie_idx, data_offset, est_size);
				}
				else
					m_filemap.AddRegion(data_offset, data_offset + m_IE[ie_idx].GetImageDataSizeInBytes(), ie_idx);
			}
		}
	}


	// Next add fixed-size IE's that don't yet have data offsets
	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			data_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
			if (data_offset == UINT32_MAX && m_IE[ie_idx].GetHeader(eEncoding) != eEncodingRLE)
				m_IE[ie_idx].SetHeader(eOffsetToData, m_filemap.FindEmptySpace(m_IE[ie_idx].GetImageDataSizeInBytes(), ie_idx));
		}
	}

	// Lastly add first variable-size IE
	int first_ie = 1;
	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			data_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
			if (data_offset == UINT32_MAX && m_IE[ie_idx].GetHeader(eEncoding) == eEncodingRLE)
			{
				uint32_t est_size = static_cast<uint32_t>((1.0 + RLE_MARGIN) * m_IE[ie_idx].GetImageDataSizeInBytes());
				est_size = ((est_size + 3) >> 2) << 2;  // round to dword boundary
				if (first_ie)
				{
					data_offset = m_filemap.FindEmptySpace(est_size, ie_idx);
					m_IE[ie_idx].SetHeader(eOffsetToData, data_offset);
					first_ie = 0;
				}
				m_filemap.AddRLEIE(ie_idx, data_offset, est_size);
			}
		}
	}

	// Figure out where image data starts
	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized && min_offset > m_IE[ie_idx].GetHeader(eOffsetToData))
			min_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
	}
	if (m_dpx_header.FileHeader.ImageOffset == UINT32_MAX)
		m_dpx_header.FileHeader.ImageOffset = min_offset;
	if (m_dpx_header.FileHeader.ImageOffset != min_offset)
		LOG_ERROR(eBadParameter, eWarning, "Image offset in main header does not match smallest image element offset");

	if (m_filemap.CheckCollisions())
		LOG_ERROR(eBadParameter, eWarning, "Image map has potentially overlapping regions");
}

uint8_t HdrDpxFile::GetActiveIE() const
{
	return m_active_rle_ie;
}

void HdrDpxFile::FillCoreFields()
{
	int number_of_ie = 0, first_ie_idx = -1;

	if (m_dpx_header.FileHeader.Magic != 0x53445058 && m_dpx_header.FileHeader.Magic != UINT32_MAX)
	{
		LOG_ERROR(eMissingCoreField, eWarning, "Unrecognized magic number");
	}
	m_dpx_header.FileHeader.Magic = 0x53445058;

	CopyStringN(m_dpx_header.FileHeader.Version, "V2.0HDR", 8);
	if (m_dpx_header.FileHeader.DatumMappingDirection == 0xff)
	{
		m_dpx_header.FileHeader.DatumMappingDirection = 1;
		LOG_ERROR(eMissingCoreField, eWarning, "Datum mapping direction not specified, defaulting to 1 (left-to-right)");
	}
	m_dpx_header.FileHeader.GenericSize = 1684;
	m_dpx_header.FileHeader.IndustrySize = 384;

	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			number_of_ie++;
			if(first_ie_idx < 0)
				first_ie_idx = ie_idx;
		}
	}
	if(first_ie_idx < 0)
		LOG_ERROR(eBadParameter, eFatal, "No initialized image elements found");

	if (m_dpx_header.ImageHeader.Orientation == UINT16_MAX)
	{
		LOG_ERROR(eMissingCoreField, eWarning, "Image orientation not specified, assuming left-to-right, top-to-bottom");
		m_dpx_header.ImageHeader.Orientation = 0;
	}
	if (m_dpx_header.ImageHeader.NumberElements == 0 || m_dpx_header.ImageHeader.NumberElements == UINT16_MAX)
		m_dpx_header.ImageHeader.NumberElements = number_of_ie;

	if (m_dpx_header.ImageHeader.PixelsPerLine == UINT32_MAX)
	{
		LOG_ERROR(eFileWriteError, eFatal, "Pixels per line must be specified");
		return;
	}
	if (m_dpx_header.ImageHeader.LinesPerElement == UINT32_MAX)
	{
		LOG_ERROR(eFileWriteError, eFatal, "Lines per element must be specified");
		return;
	}
	for (int ie_idx = 0; ie_idx < number_of_ie; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			// Default values for core fields that are not specified
			if (m_IE[ie_idx].GetHeader(eBitDepth) == eBitDepthUndefined)
			{
				LOG_ERROR(eMissingCoreField, eFatal, "Bit depth must be specified");
				return;
			}

			if (m_IE[ie_idx].GetHeader(eDescriptor) == eDescUndefined)
			{
				LOG_ERROR(eMissingCoreField, eFatal, "Descriptor must be specified");
				return;
			}

			if (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUndefined)
			{
				LOG_ERROR(eMissingCoreField, eWarning, "Data sign for image element " + std::to_string(ie_idx + 1) + " not specified, assuming unsigned");
				m_IE[ie_idx].SetHeader(eDataSign, eDataSignUnsigned);
			}
			if (std::isnan(m_IE[ie_idx].GetHeader(eReferenceLowDataCode)) || std::isnan(m_IE[ie_idx].GetHeader(eReferenceHighDataCode)))
			{
				if (m_IE[ie_idx].GetHeader(eBitDepth) < 32)
				{
					LOG_ERROR(eMissingCoreField, eWarning, "Data range for image element " + std::to_string(ie_idx + 1) + " not specified, assuming full range");
					m_IE[ie_idx].SetHeader(eReferenceLowDataCode, 0);
					m_IE[ie_idx].SetHeader(eReferenceHighDataCode, static_cast<float>((1 << m_dpx_header.ImageHeader.ImageElement[ie_idx].BitSize) - 1));
				}
				else  // Floating point samples
				{
					LOG_ERROR(eMissingCoreField, eWarning, "Data range for image element " + std::to_string(ie_idx + 1) + " not specified, assuming full range (maxval = 1.0)");
					m_IE[ie_idx].SetHeader(eReferenceLowDataCode, (float)0.0);
					m_IE[ie_idx].SetHeader(eReferenceHighDataCode, (float)1.0);
				}
			}
		}
		else {
			// Set ununsed image elements to all 1's
			memset((void *)&(m_dpx_header.ImageHeader.ImageElement[ie_idx]), 0xff, sizeof(HDRDPX_IMAGEELEMENT));
		}
		if (m_IE[ie_idx].GetHeader(eTransfer) == eTransferUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Transfer characteristic for image element " + std::to_string(ie_idx + 1) + " not specified");
		}
		if (m_IE[ie_idx].GetHeader(eColorimetric) == eColorimetricUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Colorimetric field for image element " + std::to_string(ie_idx + 1) + " not specified");
		}
		if (m_IE[ie_idx].GetHeader(ePacking) == ePackingUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Packing field not specified for image element " + std::to_string(ie_idx + 1) + ", assuming Packed");
			m_IE[ie_idx].SetHeader(ePacking, ePackingPacked);
		}
		if (m_IE[ie_idx].GetHeader(eEncoding) == eEncodingUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Encoding not specified for image element " + std::to_string(ie_idx + 1) + ", assuming uncompressed");
			m_IE[ie_idx].SetHeader(eEncoding, eEncodingNoEncoding);		// No compression (default)
		}
		if (m_IE[ie_idx].GetHeader(eEndOfLinePadding) == UINT32_MAX)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "End of line padding not specified for image element " + std::to_string(ie_idx + 1) + ", assuming no padding");
			m_IE[ie_idx].SetHeader(eEndOfLinePadding, 0);
		}
		if (m_IE[ie_idx].GetHeader(eEndOfImagePadding) == UINT32_MAX)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "End of image padding not specified for image element " + std::to_string(ie_idx + 1) + ", assuming no padding");
			m_IE[ie_idx].SetHeader(eEndOfImagePadding, 0);
		}
	}
	ComputeOffsets();
}


void HdrDpxFile::OpenForWriting(std::string filename)
{
	bool byte_swap;

	m_file_stream.open(filename, std::ios::binary | std::ios::out);

	if (m_byteorder == eNativeByteOrder)
		byte_swap = false;
	else if (m_byteorder == eLSBF)
		byte_swap = m_machine_is_msbf ? true : false;
	else
		byte_swap = m_machine_is_msbf ? false : true;

	m_file_name = filename;

	if (!m_file_stream)
	{
		LOG_ERROR(eFileOpenError, eFatal, "Unable to open file " + filename + " for output");
		return;
	}

	FillCoreFields();
	// Maybe check if core fields are valid here?

	if (m_file_stream.bad())
	{
		LOG_ERROR(eFileWriteError, eFatal, "Error writing DPX image data");
		return;
	}

	for (int ie_idx = 0; ie_idx<8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			m_IE[ie_idx].OpenForWriting(byte_swap);
		}
	}


	if (m_file_stream.bad())
	{
		LOG_ERROR(eFileWriteError, eFatal, "Error writing DPX image data");
		m_file_stream.close();
	}
	m_open_for_write = true;
	m_open_for_read = false;
	m_file_is_hdr_version = true;
}

void HdrDpxFile::Close()
{
	if (m_open_for_write)
	{
		// Compute SBM header offset if auto mode enabled
		if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset == eSBMAutoLocate)
		{
			m_dpx_header.FileHeader.StandardsBasedMetadataOffset = static_cast<uint32_t>(m_file_stream.tellp());
		}

		// Get RLE offsets if needed
		std::vector<uint32_t> data_offsets = m_filemap.GetRLEIEDataOffsets();
		for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
			if (m_dpx_header.ImageHeader.ImageElement[ie_idx].Encoding == 1)
				m_dpx_header.ImageHeader.ImageElement[ie_idx].DataOffset = data_offsets[ie_idx];

		m_file_stream.seekp(0, std::ios::beg);
		// Swap before writing header
		if ((m_byteorder == eLSBF && m_machine_is_msbf) || (m_byteorder == eMSBF && !m_machine_is_msbf))
			ByteSwapHeader();
		m_file_stream.write((char *)&m_dpx_header, sizeof(HDRDPXFILEFORMAT));
		if ((m_byteorder == eLSBF && m_machine_is_msbf) || (m_byteorder == eMSBF && !m_machine_is_msbf))
			ByteSwapHeader();

		if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UINT32_MAX)
		{
			// Write standards-based metadata
			m_file_stream.seekp(m_dpx_header.FileHeader.StandardsBasedMetadataOffset, std::ios::beg);
			if ((m_byteorder == eLSBF && m_machine_is_msbf) || (m_byteorder == eMSBF && !m_machine_is_msbf))
				ByteSwapSbmHeader();
			m_file_stream.write((char *)&m_dpx_sbmdata, 132 + m_dpx_sbmdata.SbmLength);
			if ((m_byteorder == eLSBF && m_machine_is_msbf) || (m_byteorder == eMSBF && !m_machine_is_msbf))
				ByteSwapSbmHeader();
		}
	}
	if (m_open_for_read || m_open_for_write)
	{
		m_file_stream.close();
		for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
			m_IE[ie_idx].m_isinitialized = false;
		m_open_for_read = false;
		m_open_for_write = false;
	}
}

static std::string u32_to_hex(uint32_t value)
{
	std::stringstream sstream;
	sstream << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
	return sstream.str();
}

#define PRINT_FIELD_U32(n,v) if((v)!=UNDEFINED_U32) { header += n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_FIELD_U32_HEX(n,v) if((v)!=UNDEFINED_U32) { header += n + "\t" + u32_to_hex(v) + "\n"; }
#define PRINT_RO_FIELD_U32(n,v) if((v)!=UNDEFINED_U32) { header += "// " + n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_RO_FIELD_U32_HEX(n,v) if((v)!=UNDEFINED_U32) {  header += "// " + n + "\t" + u32_to_hex(v) + "\n";  }
#define PRINT_FIELD_U8(n,v) if((v)!=UNDEFINED_U8) {  header += n + "\t" + std::to_string(v) + "\n";  }
#define PRINT_RO_FIELD_U8(n,v) if((v)!=UNDEFINED_U8) { header += "// " + n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_FIELD_U16(n,v) if((v)!=UNDEFINED_U16) {  header += n + "\t" + std::to_string(v) + "\n";  }
#define PRINT_FIELD_R32(n,v) if(memcmp(&v, &undefined_4bytes, 4)!=0) { header += n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_FIELD_ASCII(n,v,l) if((v[0])!='\0') { char s[l+1]; s[l] = '\0'; memcpy(s, v, l); header += n + "\t" + std::string(s) + "\n";  }
std::string HdrDpxFile::DumpHeader() const
{
	static const uint32_t undefined_4bytes = UINT32_MAX;  /* 4-byte block 0xffffffff used to check if floats are undefined */
	std::string header;

	header = header + "\n\n//////////////////////////////////////////////////////////////////\n" +
		"// File information header for " + m_file_name + "\n" +
		"//////////////////////////////////////////////////////////////////\n";

	PRINT_RO_FIELD_U32_HEX(std::string("Magic"), static_cast<unsigned int>(m_dpx_header.FileHeader.Magic));

	PRINT_FIELD_U32(std::string("Image_Offset"), m_dpx_header.FileHeader.ImageOffset);
	PRINT_FIELD_ASCII(std::string("// Version"), m_dpx_header.FileHeader.Version, 8);
	PRINT_RO_FIELD_U32(std::string("File_Size"), m_dpx_header.FileHeader.FileSize);
	PRINT_FIELD_U32(std::string("Ditto_Key"), m_dpx_header.FileHeader.DittoKey);
	PRINT_RO_FIELD_U32(std::string("Generic_Size"), m_dpx_header.FileHeader.GenericSize);
	PRINT_RO_FIELD_U32(std::string("Industry_Size"), m_dpx_header.FileHeader.IndustrySize);
	PRINT_RO_FIELD_U32(std::string("User_Size"), m_dpx_header.FileHeader.UserSize);
	PRINT_FIELD_ASCII(std::string("File_Name"), m_dpx_header.FileHeader.FileName, FILE_NAME_SIZE + 1);
	PRINT_FIELD_ASCII(std::string("Time_Date"), m_dpx_header.FileHeader.TimeDate, 24);
	PRINT_FIELD_ASCII(std::string("Creator"), m_dpx_header.FileHeader.Creator, 100);
	PRINT_FIELD_ASCII(std::string("Project"), m_dpx_header.FileHeader.Project, 200);
	PRINT_FIELD_ASCII(std::string("Copyright"), m_dpx_header.FileHeader.Copyright, 200);
	PRINT_FIELD_U32(std::string("Encrypt_Key"), m_dpx_header.FileHeader.EncryptKey);
	PRINT_FIELD_U32(std::string("Standards_Based_Metadata_Offset"), m_dpx_header.FileHeader.StandardsBasedMetadataOffset);
	PRINT_FIELD_U8(std::string("Datum_Mapping_Direction"), m_dpx_header.FileHeader.DatumMappingDirection);

	header += "\n// Image information header\n";
	PRINT_FIELD_U16(std::string("Orientation"), m_dpx_header.ImageHeader.Orientation);
	PRINT_FIELD_U16(std::string("Number_Elements"), m_dpx_header.ImageHeader.NumberElements);
	PRINT_RO_FIELD_U32(std::string("Pixels_Per_Line"), m_dpx_header.ImageHeader.PixelsPerLine);
	PRINT_RO_FIELD_U32(std::string("Lines_Per_Element"), m_dpx_header.ImageHeader.LinesPerElement);

	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		std::string fld_name;
		header += "//// Image element data structure " + std::to_string(ie_idx + 1) + "\n";
		fld_name = "Data_Sign_" + std::to_string(ie_idx + 1);		PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].DataSign);
		if (m_dpx_header.ImageHeader.ImageElement[ie_idx].BitSize < 32)
		{
			fld_name = "Low_Data_" + std::to_string(ie_idx + 1);			PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].LowData.d);
		}
		else
		{
			fld_name = "Low_Data_" + std::to_string(ie_idx + 1);			PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].LowData.f);
		}
		fld_name = "Low_Quantity_" + std::to_string(ie_idx + 1);		PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].LowQuantity);
		if (m_dpx_header.ImageHeader.ImageElement[ie_idx].BitSize < 32)
		{
			fld_name = "High_Data_" + std::to_string(ie_idx + 1);		PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].HighData.d);
		}
		else
		{
			fld_name = "High_Data_" + std::to_string(ie_idx + 1);		PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].HighData.f);
		}
		fld_name = "High_Quantity_" + std::to_string(ie_idx + 1);	PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].HighQuantity);
		fld_name = "Descriptor_" + std::to_string(ie_idx + 1);		PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].Descriptor);
		fld_name = "Transfer_" + std::to_string(ie_idx + 1);		PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].Transfer);
		fld_name = "Colorimetric_" + std::to_string(ie_idx + 1);	PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].Colorimetric);
		fld_name = "BitSize_" + std::to_string(ie_idx + 1);			PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].BitSize);
		fld_name = "Packing_" + std::to_string(ie_idx + 1);			PRINT_FIELD_U16(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].Packing);
		fld_name = "Encoding_" + std::to_string(ie_idx + 1);		PRINT_FIELD_U16(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].Encoding);
		fld_name = "DataOffset_" + std::to_string(ie_idx + 1);		PRINT_RO_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].DataOffset);
		fld_name = "End_Of_Line_Padding_" + std::to_string(ie_idx + 1); PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].EndOfLinePadding);
		fld_name = "End_Of_Image_Padding_" + std::to_string(ie_idx + 1); PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].EndOfImagePadding);
		fld_name = "Description_" + std::to_string(ie_idx + 1);		PRINT_FIELD_ASCII(fld_name, m_dpx_header.ImageHeader.ImageElement[ie_idx].Description, 32);
		fld_name = "Chroma_Subsampling_" + std::to_string(ie_idx + 1); PRINT_FIELD_U8(fld_name, 0xf & (m_dpx_header.ImageHeader.ChromaSubsampling >> (ie_idx * 4)));
	}

	header += "\n// Image source information header\n";
	PRINT_FIELD_U32(std::string("X_Offset"), m_dpx_header.SourceInfoHeader.XOffset);
	PRINT_FIELD_U32(std::string("Y_Offset"), m_dpx_header.SourceInfoHeader.YOffset);
	PRINT_FIELD_R32(std::string("X_Center"), m_dpx_header.SourceInfoHeader.XCenter);
	PRINT_FIELD_R32(std::string("Y_Center"), m_dpx_header.SourceInfoHeader.YCenter);
	PRINT_FIELD_U32(std::string("X_Original_Size"), m_dpx_header.SourceInfoHeader.XOriginalSize);
	PRINT_FIELD_U32(std::string("Y_Original_Size"), m_dpx_header.SourceInfoHeader.YOriginalSize);
	PRINT_FIELD_ASCII(std::string("Source_File_Name"), m_dpx_header.SourceInfoHeader.SourceFileName, FILE_NAME_SIZE + 1);
	PRINT_FIELD_ASCII(std::string("Source_Time_Date"), m_dpx_header.SourceInfoHeader.SourceTimeDate, 24);
	PRINT_FIELD_ASCII(std::string("Input_Name"), m_dpx_header.SourceInfoHeader.InputName, 32);
	PRINT_FIELD_ASCII(std::string("Input_SN"), m_dpx_header.SourceInfoHeader.InputSN, 32);
	PRINT_FIELD_U16(std::string("Border_XL"), m_dpx_header.SourceInfoHeader.Border[0]);
	PRINT_FIELD_U16(std::string("Border_XR"), m_dpx_header.SourceInfoHeader.Border[1]);
	PRINT_FIELD_U16(std::string("Border_YT"), m_dpx_header.SourceInfoHeader.Border[2]);
	PRINT_FIELD_U16(std::string("Border_YB"), m_dpx_header.SourceInfoHeader.Border[3]);
	PRINT_FIELD_U32(std::string("Aspect_Ratio_H"), m_dpx_header.SourceInfoHeader.AspectRatio[0]);
	PRINT_FIELD_U32(std::string("Aspect_Ratio_V"), m_dpx_header.SourceInfoHeader.AspectRatio[1]);

	header += "\n// Motion-picture film information header\n";
	PRINT_FIELD_ASCII(std::string("Film_Mfg_Id"), m_dpx_header.FilmHeader.FilmMfgId, 2);
	PRINT_FIELD_ASCII(std::string("Film_Type"), m_dpx_header.FilmHeader.FilmType, 2);
	PRINT_FIELD_ASCII(std::string("Offset_Perfs"), m_dpx_header.FilmHeader.OffsetPerfs, 2);
	PRINT_FIELD_ASCII(std::string("Prefix"), m_dpx_header.FilmHeader.Prefix, 6);
	PRINT_FIELD_ASCII(std::string("Count"), m_dpx_header.FilmHeader.Count, 4);
	PRINT_FIELD_ASCII(std::string("Format"), m_dpx_header.FilmHeader.Format, 32);
	PRINT_FIELD_U32(std::string("Frame_Position"), m_dpx_header.FilmHeader.FramePosition);
	PRINT_FIELD_U32(std::string("Sequence_Len"), m_dpx_header.FilmHeader.SequenceLen);
	PRINT_FIELD_U32(std::string("Held_Count"), m_dpx_header.FilmHeader.HeldCount);
	PRINT_FIELD_R32(std::string("MP_Frame_Rate"), m_dpx_header.FilmHeader.FrameRate);
	PRINT_FIELD_R32(std::string("Shutter_Angle"), m_dpx_header.FilmHeader.ShutterAngle);
	PRINT_FIELD_ASCII(std::string("Frame_Id"), m_dpx_header.FilmHeader.FrameId, 32);
	PRINT_FIELD_ASCII(std::string("Slate_Info"), m_dpx_header.FilmHeader.SlateInfo, 100);

	header += "\n// Television information header\n";
	PRINT_FIELD_U32_HEX(std::string("Time_Code"), m_dpx_header.TvHeader.TimeCode);
	PRINT_FIELD_U32_HEX(std::string("User_Bits"), m_dpx_header.TvHeader.UserBits);
	PRINT_FIELD_U8(std::string("Interlace"), m_dpx_header.TvHeader.Interlace);
	PRINT_FIELD_U8(std::string("Field_Number"), m_dpx_header.TvHeader.FieldNumber);
	PRINT_FIELD_U8(std::string("Video_Signal"), m_dpx_header.TvHeader.VideoSignal);
	PRINT_FIELD_R32(std::string("Horz_Sample_Rate"), m_dpx_header.TvHeader.HorzSampleRate);
	PRINT_FIELD_R32(std::string("Vert_Sample_Rate"), m_dpx_header.TvHeader.VertSampleRate);
	PRINT_FIELD_R32(std::string("TV_Frame_Rate"), m_dpx_header.TvHeader.FrameRate);
	PRINT_FIELD_R32(std::string("Time_Offset"), m_dpx_header.TvHeader.TimeOffset);
	PRINT_FIELD_R32(std::string("Gamma"), m_dpx_header.TvHeader.Gamma);
	PRINT_FIELD_R32(std::string("Black_Level"), m_dpx_header.TvHeader.BlackLevel);
	PRINT_FIELD_R32(std::string("Black_Gain"), m_dpx_header.TvHeader.BlackGain);
	PRINT_FIELD_R32(std::string("Breakpoint"), m_dpx_header.TvHeader.Breakpoint);
	PRINT_FIELD_R32(std::string("White_Level"), m_dpx_header.TvHeader.WhiteLevel);
	PRINT_FIELD_R32(std::string("Integration_Times"), m_dpx_header.TvHeader.IntegrationTimes);
	PRINT_FIELD_U8(std::string("Video_Identification_Code"), m_dpx_header.TvHeader.VideoIdentificationCode);
	PRINT_FIELD_U8(std::string("SMPTE_TC_Type"), m_dpx_header.TvHeader.SMPTETCType);
	PRINT_FIELD_U8(std::string("SMPTE_TC_DBB2"), m_dpx_header.TvHeader.SMPTETCDBB2);

	if (m_dpx_header.FileHeader.UserSize > 0 && m_dpx_header.FileHeader.UserSize != UINT32_MAX)
	{
		header += "\n// User data header\n";
		PRINT_FIELD_ASCII(std::string("User_Identification"), m_dpx_userdata.UserIdentification, 32);
	}

	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UINT32_MAX)
	{
		header += "\n// Standards-based metadata header\n";
		PRINT_FIELD_ASCII(std::string("Sbm_Descriptor"), m_dpx_sbmdata.SbmFormatDescriptor, 128);
		PRINT_FIELD_U32(std::string("Sbm_Length"), m_dpx_sbmdata.SbmLength);
	}
	return header;
}



bool HdrDpxFile::Validate()
{
	// TBD:: Add validation warnings/info

	return m_err.GetWorstSeverity() != eInformational;
}

bool HdrDpxFile::CopyStringN(char *dest, std::string src, size_t max_length)
{
	for (size_t idx = 0; idx < max_length; ++idx)
	{
		if (idx >= src.length())
			*(dest++) = '\0';
		else
			*(dest++) = src[idx];
	}
	return (src.length() > max_length);  // return true if the string is longer than the field
}

std::size_t utf8_length(const std::string &utf8_string)
{
	std::size_t result = 0;
	const char *ptr = utf8_string.data();
	const char *end = ptr + utf8_string.size();
	std::mblen(NULL, 0);
	while (ptr < end)
	{
		int next = std::mblen(ptr, end - ptr);
		if (next == -1)
			return 0;		// String not valid
		ptr += next;
		++result;
	}
	return result;
}


void HdrDpxFile::SetHeader(HdrDpxFieldsString field, const std::string &value)
{
	std::size_t length;
	switch (field)
	{
	case eFileName:
		if(CopyStringN(m_dpx_header.FileHeader.FileName, value, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified file name (" + value + ") exceeds header field size\n");
		break;
	case eTimeDate:
		if (CopyStringN(m_dpx_header.FileHeader.TimeDate, value, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified time & date (" + value + ") exceeds header field size\n");
		break;
	case eCreator:
		if (CopyStringN(m_dpx_header.FileHeader.Creator, value, CREATOR_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified creator (" + value + ") exceeds header field size\n");
		break;
	case eProject:
		if (CopyStringN(m_dpx_header.FileHeader.Project, value, PROJECT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified project (" + value + ") exceeds header field size\n");
		break;
	case eCopyright:
		if (CopyStringN(m_dpx_header.FileHeader.Copyright, value, COPYRIGHT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified copyright (" + value + ") exceeds header field size\n");
		break;
	case eSourceFileName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceFileName, value, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source file name (" + value + ") exceeds header field size\n");
		break;
	case eSourceTimeDate:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceTimeDate, value, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source time & date (" + value + ") exceeds header field size\n");
		break;
	case eInputName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputName, value, INPUTNAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input name (" + value + ") exceeds header field size\n");
		break;
	case eInputSN:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputSN, value, INPUTSN_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input SN (" + value + ") exceeds header field size\n");
		break;
	case eFilmMfgId:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmMfgId, value, 2))
			m_warn_messages.push_back("SetHeader(): Specified film mfg id (" + value + ") exceeds header field size\n");
		break;
	case eFilmType:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmType, value, 2))
			m_warn_messages.push_back("SetHeader(): Specified film type (" + value + ") exceeds header field size\n");
		break;
	case eOffsetPerfs:
		if (CopyStringN(m_dpx_header.FilmHeader.OffsetPerfs, value, 4))
			m_warn_messages.push_back("SetHeader(): Specified offset perfs (" + value + ") exceeds header field size\n");
		break;
	case ePrefix:
		if (CopyStringN(m_dpx_header.FilmHeader.Prefix, value, 6))
			m_warn_messages.push_back("SetHeader(): Specified prefix (" + value + ") exceeds header field size\n");
		break;
	case eCount:
		if (CopyStringN(m_dpx_header.FilmHeader.Count, value, 4))
			m_warn_messages.push_back("SetHeader(): Specified film count (" + value + ") exceeds header field size\n");
		break;
	case eFormat:
		if (CopyStringN(m_dpx_header.FilmHeader.Format, value, 32))
			m_warn_messages.push_back("SetHeader(): Specified film format (" + value + ") exceeds header field size\n");
		break;
	case eFrameId:
		if (CopyStringN(m_dpx_header.FilmHeader.FrameId, value, 32))
			m_warn_messages.push_back("SetHeader(): Specified frame ID (" + value + ") exceeds header field size\n");
		break;
	case eSlateInfo:
		if (CopyStringN(m_dpx_header.FilmHeader.SlateInfo, value, 100))
			m_warn_messages.push_back("SetHeader(): Specified slate info (" + value + ") exceeds header field size\n");
		break;
	case eUserIdentification:
		if (CopyStringN(m_dpx_userdata.UserIdentification, value, 32))
			m_warn_messages.push_back("SetHeader(): Specified user identification (" + value + ") exceeds header field size\n");
		break;
	case eUserDefinedData:
		length = utf8_length(value);
		m_dpx_userdata.UserData.resize(length);
		memcpy(m_dpx_userdata.UserData.data(), value.data(), length);
		break;
	case eSBMFormatDescriptor:
		if (CopyStringN(m_dpx_sbmdata.SbmFormatDescriptor, value, 128))
			m_warn_messages.push_back("SetHeader(): Specified SBM format descriptor (" + value + ") exceeds header field size\n");
		break;
	case eSBMetadata:
		length = utf8_length(value);
		m_dpx_sbmdata.SbmData.resize(length);
		memcpy(m_dpx_sbmdata.SbmData.data(), value.data(), length);
		break;
	}
}

std::string HdrDpxFile::CopyToStringN(const char *src, int max_length) const
{
	std::string s ("");

	for (int idx = 0; idx < max_length; ++idx)
	{
		if (*src == '\0')
			return s;
		else
			s.push_back(*(src++));
	}
	return s;
}

std::string HdrDpxFile::GetHeader(HdrDpxFieldsString field) const
{
	switch (field)
	{
	case eFileName:
		return CopyToStringN(m_dpx_header.FileHeader.FileName, FILE_NAME_SIZE);
	case eTimeDate:
		return CopyToStringN(m_dpx_header.FileHeader.TimeDate, TIMEDATE_SIZE);
	case eCreator:
		return CopyToStringN(m_dpx_header.FileHeader.Creator, CREATOR_SIZE);
	case eProject:
		return CopyToStringN(m_dpx_header.FileHeader.Project, PROJECT_SIZE);
	case eCopyright:
		return CopyToStringN(m_dpx_header.FileHeader.Copyright, COPYRIGHT_SIZE);
	case eSourceFileName:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.SourceFileName, FILE_NAME_SIZE);
	case eSourceTimeDate:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.SourceTimeDate, TIMEDATE_SIZE);
	case eInputName:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.InputName, INPUTNAME_SIZE);
	case eInputSN:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.InputSN, INPUTSN_SIZE);
	case eFilmMfgId:
		return CopyToStringN(m_dpx_header.FilmHeader.FilmMfgId, 2);
	case eFilmType:
		return CopyToStringN(m_dpx_header.FilmHeader.FilmType, 2);
	case eOffsetPerfs:
		return CopyToStringN(m_dpx_header.FilmHeader.OffsetPerfs, 4);
	case ePrefix:
		return CopyToStringN(m_dpx_header.FilmHeader.Prefix, 6);
	case eCount:
		return CopyToStringN(m_dpx_header.FilmHeader.Count, 4);
	case eFormat:
		return CopyToStringN(m_dpx_header.FilmHeader.Format, 32);
	case eFrameId:
		return CopyToStringN(m_dpx_header.FilmHeader.FrameId, 32);
	case eSlateInfo:
		return CopyToStringN(m_dpx_header.FilmHeader.SlateInfo, 100);
	case eUserIdentification:
		return CopyToStringN(m_dpx_userdata.UserIdentification, 32);
	case eUserDefinedData:
		if (!m_dpx_userdata.UserData.size())
			return "";
		return std::string((char *)m_dpx_userdata.UserData.data());
	case eSBMFormatDescriptor:
		return CopyToStringN(m_dpx_sbmdata.SbmFormatDescriptor, 128);
	case eSBMetadata:
		if (!m_dpx_sbmdata.SbmData.size())
			return "";
		return std::string((char *)m_dpx_sbmdata.SbmData.data());
	}
	return "";
}

void HdrDpxFile::SetHeader(HdrDpxFieldsU32 field, uint32_t value)
{
	switch (field)
	{
	case eImageOffset:
		m_dpx_header.FileHeader.ImageOffset = value;
		break;
	case eFileSize:
		m_dpx_header.FileHeader.FileSize = value;
		break;
	case eGenericSize:
		m_dpx_header.FileHeader.GenericSize = value;
		break;
	case eIndustrySize:
		m_dpx_header.FileHeader.IndustrySize = value;
		break;
	case eUserDefinedHeaderLength:
		//m_dpx_header.FileHeader.UserSize = value;
		LOG_ERROR(eBadParameter, eWarning, "User data length can only be set by calling SetUserData() or SetHeader() using eUserDefinedData and a string");
		break;
	case eEncryptKey:
		m_dpx_header.FileHeader.EncryptKey = value;
		break;
	case eStandardsBasedMetadataOffset:
		m_dpx_header.FileHeader.StandardsBasedMetadataOffset = value;
		break;
	case ePixelsPerLine:
		m_dpx_header.ImageHeader.PixelsPerLine = value;
		break;
	case eLinesPerElement:
		m_dpx_header.ImageHeader.LinesPerElement = value;
		break;
	case eXOffset:
		m_dpx_header.SourceInfoHeader.XOffset = value;
		break;
	case eYOffset:
		m_dpx_header.SourceInfoHeader.YOffset = value;
		break;
	case eXOriginalSize:
		m_dpx_header.SourceInfoHeader.XOriginalSize = value;
		break;
	case eYOriginalSize:
		m_dpx_header.SourceInfoHeader.YOriginalSize = value;
		break;
	case eAspectRatioH:
		m_dpx_header.SourceInfoHeader.AspectRatio[0] = value;
		break;
	case eAspectRatioV:
		m_dpx_header.SourceInfoHeader.AspectRatio[1] = value;
		break;
	case eFramePosition:
		m_dpx_header.FilmHeader.FramePosition = value;
		break;
	case eSequenceLength:
		m_dpx_header.FilmHeader.SequenceLen = value;
		break;
	case eHeldCount:
		m_dpx_header.FilmHeader.HeldCount = value;
	case eSBMLength:
		LOG_ERROR(eBadParameter, eWarning, "Standards-based metadata length can only be set by calling SetStandardsBasedMetadata() or SetHeader() using eSBMetadata and a string");
		break;
	}
}

uint32_t HdrDpxFile::GetHeader(HdrDpxFieldsU32 field) const
{
	switch (field)
	{
	case eImageOffset:
		return(m_dpx_header.FileHeader.ImageOffset);
	case eFileSize:
		return(m_dpx_header.FileHeader.FileSize);
	case eGenericSize:
		return(m_dpx_header.FileHeader.GenericSize);
	case eIndustrySize:
		return(m_dpx_header.FileHeader.IndustrySize);
	case eUserDefinedHeaderLength:
		return(m_dpx_header.FileHeader.UserSize);
	case eEncryptKey:
		return(m_dpx_header.FileHeader.EncryptKey);
	case eStandardsBasedMetadataOffset:
		return(m_dpx_header.FileHeader.StandardsBasedMetadataOffset);
	case ePixelsPerLine:
		return(m_dpx_header.ImageHeader.PixelsPerLine);
	case eLinesPerElement:
		return(m_dpx_header.ImageHeader.LinesPerElement);
	case eXOffset:
		return(m_dpx_header.SourceInfoHeader.XOffset);
	case eYOffset:
		return(m_dpx_header.SourceInfoHeader.YOffset);
	case eXOriginalSize:
		return(m_dpx_header.SourceInfoHeader.XOriginalSize);
	case eYOriginalSize:
		return(m_dpx_header.SourceInfoHeader.YOriginalSize);
	case eAspectRatioH:
		return(m_dpx_header.SourceInfoHeader.AspectRatio[0]);
	case eAspectRatioV:
		return(m_dpx_header.SourceInfoHeader.AspectRatio[1]);
	case eFramePosition:
		return(m_dpx_header.FilmHeader.FramePosition);
	case eSequenceLength:
		return(m_dpx_header.FilmHeader.SequenceLen);
	case eHeldCount:
		return(m_dpx_header.FilmHeader.HeldCount);
	case eSBMLength:
		return(m_dpx_sbmdata.SbmLength);
	}
	return 0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsU16 field, uint16_t value)
{
	switch (field)
	{
	case eNumberElements:
		m_dpx_header.ImageHeader.NumberElements = value;
		break;
	case eBorderXL:
		m_dpx_header.SourceInfoHeader.Border[0] = value;
		break;
	case eBorderXR:
		m_dpx_header.SourceInfoHeader.Border[1] = value;
		break;
	case eBorderYT:
		m_dpx_header.SourceInfoHeader.Border[2] = value;
		break;
	case eBorderYB:
		m_dpx_header.SourceInfoHeader.Border[3] = value;
	}
}

uint16_t HdrDpxFile::GetHeader(HdrDpxFieldsU16 field) const
{
	switch (field)
	{
	case eNumberElements:
		return(m_dpx_header.ImageHeader.NumberElements);
	case eBorderXL:
		return(m_dpx_header.SourceInfoHeader.Border[0]);
	case eBorderXR:
		return(m_dpx_header.SourceInfoHeader.Border[1]);
	case eBorderYT:
		return(m_dpx_header.SourceInfoHeader.Border[2]);
	case eBorderYB:
		return(m_dpx_header.SourceInfoHeader.Border[3]);
	}
	return 0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsR32 field, float value)
{
	switch (field)
	{
	case eXCenter:
		m_dpx_header.SourceInfoHeader.XCenter = value;
		break;
	case eYCenter:
		m_dpx_header.SourceInfoHeader.YCenter = value;
		break;
	case eFilmFrameRate:
		m_dpx_header.FilmHeader.FrameRate = value;
		break;
	case eShutterAngle:
		m_dpx_header.FilmHeader.ShutterAngle = value;
		break;
	case eHorzSampleRate:
		m_dpx_header.TvHeader.HorzSampleRate = value;
		break;
	case eVertSampleRate:
		m_dpx_header.TvHeader.VertSampleRate = value;
		break;
	case eTvFrameRate:
		m_dpx_header.TvHeader.FrameRate = value;
		break;
	case eTimeOffset:
		m_dpx_header.TvHeader.TimeOffset = value;
		break;
	case eGamma:
		m_dpx_header.TvHeader.Gamma = value;
		break;
	case eBlackLevel:
		m_dpx_header.TvHeader.BlackLevel = value;
		break;
	case eBlackGain:
		m_dpx_header.TvHeader.BlackGain = value;
		break;
	case eBreakpoint:
		m_dpx_header.TvHeader.Breakpoint = value;
		break;
	case eWhiteLevel:
		m_dpx_header.TvHeader.WhiteLevel = value;
		break;
	case eIntegrationTimes:
		m_dpx_header.TvHeader.IntegrationTimes = value;
	}
}

float HdrDpxFile::GetHeader(HdrDpxFieldsR32 field) const
{
	switch (field)
	{
	case eXCenter:
		return(m_dpx_header.SourceInfoHeader.XCenter);
	case eYCenter:
		return(m_dpx_header.SourceInfoHeader.YCenter);
	case eFilmFrameRate:
		return(m_dpx_header.FilmHeader.FrameRate);
	case eShutterAngle:
		return(m_dpx_header.FilmHeader.ShutterAngle);
	case eHorzSampleRate:
		return(m_dpx_header.TvHeader.HorzSampleRate);
	case eVertSampleRate:
		return(m_dpx_header.TvHeader.VertSampleRate);
	case eTvFrameRate:
		return(m_dpx_header.TvHeader.FrameRate);
	case eTimeOffset:
		return(m_dpx_header.TvHeader.TimeOffset);
	case eGamma:
		return(m_dpx_header.TvHeader.Gamma);
	case eBlackLevel:
		return(m_dpx_header.TvHeader.BlackLevel);
	case eBlackGain:
		return(m_dpx_header.TvHeader.BlackGain);
	case eBreakpoint:
		return(m_dpx_header.TvHeader.Breakpoint);
	case eWhiteLevel:
		return(m_dpx_header.TvHeader.WhiteLevel);
	case eIntegrationTimes:
		return(m_dpx_header.TvHeader.IntegrationTimes);
	}
	return 0.0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsU8 field, uint8_t value)
{
	switch (field)
	{
	case eFieldNumber:
		m_dpx_header.TvHeader.FieldNumber = value;
	}
}

uint8_t HdrDpxFile::GetHeader(HdrDpxFieldsU8 field) const
{
	switch(field)
	{
	case eFieldNumber:
		return(m_dpx_header.TvHeader.FieldNumber);
	}
	return 0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsDittoKey field, HdrDpxDittoKey value)
{
	m_dpx_header.FileHeader.DittoKey = static_cast<uint32_t>(value);
}

HdrDpxDittoKey HdrDpxFile::GetHeader(HdrDpxFieldsDittoKey field) const
{
	return static_cast<HdrDpxDittoKey>(m_dpx_header.FileHeader.DittoKey);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsDatumMappingDirection field, HdrDpxDatumMappingDirection value)
{
	m_dpx_header.FileHeader.DatumMappingDirection = static_cast<uint8_t>(value);
}

HdrDpxDatumMappingDirection HdrDpxFile::GetHeader(HdrDpxFieldsDatumMappingDirection field) const
{
	return static_cast<HdrDpxDatumMappingDirection>(m_dpx_header.FileHeader.DatumMappingDirection);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsOrientation field, HdrDpxOrientation value)
{
	m_dpx_header.ImageHeader.Orientation = static_cast<uint16_t>(value);
}

HdrDpxOrientation HdrDpxFile::GetHeader(HdrDpxFieldsOrientation field) const
{
	return static_cast<HdrDpxOrientation>(m_dpx_header.ImageHeader.Orientation);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsInterlace field, HdrDpxInterlace value)
{
	m_dpx_header.TvHeader.Interlace = static_cast<uint8_t>(value);
}

HdrDpxInterlace HdrDpxFile::GetHeader(HdrDpxFieldsInterlace field) const
{
	return static_cast<HdrDpxInterlace>(m_dpx_header.TvHeader.Interlace);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsVideoSignal field, HdrDpxVideoSignal value)
{
	m_dpx_header.TvHeader.VideoSignal = static_cast<uint8_t>(value);
}

HdrDpxVideoSignal HdrDpxFile::GetHeader(HdrDpxFieldsVideoSignal field) const
{
	return static_cast<HdrDpxVideoSignal>(m_dpx_header.TvHeader.VideoSignal);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsVideoIdentificationCode field, HdrDpxVideoIdentificationCode value)
{
	m_dpx_header.TvHeader.VideoIdentificationCode = static_cast<uint8_t>(value);
}

HdrDpxVideoIdentificationCode HdrDpxFile::GetHeader(HdrDpxFieldsVideoIdentificationCode field) const
{
	return static_cast<HdrDpxVideoIdentificationCode>(m_dpx_header.TvHeader.VideoIdentificationCode);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsByteOrder field, HdrDpxByteOrder value)
{
	m_byteorder = value;
}

HdrDpxByteOrder HdrDpxFile::GetHeader(HdrDpxFieldsByteOrder field) const
{
	return m_byteorder;
}


void HdrDpxFile::SetUserData(std::string userid, std::vector<uint8_t> userdata)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	if (userdata.size() + 32 > 1000000)
	{
		LOG_ERROR(eBadParameter, eWarning, "User data length limited to a max of 1000000 bytes");
		return;
	}
	CopyStringN(m_dpx_userdata.UserIdentification, userid, 32);
	m_dpx_userdata.UserData = userdata;
	m_dpx_header.FileHeader.UserSize = static_cast<DWORD>(userdata.size() + 32);
}

bool HdrDpxFile::GetUserData(std::string &userid, std::vector<uint8_t> &userdata) const
{
	if (m_dpx_header.FileHeader.UserSize == 0)		// nothing to do
		return false;
	userid = CopyToStringN(m_dpx_userdata.UserIdentification, 32);
	userdata = m_dpx_userdata.UserData;
	return true;
}

void HdrDpxFile::SetStandardsBasedMetadata(std::string sbm_descriptor, std::vector<uint8_t> sbmdata)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	if (sbm_descriptor.compare("ST336") == 0 && sbm_descriptor.compare("Reg-XML") == 0 && sbm_descriptor.compare("XMP") == 0)
	{
		LOG_ERROR(eBadParameter, eWarning, "Setting standards-based metadata descriptor to nonstandard type");
	}
	CopyStringN(m_dpx_sbmdata.SbmFormatDescriptor, sbm_descriptor, 128);
	m_dpx_sbmdata.SbmData = sbmdata;
	m_dpx_sbmdata.SbmLength = static_cast<uint32_t>(sbmdata.size());
}

bool HdrDpxFile::GetStandardsBasedMetadata(std::string &sbm_descriptor, std::vector<uint8_t> &sbmdata) const
{
	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset == UINT32_MAX)		// nothing to do
		return false;
	sbm_descriptor = CopyToStringN(m_dpx_sbmdata.SbmFormatDescriptor, 128);
	sbmdata = m_dpx_sbmdata.SbmData;
	return true;
}

std::list<std::string> HdrDpxFile::GetWarningList(void)
{
	return m_warn_messages;
}

bool HdrDpxFile::IsOk(void)  const
{
	return m_err.GetWorstSeverity() != eFatal;
}

int HdrDpxFile::GetNumErrors(void)  const
{
	return m_err.GetNumErrors();
}

void HdrDpxFile::GetError(int err_idx, ErrorCode &errcode, ErrorSeverity &severity, std::string &errmsg)
{
	return m_err.GetError(err_idx, errcode, severity, errmsg);
}

void HdrDpxFile::ClearErrors()
{
	m_err.Clear();
}

std::vector<uint8_t> HdrDpxFile::GetIEIndexList() const
{
	std::vector<uint8_t> ielist;
	for (int ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
			ielist.push_back(ie_idx);
	}
	return ielist;
}


void HdrDpxFile::CopyHeaderFrom(const HdrDpxFile &src)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Can't copy header to locked file\n");
		return;
	}
	// Copy header, userdata, sbmdata from other file
	m_dpx_header = src.m_dpx_header;
	m_dpx_sbmdata = src.m_dpx_sbmdata;
	m_dpx_userdata = src.m_dpx_userdata;
}
