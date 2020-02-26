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
#pragma once
#include <string>
#include "hdr_dpx.h"

enum GenericPixelArrayTypes
{
	eGPATypeInt32, eGPATypeSingle, eGPATypeDouble
};

class GenericPixelArray : public Dpx::PixelArray
{
public:
	// Constructor that reads from a file:
	GenericPixelArray(std::string fname, uint32_t w, uint32_t h, bool issigned, uint8_t bits, std::vector<Dpx::DatumLabel> components);
	// Constructor that allocates blank memory
	GenericPixelArray(uint32_t w, uint32_t h, bool issigned, uint8_t bits, std::vector<Dpx::DatumLabel> components);
	// Constructor that does not allocate memory (memory allocated by callback on DPX read)
	GenericPixelArray() { ; };

	void ReadFile(std::string fname);
	void WriteFile(std::string fname);

	// Implementation of abstract class:
	void Allocate(uint32_t w, uint32_t h, bool issigned, uint8_t bits, std::vector<Dpx::DatumLabel> components);
	void SetComponent(Dpx::DatumLabel c, int x, int y, int32_t d);
	void SetComponent(Dpx::DatumLabel c, int x, int y, float d);
	void SetComponent(Dpx::DatumLabel c, int x, int y, double d);
	void GetComponent(Dpx::DatumLabel c, int x, int y, int32_t &d);
	void GetComponent(Dpx::DatumLabel c, int x, int y, float &d);
	void GetComponent(Dpx::DatumLabel c, int x, int y, double &d);
	bool IsOk();
	bool IsAllocated();
	Dpx::ErrorObject GetErrors();

	uint32_t GetWidth();
	uint32_t GetHeight();
	uint8_t GetBitDepth();
	std::vector<Dpx::DatumLabel> GetComponentList();

private:

	std::vector<std::vector<int32_t>> m_data_i;
	std::vector<std::vector<float>> m_data_s;
	std::vector<std::vector<double>> m_data_d;

	uint32_t m_width;
	uint32_t m_height;
	bool m_issigned;
	uint8_t m_bits;
	std::vector<Dpx::DatumLabel> m_components;
	std::vector<int> m_label_to_cindex;
	bool m_is_allocated = false;

	Dpx::ErrorObject m_err;
};

class GenericPixelArrayContainer : public Dpx::PixelArrayContainer
{
public:
	GenericPixelArrayContainer() { ; }
	void HeaderCallback(HDRDPXFILEFORMAT &header)
	{ 
		int i;
		for (i = 0; i < 8; ++i)
			if (header.ImageHeader.ImageElement[i].DataOffset != UINT32_MAX)
				m_pixel_array_vec[i] = std::shared_ptr<GenericPixelArray>(new GenericPixelArray);
	}
	std::shared_ptr<Dpx::PixelArray> GetPixelArray(int ie)
	{
		return m_pixel_array_vec[ie];
	}
	bool IsOk() {
		return true;
	}
	void PrintPixels()
	{
		for (int ie = 0; ie < 8; ++ie)
		{
			uint8_t bpc = m_pixel_array_vec[ie]->GetBitDepth();
			if (m_pixel_array_vec[ie]->IsAllocated())
			{
				std::cout << "Image element #" << ie << ":\n";
				for (uint32_t i = 0; i < m_pixel_array_vec[ie]->GetHeight(); i++)
				{
					for (uint32_t j = 0; j < m_pixel_array_vec[ie]->GetWidth(); j++)
					{
						for (Dpx::DatumLabel dl : m_pixel_array_vec[ie]->GetComponentList())
						{
							if (bpc == 64)
							{
								double d;
								m_pixel_array_vec[ie]->GetComponent(dl, j, i, d);
								std::cout << Dpx::DatumLabelToName[dl] << "=" << d << " ";
							}
							else if (bpc == 32)
							{
								float d;
								m_pixel_array_vec[ie]->GetComponent(dl, j, i, d);
								std::cout << Dpx::DatumLabelToName[dl] << "=" << d << " ";
							}
							else
							{
								int32_t d;
								m_pixel_array_vec[ie]->GetComponent(dl, j, i, d);
								std::cout << Dpx::DatumLabelToName[dl] << "=" << d << " ";
							}
						}
						std::cout << "\n";
					}
				}
			}
		}
	}

private:
	std::shared_ptr<GenericPixelArray> m_pixel_array_vec[8];
};
