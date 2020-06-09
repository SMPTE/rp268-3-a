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

	if (argc < 2)
	{
		std::cerr << "Usage: dump_dpx <dpxfile>\n";
		return 0;
	}
	std::string filename_in = std::string(argv[1]);

	std::string filename(argv[1]);

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

		std::cout << "User identification: '" << userid << "'\n";
		std::cout << "User data (hex bytes):\n";
		for (int i = 0; i < userdata.size(); ++i)
		{
			std::cout << tohex(userdata[i]) << " ";
			if ((i & 0xf) == 0xf)
				std::cout << "\n";
		}
		std::cout << "\n";
		// Could also print user-defined data as a string using f.GetHeader(Dpx::eUserDefinedData)
	}

	if (f.GetStandardsBasedMetadata(sbmdesc, sbmdata))
	{
		std::cout << "Standards-based metadata type: " << sbmdesc << "\n";
		if (sbmdesc.compare("ST336") == 0)  // print as hex bytes
		{
			std::cout << "Hex bytes:\n";
			for (int i = 0; i < sbmdata.size(); ++i)
			{
				std::cout << tohex(sbmdata[i]) << " ";
				if ((i & 0xf) == 0xf)
					std::cout << "\n";
			}
		}
		else   // Print as string
		{
			std::cout << f.GetHeader(Dpx::eSBMetadata);
			// Alternatively:
			//char *c = (char *)sbmdata.data();
			//std::cout << std::string(c);
		}

		std::cout << "\n";
	}

	for(auto src_ie_idx : f.GetIEIndexList())
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
				while(datum_idx < ie->GetWidth() * ie->GetNumberOfComponents())
				{ 
					std::cout << "(";
					for (uint8_t c = 0; c < ie->GetNumberOfComponents() - 1; ++c)
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
					for (uint8_t c = 0; c < ie->GetNumberOfComponents() - 1; ++c)
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
					for (uint8_t c = 0; c < ie->GetNumberOfComponents() - 1; ++c)
					{
						std::cout << rowdata[datum_idx++] << ",";
					}
					std::cout << rowdata[datum_idx++] << ") ";
				}
			}
		}
	}
	f.Close();
	return 0;
}

