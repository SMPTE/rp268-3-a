#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "hdr_dpx.h"

extern "C"
{
	void hdr_dpx_byte_swap(HDRDPXFILEFORMAT *f);
}

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
	err.LogError(number, severity, ((severity == Dpx::eInformational) ? "INFO #" : ((severity == Dpx::eWarning) ? "WARNING #" : "ERROR #")) \
			   + std::to_string(static_cast<int>(number)) + " in " \
               + "function " + __FUNCTION__ \
               + ", file " + __FILE__ \
               + ", line " + std::to_string(__LINE__) + ":\n" \
               + msg + "\n");


using namespace std;

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

HdrDpxFile::HdrDpxFile(string s)
{
	union
	{
		uint8_t byte[4];
		uint32_t u32;
	} u;
	u.u32 = 0x12345678;
	m_machine_is_mbsf = (u.u32 == 0x12);
	this->Open(s);
}

HdrDpxFile::HdrDpxFile()
{
	union
	{
		uint8_t byte[4];
		uint32_t u32;
	} u;
	u.u32 = 0x12345678;
	m_machine_is_mbsf = (u.u32 == 0x12);
	memset(&m_dpx_header, 0xff, sizeof(HDRDPXFILEFORMAT));
}

HdrDpxFile::~HdrDpxFile()
{
	m_IE.clear();
}

Dpx::ErrorObject HdrDpxFile::Open(std::string filename)
{
	Dpx::ErrorObject err;
	ifstream infile(filename, ios::binary | ios::in);
	m_warn_messages.clear();
	m_file_name = filename;

	if (!infile)
	{
		LOG_ERROR(Dpx::eFileOpenError, Dpx::eFatal, "Unable to open file " + filename + "\n");
		return err;
	}
	infile.read((char *)&m_dpx_header, sizeof(HDRDPXFILEFORMAT));
	bool swapped = ByteSwapToMachine();
	if ((m_machine_is_mbsf && swapped) || (!m_machine_is_mbsf && !swapped))
		m_byteorder = Dpx::eLSBF;
	else
		m_byteorder = Dpx::eMSBF;
	if (infile.bad())
	{
		LOG_ERROR(Dpx::eFileReadError, Dpx::eFatal, "Error attempting to read file " + filename + "\n");
		infile.close();
		return err;
	}

	// Read any present image elements
	for (int i = 0; i < 8; ++i)
	{
		if (m_dpx_header.ImageHeader.ImageElement[i].DataOffset != UINT32_MAX)
		{
			std::shared_ptr<HdrDpxImageElement> ie = std::shared_ptr<HdrDpxImageElement>(new HdrDpxImageElement(m_dpx_header, i));
			infile.seekg(m_dpx_header.ImageHeader.ImageElement[i].DataOffset, ios::beg);
			ie->m_dpx_imageelement = m_dpx_header.ImageHeader.ImageElement[i];
			ie->ReadImageData(infile, m_dpx_header.FileHeader.DatumMappingDirection == 0, swapped);
			m_IE.push_back(ie);
		}
	}
	m_file_is_hdr_version = (static_cast<bool>(!strcmp(m_dpx_header.FileHeader.Version, "DPX2.0HDR")));
	return err;
}


std::shared_ptr<HdrDpxImageElement> HdrDpxFile::CreateImageElement(uint32_t width, uint32_t height, Dpx::HdrDpxDescriptor desc, int8_t bpc)
{
	ASSERT_MSG(m_IE.size() < 8, "Cannot allocate more than 8 image elements per file\n");
	m_IE.push_back(std::shared_ptr<HdrDpxImageElement>(new HdrDpxImageElement(width, height, desc, bpc)));
	return(m_IE.back());
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

Dpx::ErrorObject HdrDpxFile::ComputeOffsets()
{
	Dpx::ErrorObject err;
	uint32_t recommended_image_offset, min_image_offset, ud_length;

	if (m_dpx_header.FileHeader.UserSize == UINT32_MAX)
		ud_length = 0;
	else
		ud_length = m_dpx_header.FileHeader.UserSize;
	min_image_offset = ((2048 + ud_length + 3) >> 2) << 2;  // Required by spec: 2048 byte header + user data at 4-byte boundary
	recommended_image_offset = ((2048 + ud_length + 511) >> 9) << 9; // Not required by spec, but if offset is unspecified, chooses 512-byte boundary
	// Does not check location of standards-based metadata

	if (m_dpx_header.FileHeader.ImageOffset == UINT32_MAX)
	{
		m_dpx_header.FileHeader.ImageOffset = recommended_image_offset;
	}
	else if(m_dpx_header.FileHeader.ImageOffset < min_image_offset)
	{
		LOG_ERROR(Dpx::eBadParameter, Dpx::eWarning, "Specified mage offset " + std::to_string(m_dpx_header.FileHeader.ImageOffset) + " conflicts with header");
	}
	return err;
}

Dpx::ErrorObject HdrDpxFile::FillCoreFields()
{
	Dpx::ErrorObject err;
	int i, ne = 0, firstie = -1;

	if (m_dpx_header.FileHeader.Magic != 0x53445058 && m_dpx_header.FileHeader.Magic != UINT32_MAX)
	{
		LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Unrecognized magic number");
	}
	m_dpx_header.FileHeader.Magic = 0x53445058;

	CopyStringN(m_dpx_header.FileHeader.Version, "V2.0HDR", 8);
	if (m_dpx_header.FileHeader.DatumMappingDirection == 0xff)
	{
		m_dpx_header.FileHeader.DatumMappingDirection = 1;
		LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Datum mapping direction not specified, defaulting to 1 (left-to-right)");
	}
	m_dpx_header.FileHeader.GenericSize = 1684;
	m_dpx_header.FileHeader.IndustrySize = 384;
	err += ComputeOffsets();

	for (i = 0; i < m_IE.size(); ++i)
	{
		if (m_IE[i]->m_isinitialized)
		{
			ne++;
			if(firstie < 0)
				firstie = i;
		}
	}
	if(firstie < 0)
		LOG_ERROR(Dpx::eBadParameter, Dpx::eFatal, "No initialized image elements found");

	if (m_dpx_header.ImageHeader.Orientation == UINT16_MAX)
	{
		LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Image orientation not specified, assuming left-to-right, top-to-bottom");
		m_dpx_header.ImageHeader.Orientation = 0;
	}
	if (m_dpx_header.ImageHeader.NumberElements == 0 || m_dpx_header.ImageHeader.NumberElements == UINT16_MAX)
		m_dpx_header.ImageHeader.NumberElements = ne;
	if (m_dpx_header.ImageHeader.PixelsPerLine == UINT32_MAX)
		m_dpx_header.ImageHeader.PixelsPerLine = m_IE[firstie]->m_width;
	if (m_dpx_header.ImageHeader.LinesPerElement == UINT32_MAX)
		m_dpx_header.ImageHeader.LinesPerElement = m_IE[firstie]->m_height;
	for (i = 0; i < ne; ++i)
	{
		if (m_IE[i]->m_isinitialized)
		{
			m_dpx_header.ImageHeader.ImageElement[i] = m_IE[i]->m_dpx_imageelement;

			// Default values for core fields that are not specified
			if (m_dpx_header.ImageHeader.ImageElement[i].BitSize != UINT8_MAX && m_dpx_header.ImageHeader.ImageElement[i].BitSize != m_IE[i]->m_bpc)
				LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Bit size field does not match IE data structure");
			m_dpx_header.ImageHeader.ImageElement[i].BitSize = m_IE[i]->m_bpc;
			m_dpx_header.ImageHeader.ImageElement[i].Descriptor = m_IE[i]->m_descriptor;

			if (m_dpx_header.ImageHeader.ImageElement[i].DataSign == UINT32_MAX)
			{
				LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Data sign for image element " + to_string(i + 1) + " not specified, assuming unsigned");
				m_dpx_header.ImageHeader.ImageElement[i].DataSign = 0;		// default unsigned
			}
			if (m_dpx_header.ImageHeader.ImageElement[i].LowData.d == UINT32_MAX || m_dpx_header.ImageHeader.ImageElement[i].HighData.d == UINT32_MAX)
			{
				if (m_dpx_header.ImageHeader.ImageElement[i].BitSize < 32)
				{
					LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Data range for image element " + to_string(i + 1) + " not specified, assuming full range");
					m_dpx_header.ImageHeader.ImageElement[i].LowData.d = 0;
					m_dpx_header.ImageHeader.ImageElement[i].HighData.d = (1 << m_dpx_header.ImageHeader.ImageElement[i].BitSize) - 1;
				}
				else  // Floating point samples
				{
					LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Data range for image element " + to_string(i + 1) + " not specified, assuming full range (maxval = 1.0)");
					m_dpx_header.ImageHeader.ImageElement[i].LowData.f = (float)0.0;
					m_dpx_header.ImageHeader.ImageElement[i].HighData.f = (float)1.0;
				}
			}
		}
		else {
			// Set ununsed image elements to all 1's
			memset((void *)&(m_dpx_header.ImageHeader.ImageElement[i]), 0xff, sizeof(HDRDPX_IMAGEELEMENT));
		}
		if (m_dpx_header.ImageHeader.ImageElement[i].Transfer == UINT8_MAX)
		{
			LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Transfer characteristic for image element " + to_string(i + 1) + " not specified, assuming BT.709");
			m_dpx_header.ImageHeader.ImageElement[i].Transfer = Dpx::eTransferBT_709;
		}
		if (m_dpx_header.ImageHeader.ImageElement[i].Colorimetric == UINT8_MAX)
		{
			LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Colorimetric field for image element " + to_string(i + 1) + " not specified, assuming BT.709");
			m_dpx_header.ImageHeader.ImageElement[i].Colorimetric = Dpx::eColorimetricBT_709;
		}
		if (m_dpx_header.ImageHeader.ImageElement[i].Packing == UINT16_MAX)
		{
			if (m_dpx_header.ImageHeader.ImageElement[i].BitSize == 10 || m_dpx_header.ImageHeader.ImageElement[i].BitSize == 12)
			{
				LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Packing field not specified for image element " + to_string(i + 1) + ", assuming Method A");
				m_dpx_header.ImageHeader.ImageElement[i].Packing = 1;		// Method A
			}
			else
			{
				LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Packing field not specified for image element " + to_string(i + 1) + ", assuming Packed");
				m_dpx_header.ImageHeader.ImageElement[i].Packing = 0;
			}
		}
		if (m_dpx_header.ImageHeader.ImageElement[i].Encoding == UINT16_MAX)
		{
			LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "Encoding not specified for image element " + to_string(i + 1) + ", assuming uncompressed");
			m_dpx_header.ImageHeader.ImageElement[i].Encoding = 0;		// No compression (default)
		}
		if (m_dpx_header.ImageHeader.ImageElement[i].EndOfLinePadding == UINT32_MAX)
		{
			LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "End of line padding not specified for image element " + to_string(i + 1) + ", assuming no padding");
			m_dpx_header.ImageHeader.ImageElement[i].EndOfLinePadding = 0;
		}
		if (m_dpx_header.ImageHeader.ImageElement[i].EndOfImagePadding == UINT32_MAX)
		{
			LOG_ERROR(Dpx::eMissingCoreField, Dpx::eWarning, "End of image padding not specified for image element " + to_string(i + 1) + ", assuming no padding");
			m_dpx_header.ImageHeader.ImageElement[i].EndOfImagePadding = 0;
		}
		m_IE[i]->m_dpx_imageelement = m_dpx_header.ImageHeader.ImageElement[i];
	}
	return err;
}

Dpx::ErrorObject HdrDpxFile::CheckDataCollisions(uint32_t *ie_data_block_ends)
{
	Dpx::ErrorObject err;
	vector<uint32_t> range_beg, range_end;
	int i, j;

	// Add file header range
	range_beg.push_back(0);
	range_end.push_back(sizeof(HDRDPXFILEFORMAT));

	// Add user data section (if present)
	if (m_dpx_header.FileHeader.UserSize != UINT32_MAX && m_dpx_header.FileHeader.UserSize != 0)
	{
		range_beg.push_back(2048);
		range_end.push_back(m_dpx_header.FileHeader.UserSize + 2048);
	}

	// Add standards-based metadata (if present)
	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UINT32_MAX)
	{
		range_beg.push_back(m_dpx_header.FileHeader.StandardsBasedMetadataOffset);
		range_end.push_back(m_dpx_sbmdata.SbmLength + 132 + m_dpx_header.FileHeader.StandardsBasedMetadataOffset);
	}

	// Add image elements
	for (int i = 0; i<8 && i < m_IE.size(); ++i)
	{
		if (m_IE[i]->m_isinitialized)
		{
			range_beg.push_back(m_IE[i]->m_dpx_imageelement.DataOffset);
			range_end.push_back(ie_data_block_ends[i]);
		}
	}

	// Test ranges for overlap
	for (i = 0; i < range_beg.size(); ++i)
	{
		for (j = i + 1; j < range_beg.size(); ++j)
		{
			if(range_beg[i]==range_beg[j] || range_end[i]==range_end[j])
				LOG_ERROR(Dpx::eBadParameter, Dpx::eFatal, "Invalid file structure: header and/or image data is overlapping");
			if (range_beg[i] < range_beg[j])
			{
				if (range_end[i] > range_beg[j])
					LOG_ERROR(Dpx::eBadParameter, Dpx::eFatal, "Invalid file structure: header and/or image data is overlapping");
			}
			else 
			{
				if (range_end[j] > range_beg[i])
					LOG_ERROR(Dpx::eBadParameter, Dpx::eFatal, "Invalid file structure: header and/or image data is overlapping");
			}
		}
	}
	return err;
}

Dpx::ErrorObject HdrDpxFile::WriteFile(std::string fname)
{
	ofstream outfile(fname, ios::binary | ios::out);
	Dpx::ErrorObject err;
	uint32_t ie_data_block_ends[8];

	m_file_name = fname;
	
	if (!outfile)
	{
		LOG_ERROR(Dpx::eFileOpenError, Dpx::eFatal, "Unable to open file " + fname + " for output");
		return err;
	}

	err = FillCoreFields();

	outfile.seekp(m_dpx_header.FileHeader.ImageOffset, ios::beg);

	// Read any present image elements
	for (int i = 0; i<8 && i < m_IE.size(); ++i)
	{
		if(m_IE[i]->m_isinitialized)
		{
			if (m_dpx_header.ImageHeader.ImageElement[i].DataOffset == UINT32_MAX)
				m_dpx_header.ImageHeader.ImageElement[i].DataOffset = (uint32_t)outfile.tellp();

			m_IE[i]->WriteImageData(outfile, m_dpx_header.FileHeader.DatumMappingDirection == 0, false);
			ie_data_block_ends[i] = (uint32_t)outfile.tellp();
		}
	}
	if (outfile.bad())
	{
		LOG_ERROR(Dpx::eFileWriteError, Dpx::eFatal, "Error writing DPX image data");
		return err;
	}

	// Place standards-based metadata at end if location is unspecified
	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset == UINT32_MAX && m_sbm_present)
		m_dpx_header.FileHeader.StandardsBasedMetadataOffset = (uint32_t)outfile.tellp();

	// Range check where headers & image data go to ensure there are no collisions
	err += CheckDataCollisions(ie_data_block_ends);

	if (m_dpx_header.FileHeader.StandardsBasedMetadataOffset != UINT32_MAX)
	{
		// Write standards-based metadata
		outfile.seekp(m_dpx_header.FileHeader.StandardsBasedMetadataOffset, ios::beg);
		if ((m_byteorder == Dpx::eLSBF && m_machine_is_mbsf) || (m_byteorder == Dpx::eMSBF && !m_machine_is_mbsf))
			ByteSwapSbmHeader();
		outfile.write((char *)&m_dpx_sbmdata, 132 + m_dpx_sbmdata.SbmLength);
		if ((m_byteorder == Dpx::eLSBF && m_machine_is_mbsf) || (m_byteorder == Dpx::eMSBF && !m_machine_is_mbsf))
			ByteSwapSbmHeader();
	}

	outfile.seekp(0, ios::beg);

	// Swap before writing header
	if ((m_byteorder == Dpx::eLSBF && m_machine_is_mbsf) || (m_byteorder == Dpx::eMSBF && !m_machine_is_mbsf))
		ByteSwapHeader();
	outfile.write((char *)&m_dpx_header, sizeof(HDRDPXFILEFORMAT));
	if ((m_byteorder == Dpx::eLSBF && m_machine_is_mbsf) || (m_byteorder == Dpx::eMSBF && !m_machine_is_mbsf))
		ByteSwapHeader();


	if (outfile.bad())
	{
		LOG_ERROR(Dpx::eFileWriteError, Dpx::eFatal, "Error writing DPX image data");
		outfile.close();
		return err;
	}
	m_file_is_hdr_version = (static_cast<bool>(!strcmp(m_dpx_header.FileHeader.Version, "DPX2.0HDR")));
	return err;
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
string HdrDpxFile::DumpHeader()
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

Dpx::ErrorObject HdrDpxFile::Validate()
{
	Dpx::ErrorObject err;
	return err;
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

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsString f, std::string s)
{
	switch (f)
	{
	case Dpx::eFileName:
		if(CopyStringN(m_dpx_header.FileHeader.FileName, s, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified file name (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eTimeDate:
		if (CopyStringN(m_dpx_header.FileHeader.TimeDate, s, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified time & date (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eCreator:
		if (CopyStringN(m_dpx_header.FileHeader.Creator, s, CREATOR_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified creator (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eProject:
		if (CopyStringN(m_dpx_header.FileHeader.Project, s, PROJECT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified project (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eCopyright:
		if (CopyStringN(m_dpx_header.FileHeader.Copyright, s, COPYRIGHT_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified copyright (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eSourceFileName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceFileName, s, FILE_NAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source file name (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eSourceTimeDate:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.SourceTimeDate, s, TIMEDATE_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified source time & date (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eInputName:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputName, s, INPUTNAME_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input name (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eInputSN:
		if (CopyStringN(m_dpx_header.SourceInfoHeader.InputSN, s, INPUTSN_SIZE))
			m_warn_messages.push_back("SetHeader(): Specified input SN (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eFilmMfgId:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmMfgId, s, 2))
			m_warn_messages.push_back("SetHeader(): Specified film mfg id (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eFilmType:
		if (CopyStringN(m_dpx_header.FilmHeader.FilmType, s, 2))
			m_warn_messages.push_back("SetHeader(): Specified film type (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eOffsetPerfs:
		if (CopyStringN(m_dpx_header.FilmHeader.OffsetPerfs, s, 4))
			m_warn_messages.push_back("SetHeader(): Specified offset perfs (" + s + ") exceeds header field size\n");
		break;
	case Dpx::ePrefix:
		if (CopyStringN(m_dpx_header.FilmHeader.Prefix, s, 6))
			m_warn_messages.push_back("SetHeader(): Specified prefix (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eCount:
		if (CopyStringN(m_dpx_header.FilmHeader.Count, s, 4))
			m_warn_messages.push_back("SetHeader(): Specified film count (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eFormat:
		if (CopyStringN(m_dpx_header.FilmHeader.Format, s, 32))
			m_warn_messages.push_back("SetHeader(): Specified film format (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eFrameId:
		if (CopyStringN(m_dpx_header.FilmHeader.FrameId, s, 32))
			m_warn_messages.push_back("SetHeader(): Specified frame ID (" + s + ") exceeds header field size\n");
		break;
	case Dpx::eSlateInfo:
		if (CopyStringN(m_dpx_header.FilmHeader.SlateInfo, s, 100))
			m_warn_messages.push_back("SetHeader(): Specified slate info (" + s + ") exceeds header field size\n");
		break;
	}
}

string HdrDpxFile::CopyToStringN(char *src, int l)
{
	int n;
	string s ("");

	for (n = 0; n < l; ++n)
	{
		if (*src == '\0')
			return s;
		else
			s.push_back(*(src++));
	}
	return s;
}

std::string HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsString f)
{
	switch (f)
	{
	case Dpx::eFileName:
		return CopyToStringN(m_dpx_header.FileHeader.FileName, FILE_NAME_SIZE);
	case Dpx::eTimeDate:
		return CopyToStringN(m_dpx_header.FileHeader.TimeDate, TIMEDATE_SIZE);
	case Dpx::eCreator:
		return CopyToStringN(m_dpx_header.FileHeader.Creator, CREATOR_SIZE);
	case Dpx::eProject:
		return CopyToStringN(m_dpx_header.FileHeader.Project, PROJECT_SIZE);
	case Dpx::eCopyright:
		return CopyToStringN(m_dpx_header.FileHeader.Copyright, COPYRIGHT_SIZE);
	case Dpx::eSourceFileName:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.SourceFileName, FILE_NAME_SIZE);
	case Dpx::eSourceTimeDate:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.SourceTimeDate, TIMEDATE_SIZE);
	case Dpx::eInputName:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.InputName, INPUTNAME_SIZE);
	case Dpx::eInputSN:
		return CopyToStringN(m_dpx_header.SourceInfoHeader.InputSN, INPUTSN_SIZE);
	case Dpx::eFilmMfgId:
		return CopyToStringN(m_dpx_header.FilmHeader.FilmMfgId, 2);
	case Dpx::eFilmType:
		return CopyToStringN(m_dpx_header.FilmHeader.FilmType, 2);
	case Dpx::eOffsetPerfs:
		return CopyToStringN(m_dpx_header.FilmHeader.OffsetPerfs, 4);
	case Dpx::ePrefix:
		return CopyToStringN(m_dpx_header.FilmHeader.Prefix, 6);
	case Dpx::eCount:
		return CopyToStringN(m_dpx_header.FilmHeader.Count, 4);
	case Dpx::eFormat:
		return CopyToStringN(m_dpx_header.FilmHeader.Format, 32);
	case Dpx::eFrameId:
		return CopyToStringN(m_dpx_header.FilmHeader.FrameId, 32);
	case Dpx::eSlateInfo:
		return CopyToStringN(m_dpx_header.FilmHeader.SlateInfo, 100);
	}
	return "";
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsU32 f, uint32_t d)
{
	switch (f)
	{
	case Dpx::eImageOffset:
		m_dpx_header.FileHeader.ImageOffset = d;
		break;
	case Dpx::eFileSize:
		m_dpx_header.FileHeader.FileSize = d;
		break;
	case Dpx::eGenericSize:
		m_dpx_header.FileHeader.GenericSize = d;
		break;
	case Dpx::eIndustrySize:
		m_dpx_header.FileHeader.IndustrySize = d;
		break;
	case Dpx::eUserSize:
		m_dpx_header.FileHeader.UserSize = d;
		break;
	case Dpx::eEncryptKey:
		m_dpx_header.FileHeader.EncryptKey = d;
		break;
	case Dpx::eStandardsBasedMetadataOffset:
		m_dpx_header.FileHeader.StandardsBasedMetadataOffset = d;
		break;
	case Dpx::ePixelsPerLine:
		m_dpx_header.ImageHeader.PixelsPerLine = d;
		break;
	case Dpx::eLinesPerElement:
		m_dpx_header.ImageHeader.LinesPerElement = d;
		break;
	case Dpx::eXOffset:
		m_dpx_header.SourceInfoHeader.XOffset = d;
		break;
	case Dpx::eYOffset:
		m_dpx_header.SourceInfoHeader.YOffset = d;
		break;
	case Dpx::eXOriginalSize:
		m_dpx_header.SourceInfoHeader.XOriginalSize = d;
		break;
	case Dpx::eYOriginalSize:
		m_dpx_header.SourceInfoHeader.YOriginalSize = d;
		break;
	case Dpx::eAspectRatioH:
		m_dpx_header.SourceInfoHeader.AspectRatio[0] = d;
		break;
	case Dpx::eAspectRatioV:
		m_dpx_header.SourceInfoHeader.AspectRatio[1] = d;
		break;
	case Dpx::eFramePosition:
		m_dpx_header.FilmHeader.FramePosition = d;
		break;
	case Dpx::eSequenceLen:
		m_dpx_header.FilmHeader.SequenceLen = d;
		break;
	case Dpx::eHeldCount:
		m_dpx_header.FilmHeader.HeldCount = d;
	}
}

uint32_t HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsU32 f)
{
	switch (f)
	{
	case Dpx::eImageOffset:
		return(m_dpx_header.FileHeader.ImageOffset);
	case Dpx::eFileSize:
		return(m_dpx_header.FileHeader.FileSize);
	case Dpx::eGenericSize:
		return(m_dpx_header.FileHeader.GenericSize);
	case Dpx::eIndustrySize:
		return(m_dpx_header.FileHeader.IndustrySize);
	case Dpx::eUserSize:
		return(m_dpx_header.FileHeader.UserSize);
	case Dpx::eEncryptKey:
		return(m_dpx_header.FileHeader.EncryptKey);
	case Dpx::eStandardsBasedMetadataOffset:
		return(m_dpx_header.FileHeader.StandardsBasedMetadataOffset);
	case Dpx::ePixelsPerLine:
		return(m_dpx_header.ImageHeader.PixelsPerLine);
	case Dpx::eLinesPerElement:
		return(m_dpx_header.ImageHeader.LinesPerElement);
	case Dpx::eXOffset:
		return(m_dpx_header.SourceInfoHeader.XOffset);
	case Dpx::eYOffset:
		return(m_dpx_header.SourceInfoHeader.YOffset);
	case Dpx::eXOriginalSize:
		return(m_dpx_header.SourceInfoHeader.XOriginalSize);
	case Dpx::eYOriginalSize:
		return(m_dpx_header.SourceInfoHeader.YOriginalSize);
	case Dpx::eAspectRatioH:
		return(m_dpx_header.SourceInfoHeader.AspectRatio[0]);
	case Dpx::eAspectRatioV:
		return(m_dpx_header.SourceInfoHeader.AspectRatio[1]);
	case Dpx::eFramePosition:
		return(m_dpx_header.FilmHeader.FramePosition);
	case Dpx::eSequenceLen:
		return(m_dpx_header.FilmHeader.SequenceLen);
	case Dpx::eHeldCount:
		return(m_dpx_header.FilmHeader.HeldCount);
	}
	return 0;
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsU16 f, uint16_t d)
{
	switch (f)
	{
	case Dpx::eNumberElements:
		m_dpx_header.ImageHeader.NumberElements = d;
		break;
	case Dpx::eBorderXL:
		m_dpx_header.SourceInfoHeader.Border[0] = d;
		break;
	case Dpx::eBorderXR:
		m_dpx_header.SourceInfoHeader.Border[1] = d;
		break;
	case Dpx::eBorderYT:
		m_dpx_header.SourceInfoHeader.Border[2] = d;
		break;
	case Dpx::eBorderYB:
		m_dpx_header.SourceInfoHeader.Border[3] = d;
	}
}

uint16_t HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsU16 f)
{
	switch (f)
	{
	case Dpx::eNumberElements:
		return(m_dpx_header.ImageHeader.NumberElements);
	case Dpx::eBorderXL:
		return(m_dpx_header.SourceInfoHeader.Border[0]);
	case Dpx::eBorderXR:
		return(m_dpx_header.SourceInfoHeader.Border[1]);
	case Dpx::eBorderYT:
		return(m_dpx_header.SourceInfoHeader.Border[2]);
	case Dpx::eBorderYB:
		return(m_dpx_header.SourceInfoHeader.Border[3]);
	}
	return 0;
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsR32 f, float d)
{
	switch (f)
	{
	case Dpx::eXCenter:
		m_dpx_header.SourceInfoHeader.XCenter = d;
		break;
	case Dpx::eYCenter:
		m_dpx_header.SourceInfoHeader.YCenter = d;
		break;
	case Dpx::eFilmFrameRate:
		m_dpx_header.FilmHeader.FrameRate = d;
		break;
	case Dpx::eShutterAngle:
		m_dpx_header.FilmHeader.ShutterAngle = d;
		break;
	case Dpx::eHorzSampleRate:
		m_dpx_header.TvHeader.HorzSampleRate = d;
		break;
	case Dpx::eVertSampleRate:
		m_dpx_header.TvHeader.VertSampleRate = d;
		break;
	case Dpx::eTvFrameRate:
		m_dpx_header.TvHeader.FrameRate = d;
		break;
	case Dpx::eTimeOffset:
		m_dpx_header.TvHeader.TimeOffset = d;
		break;
	case Dpx::eGamma:
		m_dpx_header.TvHeader.Gamma = d;
		break;
	case Dpx::eBlackLevel:
		m_dpx_header.TvHeader.BlackLevel = d;
		break;
	case Dpx::eBlackGain:
		m_dpx_header.TvHeader.BlackGain = d;
		break;
	case Dpx::eBreakpoint:
		m_dpx_header.TvHeader.Breakpoint = d;
		break;
	case Dpx::eWhiteLevel:
		m_dpx_header.TvHeader.WhiteLevel = d;
		break;
	case Dpx::eIntegrationTimes:
		m_dpx_header.TvHeader.IntegrationTimes = d;
	}
}

float HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsR32 f)
{
	switch (f)
	{
	case Dpx::eXCenter:
		return(m_dpx_header.SourceInfoHeader.XCenter);
	case Dpx::eYCenter:
		return(m_dpx_header.SourceInfoHeader.YCenter);
	case Dpx::eFilmFrameRate:
		return(m_dpx_header.FilmHeader.FrameRate);
	case Dpx::eShutterAngle:
		return(m_dpx_header.FilmHeader.ShutterAngle);
	case Dpx::eHorzSampleRate:
		return(m_dpx_header.TvHeader.HorzSampleRate);
	case Dpx::eVertSampleRate:
		return(m_dpx_header.TvHeader.VertSampleRate);
	case Dpx::eTvFrameRate:
		return(m_dpx_header.TvHeader.FrameRate);
	case Dpx::eTimeOffset:
		return(m_dpx_header.TvHeader.TimeOffset);
	case Dpx::eGamma:
		return(m_dpx_header.TvHeader.Gamma);
	case Dpx::eBlackLevel:
		return(m_dpx_header.TvHeader.BlackLevel);
	case Dpx::eBlackGain:
		return(m_dpx_header.TvHeader.BlackGain);
	case Dpx::eBreakpoint:
		return(m_dpx_header.TvHeader.Breakpoint);
	case Dpx::eWhiteLevel:
		return(m_dpx_header.TvHeader.WhiteLevel);
	case Dpx::eIntegrationTimes:
		return(m_dpx_header.TvHeader.IntegrationTimes);
	}
	return 0.0;
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsU8 f, uint8_t d)
{
	switch (f)
	{
	case Dpx::eFieldNumber:
		m_dpx_header.TvHeader.FieldNumber = d;
	}
}

uint8_t HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsU8 f)
{
	switch(f)
	{
	case Dpx::eFieldNumber:
		return(m_dpx_header.TvHeader.FieldNumber);
	}
	return 0;
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsDittoKey f, Dpx::HdrDpxDittoKey d)
{
	m_dpx_header.FileHeader.DittoKey = static_cast<uint32_t>(d);
}

Dpx::HdrDpxDittoKey HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsDittoKey f)
{
	return static_cast<Dpx::HdrDpxDittoKey>(m_dpx_header.FileHeader.DittoKey);
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsDatumMappingDirection f, Dpx::HdrDpxDatumMappingDirection d)
{
	m_dpx_header.FileHeader.DatumMappingDirection = static_cast<uint8_t>(d);
}

Dpx::HdrDpxDatumMappingDirection HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsDatumMappingDirection f)
{
	return static_cast<Dpx::HdrDpxDatumMappingDirection>(m_dpx_header.FileHeader.DatumMappingDirection);
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsOrientation f, Dpx::HdrDpxOrientation d)
{
	m_dpx_header.ImageHeader.Orientation = static_cast<uint16_t>(d);
}

Dpx::HdrDpxOrientation HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsOrientation f)
{
	return static_cast<Dpx::HdrDpxOrientation>(m_dpx_header.ImageHeader.Orientation);
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsInterlace f, Dpx::HdrDpxInterlace d)
{
	m_dpx_header.TvHeader.Interlace = static_cast<uint8_t>(d);
}

Dpx::HdrDpxInterlace HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsInterlace f)
{
	return static_cast<Dpx::HdrDpxInterlace>(m_dpx_header.TvHeader.Interlace);
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsVideoSignal f, Dpx::HdrDpxVideoSignal d)
{
	m_dpx_header.TvHeader.VideoSignal = static_cast<uint8_t>(d);
}

Dpx::HdrDpxVideoSignal HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsVideoSignal f)
{
	return static_cast<Dpx::HdrDpxVideoSignal>(m_dpx_header.TvHeader.VideoSignal);
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsVideoIdentificationCode f, Dpx::HdrDpxVideoIdentificationCode d)
{
	m_dpx_header.TvHeader.VideoIdentificationCode = static_cast<uint8_t>(d);
}

Dpx::HdrDpxVideoIdentificationCode HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsVideoIdentificationCode f)
{
	return static_cast<Dpx::HdrDpxVideoIdentificationCode>(m_dpx_header.TvHeader.VideoIdentificationCode);
}

void HdrDpxFile::SetHeader(Dpx::HdrDpxFieldsByteOrder f, Dpx::HdrDpxByteOrder d)
{
	m_byteorder = d;
}

Dpx::HdrDpxByteOrder HdrDpxFile::GetHeader(Dpx::HdrDpxFieldsByteOrder f)
{
	return m_byteorder;
}

std::list<std::string> HdrDpxFile::GetWarningList(void)
{
	return m_warn_messages;
}
