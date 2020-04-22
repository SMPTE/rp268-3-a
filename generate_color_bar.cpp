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
#include <vector>

#include "hdr_dpx.h"

// Simple implementation of abstract class approximating a SMPTE UHD color bar pattern
enum bar_colors_e {
	WHITE_75,
	YELLOW_75,
	CYAN_75,
	GREEN_75,
	MAGENTA_75,
	RED_75,
	BLUE_75,
	GRAY_40,
	CYAN_100,
	BLUE_100,
	WHITE_100,
	YELLOW_100,
	BLACK_0,
	RED_100,
	GRAY_15,
	BLACK_M2P,
	BLACK_P2P,
	BLACK_P4P,
	COLOR_MAX
};
enum comp_type_e {
	CT_YCBCR,
	CT_RGB		// Not supported at this time
};
typedef struct rect_s
{
	uint32_t tl_x;
	uint32_t tl_y;
	uint32_t br_x;
	uint32_t br_y;
	bar_colors_e color;
} rect_t;
class CBColor
{
public:
	CBColor(uint8_t bpc, comp_type_e comptype, bool ishdr)
	{
		if (comptype != CT_YCBCR)
		{
			std::cerr << "CBColor: nly YCbCr supported\n";
			return;
		}
		if (bpc == 10 && ishdr)
		{
			AddColor(WHITE_75, 721, 512, 512);
			AddColor(YELLOW_75, 682, 176, 539);
			AddColor(CYAN_75, 548, 606, 176);
			AddColor(GREEN_75, 509, 270, 208);
			AddColor(MAGENTA_75, 276, 754, 821);
			AddColor(RED_75, 237, 418, 848);
			AddColor(BLUE_75, 103, 848, 485);
			AddColor(GRAY_40, 414, 512, 512);
			AddColor(CYAN_100, 710, 637, 64);
			AddColor(BLUE_100, 116, 960, 476);
			AddColor(WHITE_100, 940, 512, 512);
			AddColor(YELLOW_100, 888, 64, 548);
			AddColor(BLACK_0, 64, 512, 512);
			AddColor(RED_100, 294, 387, 960);
			AddColor(GRAY_15, 195, 512, 512);
			AddColor(BLACK_M2P, 46, 512, 512);
			AddColor(BLACK_P2P, 82, 512, 512);
			AddColor(BLACK_P4P, 99, 512, 512);
		}
		else if (bpc == 12 && ishdr)
		{
			AddColor(WHITE_75, 2884, 2048, 2048);
			AddColor(YELLOW_75, 2728, 704, 2156);
			AddColor(CYAN_75, 2194, 2423, 704);
			AddColor(GREEN_75, 2038, 1079, 812);
			AddColor(MAGENTA_75, 1102, 3017, 3284);
			AddColor(RED_75, 946, 1673, 3392);
			AddColor(BLUE_75, 412, 3392, 1940);
			AddColor(GRAY_40, 1658, 2048, 2048);
			AddColor(CYAN_100, 2839, 2548, 256);
			AddColor(BLUE_100, 464, 2840, 1904);
			AddColor(WHITE_100, 3760, 2048, 2048);
			AddColor(YELLOW_100, 3552, 256, 2192);
			AddColor(BLACK_0, 256, 2048, 2048);
			AddColor(RED_100, 1177, 1548, 3840);
			AddColor(GRAY_15, 782, 2048, 2048);
			AddColor(BLACK_M2P, 186, 2048, 2048);
			AddColor(BLACK_P2P, 326, 2048, 2048);
			AddColor(BLACK_P4P, 396, 2048, 2048);
		}
		else if (bpc == 10 && !ishdr)
		{
			AddColor(WHITE_75, 721, 512, 512);
			AddColor(YELLOW_75, 674, 176, 543);
			AddColor(CYAN_75, 581, 589, 176);
			AddColor(GREEN_75, 534, 253, 207);
			AddColor(MAGENTA_75, 251, 771, 817);
			AddColor(RED_75, 204, 435, 848);
			AddColor(BLUE_75, 111, 848, 481);
			AddColor(GRAY_40, 414, 512, 512);
			AddColor(CYAN_100, 755, 615, 64);
			AddColor(BLUE_100, 127, 960, 471);
			AddColor(WHITE_100, 940, 512, 512);
			AddColor(YELLOW_100, 877, 64, 553);
			AddColor(BLACK_0, 64, 512, 512);
			AddColor(RED_100, 250, 409, 960);
			AddColor(GRAY_15, 195, 512, 512);
			AddColor(BLACK_M2P, 46, 512, 512);
			AddColor(BLACK_P2P, 82, 512, 512);
			AddColor(BLACK_P4P, 99, 512, 512);
		}
		else if (bpc == 12 && !ishdr)
		{
			AddColor(WHITE_75, 2884, 2048, 2048);
			AddColor(YELLOW_75, 2694, 704, 2171);
			AddColor(CYAN_75, 2325, 2356, 704);
			AddColor(GREEN_75, 2136, 1012, 827);
			AddColor(MAGENTA_75, 1004, 3084, 3269);
			AddColor(RED_75, 815, 1740, 3392);
			AddColor(BLUE_75, 446, 3392, 1925);
			AddColor(GRAY_40, 1658, 2048, 2048);
			AddColor(CYAN_100, 3015, 2459, 256);
			AddColor(BLUE_100, 509, 3840, 1884);
			AddColor(WHITE_100, 3760, 2048, 2048);
			AddColor(YELLOW_100, 3507, 256, 2212);
			AddColor(BLACK_0, 256, 2048, 2048);
			AddColor(RED_100, 1001, 1637, 3840);
			AddColor(GRAY_15, 782, 2048, 2048);
			AddColor(BLACK_M2P, 186, 2048, 2048);
			AddColor(BLACK_P2P, 326, 2048, 2048);
			AddColor(BLACK_P4P, 396, 2048, 2048);
		}
		else {
			std::cerr << "CBColor: unsupported bit depth\n";
		}
	}
	void GetComponents(bar_colors_e color, int32_t *d)
	{
		memcpy(d, m_colors[static_cast<int>(color)], sizeof(int32_t) * 3);
	}
private:
	void AddColor(bar_colors_e c, int32_t y, int32_t cb, int32_t cr)
	{
		m_colors[c][0] = cb;
		m_colors[c][1] = y;
		m_colors[c][2] = cr;
	}
	int32_t m_colors[COLOR_MAX][3];
};

class CBRectangleList
{
public:
	void AddRectangle(bar_colors_e c, uint32_t tl_x, uint32_t tl_y, uint32_t br_x, uint32_t br_y)
	{
		rect_t r;
		r.tl_x = tl_x;
		r.tl_y = tl_y;
		r.br_x = br_x;
		r.br_y = br_y;
		r.color = c;
		m_rect.push_back(r);
	}
	bar_colors_e GetColor(uint32_t x, uint32_t y)
	{
		for (rect_t r : m_rect)
		{
			if (x >= r.tl_x && y >= r.tl_y && x < r.br_x && y < r.br_y)
				return r.color;
		}
		return COLOR_MAX;
	}
private:
	std::vector<rect_t> m_rect;
};

#define ROUND_U32(a) ((uint32_t)(a+0.5))
#define RECTANGLE_WIDTH(c,w) rx = lx + w; m_rlist.AddRectangle(c, lx, ty, rx, by); lx += w; 
class ColorBarGenerator
{
public:
	ColorBarGenerator(uint32_t w, uint32_t h, uint8_t bpc, bool ishdr)
	{
		uint32_t a, b, c, d;
		uint32_t lx, rx, ty, by;

		m_colors = std::unique_ptr<CBColor>(new CBColor(bpc, CT_YCBCR, ishdr));
		m_width = w;
		m_height = h;

		// Note that the resulting pattern is only approximate as the widths are rounded and the ramps are absent
		a = w;
		b = h;
		c = ROUND_U32(4 * b / 3 / 7);
		d = ROUND_U32((a - 7 * c) / 2);

		// Pattern 1
		lx = 0;
		ty = 0;
		by = ROUND_U32(7 * b / 12);

		RECTANGLE_WIDTH(GRAY_40, d);
		RECTANGLE_WIDTH(WHITE_75, c);
		RECTANGLE_WIDTH(YELLOW_75, c);
		RECTANGLE_WIDTH(CYAN_75, c);
		RECTANGLE_WIDTH(GREEN_75, c);
		RECTANGLE_WIDTH(MAGENTA_75, c);
		RECTANGLE_WIDTH(RED_75, c);
		RECTANGLE_WIDTH(BLUE_75, c);
		m_rlist.AddRectangle(GRAY_40, lx, ty, a, by);

		// Pattern 2
		lx = 0;
		ty = ROUND_U32(7 * b / 12);
		by = ROUND_U32(8 * b / 12);

		RECTANGLE_WIDTH(CYAN_100, d);
		RECTANGLE_WIDTH(WHITE_75, 7 * c);
		m_rlist.AddRectangle(BLUE_100, lx, ty, a, by);

		// Pattern 3
		lx = 0;
		ty = ROUND_U32(8 * b / 12);
		by = ROUND_U32(9 * b / 12);

		RECTANGLE_WIDTH(YELLOW_100, d);
		RECTANGLE_WIDTH(BLACK_0, 6 * c);  // Ramp not present
		RECTANGLE_WIDTH(WHITE_100, c);
		m_rlist.AddRectangle(RED_100, lx, ty, a, by);

		// Pattern 4 (no sub-black valley or super-white peak)
		lx = 0;
		ty = ROUND_U32(9 * b / 12);
		by = b;

		RECTANGLE_WIDTH(GRAY_15, d);
		RECTANGLE_WIDTH(BLACK_0, ROUND_U32(3 * c / 2));
		RECTANGLE_WIDTH(WHITE_100, 2 * c);
		RECTANGLE_WIDTH(BLACK_0, ROUND_U32(5 * c / 6));
		RECTANGLE_WIDTH(BLACK_M2P, ROUND_U32(c / 6));
		RECTANGLE_WIDTH(BLACK_0, ROUND_U32(c / 6));
		RECTANGLE_WIDTH(BLACK_P2P, ROUND_U32(c / 6));
		RECTANGLE_WIDTH(BLACK_0, ROUND_U32(c / 6));
		RECTANGLE_WIDTH(BLACK_P4P, ROUND_U32(c / 6));
		RECTANGLE_WIDTH(BLACK_0, c);
		m_rlist.AddRectangle(GRAY_15, lx, ty, a, by);

		m_row_of_samples.resize(3 * m_width);
		m_row_ptr = m_row_of_samples.data();
	}
	int32_t *GetPixelRow(uint32_t y)
	{
		uint32_t xpos;
		bar_colors_e cb;

		for (xpos = 0; xpos < m_width; ++xpos)
		{
			cb = m_rlist.GetColor(xpos, y);
			m_colors->GetComponents(cb, &(m_row_ptr[xpos * 3]));
		}
		return m_row_ptr;
	}
	void App2DpxComponentList(Dpx::DatumLabel *clist, int &nlabels)
	{
		clist[0] = Dpx::DATUM_CB;
		clist[1] = Dpx::DATUM_Y;
		clist[2] = Dpx::DATUM_CR;
	}

private:
	std::unique_ptr<CBColor> m_colors;
	CBRectangleList m_rlist;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_c, m_d, m_e, m_f, m_g, m_h;
	uint32_t m_i, m_i2, m_j, m_j4, m_k, m_m;
	std::vector<int32_t> m_row_of_samples;
	int32_t *m_row_ptr;
};


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



// Example calling sequence for HDR DPX write (single image element case):
void example_write()
{
	Dpx::ErrorObject err;
	std::string fname = "colorbar.dpx";
	const uint32_t width = 1920, height = 1080;
	Dpx::HdrDpxFile dpxf;
	Dpx::HdrDpxImageElement *ie = dpxf.GetImageElement(0);
	ColorBarGenerator cbgen(width, height, 10, 1);

	// Initialization can (mostly) be in any order, but changes must be frozen before Open() call
	dpxf.SetHeader(Dpx::ePixelsPerLine, width);
	dpxf.SetHeader(Dpx::eLinesPerElement, height);

	ie->SetHeader(Dpx::eBitDepth, Dpx::eBitDepth10); // Bit depth needs to be set before low/high code values
	ie->SetHeader(Dpx::eDescriptor, Dpx::eDescCbYCr);
	ie->SetHeader(Dpx::eEncoding, Dpx::eEncodingRLE);
	ie->SetHeader(Dpx::ePacking, Dpx::ePackingPacked);

	// Metadata fields can be set at any point
	ie->SetHeader(Dpx::eColorimetric, Dpx::eColorimetricBT_2020);
	ie->SetHeader(Dpx::eTransfer, Dpx::eTransferBT_2020_NCL);

	// Set header values as desired (defaults assumed based on image element structure)
	dpxf.SetHeader(Dpx::eCopyright, "(C) 20XX Not a real copyright");  // Key is a string, value matches data type
	dpxf.SetHeader(Dpx::eDatumMappingDirection, Dpx::eDatumMappingDirectionL2R);
	if (dpxf.Validate())
	{
		dump_error_log("Validation errors:\n", dpxf);
	}

	// Start writing file
	dpxf.WriteFile(fname);
	if (!dpxf.IsOk())
	{
		dump_error_log("File write message log:\n", dpxf);
	}

	// If RLE enabled, rows must be written sequentially
	// If RLE disabled, rows can be written in any order
	for(uint32_t row = 0; row < height; ++row)
		ie->App2DpxPixels(row, cbgen.GetPixelRow(row));

	// Close() automatically called if HdrDpxFile goes out of scope
	dpxf.Close();
}
