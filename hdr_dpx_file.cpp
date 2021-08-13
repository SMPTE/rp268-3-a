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

#define WARN_FOR_ALL_FF_STRINGS  1

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
#define LOG_ERROR(number, severity, msg)  \
	    m_err.LogError(number, severity, ((severity == eInformational) ? "INFO #" : ((severity == eWarning) ? "WARNING #" : "ERROR #")) \
			   + std::to_string(static_cast<int>(number)) + " in " \
               + "function " + __FUNCTION__ \
               + ", file " + __FILE__ \
               + ", line " + std::to_string(__LINE__) + ":\n" \
               + msg + "\n")


using namespace Dpx;


bool Dpx::CopyStringN(char *dest, std::string src, unsigned int max_length)
{
	for (unsigned int idx = 0; idx < max_length; ++idx)
	{
		if (idx >= src.length())
			*(dest++) = '\0';
		else
			*(dest++) = src[idx];
	}
	return (src.length() > max_length);  // return true if the string is longer than the field
}

std::string Dpx::CopyToStringN(const char *src, unsigned int max_length) 
{
	std::string s("");

	for (unsigned int idx = 0; idx < max_length; ++idx)
	{
		if (*src == '\0')
			return s;
		else
			s.push_back(*(src++));
	}
	return s;
}


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
	ClearHeader();
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
	if (m_file_stream.eof() || m_dpx_header.FileHeader.Magic != 0x53445058)
	{
		LOG_ERROR(eFileOpenError, eFatal, "Header is not valid\n");
		return;
	}

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
	for (uint8_t ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_dpx_header.ImageHeader.ImageElement[ie_idx].DataOffset != UNDEFINED_U32)
		{
			m_IE[ie_idx].Initialize(ie_idx, &m_file_stream, &m_dpx_header, &m_filemap);
			m_IE[ie_idx].OpenForReading(swapped);
		}
		else
			m_IE[ie_idx].m_isinitialized = false;
	}
	m_file_is_hdr_version = (static_cast<bool>(!strcmp(m_dpx_header.FileHeader.Version, "V2.0HDR")));

	ReadUserData();
	if (m_file_is_hdr_version)
		ReadSbmData();

	m_open_for_write = false;
	m_open_for_read = true;
	m_is_header_locked = true;
}


void HdrDpxFile::ReadUserData()
{
	if (m_dpx_header.FileHeader.UserSize == 0 || m_dpx_header.FileHeader.UserSize == UNDEFINED_U32)
		return;   // Nothing to do, no user data

	for (uint32_t i = 0; i < 32; ++i)
		m_file_stream.get(m_dpx_userdata.UserIdentification[i]);

	std::streampos cur_ptr = m_file_stream.tellg();
	m_file_stream.seekg(0, std::ios::end);
	if (static_cast<std::streamoff>(cur_ptr) + static_cast<std::streamoff>(m_dpx_header.FileHeader.UserSize) > 
			m_file_stream.tellg())
	{
		LOG_ERROR(eFileReadError, eWarning, "User data size is larger than file size\n");
		return;
	}
	m_file_stream.seekg(cur_ptr);

	m_dpx_userdata.UserData.clear();
	for (uint32_t i = 0; i < m_dpx_header.FileHeader.UserSize - 32 && !m_file_stream.bad(); ++i)
		m_dpx_userdata.UserData.push_back(static_cast<unsigned char>(m_file_stream.get()));

	if (m_file_stream.bad() || m_file_stream.eof())
	{
		LOG_ERROR(eFileReadError, eWarning, "Error attempting to read user data\n");
		return;
	}
}


void HdrDpxFile::ReadSbmData()
{
	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset == UNDEFINED_U32)
		return;		// Nothing to do, no standards-based metadata

	m_file_stream.seekg(m_dpx_header.FileHeader.StandardsBasedMetadataOffset, m_file_stream.beg);

	for (uint32_t i = 0; i < 128; ++i)
		m_file_stream.get(m_dpx_sbmdata.SbmFormatDescriptor[i]);

	unsigned char sizebytes[4];
	for (uint32_t i = 0; i < 4; ++i)
		sizebytes[i] = static_cast<unsigned char>(m_file_stream.get());

	if (m_byteorder == eLSBF)
		m_dpx_sbmdata.SbmLength = (sizebytes[3] << 24) | (sizebytes[2] << 16) | (sizebytes[1] << 8) | sizebytes[0];
	else
		m_dpx_sbmdata.SbmLength = (sizebytes[0] << 24) | (sizebytes[1] << 16) | (sizebytes[2] << 8) | sizebytes[3];

	if (m_file_stream.bad() || m_file_stream.eof())
	{
		LOG_ERROR(eFileReadError, eWarning, "Error attempting to read standards-based data\n");
		return;
	}

	for (uint32_t i = 0; i < m_dpx_sbmdata.SbmLength; ++i)
		m_dpx_sbmdata.SbmData.push_back(static_cast<unsigned char>(m_file_stream.get()));

	if (m_file_stream.bad() || m_file_stream.eof())
	{
		LOG_ERROR(eFileReadError, eWarning, "Error attempting to read standards-based data\n");
		return;
	}
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
	return (GetHeader(eVersion) == eDPX_2_0_HDR);
}

void HdrDpxFile::ComputeOffsets()
{
	uint32_t data_offset;
	uint32_t min_offset = UNDEFINED_U32;

	m_filemap.Reset();

	// Fill in what is specified
	m_filemap.AddRegion(0, sizeof(HDRDPXFILEFORMAT), 100);

	if (m_dpx_header.FileHeader.UserSize == UNDEFINED_U32)   // Undefined value is not permitted in 268-2
		m_dpx_header.FileHeader.UserSize = 0;

	if (m_dpx_header.FileHeader.UserSize != 0)
		m_filemap.AddRegion(sizeof(HDRDPXFILEFORMAT), ((sizeof(HDRDPXFILEFORMAT) + m_dpx_header.FileHeader.UserSize + 3) >> 2) << 2, 101);

	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UNDEFINED_U32 && m_dpx_header.FileHeader.StandardsBasedMetadataOffset != eSBMAutoLocate)
		m_filemap.AddRegion(m_dpx_header.FileHeader.StandardsBasedMetadataOffset, m_dpx_header.FileHeader.StandardsBasedMetadataOffset + m_dpx_sbmdata.SbmLength + 132, 102);

	// Add IEs with data offsets
	for (uint8_t ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			m_IE[ie_idx].ComputeWidthAndHeight();
			data_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
			if (data_offset != UNDEFINED_U32)
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
	for (uint8_t ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			data_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
			if (data_offset == UNDEFINED_U32 && m_IE[ie_idx].GetHeader(eEncoding) != eEncodingRLE)
				m_IE[ie_idx].SetHeader(eOffsetToData, m_filemap.FindEmptySpace(m_IE[ie_idx].GetImageDataSizeInBytes(), ie_idx));
		}
	}

	// Lastly add first variable-size IE
	uint8_t first_ie = 1;
	for (uint8_t ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			data_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
			if (data_offset == UNDEFINED_U32 && m_IE[ie_idx].GetHeader(eEncoding) == eEncodingRLE)
			{
				uint32_t est_size = CEIL_DWORD(static_cast<uint32_t>((1.0 + RLE_MARGIN) * m_IE[ie_idx].GetImageDataSizeInBytes()));

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
	for (uint8_t ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized && min_offset > m_IE[ie_idx].GetHeader(eOffsetToData))
			min_offset = m_IE[ie_idx].GetHeader(eOffsetToData);
	}
	if (m_dpx_header.FileHeader.ImageOffset == UNDEFINED_U32)
		m_dpx_header.FileHeader.ImageOffset = min_offset;
	if (m_dpx_header.FileHeader.ImageOffset != min_offset)
		LOG_ERROR(eBadParameter, eWarning, "Image offset in main header does not match smallest image element offset");

	if (m_filemap.CheckCollisions())
		LOG_ERROR(eBadParameter, eWarning, "Image map has potentially overlapping regions");
}


void HdrDpxFile::FillCoreFields()
{
	uint8_t number_of_ie = 0, first_ie_idx = UINT8_MAX;

	if (m_dpx_header.FileHeader.Magic != 0x53445058 && m_dpx_header.FileHeader.Magic != UNDEFINED_U32)
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
	m_dpx_header.FileHeader.GenericSize = 1664;
	m_dpx_header.FileHeader.IndustrySize = 384;

	for (uint8_t ie_idx = 0; ie_idx < 8; ++ie_idx)
	{
		if (m_IE[ie_idx].m_isinitialized)
		{
			number_of_ie++;
			if(first_ie_idx == UINT8_MAX)
				first_ie_idx = ie_idx;
		}
	}
	if(first_ie_idx == UINT8_MAX)
		LOG_ERROR(eBadParameter, eFatal, "No initialized image elements found");

	if (m_dpx_header.ImageHeader.Orientation == UNDEFINED_U16)
	{
		LOG_ERROR(eMissingCoreField, eWarning, "Image orientation not specified, assuming left-to-right, top-to-bottom");
		m_dpx_header.ImageHeader.Orientation = 0;
	}
	if (m_dpx_header.ImageHeader.NumberElements == 0 || m_dpx_header.ImageHeader.NumberElements == UNDEFINED_U16)
		m_dpx_header.ImageHeader.NumberElements = number_of_ie;

	if (m_dpx_header.ImageHeader.PixelsPerLine == UNDEFINED_U32)
	{
		LOG_ERROR(eFileWriteError, eFatal, "Pixels per line must be specified");
		return;
	}
	if (m_dpx_header.ImageHeader.LinesPerElement == UNDEFINED_U32)
	{
		LOG_ERROR(eFileWriteError, eFatal, "Lines per element must be specified");
		return;
	}
	for (uint8_t ie_idx = 0; ie_idx < number_of_ie; ++ie_idx)
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
			ClearHeader(ie_idx);
		}
		if (m_IE[ie_idx].GetHeader(eTransferCharacteristic) == eTransferUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Transfer characteristic for image element " + std::to_string(ie_idx + 1) + " not specified");
		}
		if (m_IE[ie_idx].GetHeader(eColorimetricSpecification) == eColorimetricUndefined)
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
		if (m_IE[ie_idx].GetHeader(eEndOfLinePadding) == UNDEFINED_U32)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "End of line padding not specified for image element " + std::to_string(ie_idx + 1) + ", assuming no padding");
			m_IE[ie_idx].SetHeader(eEndOfLinePadding, 0);
		}
		if (m_IE[ie_idx].GetHeader(eEndOfImagePadding) == UNDEFINED_U32)
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
	m_is_header_locked = true;
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

		if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UNDEFINED_U32)
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
		m_is_header_locked = false;
	}
}

static std::string u32_to_hex(uint32_t value)
{
	std::stringstream sstream;
	sstream << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
	return sstream.str();
}

static std::string u8_to_hex(uint8_t value)
{
	std::stringstream sstream;
	sstream << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(value);
	return sstream.str();
}

#define PRINT_FIELD_U32(n,v) if((v)!=UNDEFINED_U32) { header += n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_FIELD_U32_HEX(n,v) if((v)!=UNDEFINED_U32) { header += n + "\t" + u32_to_hex(v) + "\n"; }
#define PRINT_RO_FIELD_U32(n,v) if((v)!=UNDEFINED_U32) { header += "// " + n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_RO_FIELD_U32_HEX(n,v) if((v)!=UNDEFINED_U32) {  header += "// " + n + "\t" + u32_to_hex(v) + "\n";  }
#define PRINT_FIELD_U8(n,v) if((v)!=UNDEFINED_U8) {  header += n + "\t" + std::to_string(v) + "\n";  }
#define PRINT_FIELD_CS(n,v) if((v)!=0 && (v)!=0xf) {  header += n + "\t" + std::to_string(v) + "\n";  }
#define PRINT_RO_FIELD_U8(n,v) if((v)!=UNDEFINED_U8) { header += "// " + n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_FIELD_U16(n,v) if((v)!=UNDEFINED_U16) {  header += n + "\t" + std::to_string(v) + "\n";  }
#define PRINT_FIELD_R32(n,v) if(memcmp(&v, &undefined_4bytes, 4)!=0) { header += n + "\t" + std::to_string(v) + "\n"; }
#define PRINT_FIELD_ASCII(n,v,l) if((v[0])!='\0') { char s[l+1]; s[l] = '\0'; memcpy(s, v, l); header += n + "\t" + std::string(s) + "\n";  }
std::string HdrDpxFile::DumpHeader() const
{
	static const uint32_t undefined_4bytes = UNDEFINED_U32;  /* 4-byte block 0xffffffff used to check if floats are undefined */
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
		fld_name = "Chroma_Subsampling_" + std::to_string(ie_idx + 1); PRINT_FIELD_CS(fld_name, 0xf & (m_dpx_header.ImageHeader.ChromaSubsampling >> (ie_idx * 4)));
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

	if (m_dpx_header.FileHeader.UserSize > 0 && m_dpx_header.FileHeader.UserSize != UNDEFINED_U32)
	{
		header += "\n// User data header\n";
		PRINT_FIELD_ASCII(std::string("User_Identification"), m_dpx_userdata.UserIdentification, 32);
		// Read user data and print
		std::string userid;
		std::vector<uint8_t> userdata;
		if (m_ud_dump_format == eDumpFormatDefault || m_ud_dump_format == eDumpFormatAsBytes)
		{
			GetUserData(userid, userdata);
			header += "User data (hex bytes):\n";
			for (unsigned int i = 0; i < userdata.size(); ++i)
			{
				header += u8_to_hex(userdata[i]) + " ";
				if ((i & 0xf) == 0xf)
					header += "\n";
			}
			header += "\n";
		}
		else  // As string:
		{
			header += GetHeader(eUserDefinedData) + "\n";
		}
	}

	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UNDEFINED_U32)
	{
		header += "\n// Standards-based metadata header\n";
		PRINT_FIELD_ASCII(std::string("Sbm_Descriptor"), m_dpx_sbmdata.SbmFormatDescriptor, 128);
		PRINT_FIELD_U32(std::string("Sbm_Length"), m_dpx_sbmdata.SbmLength);
		// Read standards-based metadata & print
		std::string sbmdesc;
		std::vector<uint8_t> sbmdata;
		if (GetStandardsBasedMetadata(sbmdesc, sbmdata))
		{
			// Note that ST 336 (KLV) is always dumped as hex data
			if (sbmdesc.compare("ST336") == 0 || m_sbm_dump_format == eDumpFormatAsBytes)  // print as hex bytes
			{
				header += "Hex bytes:\n";
				for (unsigned int i = 0; i < sbmdata.size(); ++i)
				{
					header += u8_to_hex(sbmdata[i]) + " ";
					if ((i & 0xf) == 0xf)
						header += "\n";
				}
				header += "\n";
			}
			else   // Print as string
			{
				header += "Standards-based-metadata:\n";
				header += GetHeader(Dpx::eSBMetadata);
				// Alternatively:
				//char *c = (char *)sbmdata.data();
				//std::cout << std::string(c);
			}

			std::cout << "\n";
		}
	}
	return header;
}


static bool IsStringAllFs(std::string s, const unsigned int max_size)
{
	if (s.length() < max_size)
		return false;
	for (unsigned int i = 0; i < max_size; ++i)
		if (s[i] != (char)UINT8_MAX)
			return false;
	return true;
}

static int days_in_month[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static bool ValidateTimeDate(std::string s, std::string &errmsg)
{
	int month = 1, day, hour, minute, second;

	errmsg = "";
	if (s.length() == 0 || IsStringAllFs(s, TIMEDATE_SIZE))	// Undefined Time & Date string is valid
		return false;
	// yyyy:MM:dd:hh:mm:ssLTZ
	if (s[4] != ':' || s[7] != ':' || s[10] != ':' || s[13] != ':' || s[16] != ':')
		errmsg += "Time & Date field is not properly delimited by colons; ";
	// Validate that the year contains only numerals:
	if (!isdigit(s[0]) || !isdigit(s[1]) || !isdigit(s[2]) || !isdigit(s[3]))
		errmsg += "yyyy field is not numeric; ";
	if (!isdigit(s[5]) || !isdigit(s[6]))
		errmsg += "MM field is not numeric; ";
	else
	{
		month = std::stoi(s.substr(5, 2));
		if (month < 1 || month > 12)
			errmsg += "MM field is not between 1 and 12; ";
	}
	if (!isdigit(s[8] || !isdigit(s[9])))
		errmsg += "dd field is not numeric; ";
	else
	{
		day = std::stoi(s.substr(8, 2));
		if (day < 1 || day > days_in_month[month - 1])
			errmsg += "dd field is not valid; ";
	}
	if (!isdigit(s[11]) || !isdigit(s[12]))
		errmsg += "hh field is not numeric; ";
	else
	{
		hour = std::stoi(s.substr(11, 2));
		if (hour > 23)
			errmsg += "hh field is > 23; ";
	}
	if (!isdigit(s[14]) || !isdigit(s[15]))
		errmsg += "mm field is not numeric; ";
	else
	{
		minute = std::stoi(s.substr(14, 2));
		if (minute > 60)
			errmsg += "mm field is > 60";
	}
	if (!isdigit(s[17]) || !isdigit(s[18]))
		errmsg += "ss field is not numeric; ";
	else
	{
		second = std::stoi(s.substr(14, 2));
		if (second > 60)
			errmsg += "ss field is > 60";
	}
	return (errmsg.length() > 0);
}


bool HdrDpxFile::Validate()
{
	std::string errmsg;

	if(GetHeader(eDittoKey) > eDittoKeySame && GetHeader(eDittoKey) != eDittoKeyUndefined)
		LOG_ERROR(eValidationError, eWarning, "Ditto Key field has invalid value");
	if(GetHeader(eGenericSectionHeaderLength) != 1664)
		LOG_ERROR(eValidationError, eWarning, "Generic Size field has invalid value");
	if(GetHeader(eIndustrySpecificHeaderLength) != 384)
		LOG_ERROR(eValidationError, eWarning, "Industry Size field has invalid value");
	if(GetHeader(eUserDefinedHeaderLength) == UNDEFINED_U32)
		LOG_ERROR(eValidationError, eWarning, "User Data Size field shall not be undefined");	
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eImageFileName), 100))
		LOG_ERROR(eValidationError, eWarning, "File name field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eCreationDateTime), 24))
		LOG_ERROR(eValidationError, eWarning, "Time & date field is all 0xff bytes; undefined strings should use single null character");
	else if (ValidateTimeDate(GetHeader(eCreationDateTime), errmsg))
		LOG_ERROR(eValidationError, eWarning, "Creator time and date field not properly constructed: " + errmsg);
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eCreator), 100))
		LOG_ERROR(eValidationError, eWarning, "Creator field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eProjectName), 200))
		LOG_ERROR(eValidationError, eWarning, "Project field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eRightToUseOrCopyright), 200))
		LOG_ERROR(eValidationError, eWarning, "Copyright field is all 0xff bytes; undefined strings should use single null character");
	if(GetHeader(eDatumMappingDirection) > eDatumMappingDirectionL2R)
		LOG_ERROR(eValidationError, eWarning, "Datum mapping direction field has invalid value");
	// TBD: Verify no collisions in file map

	if(GetHeader(eOrientation) > eOrientationB2T_R2L && GetHeader(eOrientation) != eOrientationUndefined)
		LOG_ERROR(eValidationError, eWarning, "Orientation field has invalid value");
	if (GetHeader(eOrientation) != eOrientationL2R_T2B)
		LOG_ERROR(eValidationError, eWarning, "Some readers might not interpret a non-core Orientation field value");
	if(GetHeader(eNumberOfImageElements) < 1 || GetHeader(eNumberOfImageElements) > 8)
		LOG_ERROR(eValidationError, eWarning, "Number of image elements field has invalid value");
	if(GetHeader(ePixelsPerLine) == 0)
		LOG_ERROR(eValidationError, eWarning, "Pixels per line field is equal to 0");
	if(GetHeader(eLinesPerImageElement) == 0)
		LOG_ERROR(eValidationError, eWarning, "Lines per image element field is equal to 0");

	uint8_t num_ies = 0;
	for (auto ie_idx : GetIEIndexList() )
	{
		float range_lo, range_hi;
		if (m_IE[ie_idx].GetHeader(eDescriptor) == eDescUndefined)   // indicates the IE is not present
			continue;
		num_ies++;
		if(m_IE[ie_idx].GetHeader(eDataSign) > eDataSignSigned)
			LOG_ERROR(eValidationError, eWarning, "Data sign field has invalid value");
		if (m_IE[ie_idx].GetHeader(eDataSign) != eDataSignUnsigned)
			LOG_ERROR(eValidationError, eWarning, "Some readers might not interpret a signed data sign field value");
		switch (m_IE[ie_idx].GetHeader(eBitDepth))
		{
		case eBitDepth1:
			range_lo = 0;
			range_hi = 1; 
			break;
		case eBitDepth8:
			range_lo = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 0.0f : -128.0f;
			range_hi = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 255.0f : 127.0f;
			break;
		case eBitDepth10:
			range_lo = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 0.0f : -512.0f;
			range_hi = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 1023.0f : 511.0f;
			break;
		case eBitDepth12:
			range_lo = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 0.0f : -2048.0f;
			range_hi = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 4191.0f : 2047.0f;
			break;
		case eBitDepth16:
			range_lo = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 0.0f : -32768.0f;
			range_hi = (m_IE[ie_idx].GetHeader(eDataSign) == eDataSignUnsigned) ? 65535.0f : 32767.0f;
			break;
		case eBitDepthR32:
		case eBitDepthR64:
			range_lo = -INFINITY;
			range_hi = INFINITY;
			break;
		default:
			range_lo = -INFINITY;
			range_hi = INFINITY;
			LOG_ERROR(eValidationError, eWarning, "Invalid bit depth field");
		}
		if (m_IE[ie_idx].GetHeader(eReferenceHighDataCode) < range_lo || m_IE[ie_idx].GetHeader(eReferenceHighDataCode) > range_hi)
			LOG_ERROR(eValidationError, eWarning, "Range high code value is not reprentable within specified bit depth");
		if (m_IE[ie_idx].GetHeader(eReferenceLowDataCode) < range_lo || m_IE[ie_idx].GetHeader(eReferenceLowDataCode) > range_hi)
			LOG_ERROR(eValidationError, eWarning, "Range low code value is not reprentable within specified bit depth");

		if (!(m_IE[ie_idx].GetHeader(eDescriptor) <= eDescCr) &&
			!(m_IE[ie_idx].GetHeader(eDescriptor) >= eDescRGB_268_1 && m_IE[ie_idx].GetHeader(eDescriptor) <= eDescABGR) &&
			!(m_IE[ie_idx].GetHeader(eDescriptor) >= eDescCbYCrY && m_IE[ie_idx].GetHeader(eDescriptor) <= eDescCYAYA) &&
			!(m_IE[ie_idx].GetHeader(eDescriptor) >= eDescGeneric2 && m_IE[ie_idx].GetHeader(eDescriptor) <= eDescGeneric8))
			LOG_ERROR(eValidationError, eFatal, "Descriptor core field is invalid\n");
		if (m_IE[ie_idx].GetHeader(eTransferCharacteristic) > eTransferIEC_61966_2_1)
			LOG_ERROR(eValidationError, eWarning, "Transfer Characteristic core field is invalid\n");
		if (m_IE[ie_idx].GetHeader(eColorimetricSpecification) > eColorimetricST_2065_1_ACES)
			LOG_ERROR(eValidationError, eWarning, "Colormetric Specification core field is invalid\n");
		if (m_IE[ie_idx].GetHeader(eBitDepth) == eBitDepth10 || m_IE[ie_idx].GetHeader(eBitDepth) == eBitDepth12)
		{
			if (m_IE[ie_idx].GetHeader(ePacking) > ePackingFilledMethodB)
				LOG_ERROR(eValidationError, eWarning, "Packing core field is invalid\n");
		}
		else
		{
			if (m_IE[ie_idx].GetHeader(ePacking) != ePackingPacked)
				LOG_ERROR(eValidationError, eWarning, "Packing core field should be 0 (packed) for bit depths other than 10 or 12\n");
		}
		if (m_IE[ie_idx].GetHeader(eEncoding) > eEncodingRLE)
			LOG_ERROR(eValidationError, eWarning, "Encoding core field is invalid\n");
		if (m_IE[ie_idx].GetHeader(eOffsetToData) != UNDEFINED_U32 && (m_IE[ie_idx].GetHeader(eOffsetToData) & 3))
			LOG_ERROR(eValidationError, eWarning, "Offset to data is required to be multiple of 4\n");
		if (m_IE[ie_idx].GetHeader(eEndOfLinePadding) & 3)
			LOG_ERROR(eValidationError, eWarning, "End of line padding is required to be multiple of 4\n");
		if ((m_IE[ie_idx].GetHeader(eEndOfImagePadding) & 3) && m_IE[ie_idx].GetHeader(eEndOfImagePadding) != UNDEFINED_U32)
			LOG_ERROR(eValidationError, eWarning, "End of image padding is required to be multiple of 4\n");
		if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(m_IE[ie_idx].GetHeader(eDescriptionOfImageElement), 32))
			LOG_ERROR(eValidationError, eWarning, "Description of IE field is all 0xff bytes; undefined strings should use single null character");
		if (m_IE[ie_idx].GetHeader(eColorDifferenceSiting) > eSitingInterstitialHInterstitialV && m_IE[ie_idx].GetHeader(eColorDifferenceSiting) != eSitingUndefined)
			LOG_ERROR(eValidationError, eWarning, "Color difference siting field has invalid value\n");
	}
	if(num_ies != GetHeader(eNumberOfImageElements))
		LOG_ERROR(eValidationError, eWarning, "Number of image elements present does not match number of image elements header field");

	// No validation for X/Y offset, center, original size
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eSourceImageFileName), 100))
		LOG_ERROR(eValidationError, eWarning, "Source filename field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eSourceImageDateTime), 24))
		LOG_ERROR(eValidationError, eWarning, "Source time & date field is all 0xff bytes; undefined strings should use single null character");
	else if (ValidateTimeDate(GetHeader(eSourceImageDateTime), errmsg))
		LOG_ERROR(eValidationError, eWarning, "Source time and date field not properly constructed: " + errmsg);
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eInputDeviceName), 32))
		LOG_ERROR(eValidationError, eWarning, "Input device name field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eInputDeviceSN), 32))
		LOG_ERROR(eValidationError, eWarning, "Input device SN field is all 0xff bytes; undefined strings should use single null character");
	if (GetHeader(eBorderValidityXL) > GetHeader(ePixelsPerLine) && GetHeader(eBorderValidityXL) != UNDEFINED_U16)
		LOG_ERROR(eValidationError, eWarning, "Border validity XL field is larger than image width");
	if (GetHeader(eBorderValidityXR) > GetHeader(ePixelsPerLine) && GetHeader(eBorderValidityXR) != UNDEFINED_U16)
		LOG_ERROR(eValidationError, eWarning, "Border validity XR field is larger than image width");
	if (GetHeader(eBorderValidityYT) > GetHeader(eLinesPerImageElement) && GetHeader(eBorderValidityYT) != UNDEFINED_U16)
		LOG_ERROR(eValidationError, eWarning, "Border validity YT field is larger than image height");
	if (GetHeader(eBorderValidityYB) > GetHeader(eLinesPerImageElement) && GetHeader(eBorderValidityYB) != UNDEFINED_U16)
		LOG_ERROR(eValidationError, eWarning, "Border validity YB field is larger than image height");
	// No validation for pixel aspect ratio or X/Y scanned size

	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eFilmMfgIdCode), 2))
		LOG_ERROR(eValidationError, eWarning, "Film Mfg ID code field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eFilmType), 2))
		LOG_ERROR(eValidationError, eWarning, "Film type field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eOffsetInPerfs), 2))
		LOG_ERROR(eValidationError, eWarning, "Offset in perfs field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(ePrefix), 6))
		LOG_ERROR(eValidationError, eWarning, "Prefix field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eCount), 4))
		LOG_ERROR(eValidationError, eWarning, "Count field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eFormat), 32))
		LOG_ERROR(eValidationError, eWarning, "Format field is all 0xff bytes; undefined strings should use single null character");
	if (GetHeader(eFramePositionInSequence) != UNDEFINED_U32 && GetHeader(eSequenceLength) != UNDEFINED_U32 &&
		GetHeader(eFramePositionInSequence) > GetHeader(eSequenceLength))
		LOG_ERROR(eValidationError, eWarning, "Frame position field is larger than sequence length field");
	// No validation for Held count, frame rate of original, shutter angle
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eFrameIdentification), 32))
		LOG_ERROR(eValidationError, eWarning, "Frame identification field is all 0xff bytes; undefined strings should use single null character");
	if (WARN_FOR_ALL_FF_STRINGS && IsStringAllFs(GetHeader(eSlateInformation), 100))
		LOG_ERROR(eValidationError, eWarning, "Slate information field is all 0xff bytes; undefined strings should use single null character");

	// No validation for SMPTE time code or user bits
	if(GetHeader(eInterlace) != eInterlaceUndefined && GetHeader(eInterlace) > eInterlace2_1)
		LOG_ERROR(eValidationError, eWarning, "Interlace field contains unknown value");
	if(GetHeader(eFieldNumber) != UNDEFINED_U8 && GetHeader(eFieldNumber) > 12)
		LOG_ERROR(eValidationError, eWarning, "Field number field should not exceed 12 (see definition in ST 268-1)");
	if(GetHeader(eVideoSignalStandard) > eVideoSignalSECAM &&
		!(GetHeader(eVideoSignalStandard) >= eVideoSignalBT_601_525_4x3 || GetHeader(eVideoSignalStandard) <= eVideoSignalBT_601_625_4x3) &&
		!(GetHeader(eVideoSignalStandard) >= eVideoSignalBT_601_525_16x9 || GetHeader(eVideoSignalStandard) <= eVideoSignalBT_601_625_16x9) &&
		!(GetHeader(eVideoSignalStandard) >= eVideoSignalYCbCr_int_1050ln_16x9 || GetHeader(eVideoSignalStandard) <= eVideoSignalYCbCr_int_1125ln_16x9_ST_240) &&
		!(GetHeader(eVideoSignalStandard) >= eVideoSignalYCbCr_prog_525ln_16x9 || GetHeader(eVideoSignalStandard) <= eVideoSignalYCbCr_prog_1125ln_16x9_ST_274) &&
		GetHeader(eVideoSignalStandard) != eVideoSignalCTA_VIC &&
		GetHeader(eVideoSignalStandard) != eVideoSignalUndefined)
		LOG_ERROR(eValidationError, eWarning, "Video signal standard field does not contain recognized value");
	// No validation for horiz sample rate, vert sample rate, temp samle rate, time offset, gamma, black code, black gain, breakpoint, white code value, integration time

	if(GetHeader(eVideoSignalStandard) != eVideoSignalCTA_VIC && GetHeader(eVideoIdentificationCode) != eVIC_undefined)
		LOG_ERROR(eValidationError, eWarning, "Video signal standard field does not indicate CTA VIC but CTA VIC is indicated");
	if(GetHeader(eVideoSignalStandard) == eVideoSignalCTA_VIC && GetHeader(eVideoIdentificationCode) == eVIC_undefined)
		LOG_ERROR(eValidationError, eWarning, "Video signal standard field indicates CTA VIC but no CTA VIC value is indicated");

	if(GetHeader(eVideoIdentificationCode) != eVIC_undefined && 
		!(GetHeader(eVideoIdentificationCode) <= eVIC_2160p2x100) &&
		!(GetHeader(eVideoIdentificationCode) >= eVIC_2160p2x120 && GetHeader(eVideoIdentificationCode) <= eVIC_4096x2160p120))
		LOG_ERROR(eValidationError, eWarning, "Video identification code (VIC) field has unrecognized value");

	
	if (m_filemap.CheckCollisions())
		LOG_ERROR(eValidationError, eWarning, "Image map has potentially overlapping regions");
	//ComputeOffsets();   // Checks file map

	// Do we need any other validation warnings/info?


	return m_err.GetWorstSeverity() != eInformational;
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
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eImageFileName:
		if(CopyStringN(m_dpx_header.FileHeader.FileName, value, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified file name (" + value + ") exceeds header field size\n");
		break;
	case eCreationDateTime:
		if (CopyStringN(m_dpx_header.FileHeader.TimeDate, value, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified time & date (" + value + ") exceeds header field size\n");
		break;
	case eCreator:
		if (CopyStringN(m_dpx_header.FileHeader.Creator, value, CREATOR_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified creator (" + value + ") exceeds header field size\n");
		break;
	case eProjectName:
		if (CopyStringN(m_dpx_header.FileHeader.Project, value, PROJECT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified project (" + value + ") exceeds header field size\n");
		break;
	case eRightToUseOrCopyright:
		if (CopyStringN(m_dpx_header.FileHeader.Copyright, value, COPYRIGHT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified copyright (" + value + ") exceeds header field size\n");
		break;
	case eSourceImageFileName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceFileName, value, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source file name (" + value + ") exceeds header field size\n");
		break;
	case eSourceImageDateTime:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceTimeDate, value, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source time & date (" + value + ") exceeds header field size\n");
		break;
	case eInputDeviceName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputName, value, INPUTNAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input name (" + value + ") exceeds header field size\n");
		break;
	case eInputDeviceSN:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputSN, value, INPUTSN_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input SN (" + value + ") exceeds header field size\n");
		break;
	case eFilmMfgIdCode:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmMfgId, value, 2))
			m_warn_messages.push_back("SetHeader(): Specified film mfg id (" + value + ") exceeds header field size\n");
		break;
	case eFilmType:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmType, value, 2))
			m_warn_messages.push_back("SetHeader(): Specified film type (" + value + ") exceeds header field size\n");
		break;
	case eOffsetInPerfs:
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
	case eFrameIdentification:
		if (CopyStringN(m_dpx_header.FilmHeader.FrameId, value, 32))
			m_warn_messages.push_back("SetHeader(): Specified frame ID (" + value + ") exceeds header field size\n");
		break;
	case eSlateInformation:
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


std::string HdrDpxFile::GetHeader(HdrDpxFieldsString field) const
{
	switch (field)
	{
	case eImageFileName:
		return CopyToStringN(m_dpx_header.FileHeader.FileName, FILE_NAME_SIZE);
	case eCreationDateTime:
		return CopyToStringN(m_dpx_header.FileHeader.TimeDate, TIMEDATE_SIZE);
	case eCreator:
		return CopyToStringN(m_dpx_header.FileHeader.Creator, CREATOR_SIZE);
	case eProjectName:
		return CopyToStringN(m_dpx_header.FileHeader.Project, PROJECT_SIZE);
	case eRightToUseOrCopyright:
		return CopyToStringN(m_dpx_header.FileHeader.Copyright, COPYRIGHT_SIZE);
	case eSourceImageFileName:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.SourceFileName, FILE_NAME_SIZE);
	case eSourceImageDateTime:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.SourceTimeDate, TIMEDATE_SIZE);
	case eInputDeviceName:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.InputName, INPUTNAME_SIZE);
	case eInputDeviceSN:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.InputSN, INPUTSN_SIZE);
	case eFilmMfgIdCode:
		return CopyToStringN(m_dpx_header.FilmHeader.FilmMfgId, 2);
	case eFilmType:
		return CopyToStringN(m_dpx_header.FilmHeader.FilmType, 2);
	case eOffsetInPerfs:
		return CopyToStringN(m_dpx_header.FilmHeader.OffsetPerfs, 4);
	case ePrefix:
		return CopyToStringN(m_dpx_header.FilmHeader.Prefix, 6);
	case eCount:
		return CopyToStringN(m_dpx_header.FilmHeader.Count, 4);
	case eFormat:
		return CopyToStringN(m_dpx_header.FilmHeader.Format, 32);
	case eFrameIdentification:
		return CopyToStringN(m_dpx_header.FilmHeader.FrameId, 32);
	case eSlateInformation:
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
		return std::string((char *)m_dpx_sbmdata.SbmData.data(), m_dpx_sbmdata.SbmLength);
	}
	return "";
}

void HdrDpxFile::SetHeader(HdrDpxFieldsU32 field, uint32_t value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eOffsetToImageData:
		m_dpx_header.FileHeader.ImageOffset = value;
		break;
	case eTotalImageFileSize:
		m_dpx_header.FileHeader.FileSize = value;
		LOG_ERROR(eBadParameter, eWarning, "Setting total image file size header attribute has no effect");
		break;
	case eGenericSectionHeaderLength:
		m_dpx_header.FileHeader.GenericSize = value;
		break;
	case eIndustrySpecificHeaderLength:
		m_dpx_header.FileHeader.IndustrySize = value;
		break;
	case eUserDefinedHeaderLength:
		//m_dpx_header.FileHeader.UserSize = value;
		LOG_ERROR(eBadParameter, eWarning, "User data length can only be set by calling SetUserData() or SetHeader() using eUserDefinedData and a string");
		break;
	case eEncryptionKey:
		m_dpx_header.FileHeader.EncryptKey = value;
		break;
	case eStandardsBasedMetadataOffset:
		m_dpx_header.FileHeader.StandardsBasedMetadataOffset = value;
		break;
	case ePixelsPerLine:
		m_dpx_header.ImageHeader.PixelsPerLine = value;
		break;
	case eLinesPerImageElement:
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
	case ePixelAspectRatioH:
		m_dpx_header.SourceInfoHeader.AspectRatio[0] = value;
		break;
	case ePixelAspectRatioV:
		m_dpx_header.SourceInfoHeader.AspectRatio[1] = value;
		break;
	case eFramePositionInSequence:
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
	case eOffsetToImageData:
		return(m_dpx_header.FileHeader.ImageOffset);
	case eTotalImageFileSize:
		return(m_dpx_header.FileHeader.FileSize);
	case eGenericSectionHeaderLength:
		return(m_dpx_header.FileHeader.GenericSize);
	case eIndustrySpecificHeaderLength:
		return(m_dpx_header.FileHeader.IndustrySize);
	case eUserDefinedHeaderLength:
		return(m_dpx_header.FileHeader.UserSize);
	case eEncryptionKey:
		return(m_dpx_header.FileHeader.EncryptKey);
	case eStandardsBasedMetadataOffset:
		return(m_dpx_header.FileHeader.StandardsBasedMetadataOffset);
	case ePixelsPerLine:
		return(m_dpx_header.ImageHeader.PixelsPerLine);
	case eLinesPerImageElement:
		return(m_dpx_header.ImageHeader.LinesPerElement);
	case eXOffset:
		return(m_dpx_header.SourceInfoHeader.XOffset);
	case eYOffset:
		return(m_dpx_header.SourceInfoHeader.YOffset);
	case eXOriginalSize:
		return(m_dpx_header.SourceInfoHeader.XOriginalSize);
	case eYOriginalSize:
		return(m_dpx_header.SourceInfoHeader.YOriginalSize);
	case ePixelAspectRatioH:
		return(m_dpx_header.SourceInfoHeader.AspectRatio[0]);
	case ePixelAspectRatioV:
		return(m_dpx_header.SourceInfoHeader.AspectRatio[1]);
	case eFramePositionInSequence:
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
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eNumberOfImageElements:
		m_dpx_header.ImageHeader.NumberElements = value;
		break;
	case eBorderValidityXL:
		m_dpx_header.SourceInfoHeader.Border[0] = value;
		break;
	case eBorderValidityXR:
		m_dpx_header.SourceInfoHeader.Border[1] = value;
		break;
	case eBorderValidityYT:
		m_dpx_header.SourceInfoHeader.Border[2] = value;
		break;
	case eBorderValidityYB:
		m_dpx_header.SourceInfoHeader.Border[3] = value;
	}
}

uint16_t HdrDpxFile::GetHeader(HdrDpxFieldsU16 field) const
{
	switch (field)
	{
	case eNumberOfImageElements:
		return(m_dpx_header.ImageHeader.NumberElements);
	case eBorderValidityXL:
		return(m_dpx_header.SourceInfoHeader.Border[0]);
	case eBorderValidityXR:
		return(m_dpx_header.SourceInfoHeader.Border[1]);
	case eBorderValidityYT:
		return(m_dpx_header.SourceInfoHeader.Border[2]);
	case eBorderValidityYB:
		return(m_dpx_header.SourceInfoHeader.Border[3]);
	}
	return 0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsR32 field, float value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eXCenter:
		m_dpx_header.SourceInfoHeader.XCenter = value;
		break;
	case eYCenter:
		m_dpx_header.SourceInfoHeader.YCenter = value;
		break;
	case eXScannedSize:
		m_dpx_header.SourceInfoHeader.XScannedSize = value;
		break;
	case eYScannedSize:
		m_dpx_header.SourceInfoHeader.YScannedSize = value;
		break;
	case eFrameRateOfOriginal:
		m_dpx_header.FilmHeader.FrameRate = value;
		break;
	case eShutterAngleInDegrees:
		m_dpx_header.FilmHeader.ShutterAngle = value;
		break;
	case eHorizontalSamplingRate:
		m_dpx_header.TvHeader.HorzSampleRate = value;
		break;
	case eVerticalSamplingRate:
		m_dpx_header.TvHeader.VertSampleRate = value;
		break;
	case eTemporalSamplingRate:
		m_dpx_header.TvHeader.FrameRate = value;
		break;
	case eTimeOffsetFromSyncToFirstPixel:
		m_dpx_header.TvHeader.TimeOffset = value;
		break;
	case eGamma:
		m_dpx_header.TvHeader.Gamma = value;
		break;
	case eBlackLevelCode:
		m_dpx_header.TvHeader.BlackLevel = value;
		break;
	case eBlackGain:
		m_dpx_header.TvHeader.BlackGain = value;
		break;
	case eBreakpoint:
		m_dpx_header.TvHeader.Breakpoint = value;
		break;
	case eReferenceWhiteLevelCode:
		m_dpx_header.TvHeader.WhiteLevel = value;
		break;
	case eIntegrationTime:
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
	case eXScannedSize:
		return(m_dpx_header.SourceInfoHeader.XScannedSize);
	case eYScannedSize:
		return(m_dpx_header.SourceInfoHeader.YScannedSize);
	case eFrameRateOfOriginal:
		return(m_dpx_header.FilmHeader.FrameRate);
	case eShutterAngleInDegrees:
		return(m_dpx_header.FilmHeader.ShutterAngle);
	case eHorizontalSamplingRate:
		return(m_dpx_header.TvHeader.HorzSampleRate);
	case eVerticalSamplingRate:
		return(m_dpx_header.TvHeader.VertSampleRate);
	case eTemporalSamplingRate:
		return(m_dpx_header.TvHeader.FrameRate);
	case eTimeOffsetFromSyncToFirstPixel:
		return(m_dpx_header.TvHeader.TimeOffset);
	case eGamma:
		return(m_dpx_header.TvHeader.Gamma);
	case eBlackLevelCode:
		return(m_dpx_header.TvHeader.BlackLevel);
	case eBlackGain:
		return(m_dpx_header.TvHeader.BlackGain);
	case eBreakpoint:
		return(m_dpx_header.TvHeader.Breakpoint);
	case eReferenceWhiteLevelCode:
		return(m_dpx_header.TvHeader.WhiteLevel);
	case eIntegrationTime:
		return(m_dpx_header.TvHeader.IntegrationTimes);
	}
	return 0.0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsU8 field, uint8_t value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (field)
	{
	case eFieldNumber:
		m_dpx_header.TvHeader.FieldNumber = value;
	case eSMPTETCDBB2Value:
		m_dpx_header.TvHeader.SMPTETCDBB2 = value;
	}
}

uint8_t HdrDpxFile::GetHeader(HdrDpxFieldsU8 field) const
{
	switch(field)
	{
	case eFieldNumber:
		return(m_dpx_header.TvHeader.FieldNumber);
	case eSMPTETCDBB2Value:
		return(m_dpx_header.TvHeader.SMPTETCDBB2);
	}
	return 0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsDittoKey field, HdrDpxDittoKey value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.FileHeader.DittoKey = static_cast<uint32_t>(value);
}

HdrDpxDittoKey HdrDpxFile::GetHeader(HdrDpxFieldsDittoKey field) const
{
	return static_cast<HdrDpxDittoKey>(m_dpx_header.FileHeader.DittoKey);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsDatumMappingDirection field, HdrDpxDatumMappingDirection value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.FileHeader.DatumMappingDirection = static_cast<uint8_t>(value);
}

HdrDpxDatumMappingDirection HdrDpxFile::GetHeader(HdrDpxFieldsDatumMappingDirection field) const
{
	return static_cast<HdrDpxDatumMappingDirection>(m_dpx_header.FileHeader.DatumMappingDirection);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsOrientation field, HdrDpxOrientation value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.ImageHeader.Orientation = static_cast<uint16_t>(value);
}

HdrDpxOrientation HdrDpxFile::GetHeader(HdrDpxFieldsOrientation field) const
{
	return static_cast<HdrDpxOrientation>(m_dpx_header.ImageHeader.Orientation);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsTimeCode field, SMPTETimeCode value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.TvHeader.TimeCode = (value.h_tens << 28) |
									 (value.h_units << 24) |
									 (value.m_tens << 20) |
		                             (value.m_units << 16) |
		                             (value.s_tens << 12) |
		                             (value.s_units << 8) |
		                             (value.F_tens << 4) |
		                             value.F_units;
}

SMPTETimeCode HdrDpxFile::GetHeader(HdrDpxFieldsTimeCode field) const
{
	SMPTETimeCode ret;
	ret.h_tens = m_dpx_header.TvHeader.TimeCode >> 28;
	ret.h_units = (m_dpx_header.TvHeader.TimeCode >> 24) & 0xf;
	ret.m_tens = (m_dpx_header.TvHeader.TimeCode >> 20) & 0xf;
	ret.m_units = (m_dpx_header.TvHeader.TimeCode >> 16) & 0xf;
	ret.s_tens = (m_dpx_header.TvHeader.TimeCode >> 12) & 0xf;
	ret.s_units = (m_dpx_header.TvHeader.TimeCode >> 8) & 0xf;
	ret.F_tens = (m_dpx_header.TvHeader.TimeCode >> 4) & 0xf;
	ret.F_units = m_dpx_header.TvHeader.TimeCode & 0xf;
	return ret;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsUserBits field, SMPTEUserBits value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.TvHeader.UserBits = (value.UB8 << 28) |
									 (value.UB7 << 24) |
									 (value.UB6 << 20) |
									 (value.UB5 << 16) |
									 (value.UB4 << 12) |
									 (value.UB3 << 8) |
									 (value.UB2 << 4) |
									 value.UB1;
}

SMPTEUserBits HdrDpxFile::GetHeader(HdrDpxFieldsUserBits field) const
{
	SMPTEUserBits ret;

	ret.UB8 = m_dpx_header.TvHeader.UserBits >> 28;
	ret.UB7 = (m_dpx_header.TvHeader.UserBits >> 24) & 0xf;
	ret.UB6 = (m_dpx_header.TvHeader.UserBits >> 20) & 0xf;
	ret.UB5 = (m_dpx_header.TvHeader.UserBits >> 16) & 0xf;
	ret.UB4 = (m_dpx_header.TvHeader.UserBits >> 12) & 0xf;
	ret.UB3 = (m_dpx_header.TvHeader.UserBits >> 8) & 0xf;
	ret.UB2 = (m_dpx_header.TvHeader.UserBits >> 4) & 0xf;
	ret.UB1 = m_dpx_header.TvHeader.UserBits & 0xf;
	return ret;
}


void HdrDpxFile::SetHeader(HdrDpxFieldsInterlace field, HdrDpxInterlace value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.TvHeader.Interlace = static_cast<uint8_t>(value);
}

HdrDpxInterlace HdrDpxFile::GetHeader(HdrDpxFieldsInterlace field) const
{
	return static_cast<HdrDpxInterlace>(m_dpx_header.TvHeader.Interlace);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsVideoSignal field, HdrDpxVideoSignal value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.TvHeader.VideoSignal = static_cast<uint8_t>(value);
}

HdrDpxVideoSignal HdrDpxFile::GetHeader(HdrDpxFieldsVideoSignal field) const
{
	return static_cast<HdrDpxVideoSignal>(m_dpx_header.TvHeader.VideoSignal);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsVideoIdentificationCode field, HdrDpxVideoIdentificationCode value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.TvHeader.VideoIdentificationCode = static_cast<uint8_t>(value);
}

HdrDpxVideoIdentificationCode HdrDpxFile::GetHeader(HdrDpxFieldsVideoIdentificationCode field) const
{
	return static_cast<HdrDpxVideoIdentificationCode>(m_dpx_header.TvHeader.VideoIdentificationCode);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsSMPTETCType field, HdrDpxSMPTETCType value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_dpx_header.TvHeader.SMPTETCType = static_cast<uint8_t>(value);
}

HdrDpxSMPTETCType HdrDpxFile::GetHeader(HdrDpxFieldsSMPTETCType field) const
{
	return static_cast<HdrDpxSMPTETCType>(m_dpx_header.TvHeader.SMPTETCType);
}


void HdrDpxFile::SetHeader(HdrDpxFieldsByteOrder field, HdrDpxByteOrder value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	m_byteorder = value;
}

HdrDpxByteOrder HdrDpxFile::GetHeader(HdrDpxFieldsByteOrder field) const
{
	return m_byteorder;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsVersion field, HdrDpxVersion value)
{
	if (m_is_header_locked)
	{
		LOG_ERROR(eHeaderLocked, eWarning, "Attempted to change locked header field");
		return;
	}
	switch (value)
	{
	case eDPX_1_0:
		CopyStringN(m_dpx_header.FileHeader.Version, "V1.0", 8);
		LOG_ERROR(eBadParameter, eWarning, "V1.0 file output is not supported.");
		break;
	case eDPX_2_0:
		CopyStringN(m_dpx_header.FileHeader.Version, "V2.0", 8);
		LOG_ERROR(eBadParameter, eWarning, "V2.0 file output is not supported.");
		break;
	case eDPX_2_0_HDR:
		CopyStringN(m_dpx_header.FileHeader.Version, "V2.0HDR", 8);
		break;
	case eDPX_Unrecognized:
		LOG_ERROR(eBadParameter, eWarning, "Cannot set version field to unrecongized value");
	}
}

HdrDpxVersion HdrDpxFile::GetHeader(HdrDpxFieldsVersion field) const
{
	if (CopyToStringN(m_dpx_header.FileHeader.Version, 8).compare("V1.0") == 0)
		return eDPX_1_0;
	if (CopyToStringN(m_dpx_header.FileHeader.Version, 8).compare("V2.0") == 0)
		return eDPX_2_0;
	if (CopyToStringN(m_dpx_header.FileHeader.Version, 8).compare("V2.0HDR") == 0)
		return eDPX_2_0_HDR;
	return eDPX_Unrecognized;
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
	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset == UNDEFINED_U32)		// nothing to do
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

void HdrDpxFile::GetError(int err_idx, ErrorCode &errcode, ErrorSeverity &severity, std::string &errmsg) const
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

void HdrDpxFile::ClearHeader(uint8_t ie_index)
{
	if (ie_index == 0xff)
	{
		memset(&m_dpx_header, 0xff, sizeof(HDRDPXFILEFORMAT));
		SetHeader(eImageFileName, "");
		SetHeader(eCreationDateTime, "");
		SetHeader(eCreator, "");
		SetHeader(eProjectName, "");
		SetHeader(eRightToUseOrCopyright, "");
		SetHeader(eSourceImageFileName, "");
		SetHeader(eSourceImageDateTime, "");
		SetHeader(eInputDeviceName, "");
		SetHeader(eInputDeviceSN, "");
		SetHeader(eFilmMfgIdCode, "");
		SetHeader(eFilmType, "");
		SetHeader(eOffsetInPerfs, "");
		SetHeader(ePrefix, "");
		SetHeader(eCount, "");
		SetHeader(eFormat, "");
		SetHeader(eFrameIdentification, "");
		SetHeader(eSlateInformation, "");
		SetHeader(eUserIdentification, "");
		SetHeader(eSBMFormatDescriptor, "");
	}
	for (uint8_t ie = 0; ie < 8; ++ie)
	{
		if (ie == ie_index || ie_index == 0xff)
		{
			memset(&m_dpx_header.ImageHeader.ImageElement[ie], 0xff, sizeof(HDRDPX_IMAGEELEMENT));
			memset(&m_dpx_header.ImageHeader.ImageElement[ie].Description, 0, sizeof(DESCRIPTION_SIZE));
		}
	}
}

void HdrDpxFile::DumpUserDataOptions(bool dump_ud, HdrDpxDumpFormat format)
{
	m_ud_dump = dump_ud;
	m_ud_dump_format = format;
}

void HdrDpxFile::DumpStandardsBasedMetedataOptions(bool dump_sbmd, HdrDpxDumpFormat format)
{
	m_sbm_dump = dump_sbmd;
	m_sbm_dump_format = format;
}
