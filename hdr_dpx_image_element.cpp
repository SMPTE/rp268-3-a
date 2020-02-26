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
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include "hdr_dpx.h"
#include "fifo.h"


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


using namespace Dpx;

// Constructor used for writes
HdrDpxImageElement::HdrDpxImageElement(std::shared_ptr<Dpx::PixelArray> &pa, int ie_index, uint32_t width, uint32_t height, HdrDpxDescriptor desc, int8_t bpc)
{
	m_bpc = bpc; // TBD
	m_width = width;
	m_height = height;
	m_isinitialized = true;
	m_descriptor = desc;
	memset(&m_dpx_imageelement, 0xff, sizeof(HDRDPX_IMAGEELEMENT));
	if (desc == eDescCbCr || desc == eDescCbYCrY || desc == eDescCbYACrYA ||
		desc == eDescCb || desc == eDescCr || desc == eDescCYY ||
		desc == eDescCYAYA)
		m_h_subsampled = true;
	else
		m_h_subsampled = false;
	if (desc == eDescCb || desc == eDescCr || desc == eDescCYY ||
		desc == eDescCYAYA)
		m_v_subsampled = true;
	else
		m_v_subsampled = false;
	m_pixel_array = pa;
	CompileDatumPlanes();
}

//  Constructor used for reads (called from HdrDpxFile only)
HdrDpxImageElement::HdrDpxImageElement(std::shared_ptr<PixelArray> &pa, int ie_index, HDRDPXFILEFORMAT &header)
{
	HdrDpxDescriptor desc;
	m_bpc = header.ImageHeader.ImageElement[ie_index].BitSize;
	ASSERT_MSG((m_bpc == 1) || (m_bpc == 8) || (m_bpc == 10) || (m_bpc == 12) || (m_bpc == 16) || (m_bpc == 32) || (m_bpc == 64), "Read invalid bit depth\n");
	desc = m_descriptor = static_cast<HdrDpxDescriptor>(header.ImageHeader.ImageElement[ie_index].Descriptor);
	if (desc == eDescCbCr || desc == eDescCbYCrY || desc == eDescCbYACrYA ||
		desc == eDescCb || desc == eDescCr || desc == eDescCYY ||
		desc == eDescCYAYA)
		m_h_subsampled = true;
	else
		m_h_subsampled = false;
	if (desc == eDescCb || desc == eDescCr || desc == eDescCYY ||
		desc == eDescCYAYA)
		m_v_subsampled = true;
	else
		m_v_subsampled = false;
	m_width = ComputeWidth(header.ImageHeader.PixelsPerLine);
	m_height = ComputeHeight(header.ImageHeader.LinesPerElement);
	m_isinitialized = true;
	m_dpx_imageelement = header.ImageHeader.ImageElement[ie_index];
	m_pixel_array = pa;
	CompileDatumPlanes();
	pa->Allocate(m_width, m_height, (m_dpx_imageelement.DataSign == 1), m_bpc, m_planes);
}

HdrDpxImageElement::~HdrDpxImageElement()
{
	;
}

uint32_t HdrDpxImageElement::ComputeWidth(uint32_t w)
{
	return w >> (m_h_subsampled ? 1 : 0);
}

uint32_t HdrDpxImageElement::ComputeHeight(uint32_t h)
{
	return h >> (m_h_subsampled ? 1 : 0);
}


void HdrDpxImageElement::CompileDatumPlanes(void)
{
	m_planes.clear();
	switch (m_descriptor)
	{
	case eDescUser:
	case eDescUndefined:
		m_planes.push_back(DATUM_UNSPEC);
		break;
	case eDescR:
		m_planes.push_back(DATUM_R);
		break;
	case eDescG:
		m_planes.push_back(DATUM_G);
		break;
	case eDescB:
		m_planes.push_back(DATUM_B);
		break;
	case eDescA:
		m_planes.push_back(DATUM_A);
		break;
	case eDescY:
		m_planes.push_back(DATUM_Y);
		break;
	case eDescCbCr:
		m_planes.push_back(DATUM_CB);
		m_planes.push_back(DATUM_CR);
		break;
	case eDescZ:
		m_planes.push_back(DATUM_Z);
		break;
	case eDescComposite:
		m_planes.push_back(DATUM_COMPOSITE);
		break;
	case eDescCb:
		m_planes.push_back(DATUM_CB);
		break;
	case eDescCr:
		m_planes.push_back(DATUM_CR);
		break;
	case eDescRGB_268_1:
		m_planes.push_back(DATUM_R);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_B);
		break;
	case eDescRGBA_268_1:
		m_planes.push_back(DATUM_R);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_B);
		m_planes.push_back(DATUM_A);
		break;
	case eDescABGR_268_1:
		m_planes.push_back(DATUM_A);
		m_planes.push_back(DATUM_B);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_R);
		break;
	case eDescBGR:
		m_planes.push_back(DATUM_B);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_R);
		break;
	case eDescBGRA:
		m_planes.push_back(DATUM_B);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_R);
		m_planes.push_back(DATUM_A);
		break;
	case eDescARGB:
		m_planes.push_back(DATUM_A);
		m_planes.push_back(DATUM_R);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_B);
		break;
	case eDescRGB:
		m_planes.push_back(DATUM_R);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_B);
		break;
	case eDescRGBA:
		m_planes.push_back(DATUM_R);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_B);
		m_planes.push_back(DATUM_A);
		break;
	case eDescABGR:
		m_planes.push_back(DATUM_A);
		m_planes.push_back(DATUM_B);
		m_planes.push_back(DATUM_G);
		m_planes.push_back(DATUM_R);
		break;
	case eDescCbYCrY:
		m_planes.push_back(DATUM_CB);
		m_planes.push_back(DATUM_Y);
		m_planes.push_back(DATUM_CR);
		m_planes.push_back(DATUM_Y2);
		break;
	case eDescCbYACrYA:
		m_planes.push_back(DATUM_CB);
		m_planes.push_back(DATUM_Y);
		m_planes.push_back(DATUM_A);
		m_planes.push_back(DATUM_CR);
		m_planes.push_back(DATUM_Y2);
		m_planes.push_back(DATUM_A2);
		break;
	case eDescCbYCr:
		m_planes.push_back(DATUM_CB);
		m_planes.push_back(DATUM_Y);
		m_planes.push_back(DATUM_CR);
		break;
	case eDescCbYCrA:
		m_planes.push_back(DATUM_CB);
		m_planes.push_back(DATUM_Y);
		m_planes.push_back(DATUM_CR);
		m_planes.push_back(DATUM_A);
		break;
	case eDescCYY:
		m_planes.push_back(DATUM_C);
		m_planes.push_back(DATUM_Y);
		m_planes.push_back(DATUM_Y2);
		break;
	case eDescCYAYA:
		m_planes.push_back(DATUM_C);
		m_planes.push_back(DATUM_Y);
		m_planes.push_back(DATUM_A);
		m_planes.push_back(DATUM_Y2);
		m_planes.push_back(DATUM_A2);
		break;
	case eDescGeneric2:
		m_planes.push_back(DATUM_UNSPEC);
		m_planes.push_back(DATUM_UNSPEC2);
		break;
	case eDescGeneric3:
		m_planes.push_back(DATUM_UNSPEC);
		m_planes.push_back(DATUM_UNSPEC2);
		m_planes.push_back(DATUM_UNSPEC3);
		break;
	case eDescGeneric4:
		m_planes.push_back(DATUM_UNSPEC);
		m_planes.push_back(DATUM_UNSPEC2);
		m_planes.push_back(DATUM_UNSPEC3);
		m_planes.push_back(DATUM_UNSPEC4);
		break;
	case eDescGeneric5:
		m_planes.push_back(DATUM_UNSPEC);
		m_planes.push_back(DATUM_UNSPEC2);
		m_planes.push_back(DATUM_UNSPEC3);
		m_planes.push_back(DATUM_UNSPEC4);
		m_planes.push_back(DATUM_UNSPEC5);
		break;
	case eDescGeneric6:
		m_planes.push_back(DATUM_UNSPEC);
		m_planes.push_back(DATUM_UNSPEC2);
		m_planes.push_back(DATUM_UNSPEC3);
		m_planes.push_back(DATUM_UNSPEC4);
		m_planes.push_back(DATUM_UNSPEC5);
		m_planes.push_back(DATUM_UNSPEC6);
		break;
	case eDescGeneric7:
		m_planes.push_back(DATUM_UNSPEC);
		m_planes.push_back(DATUM_UNSPEC2);
		m_planes.push_back(DATUM_UNSPEC3);
		m_planes.push_back(DATUM_UNSPEC4);
		m_planes.push_back(DATUM_UNSPEC5);
		m_planes.push_back(DATUM_UNSPEC6);
		m_planes.push_back(DATUM_UNSPEC7);
		break;
	case eDescGeneric8:
		m_planes.push_back(DATUM_UNSPEC);
		m_planes.push_back(DATUM_UNSPEC2);
		m_planes.push_back(DATUM_UNSPEC3);
		m_planes.push_back(DATUM_UNSPEC4);
		m_planes.push_back(DATUM_UNSPEC5);
		m_planes.push_back(DATUM_UNSPEC6);
		m_planes.push_back(DATUM_UNSPEC7);
		m_planes.push_back(DATUM_UNSPEC8);
		break;
	}

}

#if 0
void HdrDpxImageElement::CompileDatumPlanes(void)
{
	m_planes.clear();
	switch (m_descriptor)
	{
	case eDescUser:
	case eDescUndefined:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		break;
	case eDescR:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case eDescG:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		break;
	case eDescB:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case eDescA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case eDescY:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		break;
	case eDescCbCr:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		break;
	case eDescZ:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Z)));
		break;
	case eDescComposite:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_COMPOSITE)));
		break;
	case eDescCb:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		break;
	case eDescCr:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		break;
	case eDescRGB_268_1:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case eDescRGBA_268_1:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case eDescABGR_268_1:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case eDescBGR:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case eDescBGRA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case eDescARGB:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case eDescRGB:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case eDescRGBA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case eDescABGR:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case eDescCbYCrY:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		break;
	case eDescCbYACrYA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A2)));
		break;
	case eDescCbYCr:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		break;
	case eDescCbYCrA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case eDescCYY:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_C)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		break;
	case eDescCYAYA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_C)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A2)));
		break;
	case eDescGeneric2:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		break;
	case eDescGeneric3:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		break;
	case eDescGeneric4:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		break;
	case eDescGeneric5:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5)));
		break;
	case eDescGeneric6:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6)));
		break;
	case eDescGeneric7:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC7)));
		break;
	case eDescGeneric8:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC7)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC8)));
		break;
	}
}


void HdrDpxImageElement::CompileDatumPlanes(void)
{
	m_planes.clear();
	switch (m_descriptor)
	{
	case eDescUser:
	case eDescUndefined:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		break;
	case eDescR:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case eDescG:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		break;
	case eDescB:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case eDescA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case eDescY:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		break;
	case eDescCbCr:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		break;
	case eDescZ:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Z));
		break;
	case eDescComposite:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_COMPOSITE));
		break;
	case eDescCb:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		break;
	case eDescCr:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		break;
	case eDescRGB_268_1:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case eDescRGBA_268_1:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case eDescABGR_268_1:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case eDescBGR:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case eDescBGRA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case eDescARGB:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case eDescRGB:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case eDescRGBA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case eDescABGR:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case eDescCbYCrY:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		break;
	case eDescCbYACrYA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A2));
		break;
	case eDescCbYCr:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		break;
	case eDescCbYCrA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case eDescCYY:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_C));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		break;
	case eDescCYAYA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_C));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A2));
		break;
	case eDescGeneric2:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		break;
	case eDescGeneric3:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		break;
	case eDescGeneric4:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		break;
	case eDescGeneric5:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5));
		break;
	case eDescGeneric6:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6));
		break;
	case eDescGeneric7:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC7));
		break;
	case eDescGeneric8:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC7));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC8));
		break;
	}
}

#endif

std::vector<DatumLabel> HdrDpxImageElement::GetDatumLabels(void)
{
	return m_planes;
}

int HdrDpxImageElement::GetNumberOfComponents(void)
{
	return static_cast<int>(m_planes.size());
}

#define READ_DPX_32(x)   (bswap ? (((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | ((x) >> 24))) : (x))

ErrorObject HdrDpxImageElement::ReadImageData(std::ifstream &infile, bool direction_r2l, bool bswap)
{
	ErrorObject err;
	uint32_t xpos, ypos;
	int component;
	int num_components;
	uint32_t image_data_word;
	Fifo fifo(16);
	bool is_signed = (m_dpx_imageelement.DataSign == 1);
	uint32_t expected_zero;
	union {
		double r64;
		uint32_t d[2];
	} c_r64;
	union {
		float r32;
		uint32_t d;
	} c_r32;
	int rle_state = 0;   // 0 = flag, 1-n = component value
	int32_t run_length = 0; 
	int rle_count = 0;  
	bool rle_is_same;
	bool freeze_increment = false;
	int32_t int_datum;
	int32_t rle_pixel[8];

	num_components = GetNumberOfComponents();

	xpos = 0; ypos = 0; component = 0;

	while (xpos < m_width && ypos < m_height && component < num_components)
	{
		while (fifo.m_fullness <= 32)
		{
			// Read 32-bit word & byte swap if needed
			infile.read((char *)&image_data_word, 4);
			image_data_word = READ_DPX_32(image_data_word);
			fifo.PutBits(image_data_word, 32);
		}
		switch (m_bpc)
		{
		case 1:
		case 8:
		case 16:
			int_datum = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
			break;
		case 10:
		case 12:
			if (m_dpx_imageelement.Packing == 1) // Method A
			{
				if (direction_r2l)
				{
					if (fifo.m_fullness == 48 || fifo.m_fullness == 64)   // start with padding bits
					{
						if (m_bpc == 10)
						{
							expected_zero = fifo.FlipGetBitsUi(2);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
						else // 12-bit
						{ 
							expected_zero = fifo.FlipGetBitsUi(4);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
					}
				}
				
				int_datum = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
				if (!direction_r2l)
				{
					if (fifo.m_fullness == 64 - 30)
					{
						expected_zero = fifo.GetBitsUi(2);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
					else if (fifo.m_fullness == 64 - 12 || fifo.m_fullness == 64 - 28)
					{
						expected_zero = fifo.GetBitsUi(4);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
				}
			}
			else if (m_dpx_imageelement.Packing == 2) // Method B
			{
				if (!direction_r2l)
				{
					if (fifo.m_fullness == 64 - 0 || fifo.m_fullness == 64 - 16)
					{
						if (m_bpc == 10)
						{
							expected_zero = fifo.GetBitsUi(2);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
						else
						{
							expected_zero = fifo.GetBitsUi(4);
							if (expected_zero)
							{
								m_warn_unexpected_nonzero_data_bits = true;
								m_warn_image_data_word_mask |= image_data_word;
							}
						}
					}
				}
				int_datum = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
				if (direction_r2l)
				{
					if (fifo.m_fullness == 64 - 30)
					{
						expected_zero = fifo.FlipGetBitsUi(2);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
					else if (fifo.m_fullness == 64 - 12 || fifo.m_fullness == 64 - 28)
					{
						expected_zero = fifo.FlipGetBitsUi(4);
						if (expected_zero)
						{
							m_warn_unexpected_nonzero_data_bits = true;
							m_warn_image_data_word_mask |= image_data_word;
						}
					}
				}
			}
			else   // Packed
			{ 
				int_datum = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
			}
			break;
		case 32:
			c_r32.d = fifo.GetBitsUi(32);
			m_pixel_array->SetComponent(m_planes[component], xpos, ypos, c_r32.r32);
			break;
		case 64:
			c_r64.d[0] = fifo.GetBitsUi(32);
			c_r64.d[1] = fifo.GetBitsUi(32);
			m_pixel_array->SetComponent(m_planes[component], xpos, ypos, c_r64.r64);
			break;
		}
		if (m_dpx_imageelement.Encoding == 1) // RLE
		{
			if (rle_state == 0)
			{
				rle_state++;
				run_length = (int_datum >> 1) & INT16_MAX;
				if (run_length == 0 && !m_warn_zero_run_length)
				{
					m_warn_zero_run_length = true;
					m_warnings.push_back("Encountered a run-length of zero, first occurred at " + std::to_string(xpos) + ", " + std::to_string(ypos));
				}
				freeze_increment = true;
				rle_count = 0;
				rle_is_same = (int_datum & 1);
			}
			else if (component == num_components - 1)
			{
				m_pixel_array->SetComponent(m_planes[component], xpos, ypos, int_datum);
				rle_pixel[component] = int_datum;
				if (rle_is_same)
				{
					if (xpos + run_length > m_width && !m_warn_rle_same_past_eol)
					{
						m_warn_rle_same_past_eol = true;
						m_warnings.push_back("RLE same-pixel run went past the end of a line, first occurred at " + std::to_string(xpos) + ", " + std::to_string(ypos));
					}
					for (int i = 1; i < run_length; ++i)
					{
						for (int c = 0; c < num_components; ++c)
						{
							m_pixel_array->SetComponent(m_planes[c], xpos + i, ypos, rle_pixel[c]);
						}
					}
					component = 0;
					xpos += MAX(1, run_length);
					rle_state = 0;
				}
				else
				{
					rle_count++;
					xpos++;
					component = 0;
					if (rle_count >= run_length)
						rle_state = 0;
					else
					{
						if (xpos >= m_width && !m_warn_rle_diff_past_eol)
						{
							m_warn_rle_diff_past_eol = true;
							m_warnings.push_back("RLE different-pixel run went past the end of a line, first occurred at " + std::to_string(xpos) + ", " + std::to_string(ypos));
						}
					}
				}
			}
			else
			{
				m_pixel_array->SetComponent(m_planes[component], xpos, ypos, int_datum);
				rle_pixel[component] = int_datum;
				component++;
			}
		}
		else			// No RLE
		{
			m_pixel_array->SetComponent(m_planes[component], xpos, ypos, int_datum);
			component++;
			if (component == num_components)
			{
				component = 0;
				xpos++;
			}
		}
		if (xpos >= m_width)
		{
			ypos++;
			xpos = 0;
		}
	}
	return err;
}


void HdrDpxImageElement::WriteFlush(Fifo *fifo, std::ofstream &ofile)
{
	uint32_t image_data_word;

	while (fifo->m_fullness >= 32)
	{
		image_data_word = fifo->GetBitsUi(32);
		if (m_byte_swap)
			ByteSwap32((void *)(&image_data_word));
		ofile.write((char *)(&image_data_word), 4);
	}
}


void HdrDpxImageElement::WriteDatum(Fifo *fifo, int32_t datum, std::ofstream &ofile)
{
	switch (m_dpx_imageelement.Packing)
	{
	case 0:
		fifo->PutDatum(datum, m_bpc, m_direction_r2l);
		break;
	case 1:  /* Method A */
		if (m_direction_r2l && (fifo->m_fullness == 0 || fifo->m_fullness == 16))
			fifo->FlipPutBits(0, (m_bpc == 10) ? 2 : 4);
		fifo->PutDatum(datum, m_bpc, m_direction_r2l);
		if (!m_direction_r2l && (fifo->m_fullness == 12 || fifo->m_fullness == 28 || fifo->m_fullness == 30))
			fifo->PutBits(0, (m_bpc == 10) ? 2 : 4);
		break;  /* Method B */
	case 2:
		if (!m_direction_r2l && (fifo->m_fullness == 0 || fifo->m_fullness == 16))
			fifo->PutBits(0, (m_bpc == 10) ? 2 : 4);
		fifo->PutDatum(datum, m_bpc, m_direction_r2l);
		if (m_direction_r2l && (fifo->m_fullness == 12 || fifo->m_fullness == 28 || fifo->m_fullness == 30))
			fifo->FlipPutBits(0, (m_bpc == 10) ? 2 : 4);
		break;
	}
	WriteFlush(fifo, ofile);
}

void HdrDpxImageElement::WritePixel(Fifo *fifo, uint32_t xpos, uint32_t ypos, std::ofstream &ofile)
{
	int component;
	int num_components;
	union {
		double r64;
		uint32_t d[2];
	} c_r64;
	union {
		float r32;
		uint32_t d;
	} c_r32;
	int32_t int_datum;

	num_components = GetNumberOfComponents();

	for (component = 0; component < num_components; ++component)
	{
		switch (m_bpc)
		{
		case 1:
		case 8:
		case 10:
		case 12:
		case 16:
			m_pixel_array->GetComponent(m_planes[component], xpos, ypos, int_datum);
			WriteDatum(fifo, int_datum, ofile);
			break;
		case 32:
			m_pixel_array->GetComponent(m_planes[component], xpos, ypos, c_r32.r32);
			fifo->PutBits(c_r32.d, 32);
			WriteFlush(fifo, ofile);
			break;
		case 64:
			m_pixel_array->GetComponent(m_planes[component], xpos, ypos, c_r64.r64);
			fifo->PutBits(c_r64.d[0], 32);
			fifo->PutBits(c_r64.d[1], 32);
			WriteFlush(fifo, ofile);
			break;
		}

	}
}

bool HdrDpxImageElement::IsNextSame(uint32_t xpos, uint32_t ypos, int32_t pixel[])
{
	int num_components;
	int32_t datum;

	if (xpos >= m_width - 1)
		return false;
	num_components = GetNumberOfComponents();

	for (int c = 0; c < num_components; ++c)
	{
		m_pixel_array->GetComponent(m_planes[c], xpos, ypos, datum);
		if (pixel[c] != datum)
			return false;
	}
	return true;
}

void HdrDpxImageElement::WriteLineEnd(Fifo *fifo, std::ofstream &ofile)
{
	if (fifo->m_fullness & 0x1f)   // not an even multiple of 32 
		fifo->PutDatum(0, 32 - (fifo->m_fullness & 0x1f), m_direction_r2l);
	WriteFlush(fifo, ofile);
}



ErrorObject HdrDpxImageElement::WriteImageData(std::ofstream &outfile, bool direction_r2l, bool bswap)
{
	ErrorObject err;
	uint32_t xpos, ypos;
	int component;
	int num_components;
	Fifo fifo(16);
	bool is_signed = (m_dpx_imageelement.DataSign == 1);
	uint32_t run_length = 0;
	bool freeze_increment = false;
	unsigned int max_run;
	bool run_type;
	int32_t rle_pixel[8];

	num_components = GetNumberOfComponents();
	max_run = (1 << (m_bpc - 1)) - 1;
	m_byte_swap = bswap;
	m_direction_r2l = direction_r2l;

	xpos = 0; ypos = 0;

	// write
	while (xpos < m_width && ypos < m_height)
	{
		if (m_dpx_imageelement.Encoding == 1 && m_bpc <= 16) // RLE
		{
			if (xpos == m_width - 1)  // Only one pixel left on line
			{
				WriteDatum(&fifo, 2, outfile);  // Indicates run of 1 pixel
				WritePixel(&fifo, xpos, ypos, outfile);
			}
			else {
				for (component = 0; component < num_components; ++component)
				{
					m_pixel_array->GetComponent(m_planes[component], xpos, ypos, rle_pixel[component]);
				}
				run_type = IsNextSame(xpos, ypos, rle_pixel);
				for (run_length = 1; run_length < m_width - xpos && run_length < max_run - 1; ++run_length)
				{
					if (IsNextSame(xpos + run_length, ypos, rle_pixel) != run_type)
						break;
				}
				run_length++;
				if (run_type)
				{
					WriteDatum(&fifo, 1 | (run_length << 1), outfile);
					WritePixel(&fifo, xpos, ypos, outfile);
					xpos += run_length;
				}
				else
				{
					WriteDatum(&fifo, 0 | (run_length << 1), outfile);
					while (run_length)
					{
						WritePixel(&fifo, xpos, ypos, outfile);
						xpos++;
					}
				}
			}
		} 
		else  // non-RLE (uncompressed)
		{
			for (run_length = 0; run_length < m_width; ++run_length)
			{
				WritePixel(&fifo, xpos, ypos, outfile);
				xpos++;
			}
		}

		if (xpos >= m_width)
		{
			// pad last line
			WriteLineEnd(&fifo, outfile);
			xpos = 0;
			ypos++;
		}
	}
	return err;
}


void HdrDpxImageElement::ResetWarnings(void)
{
	m_warn_unexpected_nonzero_data_bits = false;
	m_warn_image_data_word_mask = 0;
	m_warn_rle_same_past_eol = false;
	m_warn_rle_diff_past_eol = false;
	m_warn_zero_run_length = false;
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsDataSign f, HdrDpxDataSign d)
{
	m_dpx_imageelement.DataSign = static_cast<uint8_t>(d);
}

HdrDpxDataSign HdrDpxImageElement::GetHeader(HdrDpxFieldsDataSign f)
{
	return static_cast<HdrDpxDataSign>(m_dpx_imageelement.DataSign);
}

void HdrDpxImageElement::SetHeader(HdrDpxIEFieldsR32 f, float d)
{
	switch (f)
	{
	case eReferenceLowDataCode:
		if (m_bpc >= 32)
			m_dpx_imageelement.LowData.f = d;
		else
			m_dpx_imageelement.LowData.d = (uint32_t)(d + 0.5); // Could add bounds checking
		break;
	case eReferenceLowQuantity:
		m_dpx_imageelement.LowQuantity = d;
		break;
	case eReferenceHighDataCode:
		if (m_bpc >= 32)
			m_dpx_imageelement.HighData.f = d;
		else
			m_dpx_imageelement.HighData.d = (uint32_t)(d + 0.5);
		break;
	case eReferenceHighQuantity:
		m_dpx_imageelement.HighQuantity = d;
	}
}

float HdrDpxImageElement::GetHeader(HdrDpxIEFieldsR32 f)
{
	switch (f)
	{
	case eReferenceLowDataCode:
		if (m_bpc >= 32)
			return m_dpx_imageelement.LowData.f;
		else
			return (float)m_dpx_imageelement.LowData.d;
	case eReferenceLowQuantity:
		return m_dpx_imageelement.LowQuantity;
	case eReferenceHighDataCode:
		if (m_bpc == 32)
			return m_dpx_imageelement.LowData.f;
		else
			return (float)(m_dpx_imageelement.LowData.d);
	case eReferenceHighQuantity:
		return m_dpx_imageelement.HighQuantity;
	}
	return 0;
}


void HdrDpxImageElement::SetHeader(HdrDpxFieldsDescriptor f, HdrDpxDescriptor d)
{
	if(m_isinitialized)	
		m_warnings.push_back("SetHeader() call to set descriptor ignored; descriptor cannot be changed after IE is initialized");
	else
		m_dpx_imageelement.Descriptor = static_cast<uint8_t>(d);
}

HdrDpxDescriptor HdrDpxImageElement::GetHeader(HdrDpxFieldsDescriptor f)
{
	return static_cast<HdrDpxDescriptor>(m_dpx_imageelement.Descriptor);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsTransfer f, HdrDpxTransfer d)
{
	m_dpx_imageelement.Transfer = static_cast<uint8_t>(d);
}

HdrDpxTransfer HdrDpxImageElement::GetHeader(HdrDpxFieldsTransfer f)
{
	return static_cast<HdrDpxTransfer>(m_dpx_imageelement.Transfer);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsColorimetric f, HdrDpxColorimetric d)
{
	m_dpx_imageelement.Colorimetric = static_cast<uint8_t>(d);
}

HdrDpxColorimetric HdrDpxImageElement::GetHeader(HdrDpxFieldsColorimetric f)
{
	return static_cast<HdrDpxColorimetric>(m_dpx_imageelement.Colorimetric);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsBitDepth f, HdrDpxBitDepth d)
{
	if(m_isinitialized)
		m_warnings.push_back("SetHeader() call to set bit depth is ignored; bit depth cannot be changed after IE is initialized");
	else
		m_dpx_imageelement.BitSize = static_cast<uint8_t>(d);
}

HdrDpxBitDepth HdrDpxImageElement::GetHeader(HdrDpxFieldsBitDepth f)
{
	return static_cast<HdrDpxBitDepth>(m_dpx_imageelement.BitSize);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsPacking f, HdrDpxPacking d)
{
	m_dpx_imageelement.Packing = static_cast<uint16_t>(d);
}

HdrDpxPacking HdrDpxImageElement::GetHeader(HdrDpxFieldsPacking f)
{
	return static_cast<HdrDpxPacking>(m_dpx_imageelement.Packing);
}

void HdrDpxImageElement::SetHeader(HdrDpxFieldsEncoding f, HdrDpxEncoding d)
{
	m_dpx_imageelement.Encoding = static_cast<uint16_t>(d);
}

HdrDpxEncoding HdrDpxImageElement::GetHeader(HdrDpxFieldsEncoding f)
{
	return static_cast<HdrDpxEncoding>(m_dpx_imageelement.Packing);
}

void HdrDpxImageElement::SetHeader(HdrDpxIEFieldsU32 f, uint32_t d)
{
	switch (f)
	{
	case eOffsetToData:
		m_dpx_imageelement.DataOffset = d;
		break;
	case eEndOfLinePadding:
		m_dpx_imageelement.EndOfLinePadding = d;
		break;
	case eEndOfImagePadding:
		m_dpx_imageelement.EndOfImagePadding = d;
	}
}

uint32_t HdrDpxImageElement::GetHeader(HdrDpxIEFieldsU32 f)
{
	switch (f)
	{
	case eOffsetToData:
		return m_dpx_imageelement.DataOffset;
	case eEndOfLinePadding:
		return m_dpx_imageelement.EndOfLinePadding;
	case eEndOfImagePadding:
		return m_dpx_imageelement.EndOfImagePadding;
	}
	return 0;
}


void HdrDpxImageElement::AddPlane(DatumLabel dlabel)
{
	ASSERT_MSG(m_isinitialized, "Can't add a plane until image element is initialized");

	m_planes.push_back(dlabel);
}

uint32_t HdrDpxImageElement::GetWidth(void)
{
	return m_width;
}

uint32_t HdrDpxImageElement::GetHeight(void)
{
	return m_height;
}

std::string HdrDpxImageElement::DatumLabelToName(DatumLabel dl)
{
	switch (dl)
	{
	case DATUM_UNSPEC:
		return "Unspec";
	case DATUM_R:
		return "R";
	case DATUM_G:
		return "G";
	case DATUM_B:
		return "B";
	case DATUM_A:
		return "A";
	case DATUM_Y:
		return "Y";
	case DATUM_CB:
		return "Cb";
	case DATUM_CR:
		return "Cr";
	case DATUM_Z:
		return "Z";
	case DATUM_COMPOSITE:
		return "Composite";
	case DATUM_A2:
		return "A2";
	case DATUM_Y2:
		return "Y2";
	case DATUM_C:
		return "C";
	case DATUM_UNSPEC2:
		return "Unspec2";
	case DATUM_UNSPEC3:
		return "Unspec3";
	case DATUM_UNSPEC4:
		return "Unspec4";
	case DATUM_UNSPEC5:
		return "Unspec5";
	case DATUM_UNSPEC6:
		return "Unspec6";
	case DATUM_UNSPEC7:
		return "Unspec7";
	case DATUM_UNSPEC8:
		return "Unspec8";
	}
	return "Unrecognized";
}

/*
bool HdrDpxImageElement::IsOk(void)
{
	return m_err.GetWorstSeverity() != eFatal;
}

int HdrDpxImageElement::GetNumErrors(void)
{
	return m_err.GetNumErrors();
}

void HdrDpxImageElement::GetError(int index, ErrorCode &errcode, ErrorSeverity &severity, std::string &errmsg)
{
	return m_err.GetError(index, errcode, severity, errmsg);
}

void HdrDpxImageElement::ClearErrors()
{
	m_err.Clear();
} */

bool HdrDpxImageElement::IsFloat(void)
{
	return m_dpx_imageelement.BitSize >= 32;
}
