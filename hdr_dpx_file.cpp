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

HdrDpxFile::HdrDpxFile(std::string s)
{
	union
	{
		uint8_t byte[4];
		uint32_t u32;
	} u;
	u.u32 = 0x12345678;
	m_machine_is_msbf = (u.u32 == 0x12);
	this->ReadFile(s);
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

void HdrDpxFile::ReadFile(std::string filename)
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
	for (int i = 0; i < 8; ++i)
	{
		if (m_dpx_header.ImageHeader.ImageElement[i].DataOffset != UINT32_MAX)
		{
			m_IE[i].Initialize(i, &m_file_stream, &m_dpx_header, &m_filemap);
			m_IE[i].OpenForReading(swapped);
		}
		else
			m_IE[i].m_isinitialized = false;
	}
	m_file_is_hdr_version = (static_cast<bool>(!strcmp(m_dpx_header.FileHeader.Version, "DPX2.0HDR")));

	m_open_for_write = false;
	m_open_for_read = true;
	// TBD: Read user data & standards-based metadata
}

HdrDpxImageElement *HdrDpxFile::GetImageElement(uint8_t ie_index)
{
	if (ie_index < 0 || ie_index > 7)
	{
		LOG_ERROR(eBadParameter, eFatal, "Image element number out of bounds\n");
		return NULL;
	}
	if (!m_IE[ie_index].m_isinitialized)
	{
		m_IE[ie_index].Initialize(ie_index, &m_file_stream, &m_dpx_header, &m_filemap);
	}
	return &(m_IE[ie_index]);
}


//#define BS_32(f,x)   { uint8_t *f_ptr; size_t o; uint8_t t; f_ptr=(uint8_t *)&(f); o=offsetof(HDRDPXFILEFORMAT,x); t=f_ptr[o]; f_ptr[o]=f_ptr[o+3]; f_ptr[o+3]=t; t=f_ptr[o+1]; f_ptr[o+1]=f_ptr[o+2]; f_ptr[o+2]=t; }
//#define BS_16(f,x)   { uint8_t *f_ptr; size_t o; uint8_t t; f_ptr=(uint8_t *)&(f); o=offsetof(HDRDPXFILEFORMAT,x); t=f_ptr[o]; f_ptr[o]=f_ptr[o+1]; f_ptr[o]=t; }

void HdrDpxFile::ByteSwapHeader(void)
{
	//HDRDPXFILEFORMAT *f;
	int i;

	//f = &m_dpx_header;

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

	for (i = 0; i < NUM_IMAGE_ELEMENTS; ++i)
	{
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].DataSign));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].LowData.d));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].LowQuantity));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].HighData.d));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].HighQuantity));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].DataOffset));
		ByteSwap16(&(m_dpx_header.ImageHeader.ImageElement[i].Packing));
		ByteSwap16(&(m_dpx_header.ImageHeader.ImageElement[i].Encoding));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].DataOffset));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].EndOfLinePadding));
		ByteSwap32(&(m_dpx_header.ImageHeader.ImageElement[i].EndOfImagePadding));
	}

	/* Image source info header details */
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.XOffset));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.YOffset));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.XCenter));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.YCenter));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.XOriginalSize));
	ByteSwap32(&(m_dpx_header.SourceInfoHeader.YOriginalSize));
	for (i = 0; i<4; ++i)
		ByteSwap16(&(m_dpx_header.SourceInfoHeader.Border[i]));
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


bool HdrDpxFile::IsHdr(void)
{
	return (strcmp(m_dpx_header.FileHeader.Version, "V2.0HDR") == 0);
}

void HdrDpxFile::ComputeOffsets()
{
	uint32_t data_offset;

	m_filemap.Reset();

	// Fill in what is specified
	m_filemap.AddRegion(0, sizeof(HDRDPXFILEFORMAT), 100);

	if (m_dpx_header.FileHeader.UserSize != 0 && m_dpx_header.FileHeader.UserSize != UINT32_MAX)
		m_filemap.AddRegion(sizeof(HDRDPXFILEFORMAT), ((sizeof(HDRDPXFILEFORMAT) + m_dpx_header.FileHeader.UserSize + 3) >> 2) << 2, 101);

	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UINT32_MAX)
		m_filemap.AddRegion(m_dpx_header.FileHeader.StandardsBasedMetadataOffset, m_dpx_header.FileHeader.StandardsBasedMetadataOffset + m_dpx_sbmdata.SbmLength + 132, 102);

	// Add IEs with data offsets
	for (int i = 0; i < 8; ++i)
	{
		if (m_IE[i].m_isinitialized)
		{
			data_offset = m_IE[i].GetHeader(eOffsetToData);
			if (data_offset != UINT32_MAX)
			{
				if (m_IE[i].GetHeader(eEncoding) == eEncodingRLE)   // RLE can theoretically use more bits than compressed
				{
					uint32_t est_size = static_cast<uint32_t>((1.0 + RLE_MARGIN) * m_IE[i].BytesUsed());
					m_filemap.AddRegion(data_offset, data_offset + est_size, i);
					m_filemap.AddRLEIE(i, data_offset, est_size);
				}
				else
					m_filemap.AddRegion(data_offset, data_offset + m_IE[i].BytesUsed(), i);
			}
		}
	}


	// Next add fixed-size IE's that don't yet have data offsets
	for (int i = 0; i < 8; ++i)
	{
		if (m_IE[i].m_isinitialized)
		{
			data_offset = m_IE[i].GetHeader(eOffsetToData);
			if (data_offset == UINT32_MAX && m_IE[i].GetHeader(eEncoding) != eEncodingRLE)
				m_IE[i].SetHeader(eOffsetToData, m_filemap.FindEmptySpace(m_IE[i].BytesUsed(), i));
		}
	}

	// Lastly add first variable-size IE
	for (int i = 0; i < 8; ++i)
	{
		if (m_IE[i].m_isinitialized)
		{
			data_offset = m_IE[i].GetHeader(eOffsetToData);
			if (data_offset == UINT32_MAX && m_IE[i].GetHeader(eEncoding) == eEncodingRLE)
			{
				uint32_t est_size = static_cast<uint32_t>((1.0 + RLE_MARGIN) * m_IE[i].BytesUsed());
				data_offset = m_filemap.FindEmptySpace(est_size, i);
				m_IE[i].SetHeader(eOffsetToData, data_offset);
				m_filemap.AddRLEIE(i, data_offset, est_size);
				break;
			}
		}
	}
	if (m_filemap.CheckCollisions())
		LOG_ERROR(eBadParameter, eWarning, "Image map has potentially overlapping regions");
}

uint8_t HdrDpxFile::GetActiveIE()
{
	return m_active_rle_ie;
}

void HdrDpxFile::FillCoreFields()
{
	int i, ne = 0, firstie = -1;

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
	ComputeOffsets();

	for (i = 0; i < 8; ++i)
	{
		if (m_IE[i].m_isinitialized)
		{
			ne++;
			if(firstie < 0)
				firstie = i;
		}
	}
	if(firstie < 0)
		LOG_ERROR(eBadParameter, eFatal, "No initialized image elements found");

	if (m_dpx_header.ImageHeader.Orientation == UINT16_MAX)
	{
		LOG_ERROR(eMissingCoreField, eWarning, "Image orientation not specified, assuming left-to-right, top-to-bottom");
		m_dpx_header.ImageHeader.Orientation = 0;
	}
	if (m_dpx_header.ImageHeader.NumberElements == 0 || m_dpx_header.ImageHeader.NumberElements == UINT16_MAX)
		m_dpx_header.ImageHeader.NumberElements = ne;

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
	for (i = 0; i < ne; ++i)
	{
		if (m_IE[i].m_isinitialized)
		{
			// Default values for core fields that are not specified
			if (m_IE[i].GetHeader(eBitDepth) == eBitDepthUndefined)
			{
				LOG_ERROR(eMissingCoreField, eFatal, "Bit depth must be specified");
				return;
			}

			if (m_IE[i].GetHeader(eDescriptor) == eDescUndefined)
			{
				LOG_ERROR(eMissingCoreField, eFatal, "Descriptor must be specified");
				return;
			}

			if (m_IE[i].GetHeader(eDataSign) == eDataSignUndefined)
			{
				LOG_ERROR(eMissingCoreField, eWarning, "Data sign for image element " + std::to_string(i + 1) + " not specified, assuming unsigned");
				m_IE[i].SetHeader(eDataSign, eDataSignUnsigned);
			}
			if (std::isnan(m_IE[i].GetHeader(eReferenceLowDataCode)) || std::isnan(m_IE[i].GetHeader(eReferenceHighDataCode)))
			{
				if (m_IE[i].GetHeader(eBitDepth) < 32)
				{
					LOG_ERROR(eMissingCoreField, eWarning, "Data range for image element " + std::to_string(i + 1) + " not specified, assuming full range");
					m_IE[i].SetHeader(eReferenceLowDataCode, 0);
					m_IE[i].SetHeader(eReferenceHighDataCode, static_cast<float>((1 << m_dpx_header.ImageHeader.ImageElement[i].BitSize) - 1));
				}
				else  // Floating point samples
				{
					LOG_ERROR(eMissingCoreField, eWarning, "Data range for image element " + std::to_string(i + 1) + " not specified, assuming full range (maxval = 1.0)");
					m_IE[i].SetHeader(eReferenceLowDataCode, (float)0.0);
					m_IE[i].SetHeader(eReferenceHighDataCode, (float)1.0);
				}
			}
		}
		else {
			// Set ununsed image elements to all 1's
			memset((void *)&(m_dpx_header.ImageHeader.ImageElement[i]), 0xff, sizeof(HDRDPX_IMAGEELEMENT));
		}
		if (m_IE[i].GetHeader(eTransfer) == eTransferUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Transfer characteristic for image element " + std::to_string(i + 1) + " not specified");
		}
		if (m_IE[i].GetHeader(eColorimetric) == eColorimetricUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Colorimetric field for image element " + std::to_string(i + 1) + " not specified");
		}
		if (m_IE[i].GetHeader(ePacking) == ePackingUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Packing field not specified for image element " + std::to_string(i + 1) + ", assuming Packed");
			m_IE[i].SetHeader(ePacking, ePackingPacked);
		}
		if (m_IE[i].GetHeader(eEncoding) == eEncodingUndefined)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "Encoding not specified for image element " + std::to_string(i + 1) + ", assuming uncompressed");
			m_IE[i].SetHeader(eEncoding, eEncodingNoEncoding);		// No compression (default)
		}
		if (m_IE[i].GetHeader(eEndOfLinePadding) == UINT32_MAX)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "End of line padding not specified for image element " + std::to_string(i + 1) + ", assuming no padding");
			m_IE[i].SetHeader(eEndOfLinePadding, 0);
		}
		if (m_IE[i].GetHeader(eEndOfImagePadding) == UINT32_MAX)
		{
			LOG_ERROR(eMissingCoreField, eWarning, "End of image padding not specified for image element " + std::to_string(i + 1) + ", assuming no padding");
			m_IE[i].SetHeader(eEndOfImagePadding, 0);
		}
	}
}


void HdrDpxFile::WriteFile(std::string fname)
{
	bool byte_swap;

	m_file_stream.open(fname, std::ios::binary | std::ios::out);

	if (m_byteorder == eNativeByteOrder)
		byte_swap = false;
	else if (m_byteorder == eLSBF)
		byte_swap = m_machine_is_msbf ? true : false;
	else
		byte_swap = m_machine_is_msbf ? false : true;

	m_file_name = fname;
	
	if (!m_file_stream)
	{
		LOG_ERROR(eFileOpenError, eFatal, "Unable to open file " + fname + " for output");
		return;
	}

	FillCoreFields();
	// Maybe check if core fields are valid here?

	if (m_file_stream.bad())
	{
		LOG_ERROR(eFileWriteError, eFatal, "Error writing DPX image data");
		return;
	}

	for (int i = 0; i<8; ++i)
	{
		if (m_IE[i].m_isinitialized)
		{
			m_IE[i].OpenForWriting(byte_swap);
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
		// Get RLE offsets if needed
		std::vector<uint32_t> data_offsets = m_filemap.GetRLEIEDataOffsets();
		for (int i = 0; i < 8;++i)
			if (m_dpx_header.ImageHeader.ImageElement[i].Encoding == 1)
				m_dpx_header.ImageHeader.ImageElement[i].DataOffset = data_offsets[i];

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
		for (int i = 0; i < 8; ++i)
			m_IE[i].m_isinitialized = false;
		m_open_for_read = false;
		m_open_for_write = false;
	}
}

static std::string u32_to_hex(uint32_t v)
{
	std::stringstream s;
	s << "0x" << std::setfill('0') << std::setw(8) << std::hex << v;
	return s.str();
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

	int i;
	
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

	for (i = 0; i < 8; ++i)
	{
		std::string fld_name;
		header += "//// Image element data structure " + std::to_string(i + 1) + "\n";
		fld_name = "Data_Sign_" + std::to_string(i + 1);		PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].DataSign);
		if (m_dpx_header.ImageHeader.ImageElement[i].BitSize < 32)
		{
			fld_name = "Low_Data_" + std::to_string(i + 1);			PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].LowData.d);
		}
		else
		{
			fld_name = "Low_Data_" + std::to_string(i + 1);			PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].LowData.f);
		}
		fld_name = "Low_Quantity_" + std::to_string(i + 1);		PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].LowQuantity);
		if (m_dpx_header.ImageHeader.ImageElement[i].BitSize < 32)
		{
			fld_name = "High_Data_" + std::to_string(i + 1);		PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].HighData.d);
		}
		else
		{
			fld_name = "High_Data_" + std::to_string(i + 1);		PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].HighData.f);
		}
		fld_name = "High_Quantity_" + std::to_string(i + 1);	PRINT_FIELD_R32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].HighQuantity);
		fld_name = "Descriptor_" + std::to_string(i + 1);		PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[i].Descriptor);
		fld_name = "Transfer_" + std::to_string(i + 1);		PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[i].Transfer);
		fld_name = "Colorimetric_" + std::to_string(i + 1);	PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[i].Colorimetric);
		fld_name = "BitSize_" + std::to_string(i + 1);			PRINT_FIELD_U8(fld_name, m_dpx_header.ImageHeader.ImageElement[i].BitSize);
		fld_name = "Packing_" + std::to_string(i + 1);			PRINT_FIELD_U16(fld_name, m_dpx_header.ImageHeader.ImageElement[i].Packing);
		fld_name = "Encoding_" + std::to_string(i + 1);		PRINT_FIELD_U16(fld_name, m_dpx_header.ImageHeader.ImageElement[i].Encoding);
		fld_name = "DataOffset_" + std::to_string(i + 1);		PRINT_RO_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].DataOffset);
		fld_name = "End_Of_Line_Padding_" + std::to_string(i + 1); PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].EndOfLinePadding);
		fld_name = "End_Of_Image_Padding_" + std::to_string(i + 1); PRINT_FIELD_U32(fld_name, m_dpx_header.ImageHeader.ImageElement[i].EndOfImagePadding);
		fld_name = "Description_" + std::to_string(i + 1);		PRINT_FIELD_ASCII(fld_name, m_dpx_header.ImageHeader.ImageElement[i].Description, 32);
		fld_name = "Chroma_Subsampling_" + std::to_string(i + 1); PRINT_FIELD_U8(fld_name, 0xf & (m_dpx_header.ImageHeader.ChromaSubsampling >> (i * 4)));
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

bool HdrDpxFile::CopyStringN(char *dest, std::string src, int l)
{
	int n;

	for (n = 0; n < l; ++n)
	{
		if (n >= src.length())
			*(dest++) = '\0';
		else
			*(dest++) = src[n];
	}
	return (src.length() > l);  // return true if the string is longer than the field
}

void HdrDpxFile::SetHeader(HdrDpxFieldsString f, std::string s)
{
	switch (f)
	{
	case eFileName:
		if(CopyStringN(m_dpx_header.FileHeader.FileName, s, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified file name (" + s + ") exceeds header field size\n");
		break;
	case eTimeDate:
		if (CopyStringN(m_dpx_header.FileHeader.TimeDate, s, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified time & date (" + s + ") exceeds header field size\n");
		break;
	case eCreator:
		if (CopyStringN(m_dpx_header.FileHeader.Creator, s, CREATOR_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified creator (" + s + ") exceeds header field size\n");
		break;
	case eProject:
		if (CopyStringN(m_dpx_header.FileHeader.Project, s, PROJECT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified project (" + s + ") exceeds header field size\n");
		break;
	case eCopyright:
		if (CopyStringN(m_dpx_header.FileHeader.Copyright, s, COPYRIGHT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified copyright (" + s + ") exceeds header field size\n");
		break;
	case eSourceFileName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceFileName, s, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source file name (" + s + ") exceeds header field size\n");
		break;
	case eSourceTimeDate:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceTimeDate, s, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source time & date (" + s + ") exceeds header field size\n");
		break;
	case eInputName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputName, s, INPUTNAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input name (" + s + ") exceeds header field size\n");
		break;
	case eInputSN:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputSN, s, INPUTSN_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input SN (" + s + ") exceeds header field size\n");
		break;
	case eFilmMfgId:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmMfgId, s, 2))
			m_warn_messages.push_back("SetHeader(): Specified film mfg id (" + s + ") exceeds header field size\n");
		break;
	case eFilmType:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmType, s, 2))
			m_warn_messages.push_back("SetHeader(): Specified film type (" + s + ") exceeds header field size\n");
		break;
	case eOffsetPerfs:
		if (CopyStringN(m_dpx_header.FilmHeader.OffsetPerfs, s, 4))
			m_warn_messages.push_back("SetHeader(): Specified offset perfs (" + s + ") exceeds header field size\n");
		break;
	case ePrefix:
		if (CopyStringN(m_dpx_header.FilmHeader.Prefix, s, 6))
			m_warn_messages.push_back("SetHeader(): Specified prefix (" + s + ") exceeds header field size\n");
		break;
	case eCount:
		if (CopyStringN(m_dpx_header.FilmHeader.Count, s, 4))
			m_warn_messages.push_back("SetHeader(): Specified film count (" + s + ") exceeds header field size\n");
		break;
	case eFormat:
		if (CopyStringN(m_dpx_header.FilmHeader.Format, s, 32))
			m_warn_messages.push_back("SetHeader(): Specified film format (" + s + ") exceeds header field size\n");
		break;
	case eFrameId:
		if (CopyStringN(m_dpx_header.FilmHeader.FrameId, s, 32))
			m_warn_messages.push_back("SetHeader(): Specified frame ID (" + s + ") exceeds header field size\n");
		break;
	case eSlateInfo:
		if (CopyStringN(m_dpx_header.FilmHeader.SlateInfo, s, 100))
			m_warn_messages.push_back("SetHeader(): Specified slate info (" + s + ") exceeds header field size\n");
		break;
	}
}

std::string HdrDpxFile::CopyToStringN(char *src, int l)
{
	int n;
	std::string s ("");

	for (n = 0; n < l; ++n)
	{
		if (*src == '\0')
			return s;
		else
			s.push_back(*(src++));
	}
	return s;
}

std::string HdrDpxFile::GetHeader(HdrDpxFieldsString f)
{
	switch (f)
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
	}
	return "";
}

void HdrDpxFile::SetHeader(HdrDpxFieldsU32 f, uint32_t d)
{
	switch (f)
	{
	case eImageOffset:
		m_dpx_header.FileHeader.ImageOffset = d;
		break;
	case eFileSize:
		m_dpx_header.FileHeader.FileSize = d;
		break;
	case eGenericSize:
		m_dpx_header.FileHeader.GenericSize = d;
		break;
	case eIndustrySize:
		m_dpx_header.FileHeader.IndustrySize = d;
		break;
	case eUserSize:
		m_dpx_header.FileHeader.UserSize = d;
		break;
	case eEncryptKey:
		m_dpx_header.FileHeader.EncryptKey = d;
		break;
	case eStandardsBasedMetadataOffset:
		m_dpx_header.FileHeader.StandardsBasedMetadataOffset = d;
		break;
	case ePixelsPerLine:
		m_dpx_header.ImageHeader.PixelsPerLine = d;
		break;
	case eLinesPerElement:
		m_dpx_header.ImageHeader.LinesPerElement = d;
		break;
	case eXOffset:
		m_dpx_header.SourceInfoHeader.XOffset = d;
		break;
	case eYOffset:
		m_dpx_header.SourceInfoHeader.YOffset = d;
		break;
	case eXOriginalSize:
		m_dpx_header.SourceInfoHeader.XOriginalSize = d;
		break;
	case eYOriginalSize:
		m_dpx_header.SourceInfoHeader.YOriginalSize = d;
		break;
	case eAspectRatioH:
		m_dpx_header.SourceInfoHeader.AspectRatio[0] = d;
		break;
	case eAspectRatioV:
		m_dpx_header.SourceInfoHeader.AspectRatio[1] = d;
		break;
	case eFramePosition:
		m_dpx_header.FilmHeader.FramePosition = d;
		break;
	case eSequenceLen:
		m_dpx_header.FilmHeader.SequenceLen = d;
		break;
	case eHeldCount:
		m_dpx_header.FilmHeader.HeldCount = d;
	}
}

uint32_t HdrDpxFile::GetHeader(HdrDpxFieldsU32 f)
{
	switch (f)
	{
	case eImageOffset:
		return(m_dpx_header.FileHeader.ImageOffset);
	case eFileSize:
		return(m_dpx_header.FileHeader.FileSize);
	case eGenericSize:
		return(m_dpx_header.FileHeader.GenericSize);
	case eIndustrySize:
		return(m_dpx_header.FileHeader.IndustrySize);
	case eUserSize:
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
	case eSequenceLen:
		return(m_dpx_header.FilmHeader.SequenceLen);
	case eHeldCount:
		return(m_dpx_header.FilmHeader.HeldCount);
	}
	return 0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsU16 f, uint16_t d)
{
	switch (f)
	{
	case eNumberElements:
		m_dpx_header.ImageHeader.NumberElements = d;
		break;
	case eBorderXL:
		m_dpx_header.SourceInfoHeader.Border[0] = d;
		break;
	case eBorderXR:
		m_dpx_header.SourceInfoHeader.Border[1] = d;
		break;
	case eBorderYT:
		m_dpx_header.SourceInfoHeader.Border[2] = d;
		break;
	case eBorderYB:
		m_dpx_header.SourceInfoHeader.Border[3] = d;
	}
}

uint16_t HdrDpxFile::GetHeader(HdrDpxFieldsU16 f)
{
	switch (f)
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

void HdrDpxFile::SetHeader(HdrDpxFieldsR32 f, float d)
{
	switch (f)
	{
	case eXCenter:
		m_dpx_header.SourceInfoHeader.XCenter = d;
		break;
	case eYCenter:
		m_dpx_header.SourceInfoHeader.YCenter = d;
		break;
	case eFilmFrameRate:
		m_dpx_header.FilmHeader.FrameRate = d;
		break;
	case eShutterAngle:
		m_dpx_header.FilmHeader.ShutterAngle = d;
		break;
	case eHorzSampleRate:
		m_dpx_header.TvHeader.HorzSampleRate = d;
		break;
	case eVertSampleRate:
		m_dpx_header.TvHeader.VertSampleRate = d;
		break;
	case eTvFrameRate:
		m_dpx_header.TvHeader.FrameRate = d;
		break;
	case eTimeOffset:
		m_dpx_header.TvHeader.TimeOffset = d;
		break;
	case eGamma:
		m_dpx_header.TvHeader.Gamma = d;
		break;
	case eBlackLevel:
		m_dpx_header.TvHeader.BlackLevel = d;
		break;
	case eBlackGain:
		m_dpx_header.TvHeader.BlackGain = d;
		break;
	case eBreakpoint:
		m_dpx_header.TvHeader.Breakpoint = d;
		break;
	case eWhiteLevel:
		m_dpx_header.TvHeader.WhiteLevel = d;
		break;
	case eIntegrationTimes:
		m_dpx_header.TvHeader.IntegrationTimes = d;
	}
}

float HdrDpxFile::GetHeader(HdrDpxFieldsR32 f)
{
	switch (f)
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

void HdrDpxFile::SetHeader(HdrDpxFieldsU8 f, uint8_t d)
{
	switch (f)
	{
	case eFieldNumber:
		m_dpx_header.TvHeader.FieldNumber = d;
	}
}

uint8_t HdrDpxFile::GetHeader(HdrDpxFieldsU8 f)
{
	switch(f)
	{
	case eFieldNumber:
		return(m_dpx_header.TvHeader.FieldNumber);
	}
	return 0;
}

void HdrDpxFile::SetHeader(HdrDpxFieldsDittoKey f, HdrDpxDittoKey d)
{
	m_dpx_header.FileHeader.DittoKey = static_cast<uint32_t>(d);
}

HdrDpxDittoKey HdrDpxFile::GetHeader(HdrDpxFieldsDittoKey f)
{
	return static_cast<HdrDpxDittoKey>(m_dpx_header.FileHeader.DittoKey);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsDatumMappingDirection f, HdrDpxDatumMappingDirection d)
{
	m_dpx_header.FileHeader.DatumMappingDirection = static_cast<uint8_t>(d);
}

HdrDpxDatumMappingDirection HdrDpxFile::GetHeader(HdrDpxFieldsDatumMappingDirection f)
{
	return static_cast<HdrDpxDatumMappingDirection>(m_dpx_header.FileHeader.DatumMappingDirection);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsOrientation f, HdrDpxOrientation d)
{
	m_dpx_header.ImageHeader.Orientation = static_cast<uint16_t>(d);
}

HdrDpxOrientation HdrDpxFile::GetHeader(HdrDpxFieldsOrientation f)
{
	return static_cast<HdrDpxOrientation>(m_dpx_header.ImageHeader.Orientation);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsInterlace f, HdrDpxInterlace d)
{
	m_dpx_header.TvHeader.Interlace = static_cast<uint8_t>(d);
}

HdrDpxInterlace HdrDpxFile::GetHeader(HdrDpxFieldsInterlace f)
{
	return static_cast<HdrDpxInterlace>(m_dpx_header.TvHeader.Interlace);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsVideoSignal f, HdrDpxVideoSignal d)
{
	m_dpx_header.TvHeader.VideoSignal = static_cast<uint8_t>(d);
}

HdrDpxVideoSignal HdrDpxFile::GetHeader(HdrDpxFieldsVideoSignal f)
{
	return static_cast<HdrDpxVideoSignal>(m_dpx_header.TvHeader.VideoSignal);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsVideoIdentificationCode f, HdrDpxVideoIdentificationCode d)
{
	m_dpx_header.TvHeader.VideoIdentificationCode = static_cast<uint8_t>(d);
}

HdrDpxVideoIdentificationCode HdrDpxFile::GetHeader(HdrDpxFieldsVideoIdentificationCode f)
{
	return static_cast<HdrDpxVideoIdentificationCode>(m_dpx_header.TvHeader.VideoIdentificationCode);
}

void HdrDpxFile::SetHeader(HdrDpxFieldsByteOrder f, HdrDpxByteOrder d)
{
	m_byteorder = d;
}

HdrDpxByteOrder HdrDpxFile::GetHeader(HdrDpxFieldsByteOrder f)
{
	return m_byteorder;
}

std::list<std::string> HdrDpxFile::GetWarningList(void)
{
	return m_warn_messages;
}

bool HdrDpxFile::IsOk(void)
{
	return m_err.GetWorstSeverity() != eFatal;
}

int HdrDpxFile::GetNumErrors(void)
{
	return m_err.GetNumErrors();
}

void HdrDpxFile::GetError(int index, ErrorCode &errcode, ErrorSeverity &severity, std::string &errmsg)
{
	return m_err.GetError(index, errcode, severity, errmsg);
}

void HdrDpxFile::ClearErrors()
{
	m_err.Clear();
}