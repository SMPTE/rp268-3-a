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
#include "generic_pixel_array.h"
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

	//example_write();
	example_read();

	release_argn(argc, argn);
	return(0);
}


void dump_error_log(std::string logmessage, Dpx::HdrDpxFile &f)
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

// Example calling sequence for HDR DPX read:
void example_read()
{
	GenericPixelArrayContainer pac;
	std::string filename("mydpxfile.dpx");
	Dpx::HdrDpxFile  f(filename, pac);
	
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

	//std::cout << f;  // Dump header

	pac.PrintPixels();

}


// Simple class to illustrate returning a constant pixel value
class ConstPixelArray : public Dpx::PixelArray
{
public:
	void Allocate(uint32_t w, uint32_t h, bool issigned, uint8_t bits, std::vector<Dpx::DatumLabel> components) { ; }
	void SetComponent(Dpx::DatumLabel c, int x, int y, int32_t d) { ; }
	void SetComponent(Dpx::DatumLabel c, int x, int y, float d) { ; }
	void SetComponent(Dpx::DatumLabel c, int x, int y, double d) { ; }
	void GetComponent(Dpx::DatumLabel c, int x, int y, int32_t &d)
	{
		if (c == Dpx::DATUM_R) d = 50;
		else if (c == Dpx::DATUM_G) d = 60;
		else if (c == Dpx::DATUM_B) d = 70;
		else d = 0;
	}
	void GetComponent(Dpx::DatumLabel c, int x, int y, float &d) { ;  }
	void GetComponent(Dpx::DatumLabel c, int x, int y, double &d) { ; }
	bool IsOk(void) {
		return true;
	}
	Dpx::ErrorObject GetErrors() {
		return m_err;
	}

private:
	Dpx::ErrorObject m_err;
};

// Example calling sequence for HDR DPX write (single image element case):
void example_write()
{
	Dpx::ErrorObject err;
	std::string fname = "mydpxfile.dpx";
	const int width = 1920, height = 1080;
	const int bit_depth = 8;
	//HdrDpxImageElement ie(width, height, Dpx::eDescBGR, 8);  // Single image element with 8-bit components
	//uint32_t i, j;
	Dpx::HdrDpxFile dpxf;
	std::shared_ptr<Dpx::HdrDpxImageElement> ie;
	std::shared_ptr<ConstPixelArray> pixel_array = static_cast<std::shared_ptr<ConstPixelArray>>(new ConstPixelArray);
	
	// accept typed array overload
	ie = dpxf.CreateImageElement(pixel_array, 0, width, height, Dpx::eDescBGR, bit_depth);

	// Create a picture (planes aren't ordered inside this data structure, so this sequence is arbitrary:)
	//ie->SetHeader(Dpx::eBitDepth, 8);   // this would be invalid!!
	//ie->SetHeader(Dpx::eDescriptor, Dpx::eDescBGR);  // this would be invalid!!
	ie->SetHeader(Dpx::eEncoding, Dpx::eEncodingRLE);

#if 0   // No longer needed
	for (i = 0; i < ie->GetHeight(); i++)
	{
		for (j = 0; j < ie->GetWidth(); j++)
		{
			if (bit_depth <= 16)  // use data type function?
			{
				ie->SetPixelI(j, i, DATUM_R, 50);  // Set all pixels to R,G,B = (50, 60, 70)
				ie->SetPixelI(j, i, DATUM_G, 60);
				ie->SetPixelI(j, i, DATUM_B, 70);
			}
			else
			{
				ie->SetPixelF(j, i, DATUM_R, 0.5);  // Set all pixels to R,G,B = (.5, .6, .7)
				ie->SetPixelF(j, i, DATUM_G, 0.6);
				ie->SetPixelF(j, i, DATUM_B, 0.7);
			}
		}
	}
#endif 

	// Create 2nd IE (TBD)

	// Metadata fields can be set at any point
	ie->SetHeader(Dpx::eColorimetric, Dpx::eColorimetricBT_709);

	// Set header values as desired (defaults assumed based on image element structure)
	dpxf.SetHeader(Dpx::eCopyright, "(C) 2019 XYZ Productions");  // Key is a string, value matches data type
	dpxf.SetHeader(Dpx::eDatumMappingDirection, Dpx::eDatumMappingDirectionL2R);
	if (dpxf.Validate())
	{
		dump_error_log("Validation errors:\n", dpxf);
	}

	// Write the file
	dpxf.WriteFile(fname);
	if (!dpxf.IsOk())
	{
		dump_error_log("File write message log:\n", dpxf);
	}
}

/*
std::ostream& operator<< (std::ostream& s, const Pixel3& rgb)
{
	return s << "{" << rgb.r << "," << rgb.g << "," << rgb.b << "}";
}
*/