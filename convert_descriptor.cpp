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

/** @file convert_descriptor.cpp
	@brief Converts an RGB DPX file to a different order or to/from a planar format; optionally adds or remove alpha plane
           (only works with non-floating point DPX files where all image elements have same bit depth)

	This tool can change component order of a DPX file or change between planar and interleaved formats.
*/

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

int convert_descriptor(int argc, char ** argv);

/**
	Returns number of _TCHARs in string

	@param wstr	input string
	@return		number of _TCHARS in string
*/
int wstrlen(_TCHAR * wstr)
{
	int l_idx = 0;
	while (((char*)wstr)[l_idx] != 0) l_idx += 2;
	return l_idx;
}


/**
	Allocate char string and copy _TCHAR->char->string

	@param wSrc input string
	@return		pointer to new char * string containing copy of input string
*/
char * wstrdup(_TCHAR * wSrc)
{
	int l_idx = 0;
	int l_len = wstrlen(wSrc);
	char * l_nstr = (char*)malloc(l_len);
	if (l_nstr) {
		do {
			l_nstr[l_idx] = (char)wSrc[l_idx];
			l_idx++;
		} while ((char)wSrc[l_idx] != 0);
		l_nstr[l_idx] = 0;
	}
	return l_nstr;
}



/**
	Allocate argn structure parallel to argv. argn must be released.

	@param argc	number of arguments
	@param argv	arguments as _TCHAR strings
	@return		pointer to array of arguments as char strings
*/
char ** allocate_argn(int argc, _TCHAR* argv[])
{
	char ** l_argn = (char **)malloc(argc * sizeof(char*));
	assert(l_argn != NULL);
	for (int idx = 0; idx<argc; idx++) {
		l_argn[idx] = wstrdup(argv[idx]);
	}
	return l_argn;
}

/**
	Release argn and its allocated memory

	@param argc number of arguments
	@param nargv	pointer to array of arguments as char strings
*/
void release_argn(int argc, char ** nargv)
{
	for (int idx = 0; idx<argc; idx++) {
		free(nargv[idx]);
	}
	free(nargv);
}


/**
	Entry point for Windows console app

	@param argc	number of arguments
	@param argv array of arguments (as _TCHAR strings)
	@return		exit code
*/
int _tmain(int argc, _TCHAR* argv[])
{
	char ** argn = allocate_argn(argc, argv);

	convert_descriptor(argc, argn);

	release_argn(argc, argn);
	return(0);
}
#endif

/**
	Prints to cerr a specified message along with the error log from the DPX file

	@param logmessage	Message to include with the error log
	@param f			HdrDpxFile associated with DPX file
*/
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

/**
	Return a 2-character string that shows the hex value of a byte (equivalent to sprintf(s,"%02x))

	@param v			Byte value to convert
	@return				String representation of byte
*/
inline string tohex(uint8_t v)
{
	stringstream ss;

	ss << setfill('0');

	ss << hex << setw(2) << v;

	return ss.str();
}

/** 
	Compare two descriptors. If a datum at the specified index in the first descriptor is found in the 2nd, return the index in the 2nd. 
	@param[in] destdesc			Reference descriptor
	@param[in] srcdesc			Descriptor to search
	@param[in] datum			Index within reference descriptor to look for
	@param[out] foundpos		Index where descriptor was found (if found)
	@return						true if found, false if not found */
bool compare_datum_desc(Dpx::HdrDpxDescriptor destdesc, Dpx::HdrDpxDescriptor srcdesc, int datum, int &foundpos)
{
	std::vector<Dpx::DatumLabel> dl_src = Dpx::DescriptorToDatumList(srcdesc);
	std::vector<Dpx::DatumLabel> dl_dest = Dpx::DescriptorToDatumList(destdesc);

	for (int i = 0; i < dl_dest.size(); ++i)
		if (dl_dest[datum] == dl_src[i])
		{
			foundpos = i;
			return true;
		}

	return false;
}

/**
	Build a mapping from one or more source descriptors to a destination descriptor
	@param[in] dest					Destination descriptor
	@param[in] srclist				List source descriptors
	@param[out] matching_ie			List of IE indexes (one for each datum in the destination descriptor)
	@param[out] matching_component	List of component indexes within source IEs (one for each datum in the destination descriptor) */
void build_datum_map(Dpx::HdrDpxDescriptor dest, std::vector<Dpx::HdrDpxDescriptor> srclist, std::vector<int> &matching_ie, std::vector<int> &matching_component)
{
	std::vector<Dpx::DatumLabel> dl_dest = Dpx::DescriptorToDatumList(dest);
	int foundpos;

	matching_ie.resize(dl_dest.size());
	matching_component.resize(dl_dest.size());
	for (int i = 0; i < dl_dest.size(); ++i)
		matching_ie[i] = -1;					// -1 means not found
	for (int i = 0; i < dl_dest.size(); ++i)
	{
		for (int j = 0; j < srclist.size(); ++j)
		{
			if (compare_datum_desc(dest, srclist[j], i, foundpos))
			{
				matching_ie[i] = j;
				matching_component[i] = foundpos;
				break;
			}
		}
	}
}

// Usage:
//  convert_descriptor input.dpx output.dpx <neworder> <planar>
//
// Desc types:
//   BGR
//   BGRA
//   ARGB
//   RGB
//   RGBA
//   ABGR
/**
	Main program
	@param argc					Number of arguments
	@param argv					Array of argument strings */
#ifdef _MSC_VER
int convert_descriptor(int argc, char ** argv)
#else
int main(int argc, char *argv[])
#endif
{
	if (argc < 5)
	{
		std::cerr << "Usage: convert_descriptor input.dpx output.dpx <neworder> <planar>\n";
		std::cerr << "  <neworder> : RGB, BGRA, etc. Components must be present in source file except A; if A is not present all output A values will be set to 1.0)\n";
		std::cerr << "  <planar> : 0 means use interleaved format in single IE, 1 means split components into separate planes, one per IE\n";
		return 0;
	}

	std::string filename_in = std::string(argv[1]);
	std::string filename_out = std::string(argv[2]);
	std::string order = std::string(argv[3]);
	bool planar = (atoi(argv[4]) == 1);
	// User might add error checking code here

	// Create and open f_in for reading:
	Dpx::HdrDpxFile  f_in(filename_in);
	
	Dpx::HdrDpxFile  f_out;
	Dpx::HdrDpxImageElement *ie_in, *ie_out;
	std::string userid, sbmdesc;
	std::vector<uint8_t> userdata, sbmdata;
	int32_t maxval;

	if (!f_in.IsOk())   // can't open, unrecognized format
	{
		dump_error_log("Read failed:\n", f_in);
		return 1;// Code to handle failure to open
	}

	if (!f_in.IsHdr())
	{
		// User might add code to handle V1.0/V2.0 DPX files

		std::cout << "DPX header:\n" << f_in.DumpHeader();
		return 0;
	}

	if (f_in.Validate())  // fields have to be valid, at least 1 image element, image data is interpretable, not out of bounds for offsets
	{
		dump_error_log("Validation errors:\n", f_in);
	}

	std::cout << f_in;  // Dump header

	bool ud_present = f_in.GetUserData(userid, userdata);
	bool sbm_present = f_in.GetStandardsBasedMetadata(sbmdesc, sbmdata);

	// Copy header info from f_in to f_out...
	// note that the following does not copy IE headers:
	f_out.CopyHeaderFrom(f_in);
	if (ud_present)
		f_out.SetUserData(userid, userdata);
	if (sbm_present)
	{
		f_out.SetStandardsBasedMetadata(sbmdesc, sbmdata);
		f_out.SetHeader(Dpx::eStandardsBasedMetadataOffset, Dpx::eSBMAutoLocate);  // Auto-locate means the DPX writer puts the standards-based metadata right after the last image data
	}

	// Build destination IE types
	std::vector<Dpx::HdrDpxDescriptor> dest_ie_desc_list;
	if (planar)
	{
		for (int idx = 0; idx < order.length(); ++idx)
		{
			if (order.substr(idx, 1) == "R")
				dest_ie_desc_list.push_back(Dpx::eDescR);
			else if (order.substr(idx, 1) == "G")
				dest_ie_desc_list.push_back(Dpx::eDescG);
			else if (order.substr(idx, 1) == "B")
				dest_ie_desc_list.push_back(Dpx::eDescB);
			else if (order.substr(idx, 1) == "A")
				dest_ie_desc_list.push_back(Dpx::eDescA);
			else
			{
				std::cerr << "Unkown component type " << order.substr(idx, 1) << "\n";
				return 1;
			}
		}
	}
	else
	{
		if (order == "BGR")
			dest_ie_desc_list.push_back(Dpx::eDescBGR);
		else if (order == "BGRA")
			dest_ie_desc_list.push_back(Dpx::eDescBGRA);
		else if (order == "ARGB")
			dest_ie_desc_list.push_back(Dpx::eDescARGB);
		else if (order == "RGB")
			dest_ie_desc_list.push_back(Dpx::eDescRGB);
		else if (order == "RGBA")
			dest_ie_desc_list.push_back(Dpx::eDescRGBA);
		else if (order == "ABGR")
			dest_ie_desc_list.push_back(Dpx::eDescABGR);
		else
		{
			std::cerr << "Unkown component descriptor " << order << "\n";
			return 1;
		}
	}

	// Configure output image element headers
	std::vector<uint8_t> src_ie_list = f_in.GetIEIndexList();
	for (uint8_t out_ie_idx = 0; out_ie_idx < dest_ie_desc_list.size(); ++out_ie_idx)
	{
		ie_out = f_out.GetImageElement(out_ie_idx);
		ie_out->CopyHeaderFrom(f_in.GetImageElement(src_ie_list[0]));  // All IE headers match the first one from the source
		ie_out->SetHeader(Dpx::eDescriptor, dest_ie_desc_list[out_ie_idx]);
		ie_out->SetHeader(Dpx::eOffsetToData, Dpx::eOffsetAutoLocate);  // Auto-locate means the DPX writer puts image elements sequentially following the main header
	}

	// Start writing
	f_out.OpenForWriting(filename_out);

	// If RLE enabled, rows must be written sequentially
	// If RLE disabled, rows can be written in any order
	// Because we wanted to support RLE, we loop over IEs then rows. If RLE is not a consideration, the loops can be switched
	// Loop over destination IEs
	for(uint8_t out_ie_idx = 0; out_ie_idx < dest_ie_desc_list.size(); ++out_ie_idx)
	{ 
		std::vector<Dpx::DatumLabel> dl_dest = Dpx::DescriptorToDatumList(dest_ie_desc_list[out_ie_idx]);
		std::vector<int32_t> datum_row_out;
		std::vector<int32_t> datum_row_in[8];
		std::vector<Dpx::DatumLabel> dl_src[8];
		uint8_t datum_stride[8];
		std::vector<int> datum_ie_map;
		std::vector<int> datum_loc;
		std::vector<Dpx::HdrDpxDescriptor> ie_desc_list;

		// Create map of IEs
		ie_desc_list.resize(8);

		for (auto src_ie_index : src_ie_list)
		{
			ie_desc_list[src_ie_index] = f_in.GetImageElement(src_ie_index)->GetHeader(Dpx::eDescriptor);
			dl_src[src_ie_index] = Dpx::DescriptorToDatumList(ie_desc_list[src_ie_index]);
			datum_stride[src_ie_index] = static_cast<uint8_t>(dl_src[src_ie_index].size());
			datum_row_in[src_ie_index].resize(datum_stride[src_ie_index] * f_in.GetHeader(Dpx::ePixelsPerLine));
		}

		build_datum_map(dest_ie_desc_list[out_ie_idx], ie_desc_list, datum_ie_map, datum_loc);

		ie_out = f_out.GetImageElement(out_ie_idx);
		datum_row_out.resize(dl_dest.size() * f_in.GetHeader(Dpx::ePixelsPerLine));
		maxval = (1 << static_cast<uint8_t>(ie_out->GetHeader(Dpx::eBitDepth))) - 1;

		for(uint32_t row = 0; row < f_in.GetHeader(Dpx::eLinesPerImageElement); ++row)
		{
			bool row_read[8] = {
				false, false, false, false, false, false, false, false
			};
			uint8_t num_components_out = ie_out->GetNumberOfComponents();

			// Read the rows we need
			for (int component = 0; component < num_components_out; ++component)
			{
				ie_in = f_in.GetImageElement(datum_ie_map[component]);
				if (!row_read[datum_ie_map[component]])
				{
					ie_in->Dpx2AppPixels(row, datum_row_in[datum_ie_map[component]].data());
					row_read[datum_ie_map[component]] = true;
				}
			}

			for (uint32_t column = 0; column < f_in.GetHeader(Dpx::ePixelsPerLine); ++column)
			{
				for (int component = 0; component < num_components_out; ++component)
				{
					if (datum_ie_map[component] == -1)
					{
						if (dl_dest[component] == Dpx::DATUM_A)
							datum_row_out[column * num_components_out + component] = maxval;
						else
							std::cerr << "Unrecognized datum\n";
					} else
						datum_row_out[column * num_components_out + component] = datum_row_in[datum_ie_map[component]][column * datum_stride[datum_ie_map[component]] + datum_loc[component]];
				}
			}
			ie_out->App2DpxPixels(row, datum_row_out.data());
		}
	}

	f_in.Close();
	f_out.Close();
	return 0;
}

