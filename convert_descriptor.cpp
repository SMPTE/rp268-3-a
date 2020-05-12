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

//! \file main.cpp  Defines the entry point for the console application.
//
//  These are wrapper functions for Win32 to be able to call the mainline in codec_main.c

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <assert.h>
#include "hdr_dpx.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

void convert_descriptor(int argc, char ** argv);

// returns number of TCHARs in string
int wstrlen(_TCHAR * wstr)
{
	int l_idx = 0;
	while (((char*)wstr)[l_idx] != 0) l_idx += 2;
	return l_idx;
}


// Allocate char string and copy TCHAR->char->string
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



// allocate argn structure parallel to argv
// argn must be released
char ** allocate_argn(int argc, _TCHAR* argv[])
{
	char ** l_argn = (char **)malloc(argc * sizeof(char*));
	assert(l_argn != NULL);
	for (int idx = 0; idx<argc; idx++) {
		l_argn[idx] = wstrdup(argv[idx]);
	}
	return l_argn;
}

// release argn and its content
void release_argn(int argc, char ** nargv)
{
	for (int idx = 0; idx<argc; idx++) {
		free(nargv[idx]);
	}
	free(nargv);
}

int _tmain(int argc, _TCHAR* argv[])
{
	char ** argn = allocate_argn(argc, argv);

	convert_descriptor(argc, argn);

	release_argn(argc, argn);
	return(0);
}


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
//  Converts an RGB DPX file to a different order or to/from a planar format; optionally adds or remove alpha plane
//   (only works with non-floating point DPX files where all image elements have same bit depth)
//
// Desc types:
//   BGR
//   BGRA
//   ARGB
//   RGB
//   RGBA
//   ABGR

// Example calling sequence for HDR DPX read:
void convert_descriptor(int argc, char ** argv)
{
	if (argc < 5)
	{
		std::cerr << "Usage: convert_descriptor input.dpx output.dpx <neworder> <planar>\n";
		std::cerr << "  <neworder> : RGB, BGRA, etc. (components must match source file; if A is added all A values will be set to 1.0)\n";
		std::cerr << "  <planar> : 0 means use interleaved format in single IE, 1 means split components into separate planes, one per IE\n";
		return;
	}

	std::string filename_in = std::string(argv[1]);
	std::string filename_out = std::string(argv[2]);
	std::string order = std::string(argv[3]);
	bool planar = (atoi(argv[4]) == 1);

	Dpx::HdrDpxFile  f_in(filename_in);
	Dpx::HdrDpxFile  f_out;
	Dpx::HdrDpxImageElement *ie_in, *ie_out;
	std::string userid, sbmdesc;
	std::vector<uint8_t> userdata, sbmdata;
	int32_t maxval;

	if (!f_in.IsOk())   // can't open, unrecognized format
	{
		dump_error_log("Read failed:\n", f_in);
		return;// Code to handle failure to open
	}

	if (!f_in.IsHdr())
	{
		// Code to handle V1.0/V2.0 DPX files

		std::cout << "DPX header:\n" << f_in.DumpHeader();
		return;
	}

	if (f_in.Validate())  // fields have to be valid, at least 1 image element, image data is interpretable, not out of bounds for offsets
	{
		dump_error_log("Validation errors:\n", f_in);
	}

	std::cout << f_in;  // Dump header

	bool ud_present = f_in.GetUserData(userid, userdata);
	bool sbm_present = f_in.GetStandardsBasedMetadata(sbmdesc, sbmdata);

	// Note that the following does not copy IE headers:
	f_out.CopyHeaderFrom(f_in);
	if (ud_present)
		f_out.SetUserData(userid, userdata);
	if (sbm_present)
	{
		f_out.SetStandardsBasedMetadata(sbmdesc, sbmdata);
		f_out.SetHeader(Dpx::eStandardsBasedMetadataOffset, Dpx::eSBMAutoLocate);
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
				return;
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
			return;
		}
	}

	// Configure descriptor headers
	std::vector<uint8_t> src_ie_list = f_in.GetIEIndexList();
	uint8_t out_ie_index = 0;
	for (auto d : dest_ie_desc_list)
	{
		ie_out = f_out.GetImageElement(out_ie_index);
		ie_out->CopyHeaderFrom(f_in.GetImageElement(src_ie_list[0]));
		ie_out->SetHeader(Dpx::eDescriptor, d);
		ie_out->SetHeader(Dpx::eOffsetToData, Dpx::eOffsetAutoLocate);
		out_ie_index++;
	}

	// Start writing
	f_out.WriteFile(filename_out);

	out_ie_index = 0;
	// Loop over destination IEs
	for(auto dest_desc : dest_ie_desc_list)
	{ 
		std::vector<Dpx::DatumLabel> dl_dest = Dpx::DescriptorToDatumList(dest_desc);
		std::vector<int32_t> datum_row_out;
		std::vector<int32_t> datum_row_in[8];
		std::vector<Dpx::DatumLabel> dl_src[8];
		uint8_t datum_stride[8];
		std::vector<int> datum_ie_map;
		std::vector<int> datum_loc;
		std::vector<Dpx::HdrDpxDescriptor> ie_desc_list;

		ie_desc_list.resize(8);

		for (auto src_ie_index : src_ie_list)
		{
			ie_desc_list[src_ie_index] = f_in.GetImageElement(src_ie_index)->GetHeader(Dpx::eDescriptor);
			dl_src[src_ie_index] = Dpx::DescriptorToDatumList(ie_desc_list[src_ie_index]);
			datum_stride[src_ie_index] = static_cast<uint8_t>(dl_src[src_ie_index].size());
			datum_row_in[src_ie_index].resize(datum_stride[src_ie_index] * f_in.GetHeader(Dpx::ePixelsPerLine));
		}

		build_datum_map(dest_desc, ie_desc_list, datum_ie_map, datum_loc);

		ie_out = f_out.GetImageElement(out_ie_index);
		datum_row_out.resize(dl_dest.size() * f_in.GetHeader(Dpx::ePixelsPerLine));
		maxval = (1 << static_cast<uint8_t>(ie_out->GetHeader(Dpx::eBitDepth))) - 1;

		for(uint32_t y = 0; y < f_in.GetHeader(Dpx::eLinesPerElement); ++y)
		{
			bool row_read[8] = {
				false, false, false, false, false, false, false, false
			};

			// Read the rows we need
			for (int c = 0; c < dl_dest.size(); ++c)
			{
				ie_in = f_in.GetImageElement(datum_ie_map[c]);
				if (!row_read[datum_ie_map[c]])
				{
					ie_in->Dpx2AppPixels(y, datum_row_in[datum_ie_map[c]].data());
					row_read[datum_ie_map[c]] = true;
				}
			}

			for (uint32_t x = 0; x < f_in.GetHeader(Dpx::ePixelsPerLine); ++x)
			{
				for (int c = 0; c < dl_dest.size(); ++c)
				{
					if (datum_ie_map[c] == -1)
					{
						if (dl_dest[c] == Dpx::DATUM_A)
							datum_row_out[x * dl_dest.size() + c] = maxval;
						else
							std::cerr << "Unrecognized datum\n";
					} else
						datum_row_out[x * dl_dest.size() + c] = datum_row_in[datum_ie_map[c]][x * datum_stride[datum_ie_map[c]] + datum_loc[c]];
				}
			}
			ie_out->App2DpxPixels(y, datum_row_out.data());
		}
		out_ie_index++;
	}

	f_in.Close();
	f_out.Close();
}
