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
#include <iostream>
#include <fstream>
#include "generic_pixel_array.h"

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
	m_err.LogError(number, severity, ((severity == Dpx::eInformational) ? "INFO #" : ((severity == Dpx::eWarning) ? "WARNING #" : "ERROR #")) \
			   + std::to_string(static_cast<int>(number)) + " in " \
               + "function " + __FUNCTION__ \
               + ", file " + __FILE__ \
               + ", line " + std::to_string(__LINE__) + ":\n" \
               + msg + "\n");

using namespace std;

GenericPixelArray::GenericPixelArray(std::string fname, uint32_t w, uint32_t h, bool issigned, uint8_t bits, std::vector<Dpx::DatumLabel> components)
{
	Allocate(w, h, issigned, bits, components);
	ReadFile(fname);
}

GenericPixelArray::GenericPixelArray(uint32_t w, uint32_t h, bool issigned, uint8_t bits, std::vector<Dpx::DatumLabel> components)
{
	Allocate(w, h, issigned, bits, components);
}

void GenericPixelArray::Allocate(uint32_t w, uint32_t h, bool issigned, uint8_t bits, std::vector<Dpx::DatumLabel> components)
{
	uint32_t row;

	m_width = w;
	m_height = h;
	m_issigned = issigned;
	m_bits = bits;
	m_components = components;
	m_label_to_cindex.resize(Dpx::DATUM_SIZE);
	for (int i = 0; i < components.size(); ++i)
		m_label_to_cindex[static_cast<int>(components[i])] = i;

	m_data_i.resize(m_height);
	// Allocate memory
	for (row = 0; row < m_height; ++row)
	{
		if (m_bits <= 16)
		{
			m_data_i[row].resize(m_width * m_components.size());
		}
		else if (m_bits == 32)
		{
			m_data_s[row].resize(m_width * m_components.size());
		}
		else if (m_bits == 64)
		{
			m_data_d[row].resize(m_width * m_components.size());
		}
	}
	m_is_allocated = true;
}


void GenericPixelArray::ReadFile(std::string fname)
{
	uint32_t row;

	ifstream infile(fname, ios::binary | ios::in);
	if (!infile)
	{
		LOG_ERROR(Dpx::eFileOpenError, Dpx::eFatal, "Unable to open file " + fname + "\n");
		return;
	}

	for (row = 0; row < m_height; ++row)
	{
		if (m_bits <= 16)
		{
			infile.read((char *)m_data_i[row].data(), m_data_i[row].size() * sizeof(int32_t));
		}
		else if (m_bits == 32)
		{
			infile.read((char *)m_data_s[row].data(), m_data_s[row].size() * sizeof(float));
		}
		else if (m_bits == 64)
		{
			infile.read((char *)m_data_d[row].data(), m_data_d[row].size() * sizeof(double));
		}
		else
			LOG_ERROR(Dpx::eFileReadError, Dpx::eFatal, "Unsupported bit depth");
	}

	if (infile.bad())
	{
		LOG_ERROR(Dpx::eFileReadError, Dpx::eFatal, "Error attempting to read file " + fname + "\n");
		infile.close();
		return;
	}

	infile.close();
}

void GenericPixelArray::WriteFile(std::string fname)
{
	ofstream outfile(fname, ios::binary | ios::out);
	uint32_t row;

	if (!outfile)
	{
		LOG_ERROR(Dpx::eFileOpenError, Dpx::eFatal, "Unable to open file " + fname + " for output");
		return;
	}

	for (row = 0; row < m_height; ++row)
	{
		if (m_bits <= 16)
			outfile.write((char *)m_data_i[row].data(), m_data_i[row].size() * sizeof(int32_t));
		else if (m_bits == 32)
			outfile.write((char *)m_data_s[row].data(), m_data_s[row].size() * sizeof(float));
		else if (m_bits == 64)
			outfile.write((char *)m_data_d[row].data(), m_data_d[row].size() * sizeof(double));
		else
			LOG_ERROR(Dpx::eFileReadError, Dpx::eFatal, "Unsupported bit depth");
	}

	if (outfile.bad())
	{
		LOG_ERROR(Dpx::eFileWriteError, Dpx::eFatal, "Error writing pixel array");
		return;
	}
	outfile.close();
}

void GenericPixelArray::SetComponent(Dpx::DatumLabel c, int x, int y, int32_t d)
{
	m_data_i[y][m_components.size() * x + m_label_to_cindex[c]] = d;
}

void GenericPixelArray::SetComponent(Dpx::DatumLabel c, int x, int y, float d)
{
	m_data_s[y][m_components.size() * x + m_label_to_cindex[c]] = d;
}

void GenericPixelArray::SetComponent(Dpx::DatumLabel c, int x, int y, double d)
{
	m_data_d[y][m_components.size() * x + m_label_to_cindex[c]] = d;
}

void GenericPixelArray::GetComponent(Dpx::DatumLabel c, int x, int y, int32_t &d)
{
	d = m_data_i[y][m_components.size() * x + m_label_to_cindex[c]];
}

void GenericPixelArray::GetComponent(Dpx::DatumLabel c, int x, int y, float &d)
{
	d = m_data_s[y][m_components.size() * x + m_label_to_cindex[c]];
}

void GenericPixelArray::GetComponent(Dpx::DatumLabel c, int x, int y, double &d)
{
	d = m_data_d[y][m_components.size() * x + m_label_to_cindex[c]];
}

bool GenericPixelArray::IsOk(void)
{
	return m_err.GetWorstSeverity() == Dpx::eFatal;
}

Dpx::ErrorObject GenericPixelArray::GetErrors(void)
{
	return m_err;
}

uint32_t GenericPixelArray::GetWidth()
{
	return m_width;
}

uint32_t GenericPixelArray::GetHeight()
{
	return m_height;
}

uint8_t GenericPixelArray::GetBitDepth()
{
	return m_bits;
}

std::vector<Dpx::DatumLabel> GenericPixelArray::GetComponentList()
{
	return m_components;
}

bool GenericPixelArray::IsAllocated()
{
	return m_is_allocated;
}