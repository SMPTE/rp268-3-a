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

//! \file main.cpp  Defines the entry point for the console application.
//
//  These are wrapper functions for Win32 to be able to call the mainline in codec_main.c

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hdr_dpx.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

#ifdef _MSC_VER
#include <tchar.h>

int dump_dpx(int argc, char **argv);

// returns number of TCHARs in string
int wstrlen(_TCHAR * wstr)
{
    int l_idx = 0;
    while (((char*)wstr)[l_idx]!=0) l_idx+=2;
    return l_idx;
}

  
// Allocate char string and copy TCHAR->char->string
char * wstrdup(_TCHAR * wSrc)
{
    int l_idx=0;
    int l_len = wstrlen(wSrc);
    char * l_nstr = (char*)malloc(l_len);
    if (l_nstr) {
        do {
           l_nstr[l_idx] = (char)wSrc[l_idx];
           l_idx++;
        } while ((char)wSrc[l_idx]!=0);
	    l_nstr[l_idx] = 0;
    }
    return l_nstr;
}
 
  
 
// allocate argn structure parallel to argv
// argn must be released
char ** allocate_argn (int argc, _TCHAR* argv[])
{
    char ** l_argn = (char **)malloc(argc * sizeof(char*));
	assert(l_argn != NULL);
    for (int idx=0; idx<argc; idx++) {
        l_argn[idx] = wstrdup(argv[idx]);
    }
    return l_argn;
}
 
// release argn and its content
void release_argn(int argc, char ** nargv)
{
    for (int idx=0; idx<argc; idx++) {
        free(nargv[idx]);
    }
    free(nargv);
}

int _tmain(int argc, _TCHAR* argv[])
{
	char ** argn = allocate_argn(argc, argv);

	dump_dpx(argc, argn);

	release_argn(argc, argn);
	return(0);
}
#endif

static void dump_error_log(std::string logmessage, Dpx::HdrDpxFile &f)
{
	Dpx::ErrorCode code;
	Dpx::ErrorSeverity severity;
	std::string msg;

	std::cerr << logmessage;

	for (int i = 0; i < f.GetNumErrors(); ++i)
	{
		f.GetError(i, code, severity, msg);
		std::cerr << msg << std::endl;
	}
}

inline string tohex(uint8_t v)
{
	stringstream ss;
	
	ss << setfill('0');

	ss << hex << setw(2) << v;

	return ss.str();
}

std::string datum_label_to_ext(Dpx::DatumLabel dl, uint8_t chroma_idx)
{
	switch (dl)
	{
	case Dpx::DATUM_A:
		return "a";
	case Dpx::DATUM_B:
		return "b";
	case Dpx::DATUM_C:
		if (chroma_idx == 0)
			return "u";
		else
			return "v";
	case Dpx::DATUM_CB:
		return "u";
	case Dpx::DATUM_COMPOSITE:
		return "comp";
	case Dpx::DATUM_CR:
		return "v";
	case Dpx::DATUM_G:
		return "g";
	case Dpx::DATUM_R:
		return "r";
	case Dpx::DATUM_Y:
		return "y";
	case Dpx::DATUM_Z:
		return "z";
	case Dpx::DATUM_UNSPEC:
		return "g1";
	case Dpx::DATUM_UNSPEC2:
		return "g2";
	case Dpx::DATUM_UNSPEC3:
		return "g3";
	case Dpx::DATUM_UNSPEC4:
		return "g4";
	case Dpx::DATUM_UNSPEC5:
		return "g5";
	case Dpx::DATUM_UNSPEC6:
		return "g6";
	case Dpx::DATUM_UNSPEC7:
		return "g7";
	case Dpx::DATUM_UNSPEC8:
		return "g8";
	}
	return "unk";
}

inline double int_to_norm_double(int32_t i, bool is_chroma, float lowcode, bool is_signed, uint8_t bit_depth_in)
{
	// Output is normalized [0..1.0]
	if (is_signed)  // Signed assumes full range
	{
		return static_cast<double>(i) / ((1 << bit_depth_in) - 1);
	}
	if (is_chroma && lowcode > 1)  // Any low code value > 1 assumes limited range. Output floats are normalized [0..1.0]
		return (static_cast<double>(i) - (1 << (bit_depth_in - 1))) / (240 << (bit_depth_in - 8)) + 0.5;
	else if (!is_chroma && lowcode > 1)
		return (static_cast<double>(i) - (16 << (bit_depth_in - 8))) / (219 << (bit_depth_in - 8));
	else   // Full range
		return static_cast<double>(i) / ((1 << bit_depth_in) - 1);
}

inline uint16_t norm_double_to_uint(double f, uint8_t bits, bool full_range, bool is_chroma)
{
	if (full_range)
	{
		return static_cast<uint16_t>(f * ((1 << bits) - 1) + 0.5);
	}
	else
	{
		if (is_chroma)
			return static_cast<uint16_t>((f - 0.5) * (240 << (bits - 8)) + (1 << (bits - 1)) + 0.5);
		else
			return static_cast<uint16_t>(f * (219 << (bits - 8)) + (16 << (bits - 8)) + 0.5);
	}
}

void write_row_to_file(std::vector<int32_t> rowdata, uint8_t datum_offset, uint8_t num_components, uint8_t bit_depth_in, uint8_t bit_depth_conv, float hicode, float lowcode, bool is_signed, bool write_full_range, bool is_chroma, ofstream &raw_fp)
{
	if (bit_depth_conv == 0)
		bit_depth_conv = bit_depth_in;

	if (bit_depth_conv == 32)
	{
		std::vector<float> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = static_cast<float>(int_to_norm_double(rowdata[x * num_components + datum_offset], is_chroma, lowcode, is_signed, bit_depth_in));
		}
		raw_fp.write((char *)outrow.data(), sizeof(float) * outrow.size());
	}
	else if (bit_depth_conv == 64)
	{
		std::vector<double> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = (int_to_norm_double(rowdata[x * num_components + datum_offset], is_chroma, lowcode, is_signed, bit_depth_in));
		}
		raw_fp.write((char *)outrow.data(), sizeof(double) * outrow.size());
	}
	// Converts to integer (unsigned) format
	else if (bit_depth_conv > 8)
	{
		std::vector<uint16_t> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = norm_double_to_uint(int_to_norm_double(rowdata[x * num_components + datum_offset], is_chroma, lowcode, is_signed, bit_depth_in), bit_depth_conv, write_full_range, is_chroma);
		}
		raw_fp.write((char *)outrow.data(), sizeof(uint16_t) * outrow.size());
	}
	else
	{
		std::vector<uint8_t> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = static_cast<uint8_t>(norm_double_to_uint(int_to_norm_double(rowdata[x * num_components + datum_offset], is_chroma, lowcode, is_signed, bit_depth_in), bit_depth_conv, write_full_range, is_chroma));
		}
		raw_fp.write((char *)outrow.data(), sizeof(uint8_t) * outrow.size());
	}

}



void write_row_to_file(std::vector<float> rowdata, uint8_t datum_offset, uint8_t num_components, uint8_t bit_depth_conv, float hicode, float lowcode, bool write_full_range, bool is_chroma, ofstream &raw_fp)
{

	// Just passes through to float / double formats
	if (bit_depth_conv == 32 || bit_depth_conv == 0)
	{
		std::vector<float> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = rowdata[x * num_components + datum_offset];
		}
		raw_fp.write((char *)outrow.data(), sizeof(float) * outrow.size());
	}
	else if (bit_depth_conv == 64)
	{
		std::vector<double> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = rowdata[x * num_components + datum_offset];
		}
		raw_fp.write((char *)outrow.data(), sizeof(double) * outrow.size());
	}
	// Converts to integer (unsigned) format
	else if (bit_depth_conv > 8)
	{
		std::vector<uint16_t> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = norm_double_to_uint(rowdata[x * num_components + datum_offset], bit_depth_conv, write_full_range, is_chroma);
		}
		raw_fp.write((char *)outrow.data(), sizeof(uint16_t) * outrow.size());
	}
	else
	{
		std::vector<uint8_t> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = static_cast<uint8_t>(norm_double_to_uint(rowdata[x * num_components + datum_offset], bit_depth_conv, write_full_range, is_chroma));
		}
		raw_fp.write((char *)outrow.data(), sizeof(uint16_t) * outrow.size());
	}

}

void write_row_to_file(std::vector<double> rowdata, uint8_t datum_offset, uint8_t num_components, uint8_t bit_depth_conv, float hicode, float lowcode, bool write_full_range, bool is_chroma, ofstream &raw_fp)
{

	// Just passes through to float / double formats
	if (bit_depth_conv == 32)
	{
		std::vector<float> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = static_cast<float>(rowdata[x * num_components + datum_offset]);
		}
		raw_fp.write((char *)outrow.data(), sizeof(float) * outrow.size());
	}
	else if (bit_depth_conv == 64 || bit_depth_conv == 0)
	{
		std::vector<double> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = rowdata[x * num_components + datum_offset];
		}
		raw_fp.write((char *)outrow.data(), sizeof(double) * outrow.size());
	}
	// Converts to integer (unsigned) format
	else if (bit_depth_conv > 8)
	{
		std::vector<uint16_t> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = norm_double_to_uint(rowdata[x * num_components + datum_offset], bit_depth_conv, write_full_range, is_chroma);
		}
		raw_fp.write((char *)outrow.data(), sizeof(uint16_t) * outrow.size());
	}
	else
	{
		std::vector<uint8_t> outrow;
		outrow.resize(rowdata.size() / num_components);
		for (int x = 0; x < rowdata.size() / num_components; ++x)
		{
			outrow[x] = static_cast<uint8_t>(norm_double_to_uint(rowdata[x * num_components + datum_offset], bit_depth_conv, write_full_range, is_chroma));
		}
		raw_fp.write((char *)outrow.data(), sizeof(uint16_t) * outrow.size());
	}

}

void write_raw_datum(float rowdata, uint8_t bit_depth_conv, float hicode, float lowcode, bool write_full_range, bool is_chroma, std::shared_ptr<std::ofstream> raw_fp)
{

	// Just passes through to float / double formats
	if (bit_depth_conv == 32 || bit_depth_conv == 0)
	{
		float out = rowdata;
		raw_fp->write((char *)&out, sizeof(float));
	}
	else if (bit_depth_conv == 64)
	{
		double out;
		out = rowdata;
		raw_fp->write((char *)&out, sizeof(double));
	}
	// Converts to integer (unsigned) format
	else if (bit_depth_conv > 8)
	{
		uint16_t out;
		out = norm_double_to_uint(rowdata, bit_depth_conv, write_full_range, is_chroma);
		raw_fp->write((char *)&out, sizeof(uint16_t));
	}
	else
	{
		uint8_t out;
		out = static_cast<uint8_t>(norm_double_to_uint(rowdata, bit_depth_conv, write_full_range, is_chroma));
		raw_fp->write((char *)&out, sizeof(uint8_t));
	}
}

void write_raw_datum(double rowdata, uint8_t bit_depth_conv, float hicode, float lowcode, bool write_full_range, bool is_chroma, std::shared_ptr<std::ofstream> raw_fp)
{

	// Just passes through to float / double formats
	if (bit_depth_conv == 32)
	{
		float out = static_cast<float>(rowdata);
		raw_fp->write((char *)&out, sizeof(float));
	}
	else if (bit_depth_conv == 64 || bit_depth_conv == 0)
	{
		double out;
		out = rowdata;
		raw_fp->write((char *)&out, sizeof(double));
	}
	// Converts to integer (unsigned) format
	else if (bit_depth_conv > 8)
	{
		uint16_t out;
		out = norm_double_to_uint(rowdata, bit_depth_conv, write_full_range, is_chroma);
		raw_fp->write((char *)&out, sizeof(uint16_t));
	}
	else
	{
		uint8_t out;
		out = static_cast<uint8_t>(norm_double_to_uint(rowdata, bit_depth_conv, write_full_range, is_chroma));
		raw_fp->write((char *)&out, sizeof(uint8_t));
	}
}

int datacount;
void write_raw_datum(int32_t rowdata, uint8_t bit_depth_in, uint8_t bit_depth_conv, float hicode, float lowcode, bool is_signed, bool write_full_range, bool is_chroma, std::shared_ptr<ofstream> raw_fp)
{
	if (bit_depth_conv == 0)
		bit_depth_conv = bit_depth_in;

	if (bit_depth_conv == 32)
	{
		float outrow;
		outrow = static_cast<float>(int_to_norm_double(rowdata, is_chroma, lowcode, is_signed, bit_depth_in));
		raw_fp->write((char *)&outrow, sizeof(float));
	}
	else if (bit_depth_conv == 64)
	{
		double outrow;
		outrow = (int_to_norm_double(rowdata, is_chroma, lowcode, is_signed, bit_depth_in));
		raw_fp->write((char *)&outrow, sizeof(double));
	}
	// Converts to integer (unsigned) format
	else if (bit_depth_conv > 8)
	{
		uint16_t outrow;
		outrow = norm_double_to_uint(int_to_norm_double(rowdata, is_chroma, lowcode, is_signed, bit_depth_in), bit_depth_conv, write_full_range, is_chroma);
		raw_fp->write((char *)&outrow, sizeof(uint16_t));
	}
	else
	{
		uint8_t outrow;
		outrow = static_cast<uint8_t>(norm_double_to_uint(int_to_norm_double(rowdata, is_chroma, lowcode, is_signed, bit_depth_in), bit_depth_conv, write_full_range, is_chroma));
		raw_fp->write((char *)&outrow, sizeof(uint8_t));
	}
	datacount++;

}


void open_raw_files(std::vector<std::shared_ptr<std::ofstream>> &fp_list, uint8_t ie_idx, Dpx::HdrDpxImageElement *ie, std::string raw_base_name, uint8_t &alt_chroma, std::vector<bool> &is_chroma)
{
	std::shared_ptr<std::ofstream> first_y_fp = NULL;
	std::shared_ptr<std::ofstream> first_a_fp = NULL;
	std::shared_ptr<std::ofstream> new_fp = NULL;
	int i = 0;

	alt_chroma = UINT8_MAX;
	for (auto c : ie->GetDatumLabels())
	{
		is_chroma.push_back((c == Dpx::DATUM_C) || (c == Dpx::DATUM_CB) || (c == Dpx::DATUM_CR));
		if (c == Dpx::DATUM_Y2)
			fp_list.push_back(first_y_fp);
		else if (c == Dpx::DATUM_A2)
			fp_list.push_back(first_a_fp);
		else
		{
			if (c == Dpx::DATUM_C)
				alt_chroma = static_cast<uint8_t>(fp_list.size());
			new_fp = static_cast<std::shared_ptr<std::ofstream>>(new std::ofstream);
			std::string rawfilename(raw_base_name + "." + std::to_string(ie_idx) + "." + datum_label_to_ext(c, 0));
			new_fp->open(rawfilename, ios::out | ios::binary);
			fp_list.push_back(new_fp);
			if (c == Dpx::DATUM_Y)
				first_y_fp = new_fp;
			if (c == Dpx::DATUM_A)
				first_a_fp = new_fp;
		}
	}

	if (alt_chroma != UINT8_MAX)
	{
		new_fp = static_cast<std::shared_ptr<std::ofstream>>(new std::ofstream);
		std::string rawfilename(raw_base_name + "." + std::to_string(ie_idx) + "." + datum_label_to_ext(Dpx::DATUM_C, 1));
		new_fp->open(rawfilename, ios::out | ios::binary);
		fp_list.push_back(new_fp);
	}
}

// Example calling sequence for HDR DPX read:
#ifdef _MSC_VER
int dump_dpx(int argc, char **argv)
#else
int main(int argc, char *argv[])
#endif
{
	Dpx::HdrDpxImageElement *ie;
	std::string userid, sbmdesc;
	std::vector<uint8_t> userdata, sbmdata;
	int bit_depth_conv = 0;
	std::string plane_order;
	bool output_raw = false;
	std::string raw_base_name;
	bool write_full_range = false;;

	if (argc < 2)
	{
		std::cerr << "Usage: dump_dpx <dpxfile> (-rawout <out_rawfile_base> (-plane_order <plane_order>) (-bit_depth_conv <bit_depth_conv>) (-dump_full_range))\n";
		std::cerr << "  <dpxfile> - DPX file to dump\n";
		std::cerr << "  <out_rawfile_base> - Output filename base name. The output extension will be .#.<plane_type>, where # is the first image element number and <plane_type> is y,u,v,r,g,b,a, or g0-7\n\n";
		std::cerr << "  <bit_depth_conv> - Convert DPX file to specified bit depth. If unspecified, uses bit depth from DPX.\n";
		std::cerr << "  -dump_full_range - If present, the output raw files will be full range; if absent, the output raw files will be limited range (float/double output always scaled to 0-1.0)\n";
		std::cerr << "  A sample in a file is stored within the smallest of (1, 2, 4, or 8) bytes that fits a <bit_depth_conv>-bit sample.\n";
		std::cerr << "  For example, if we have an 8-bit DPX image element and bit_depth_conv = 12, then the IE is converted from 8 to 12 bits (<<4) and\n";
		std::cerr << "  each 12-bit sample is placed in a 2-byte (16-bit) field in the raw file output (right-justified).\n\n";
		std::cerr << "  Byte order for raw file is always the native ordering of the machine\n\n";
		std::cerr << "  A suggested means of sanity checking an image would be to specify -rawout out -bit_depth_conv = 8; the resulting file can then be converted to a desired\n";
		std::cerr << "  viewing format using ffmpeg with the option -pix_fmt set to yuv420p, yuv422p, yuv444p, yuva420p, yuva422p, yuva444p rgb, or rgba based on the file type\n";
		return 0;
	}
	std::string filename(argv[1]);

	for (int i = 2; i < argc; ++i)
	{
		std::string arg = std::string(argv[i]);
		if (!arg.compare("-rawout"))
		{
			output_raw = true;
			raw_base_name = std::string(argv[++i]);
		}
		else if (!arg.compare("-plane_order"))
		{
			plane_order = std::string(argv[++i]);
		}
		else if (!arg.compare("-bit_depth_conv"))
		{
			bit_depth_conv = atoi(argv[++i]);
		}
		else if (!arg.compare("-dump_full_range"))
		{
			write_full_range = true;
		}
		else
		{
			std::cerr << "Unrecognized argument " << arg << "\n";
			return -1;
		}
	}

	// Shorthand to open a DPX file for reading
	Dpx::HdrDpxFile  f(filename);

	if (!f.IsOk())   // can't open, unrecognized format
	{
		dump_error_log("Read failed:\n", f);
		return 1;// Code to handle failure to open
	}

	if (!f.IsHdr())
	{
		// Code to handle V1.0/V2.0 DPX files

		std::cout << "DPX header:\n" << f.DumpHeader();
		return 0;
	}

	if (f.Validate())  // fields have to be valid, at least 1 image element, image data is interpretable, not out of bounds for offsets
	{
		dump_error_log("Validation errors:\n", f);
	}

	std::cout << f;  // Dump header

	if (f.GetUserData(userid, userdata))
	{

		std::cout << "User data (hex bytes):\n";
		for (int i = 0; i < userdata.size(); ++i)
		{
			std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(userdata[i]) << " ";
			if ((i & 0xf) == 0xf)
				std::cout << "\n";
		}
		std::cout << "\n";
		// Could also print user-defined data as a string using f.GetHeader(Dpx::eUserDefinedData)
	}

	if (f.GetStandardsBasedMetadata(sbmdesc, sbmdata))
	{
		if (sbmdesc.compare("ST336") == 0)  // print as hex bytes
		{
			std::cout << "ST336 (KLV) hex bytes:\n";
			for (int i = 0; i < sbmdata.size(); ++i)
			{
				std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(sbmdata[i]) << " ";
				if ((i & 0xf) == 0xf)
					std::cout << "\n";
			}
		}
		else   // Print as string
		{
			std::cout << "Standards-based-metadata:\n";
			std::cout << f.GetHeader(Dpx::eSBMetadata);
			// Alternatively:
			//char *c = (char *)sbmdata.data();
			//std::cout << std::string(c);
		}

		std::cout << "\n";
	}

	if (output_raw)
	{
		// Make a list of all planes with corresponding IEs
		for (auto src_ie_idx : f.GetIEIndexList())
		{
			std::vector<std::shared_ptr<std::ofstream>> raw_fp_list;
			ie = f.GetImageElement(src_ie_idx);
			uint8_t num_components = ie->GetNumberOfComponents();
			uint8_t alt_chroma;
			std::vector<bool> is_chroma;
			open_raw_files(raw_fp_list, src_ie_idx, ie, raw_base_name, alt_chroma, is_chroma);
			float hicode = ie->GetHeader(Dpx::eReferenceHighDataCode);
			float lowcode = ie->GetHeader(Dpx::eReferenceLowDataCode);
			bool is_signed = (ie->GetHeader(Dpx::eDataSign) == Dpx::eDataSignSigned);
			uint8_t bit_depth_in = static_cast<uint8_t>(ie->GetHeader(Dpx::eBitDepth));
			for (uint32_t row = 0; row < ie->GetHeight(); ++row)
			{
				if (ie->GetHeader(Dpx::eBitDepth) == Dpx::eBitDepthR32)  // 32-bit float
				{
					std::vector<float> rowdata;
					rowdata.resize(ie->GetRowSizeInDatums());
					ie->Dpx2AppPixels(row, static_cast<float *>(rowdata.data()));
					for (uint32_t column = 0; column < ie->GetWidth(); column++)
					{
						for (uint8_t c = 0; c < num_components; ++c)
						{
							if ((row & 1) && c == alt_chroma)
								write_raw_datum(rowdata[column * num_components + c], bit_depth_conv, hicode, lowcode, write_full_range, true, raw_fp_list[num_components]);
							else
								write_raw_datum(rowdata[column * num_components + c], bit_depth_conv, hicode, lowcode, write_full_range, is_chroma[c], raw_fp_list[c]);
						}
					}
				}
				else if (ie->GetHeader(Dpx::eBitDepth) == Dpx::eBitDepthR64)  // 64-bit float
				{
					std::vector<double> rowdata;
					rowdata.resize(ie->GetRowSizeInDatums());
					ie->Dpx2AppPixels(row, static_cast<double *>(rowdata.data()));
					for (uint32_t column = 0; column < ie->GetWidth(); ++column)
					{
						for (uint8_t c = 0; c < num_components; ++c)
						{
							if ((row & 1) && c == alt_chroma)
								write_raw_datum(rowdata[column * num_components + c], bit_depth_conv, hicode, lowcode, write_full_range, true, raw_fp_list[num_components]);
							else
								write_raw_datum(rowdata[column * num_components + c], bit_depth_conv, hicode, lowcode, write_full_range, is_chroma[c], raw_fp_list[c]);
						}
					}
				}
				else
				{
					std::vector<int32_t> rowdata;
					int32_t datum_idx = 0;
					rowdata.resize(ie->GetRowSizeInDatums());
					ie->Dpx2AppPixels(row, static_cast<int32_t *>(rowdata.data()));
					for (uint32_t column = 0; column < ie->GetWidth(); ++column)
					{
						for (uint8_t c = 0; c < num_components; ++c)
						{
							if ((row & 1) && c == alt_chroma)
								write_raw_datum(rowdata[column * num_components + c], bit_depth_in, bit_depth_conv, hicode, lowcode, is_signed, write_full_range, true, raw_fp_list[num_components]);
							else
								write_raw_datum(rowdata[column * num_components + c], bit_depth_in, bit_depth_conv, hicode, lowcode, is_signed, write_full_range, is_chroma[c], raw_fp_list[c]);
						}
					}
				}
			}
			for (auto fp : raw_fp_list)
			{
				cout << fp->tellp() << "\n";
				fp->close();
			}
		}
	}
	else
	{ 
		for (auto src_ie_idx : f.GetIEIndexList())
		{
			ie = f.GetImageElement(src_ie_idx);
			std::cout << "Component types for image element " << src_ie_idx << ": ";
			for (auto c : ie->GetDatumLabels())
				std::cout << ie->DatumLabelToName(c) << " ";
			std::cout << "\n";
			for (uint32_t row = 0; row < ie->GetHeight(); ++row)
			{
				std::cout << "\n" << row << ": ";
				if (ie->GetHeader(Dpx::eBitDepth) == Dpx::eBitDepthR32)  // 32-bit float
				{
					std::vector<float> rowdata;
					uint32_t datum_idx = 0;
					rowdata.resize(ie->GetRowSizeInDatums());
					ie->Dpx2AppPixels(row, static_cast<float *>(rowdata.data()));
					while (datum_idx < ie->GetWidth() * ie->GetNumberOfComponents())
					{
						std::cout << "(";
						for (uint8_t c = 0; c < ie->GetNumberOfComponents(); ++c)
						{
							std::cout << rowdata[datum_idx++] << ",";
						}
						std::cout << rowdata[datum_idx++] << ") ";
					}
				}
				else if (ie->GetHeader(Dpx::eBitDepth) == Dpx::eBitDepthR64)  // 64-bit float
				{
					std::vector<double> rowdata;
					uint32_t datum_idx = 0;
					rowdata.resize(ie->GetRowSizeInDatums());
					ie->Dpx2AppPixels(row, static_cast<double *>(rowdata.data()));
					for (uint32_t x = 0; x < ie->GetWidth(); ++x)
					{
						std::cout << "(";
						for (uint8_t c = 0; c < ie->GetNumberOfComponents(); ++c)
						{
							std::cout << rowdata[datum_idx++] << ",";
						}
						std::cout << rowdata[datum_idx++] << ") ";
					}
				}
				else
				{
					std::vector<int32_t> rowdata;
					int32_t datum_idx = 0;
					rowdata.resize(ie->GetRowSizeInDatums());
					ie->Dpx2AppPixels(row, static_cast<int32_t *>(rowdata.data()));
					for (uint32_t x = 0; x < ie->GetWidth(); ++x)
					{
						std::cout << "(";
						for (uint8_t c = 0; c < ie->GetNumberOfComponents(); ++c)
						{
							std::cout << rowdata[datum_idx++] << ",";
						}
						std::cout << rowdata[datum_idx++] << ") ";
					}
				}
			}
		}
	}
	f.Close();
	return 0;
}

