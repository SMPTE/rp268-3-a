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
#include <tchar.h>
#include <assert.h>
#include "hdr_dpx.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

void example_read(void);
void example_write(void);

extern "C"
{
	int conv_main(int argc, char *argv[]);
}

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

	example_read();
	//example_write();

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

// Example calling sequence for HDR DPX read:
void example_read()
{
	std::string filename("colorbar.dpx");
	Dpx::HdrDpxFile  f(filename);
	Dpx::HdrDpxImageElement *ie;
	std::string userid, sbmdesc;
	std::vector<uint8_t> userdata, sbmdata;

	if (!f.IsOk())   // can't open, unrecognized format
	{
		dump_error_log("Read failed:\n", f);
		return;// Code to handle failure to open
	}

	if (!f.IsHdr())
	{
		// Code to handle V1.0/V2.0 DPX files

		std::cout << "DPX header:\n" << f.DumpHeader();
		return;
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
			for (int i = 0; i < userdata.size(); ++i)
			{
				std::cout << tohex(userdata[i]) << " ";
				if ((i & 0xf) == 0xf)
					std::cout << "\n";
			}
		}
		else   // Print as string
		{
			std::cout << f.GetHeader(Dpx::eSBMetadata);
		}

		std::cout << "\n";
	}

	for(auto src_ie_index : f.GetIEIndexList())
	{ 
		ie = f.GetImageElement(src_ie_index);
		std::cout << "Component types for image element " << src_ie_index << ": ";
		for (auto c : ie->GetDatumLabels())
			std::cout << ie->DatumLabelToName(c) << " ";
		std::cout << "\n";
		for (uint32_t row = 0; row < ie->GetHeight(); ++row)
		{
			if (ie->GetHeader(Dpx::eBitDepth) == Dpx::eBitDepth32)
			{
				std::vector<float> rowdata;
				uint32_t rowidx = 0;
				rowdata.resize(ie->GetRowSizeInBytes());
				ie->Dpx2AppPixels(row, static_cast<float *>(rowdata.data()));
				for (uint32_t x = 0; x < ie->GetWidth(); ++x)
				{
					std::cout << "(" << x << ", " << row << ") = ";
					for (uint8_t c = 0; c < ie->GetNumberOfComponents(); ++c)
					{
						std::cout << rowdata[rowidx++] << ",";
					}
					std::cout << "\n";
				}
			}
			else if (ie->GetHeader(Dpx::eBitDepth) == Dpx::eBitDepth64)
			{
				std::vector<double> rowdata;
				uint32_t rowidx = 0;
				rowdata.resize(ie->GetRowSizeInBytes());
				ie->Dpx2AppPixels(row, static_cast<double *>(rowdata.data()));
				for (uint32_t x = 0; x < ie->GetWidth(); ++x)
				{
					std::cout << "(" << x << ", " << row << ") = ";
					for (uint8_t c = 0; c < ie->GetNumberOfComponents(); ++c)
					{
						std::cout << rowdata[rowidx++] << ",";
					}
					std::cout << "\n";
				}
			}
			else
			{
				std::vector<int32_t> rowdata;
				int32_t rowidx = 0;
				rowdata.resize(ie->GetRowSizeInBytes());
				ie->Dpx2AppPixels(row, static_cast<int32_t *>(rowdata.data()));
				for (uint32_t x = 0; x < ie->GetWidth(); ++x)
				{
					std::cout << "(" << x << ", " << row << ") = ";
					for (uint8_t c = 0; c < ie->GetNumberOfComponents() - 1; ++c)
					{
						std::cout << rowdata[rowidx++] << ",";
					}
					std::cout << rowdata[rowidx++] << "\n";
				}
			}
		}
	}
	f.Close();
}

