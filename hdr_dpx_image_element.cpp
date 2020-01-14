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


using namespace std;

HdrDpxImageElement::HdrDpxImageElement(uint32_t width, uint32_t height, Dpx::HdrDpxDescriptor desc, int8_t bpc)
{
	m_bpc = bpc; // TBD
	m_width = width;
	m_height = height;
	m_isinitialized = true;
	m_descriptor = desc;
	if (bpc < 32)
		m_datum_type = DATUM_INT32;
	else
		m_datum_type = DATUM_DOUBLE;
	memset(&m_dpx_imageelement, 0xff, sizeof(HDRDPX_IMAGEELEMENT));
	if (desc == Dpx::eDescCbCr || desc == Dpx::eDescCbYCrY || desc == Dpx::eDescCbYACrYA ||
		desc == Dpx::eDescCb || desc == Dpx::eDescCr || desc == Dpx::eDescCYY ||
		desc == Dpx::eDescCYAYA)
		m_h_subsampled = true;
	else
		m_h_subsampled = false;
	if (desc == Dpx::eDescCb || desc == Dpx::eDescCr || desc == Dpx::eDescCYY ||
		desc == Dpx::eDescCYAYA)
		m_v_subsampled = true;
	else
		m_v_subsampled = false;
	CompileDatumPlanes();
}

HdrDpxImageElement::HdrDpxImageElement(HDRDPXFILEFORMAT header, int ie)
{
	Dpx::HdrDpxDescriptor desc;
	m_bpc = header.ImageHeader.ImageElement[ie].BitSize;
	ASSERT_MSG((m_bpc == 1) || (m_bpc == 8) || (m_bpc == 10) || (m_bpc == 12) || (m_bpc == 16) || (m_bpc == 32) || (m_bpc == 64), "Read invalid bit depth\n");
	desc = m_descriptor = static_cast<Dpx::HdrDpxDescriptor>(header.ImageHeader.ImageElement[ie].Descriptor);
	if (desc == Dpx::eDescCbCr || desc == Dpx::eDescCbYCrY || desc == Dpx::eDescCbYACrYA ||
		desc == Dpx::eDescCb || desc == Dpx::eDescCr || desc == Dpx::eDescCYY ||
		desc == Dpx::eDescCYAYA)
		m_h_subsampled = true;
	else
		m_h_subsampled = false;
	if (desc == Dpx::eDescCb || desc == Dpx::eDescCr || desc == Dpx::eDescCYY ||
		desc == Dpx::eDescCYAYA)
		m_v_subsampled = true;
	else
		m_v_subsampled = false;
	m_width = ComputeWidth(header.ImageHeader.PixelsPerLine);
	m_height = ComputeHeight(header.ImageHeader.LinesPerElement);
	m_isinitialized = true;
	if (m_bpc < 32)
		m_datum_type = DATUM_INT32;
	else
		m_datum_type = DATUM_DOUBLE;
	m_dpx_imageelement = header.ImageHeader.ImageElement[ie];
	CompileDatumPlanes();
}

HdrDpxImageElement::HdrDpxImageElement(void)
{
	m_isinitialized = false;
}

HdrDpxImageElement::~HdrDpxImageElement()
{
	// Make sure we deallocate DatumPlanes
	for (std::shared_ptr<DatumPlane> dp : m_planes)
		dp->Destroy();
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
	case Dpx::eDescUser:
	case Dpx::eDescUndefined:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		break;
	case Dpx::eDescR:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case Dpx::eDescG:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		break;
	case Dpx::eDescB:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case Dpx::eDescA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case Dpx::eDescY:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		break;
	case Dpx::eDescCbCr:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		break;
	case Dpx::eDescZ:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Z)));
		break;
	case Dpx::eDescComposite:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_COMPOSITE)));
		break;
	case Dpx::eDescCb:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		break;
	case Dpx::eDescCr:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		break;
	case Dpx::eDescRGB_268_1:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case Dpx::eDescRGBA_268_1:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case Dpx::eDescABGR_268_1:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case Dpx::eDescBGR:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case Dpx::eDescBGRA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case Dpx::eDescARGB:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case Dpx::eDescRGB:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		break;
	case Dpx::eDescRGBA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case Dpx::eDescABGR:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_B)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_G)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_R)));
		break;
	case Dpx::eDescCbYCrY:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		break;
	case Dpx::eDescCbYACrYA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A2)));
		break;
	case Dpx::eDescCbYCr:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		break;
	case Dpx::eDescCbYCrA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CB)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_CR)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		break;
	case Dpx::eDescCYY:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_C)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		break;
	case Dpx::eDescCYAYA:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_C)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_A2)));
		break;
	case Dpx::eDescGeneric2:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		break;
	case Dpx::eDescGeneric3:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		break;
	case Dpx::eDescGeneric4:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		break;
	case Dpx::eDescGeneric5:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5)));
		break;
	case Dpx::eDescGeneric6:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6)));
		break;
	case Dpx::eDescGeneric7:
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6)));
		m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC7)));
		break;
	case Dpx::eDescGeneric8:
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


#if 0
void HdrDpxImageElement::CompileDatumPlanes(void)
{
	m_planes.clear();
	switch (m_descriptor)
	{
	case Dpx::eDescUser:
	case Dpx::eDescUndefined:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		break;
	case Dpx::eDescR:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case Dpx::eDescG:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		break;
	case Dpx::eDescB:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case Dpx::eDescA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case Dpx::eDescY:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		break;
	case Dpx::eDescCbCr:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		break;
	case Dpx::eDescZ:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Z));
		break;
	case Dpx::eDescComposite:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_COMPOSITE));
		break;
	case Dpx::eDescCb:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		break;
	case Dpx::eDescCr:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		break;
	case Dpx::eDescRGB_268_1:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case Dpx::eDescRGBA_268_1:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case Dpx::eDescABGR_268_1:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case Dpx::eDescBGR:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case Dpx::eDescBGRA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case Dpx::eDescARGB:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case Dpx::eDescRGB:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		break;
	case Dpx::eDescRGBA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case Dpx::eDescABGR:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_B));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_G));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_R));
		break;
	case Dpx::eDescCbYCrY:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		break;
	case Dpx::eDescCbYACrYA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A2));
		break;
	case Dpx::eDescCbYCr:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		break;
	case Dpx::eDescCbYCrA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CB));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_CR));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		break;
	case Dpx::eDescCYY:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_C));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		break;
	case Dpx::eDescCYAYA:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_C));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_Y2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_A2));
		break;
	case Dpx::eDescGeneric2:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		break;
	case Dpx::eDescGeneric3:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		break;
	case Dpx::eDescGeneric4:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		break;
	case Dpx::eDescGeneric5:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5));
		break;
	case Dpx::eDescGeneric6:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6));
		break;
	case Dpx::eDescGeneric7:
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC2));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC3));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC4));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC5));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC6));
		m_planes.push_back(DatumPlane(m_width, m_height, m_datum_type, DATUM_UNSPEC7));
		break;
	case Dpx::eDescGeneric8:
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
	std::vector<DatumLabel> dl_list;

	for (std::shared_ptr<DatumPlane> dp : m_planes)
		dl_list.push_back(dp->m_datum_label);
	return dl_list;
}

int HdrDpxImageElement::GetNumberOfComponents(void)
{
	return static_cast<int>(m_planes.size());
}

#define READ_DPX_32(x)   (bswap ? (((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | ((x) >> 24))) : (x))

Dpx::ErrorObject HdrDpxImageElement::ReadImageData(ifstream &infile, bool direction_r2l, bool bswap)
{
	Dpx::ErrorObject err;
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
			m_planes[component]->i_datum[ypos][xpos] = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
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
				m_planes[component]->i_datum[ypos][xpos] = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
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
				m_planes[component]->i_datum[ypos][xpos] = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
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
				m_planes[component]->i_datum[ypos][xpos] = fifo.GetDatum(m_bpc, is_signed, direction_r2l);
			}
			break;
		case 32:
			c_r32.d = fifo.GetBitsUi(32);
			m_planes[component]->lf_datum[ypos][xpos] = c_r32.r32;
			break;
		case 64:
			c_r64.d[0] = fifo.GetBitsUi(32);
			c_r64.d[1] = fifo.GetBitsUi(32);
			m_planes[component]->lf_datum[ypos][xpos] = c_r64.r64;
			break;
		}
		if (m_dpx_imageelement.Encoding == 1) // RLE
		{
			if (rle_state == 0)
			{
				rle_state++;
				run_length = (m_planes[component]->i_datum[ypos][xpos] >> 1) & INT32_MAX;
				if (run_length == 0 && !m_warn_zero_run_length)
				{
					m_warn_zero_run_length = true;
					m_warnings.push_back("Encountered a run-length of zero, first occurred at " + to_string(xpos) + ", " + to_string(ypos));
				}
				freeze_increment = true;
				rle_count = 0;
				rle_is_same = (m_planes[component]->i_datum[ypos][xpos] & 1);
			}
			else if (component == num_components - 1)
			{
				if (rle_is_same)
				{
					if (xpos + run_length > m_width && !m_warn_rle_same_past_eol)
					{
						m_warn_rle_same_past_eol = true;
						m_warnings.push_back("RLE same-pixel run went past the end of a line, first occurred at " + to_string(xpos) + ", " + to_string(ypos));
					}
					for (int i = 1; i < run_length; ++i)
					{
						for (int c = 0; c < num_components; ++c)
						{
							m_planes[c]->i_datum[ypos][xpos + i] = m_planes[c]->i_datum[ypos][xpos];
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
							m_warnings.push_back("RLE different-pixel run went past the end of a line, first occurred at " + to_string(xpos) + ", " + to_string(ypos));
						}
					}
				}
			}
			else 
				component++;
		}
		else			// No RLE
		{
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


void HdrDpxImageElement::WriteFlush(Fifo *fifo, ofstream &ofile)
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


void HdrDpxImageElement::WriteDatum(Fifo *fifo, int32_t datum, ofstream &ofile)
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

void HdrDpxImageElement::WritePixel(Fifo *fifo, uint32_t xpos, uint32_t ypos, ofstream &ofile)
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
			WriteDatum(fifo, m_planes[component]->i_datum[ypos][xpos], ofile);
			break;
		case 32:
			c_r32.r32 = static_cast<float>(m_planes[component]->lf_datum[ypos][xpos]);
			fifo->PutBits(c_r32.d, 32);
			WriteFlush(fifo, ofile);
			break;
		case 64:
			c_r64.r64 = m_planes[component]->lf_datum[ypos][xpos];
			fifo->PutBits(c_r64.d[0], 32);
			fifo->PutBits(c_r64.d[1], 32);
			WriteFlush(fifo, ofile);
			break;
		}

	}
}

bool HdrDpxImageElement::IsNextSame(uint32_t xpos, uint32_t ypos)
{
	bool same = true;
	int num_components;

	if (xpos >= m_width - 1)
		return false;
	num_components = GetNumberOfComponents();

	if (m_bpc < 32)		// Integer
	{
		for (int c = 0; c < num_components; ++c)
		{
			if (m_planes[c]->i_datum[ypos][xpos] != m_planes[c]->i_datum[ypos][xpos + 1])
				same = false;
		}
	}
	else
	{   // Float
		for (int c = 0; c < num_components; ++c)
		{
			if (m_planes[c]->lf_datum[ypos][xpos] != m_planes[c]->lf_datum[ypos][xpos + 1])
				same = false;
		}
	}
	return same;
}

void HdrDpxImageElement::WriteLineEnd(Fifo *fifo, ofstream &ofile)
{
	if (fifo->m_fullness & 0x1f)   // not an even multiple of 32 
		fifo->PutDatum(0, 32 - (fifo->m_fullness & 0x1f), m_direction_r2l);
	WriteFlush(fifo, ofile);
}



Dpx::ErrorObject HdrDpxImageElement::WriteImageData(std::ofstream &outfile, bool direction_r2l, bool bswap)
{
	Dpx::ErrorObject err;
	uint32_t xpos, ypos;
	int component;
	int num_components;
	Fifo fifo(16);
	bool is_signed = (m_dpx_imageelement.DataSign == 1);
	uint32_t run_length = 0;
	bool freeze_increment = false;
	unsigned int max_run;
	bool run_type;

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
				WriteDatum(&fifo, 2, outfile);
				for (component = 0; component < num_components; ++component)
					WriteDatum(&fifo, m_planes[component]->i_datum[ypos][xpos], outfile);
			}
			else {
				run_type = IsNextSame(xpos, ypos);
				for (run_length = 1; run_length < m_width - xpos && run_length < max_run - 1; ++run_length)
				{
					if (IsNextSame(xpos + run_length, ypos) != run_type)
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

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxFieldsDataSign f, Dpx::HdrDpxDataSign d)
{
	m_dpx_imageelement.DataSign = static_cast<uint8_t>(d);
}

Dpx::HdrDpxDataSign HdrDpxImageElement::GetHeader(Dpx::HdrDpxFieldsDataSign f)
{
	return static_cast<Dpx::HdrDpxDataSign>(m_dpx_imageelement.DataSign);
}

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxIEFieldsR32 f, float d)
{
	switch (f)
	{
	case Dpx::eReferenceLowDataCode:
		if (m_bpc >= 32)
			m_dpx_imageelement.LowData.f = d;
		else
			m_dpx_imageelement.LowData.d = (uint32_t)(d + 0.5); // Could add bounds checking
		break;
	case Dpx::eReferenceLowQuantity:
		m_dpx_imageelement.LowQuantity = d;
		break;
	case Dpx::eReferenceHighDataCode:
		if (m_bpc >= 32)
			m_dpx_imageelement.HighData.f = d;
		else
			m_dpx_imageelement.HighData.d = (uint32_t)(d + 0.5);
		break;
	case Dpx::eReferenceHighQuantity:
		m_dpx_imageelement.HighQuantity = d;
	}
}

float HdrDpxImageElement::GetHeader(Dpx::HdrDpxIEFieldsR32 f)
{
	switch (f)
	{
	case Dpx::eReferenceLowDataCode:
		if (m_bpc >= 32)
			return m_dpx_imageelement.LowData.f;
		else
			return (float)m_dpx_imageelement.LowData.d;
	case Dpx::eReferenceLowQuantity:
		return m_dpx_imageelement.LowQuantity;
	case Dpx::eReferenceHighDataCode:
		if (m_bpc == 32)
			return m_dpx_imageelement.LowData.f;
		else
			return (float)(m_dpx_imageelement.LowData.d);
	case Dpx::eReferenceHighQuantity:
		return m_dpx_imageelement.HighQuantity;
	}
	return 0;
}


void HdrDpxImageElement::SetHeader(Dpx::HdrDpxFieldsDescriptor f, Dpx::HdrDpxDescriptor d)
{
	if(m_isinitialized)	
		m_warnings.push_back("SetHeader() call to set descriptor ignored; descriptor cannot be changed after IE is initialized");
	else
		m_dpx_imageelement.Descriptor = static_cast<uint8_t>(d);
}

Dpx::HdrDpxDescriptor HdrDpxImageElement::GetHeader(Dpx::HdrDpxFieldsDescriptor f)
{
	return static_cast<Dpx::HdrDpxDescriptor>(m_dpx_imageelement.Descriptor);
}

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxFieldsTransfer f, Dpx::HdrDpxTransfer d)
{
	m_dpx_imageelement.Transfer = static_cast<uint8_t>(d);
}

Dpx::HdrDpxTransfer HdrDpxImageElement::GetHeader(Dpx::HdrDpxFieldsTransfer f)
{
	return static_cast<Dpx::HdrDpxTransfer>(m_dpx_imageelement.Transfer);
}

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxFieldsColorimetric f, Dpx::HdrDpxColorimetric d)
{
	m_dpx_imageelement.Colorimetric = static_cast<uint8_t>(d);
}

Dpx::HdrDpxColorimetric HdrDpxImageElement::GetHeader(Dpx::HdrDpxFieldsColorimetric f)
{
	return static_cast<Dpx::HdrDpxColorimetric>(m_dpx_imageelement.Colorimetric);
}

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxFieldsBitDepth f, Dpx::HdrDpxBitDepth d)
{
	if(m_isinitialized)
		m_warnings.push_back("SetHeader() call to set bit depth is ignored; bit depth cannot be changed after IE is initialized");
	else
		m_dpx_imageelement.BitSize = static_cast<uint8_t>(d);
}

Dpx::HdrDpxBitDepth HdrDpxImageElement::GetHeader(Dpx::HdrDpxFieldsBitDepth f)
{
	return static_cast<Dpx::HdrDpxBitDepth>(m_dpx_imageelement.BitSize);
}

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxFieldsPacking f, Dpx::HdrDpxPacking d)
{
	m_dpx_imageelement.Packing = static_cast<uint16_t>(d);
}

Dpx::HdrDpxPacking HdrDpxImageElement::GetHeader(Dpx::HdrDpxFieldsPacking f)
{
	return static_cast<Dpx::HdrDpxPacking>(m_dpx_imageelement.Packing);
}

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxFieldsEncoding f, Dpx::HdrDpxEncoding d)
{
	m_dpx_imageelement.Encoding = static_cast<uint16_t>(d);
}

Dpx::HdrDpxEncoding HdrDpxImageElement::GetHeader(Dpx::HdrDpxFieldsEncoding f)
{
	return static_cast<Dpx::HdrDpxEncoding>(m_dpx_imageelement.Packing);
}

void HdrDpxImageElement::SetHeader(Dpx::HdrDpxIEFieldsU32 f, uint32_t d)
{
	switch (f)
	{
	case Dpx::eOffsetToData:
		m_dpx_imageelement.DataOffset = d;
		break;
	case Dpx::eEndOfLinePadding:
		m_dpx_imageelement.EndOfLinePadding = d;
		break;
	case Dpx::eEndOfImagePadding:
		m_dpx_imageelement.EndOfImagePadding = d;
	}
}

uint32_t HdrDpxImageElement::GetHeader(Dpx::HdrDpxIEFieldsU32 f)
{
	switch (f)
	{
	case Dpx::eOffsetToData:
		return m_dpx_imageelement.DataOffset;
	case Dpx::eEndOfLinePadding:
		return m_dpx_imageelement.EndOfLinePadding;
	case Dpx::eEndOfImagePadding:
		return m_dpx_imageelement.EndOfImagePadding;
	}
	return 0;
}

int32_t HdrDpxImageElement::GetPixelI(int x, int y, DatumLabel dtype)
{
	//for (std::shared_ptr<DatumPlane> dp : m_planes)
	for (std::shared_ptr<DatumPlane> dp : m_planes)
	{
		if (dp->m_datum_label == dtype)
		{
			return dp->i_datum[y][x];
		}
	}
	ASSERT_MSG(0, "Could not find plane with specified type");
	return 0;
}

double HdrDpxImageElement::GetPixelLf(int x, int y, DatumLabel dtype)
{
	//for (std::shared_ptr<DatumPlane> dp : m_planes)
	for (std::shared_ptr<DatumPlane> dp : m_planes)
	{
		if (dp->m_datum_label == dtype)
		{
			return dp->lf_datum[y][x];
		}
	}
	ASSERT_MSG(0, "Could not find plane with specified type");
	return 0;
}

void HdrDpxImageElement::SetPixelI(int x, int y, DatumLabel dtype, int32_t d)
{
	for (std::shared_ptr<DatumPlane> dp : m_planes)
	{
		if (dp->m_datum_label == dtype)
		{
			dp->i_datum[y][x] = d;
			return;
		}
	}
	ASSERT_MSG(0, "Could not find plane with specified type");
}

void HdrDpxImageElement::SetPixelLf(int x, int y, DatumLabel dtype, double d)
{
	for (std::shared_ptr<DatumPlane> dp : m_planes)
	{
		if (dp->m_datum_label == dtype)
		{
			dp->lf_datum[y][x] = d;
			return;
		}
	}
	ASSERT_MSG(0, "Could not find plane with specified type");
}

void HdrDpxImageElement::AddPlane(DatumLabel dlabel)
{
	ASSERT_MSG(m_isinitialized, "Can't add a plane until image element is initialized");

	m_planes.push_back(std::shared_ptr<DatumPlane>(new DatumPlane(m_width, m_height, m_datum_type, dlabel)));
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
