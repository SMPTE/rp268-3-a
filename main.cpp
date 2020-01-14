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

	release_argn(argc, argn);
	return(0);
}


void dump_error_log(Dpx::ErrorObject err)
{
	Dpx::ErrorCode code;
	Dpx::ErrorSeverity severity;
	std::string msg;

	for (int i = 0; i < err.GetNumErrors(); ++i)
	{
		err.GetError(i, code, severity, msg);
		std::cerr << msg << std::endl;
	}
}

// Example calling sequence for HDR DPX read:
void example_read()
{
	HdrDpxFile  f;
	Dpx::ErrorObject err;
	
	//err = f.Open("Clipboard-1.dpx");
	err = f.Open("mydpxfile.dpx");
	if (err.GetWorstSeverity() == Dpx::eFatal)   // can't open, unrecognized format
	{
		std::cerr << "Read failed:\n";
		dump_error_log(err);
		return;// Code to handle failure to open
	}

	if (!f.IsHdr())
	{
		// Code to handle V1.0/V2.0 DPX files

		std::cout << "DPX header:\n" << f.DumpHeader();
		return;
	}
	
	if (err = f.Validate())  // fields have to be valid, at least 1 image element, image data is interpretable, not out of bounds for offsets
	{
		std::cerr << "Validation errors:\n";
		dump_error_log(err);
	}

	std::cout << f.DumpHeader();
	for (std::shared_ptr<HdrDpxImageElement> ie : f.m_IE)
	{
		if (ie->m_isinitialized)
		{
			for (uint32_t i = 0; i < ie->GetHeight(); i++)
			{
				for (uint32_t j = 0; j < ie->GetWidth(); j++)
				{
					for (DatumLabel dl : ie->GetDatumLabels())
					{
						if (ie->GetHeader(Dpx::eBitDepth) <= 16)
							std::cout << ie->DatumLabelToName(dl) << "=" << ie->GetPixelI(j, i, dl) << " ";
						else
							std::cout << ie->DatumLabelToName(dl) << "=" << ie->GetPixelLf(j, i, dl) << " ";
					}
					std::cout << "\n";
				}
			}
		}
	}
}


// Example calling sequence for HDR DPX write (single image element case):
void example_write()
{
	Dpx::ErrorObject err;
	std::string fname = "mydpxfile.dpx";
	const int width = 1920, height = 1080;
	const int bit_depth = 8;
	//HdrDpxImageElement ie(width, height, Dpx::eDescBGR, 8);  // Single image element with 8-bit components
	uint32_t i, j;
	HdrDpxFile dpxf;
	std::shared_ptr<HdrDpxImageElement> ie;

	ie = dpxf.CreateImageElement(width, height, Dpx::eDescBGR, bit_depth);

	// Create a picture (planes aren't ordered inside this data structure, so this sequence is arbitrary:)
	//ie.SetHeader(Dpx::eBitDepth, 8);   // this would be invalid!!
	//ie.SetHeader(Dpx::eDescriptor, Dpx::eDescBGR);  // this would be invalid!!
	for (i = 0; i < ie->GetHeight(); i++)
	{
		for (j = 0; j < ie->GetWidth(); j++)
		{
			if (bit_depth <= 16)
			{
				ie->SetPixelI(j, i, DATUM_R, 50);  // Set all pixels to R,G,B = (50, 60, 70)
				ie->SetPixelI(j, i, DATUM_G, 60);
				ie->SetPixelI(j, i, DATUM_B, 70);
			}
			else
			{
				ie->SetPixelLf(j, i, DATUM_R, 0.5);  // Set all pixels to R,G,B = (.5, .6, .7)
				ie->SetPixelLf(j, i, DATUM_G, 0.6);
				ie->SetPixelLf(j, i, DATUM_B, 0.7);
			}
		}
	}

	// Create 2nd IE (TBD)

	// Metadata fields can be set at any point
	ie->SetHeader(Dpx::eColorimetric, Dpx::eColorimetricBT_709);

	// Set header values as desired (defaults assumed based on image element structure)
	dpxf.SetHeader(Dpx::eCopyright, "(C) 2019 XYZ Productions");  // Key is a string, value matches data type
	dpxf.SetHeader(Dpx::eDatumMappingDirection, Dpx::eDatumMappingDirectionL2R);
	if (err = dpxf.Validate())
	{
		std::cerr << "Validation errors:\n";
		dump_error_log(err);
	}

	// Write the file
	if (err = dpxf.WriteFile(fname))
	{
		std::cerr << "File write message log:\n";
		dump_error_log(err);
	}
}

