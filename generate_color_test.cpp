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

/** @file generate_color_test.cpp
	@brief Defines the entry point for the generate_color_test application.

	This file provides a sample application that creates ST 268-2-compliant files of color test pattern.
	The image element structure can be specified on the command line.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hdr_dpx.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>

using namespace std;


/**
	Prints to cerr a specified message along with the error log from the DPX file

	@param logmessage	Message to include with the error log
	@param f			HdrDpxFile associated with DPX file
*/
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

/** List of colors that are represented in the pattern */
enum bar_colors_e {
	WHITE_75,    ///< 75% white
	YELLOW_75,   ///< 75% yellow
	CYAN_75,     ///< 75% cyan
	GREEN_75,    ///< 75% green
	MAGENTA_75,  ///< 75% magenta
	RED_75,      ///< 75% red
	BLUE_75,     ///< 75% blue
	GRAY_40,     ///< 40% gray
	CYAN_100,    ///< 100% cyan
	BLUE_100,    ///< 100% blue
	WHITE_100,   ///< 100% white
	YELLOW_100,  ///< 100% yellow
	BLACK_0,     ///< 0% black
	RED_100,     ///< 100% red
	GRAY_15,     ///< 15% gray
	BLACK_M2P,	 ///< -2% black
	BLACK_P2P,   ///< 2% black
	BLACK_P4P,   ///< 4% black
	RAMP,        ///< luma ramp
	COLOR_MAX    ///< Do not use
};

/** YCbCr or RGB selection */
enum comp_type_e {
	CT_YCBCR,    ///< Use YCbCr components
	CT_RGB		///< Use RGB components
};

/** Transfer function options */
enum tf_type_e {
	TF_SDR = 0,   ///< Use SDR transfer function
	TF_PQ = 1,    ///< Use PQ transfer function
	TF_HLG = 2   ///< Use HLG transfer function
};

/** structure to represent a rectangle of a specific color */
typedef struct rect_s
{
	uint32_t tl_x;   ///< Top-left corner x coordinate
	uint32_t tl_y;   ///< Top-left corner y coordinate
	uint32_t br_x;   ///< Bottom-right corner x coordinate
	uint32_t br_y;   ///< Bottom-right corner y coordinate
	bar_colors_e color;  ///< color
} rect_t;

/** Simple implementation of abstract class approximating a SMPTE UHD color bar pattern */
class CBColor
{
public:
	CBColor() { }
	/** Constructor 
		@param bpc				Component bit depth
		@param comptype			Whether to use YCbCr or RGB
		@param tftype			Transfer function
		@param usefullrange		true => use full range components, false => use limited range components */
	CBColor(uint8_t bpc, comp_type_e comptype, tf_type_e tftype, bool usefullrange)
	{
		m_ct = comptype;
		if (usefullrange)
		{
			m_offset = 0;
			m_range = (1 << bpc) - 1;
			m_mid = 1 << (bpc - 1);
		}
		else
		{
			m_offset = 16 << (bpc - 8);
			m_range = ((235 - 16) << (bpc - 8)) - 1;
			m_mid = 1 << (bpc - 1);
		}
		// Values taken from SMTPE RP 209-2 and ITU-R BT.2111-1
		if (comptype == CT_YCBCR)
		{
			if (usefullrange)
			{
				std::cerr << "Full-range not available for YCbCr color test\n";
				return;
			}
			if (bpc == 10 && tftype == TF_PQ)
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
				m_isvalid = true;
			}
			else if (bpc == 12 && tftype == TF_PQ)
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
				m_isvalid = true;
			}
			else if (bpc == 8 && tftype == TF_SDR)
			{
				AddColor(WHITE_75, 180, 128, 128);
				AddColor(YELLOW_75, 168, 44, 136);
				AddColor(CYAN_75, 145, 147, 44);
				AddColor(GREEN_75, 133, 63, 52);
				AddColor(MAGENTA_75, 63, 193, 204);
				AddColor(RED_75, 51, 109, 212);
				AddColor(BLUE_75, 28, 212, 120);
				AddColor(GRAY_40, 104, 512, 512);
				AddColor(CYAN_100, 188, 154, 16);
				AddColor(BLUE_100, 32, 240, 118);
				AddColor(WHITE_100, 235, 128, 128);
				AddColor(YELLOW_100, 219, 16, 38);
				AddColor(BLACK_0, 16, 128, 128);
				AddColor(RED_100, 63, 102, 240);
				AddColor(GRAY_15, 49, 128, 128);
				AddColor(BLACK_M2P, 12, 128, 128);
				AddColor(BLACK_P2P, 21, 128, 128);
				AddColor(BLACK_P4P, 25, 128, 128);
				m_isvalid = true;
			}
			else if (bpc == 10 && tftype == TF_SDR)
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
				m_isvalid = true;
			}
			else if (bpc == 12 && tftype == TF_SDR)
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
				m_isvalid = true;
			}
			else
				std::cerr << "CBColor: unsupported bit depth\n";
		}
		else if (comptype == CT_RGB)				// RGB options
		{
			if (usefullrange)
			{
				if (bpc == 10 && tftype == TF_PQ)
				{
					AddColor(WHITE_75, 593, 593, 593);   // BT2111 uses 58% for the 75% values
					AddColor(YELLOW_75, 593, 593, 0);
					AddColor(CYAN_75, 0, 593, 593);
					AddColor(GREEN_75, 0, 593, 0);
					AddColor(MAGENTA_75, 593, 0, 593);
					AddColor(RED_75, 593, 0, 0);
					AddColor(BLUE_75, 0, 0, 593);
					AddColor(GRAY_40, 409, 409, 409);
					AddColor(CYAN_100, 0, 1023, 1023);
					AddColor(BLUE_100, 0, 0, 1023);
					AddColor(WHITE_100, 1023, 1023, 1023);
					AddColor(YELLOW_100, 1023, 1023, 0);
					AddColor(BLACK_0, 0, 0, 0);
					AddColor(RED_100, 1023, 0, 0);
					AddColor(GRAY_15, 153, 153, 153);
					AddColor(BLACK_M2P, 0, 0, 0);
					AddColor(BLACK_P2P, 20, 20, 20);
					AddColor(BLACK_P4P, 41, 41, 41);
					m_isvalid = true;
				}
				else if (bpc == 12 && tftype == TF_PQ)
				{
					AddColor(WHITE_75, 2375, 2375, 2375);
					AddColor(YELLOW_75, 2375, 2375, 0);
					AddColor(CYAN_75, 0, 2375, 2375);
					AddColor(GREEN_75, 0, 0, 2375);
					AddColor(MAGENTA_75, 2375, 0, 2375);
					AddColor(RED_75, 2375, 0, 0);
					AddColor(BLUE_75, 0, 0, 2375);
					AddColor(GRAY_40, 1638, 1638, 1638);
					AddColor(CYAN_100, 0, 4095, 4095);
					AddColor(BLUE_100, 0, 0, 4095);
					AddColor(WHITE_100, 4095, 4095, 4095);
					AddColor(YELLOW_100, 4095, 4095, 0);
					AddColor(BLACK_0, 0, 0, 0);
					AddColor(RED_100, 4095, 0, 0);
					AddColor(GRAY_15, 614, 614, 614);
					AddColor(BLACK_M2P, 0, 0, 0);
					AddColor(BLACK_P2P, 82, 82, 82);
					AddColor(BLACK_P4P, 164, 164, 164);
					m_isvalid = true;
				}
				else
					std::cerr << "CBColor: unsupported bit depth\n";
			}
			else  // narrow range
			{
				if (bpc == 8 && tftype == TF_SDR)
				{
					AddColor(WHITE_75, 180, 180, 180);
					AddColor(YELLOW_75, 180, 180, 16);
					AddColor(CYAN_75, 16, 180, 180);
					AddColor(GREEN_75, 16, 180, 16);
					AddColor(MAGENTA_75, 180, 16, 180);
					AddColor(RED_75, 180, 16, 16);
					AddColor(BLUE_75, 16, 16, 180);
					AddColor(GRAY_40, 104, 104, 104);
					AddColor(CYAN_100, 16, 235, 235);
					AddColor(BLUE_100, 16, 16, 235);
					AddColor(WHITE_100, 235, 235, 235);
					AddColor(YELLOW_100, 235, 235, 16);
					AddColor(BLACK_0, 16, 16, 16);
					AddColor(RED_100, 235, 16, 16);
					AddColor(GRAY_15, 49, 49, 49);
					AddColor(BLACK_M2P, 12, 12, 12);
					AddColor(BLACK_P2P, 20, 20, 20);
					AddColor(BLACK_P4P, 25, 25, 25);
					m_isvalid = true;
				}
				else if (bpc == 10 && tftype == TF_PQ)
				{
					AddColor(WHITE_75, 572, 572, 572);
					AddColor(YELLOW_75, 572, 572, 64);
					AddColor(CYAN_75, 64, 572, 572);
					AddColor(GREEN_75, 64, 572, 64);
					AddColor(MAGENTA_75, 572, 64, 572);
					AddColor(RED_75, 572, 64, 64);
					AddColor(BLUE_75, 64, 64, 572);
					AddColor(GRAY_40, 414, 414, 414);
					AddColor(CYAN_100, 64, 940, 940);
					AddColor(BLUE_100, 64, 64, 940);
					AddColor(WHITE_100, 940, 940, 940);
					AddColor(YELLOW_100, 940, 940, 64);
					AddColor(BLACK_0, 64, 64, 64);
					AddColor(RED_100, 940, 64, 64);
					AddColor(GRAY_15, 195, 195, 195);
					AddColor(BLACK_M2P, 48, 48, 48);
					AddColor(BLACK_P2P, 80, 80, 80);
					AddColor(BLACK_P4P, 99, 99, 99);
					m_isvalid = true;
				}
				else if (bpc == 12 && tftype == TF_PQ)
				{
					AddColor(WHITE_75, 2288, 2288, 2288);
					AddColor(YELLOW_75, 2288, 2288, 256);
					AddColor(CYAN_75, 256, 2288, 2288);
					AddColor(GREEN_75, 256, 2288, 256);
					AddColor(MAGENTA_75, 2288, 256, 2288);
					AddColor(RED_75, 2288, 256, 256);
					AddColor(BLUE_75, 256, 256, 2288);
					AddColor(GRAY_40, 1658, 1658, 1658);
					AddColor(CYAN_100, 256, 3760, 3760);
					AddColor(BLUE_100, 256, 256, 3760);
					AddColor(WHITE_100, 3760, 3760, 3760);
					AddColor(YELLOW_100, 3760, 3760, 256);
					AddColor(BLACK_0, 256, 256, 256);
					AddColor(RED_100, 3760, 256, 256);
					AddColor(GRAY_15, 782, 782, 782);
					AddColor(BLACK_M2P, 192, 192, 192);
					AddColor(BLACK_P2P, 320, 320, 320);
					AddColor(BLACK_P4P, 396, 396, 396);
					m_isvalid = true;
				}
				else if (bpc == 10 && tftype == TF_HLG)
				{
					AddColor(WHITE_75, 721, 721, 721);
					AddColor(YELLOW_75, 721, 721, 64);
					AddColor(CYAN_75, 64, 721, 721);
					AddColor(GREEN_75, 64, 721, 64);
					AddColor(MAGENTA_75, 721, 64, 721);
					AddColor(RED_75, 721, 64, 64);
					AddColor(BLUE_75, 64, 64, 721);
					AddColor(GRAY_40, 414, 414, 414);
					AddColor(CYAN_100, 64, 940, 940);
					AddColor(BLUE_100, 64, 64, 940);
					AddColor(WHITE_100, 940, 940, 940);
					AddColor(YELLOW_100, 940, 940, 64);
					AddColor(BLACK_0, 64, 64, 64);
					AddColor(RED_100, 940, 64, 64);
					AddColor(GRAY_15, 195, 195, 195);
					AddColor(BLACK_M2P, 48, 48, 48);
					AddColor(BLACK_P2P, 80, 80, 80);
					AddColor(BLACK_P4P, 99, 99, 99);
					m_isvalid = true;
				}
				else if (bpc == 12 && tftype == TF_HLG)
				{
					AddColor(WHITE_75, 2884, 2884, 2884);
					AddColor(YELLOW_75, 2884, 2884, 256);
					AddColor(CYAN_75, 256, 2884, 2884);
					AddColor(GREEN_75, 256, 2884, 256);
					AddColor(MAGENTA_75, 2884, 256, 2884);
					AddColor(RED_75, 2884, 256, 256);
					AddColor(BLUE_75, 256, 256, 2884);
					AddColor(GRAY_40, 1656, 1656, 1656);
					AddColor(CYAN_100, 256, 3760, 3760);
					AddColor(BLUE_100, 256, 256, 3760);
					AddColor(WHITE_100, 3760, 3760, 3760);
					AddColor(YELLOW_100, 3760, 3760, 256);
					AddColor(BLACK_0, 256, 256, 256);
					AddColor(RED_100, 3760, 256, 256);
					AddColor(GRAY_15, 782, 782, 782);
					AddColor(BLACK_M2P, 192, 192, 192);
					AddColor(BLACK_P2P, 320, 320, 320);
					AddColor(BLACK_P4P, 396, 396, 396);
					m_isvalid = true;
				}
				else
					std::cerr << "CBColor: unsupported bit depth\n";
			}
		}
		else {
			std::cerr << "CBColor: unsupported bit depth\n";
		}
	}
	/** Get the component values for a pixel of specified color 
		@param[in] color				which color 
		@param[out] d					pointer to array for storing the 3 component values for the pixel
		@param[in] frac					for ramps, specifies the luminance fraction for the current position (from 0-1.0) */
	void GetComponents(bar_colors_e color, int32_t *d, float frac)
	{
		if (color == RAMP)
		{
			int32_t y;
			y = (int32_t)(frac * m_range + m_offset + 0.5);
			if (m_ct == CT_RGB)
			{
				d[0] = d[1] = d[2] = y;
			}
			else
			{
				d[0] = y;
				d[1] = d[2] = m_mid;
			}
		}
		else
			memcpy(d, m_colors[static_cast<int>(color)], sizeof(int32_t) * 3);
	}
	bool m_isvalid = false;  ///< Flag indicating whether configuration is valid
private:
	/** Adds a mapping from a color to a set of 3 components 
		@param c					which color
		@param c1					first component value
		@param c2					second component value
		@param c3					third component value */
	void AddColor(bar_colors_e c, int32_t c1, int32_t c2, int32_t c3)
	{
		m_colors[c][0] = c1;
		m_colors[c][1] = c2;
		m_colors[c][2] = c3;
	}
	int32_t m_colors[COLOR_MAX][3];  ///< Table of colors
	comp_type_e m_ct;   ///< component type
	int32_t m_offset;    ///< black level
	int32_t m_range;    ///< Range of values
	int32_t m_mid;     ///< midpoint of code values
};

/** Tracks a list of non-overlapping rectangles, returns the color of the rectangle at a specific x,y value */
class CBRectangleList
{
public:
	/** Add a rectangle to the list
		@param c					color of rectangle
		@param tl_x					top-left corner x coordinate
		@param tl_y					top-left corner y coordinate
		@param br_x					bottom-right corner x coordinate
		@param br_y					bottom-right corner y coordinate */
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
	/** Get a color at a specific x,y location
		@param x					x coordinate
		@param y					y coordinate */
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
	std::vector<rect_t> m_rect;  ///< list of rectangles
};

/** Round the parameter to the nearest U32 value */
#define ROUND_U32(a) ((uint32_t)(a+0.5))
/** Macro to simplify range comparisons */
#define RECTANGLE_WIDTH(c,w) rx = lx + w; if(x < rx) return c; lx = rx;
/** Color test patter generator */
class ColorBarGenerator
{
public:
	ColorBarGenerator() { }
	/** Constructor 
		@param w				raster width
		@param h				raster height */
	ColorBarGenerator(uint32_t w, uint32_t h)
	{
		m_a = w;
		m_b = h;
		// Note that the resulting pattern is only approximate as the widths are rounded and the ramps are absent
		m_c = ROUND_U32(4 * m_b / 3 / 7);
		m_d = ROUND_U32((m_a - 7 * m_c) / 2);
	}
	/** Retrieve the color of a specific pixel 
		@param x				x coordinate
		@param y				y coordinate
		@return					pixel color */
	bar_colors_e GetPixelColor(uint32_t x, uint32_t y)
	{
		uint32_t lx, rx;

		lx = 0;
		rx = 0;
		if (y < ROUND_U32(7 * m_b / 12))
		{
			// Pattern 1

			RECTANGLE_WIDTH(GRAY_40, m_d);
			RECTANGLE_WIDTH(WHITE_75, m_c);
			RECTANGLE_WIDTH(YELLOW_75, m_c);
			RECTANGLE_WIDTH(CYAN_75, m_c);
			RECTANGLE_WIDTH(GREEN_75, m_c);
			RECTANGLE_WIDTH(MAGENTA_75, m_c);
			RECTANGLE_WIDTH(RED_75, m_c);
			RECTANGLE_WIDTH(BLUE_75, m_c);
			return GRAY_40;
		}
		else if (y < ROUND_U32(8 * m_b / 12))
		{
			// Pattern 2

			RECTANGLE_WIDTH(CYAN_100, m_d);
			RECTANGLE_WIDTH(WHITE_75, 7 * m_c);
			return BLUE_100;
		}
		else if (y < ROUND_U32(9 * m_b / 12))
		{
			RECTANGLE_WIDTH(YELLOW_100, m_d);
			rx = lx + 6 * m_c;
			if (x < rx)
			{
				m_ramp_frac = ((float)(x - lx) / (rx - lx));
				return (RAMP);
			}
			RECTANGLE_WIDTH(BLACK_0, 6 * m_c);  // Ramp not present
			RECTANGLE_WIDTH(WHITE_100, m_c);
			return RED_100;
		}
		else
		{
			RECTANGLE_WIDTH(GRAY_15, m_d);
			RECTANGLE_WIDTH(BLACK_0, ROUND_U32(3 * m_c / 2));
			RECTANGLE_WIDTH(WHITE_100, 2 * m_c);
			RECTANGLE_WIDTH(BLACK_0, ROUND_U32(5 * m_c / 6));
			RECTANGLE_WIDTH(BLACK_M2P, ROUND_U32(m_c / 6));
			RECTANGLE_WIDTH(BLACK_0, ROUND_U32(m_c / 6));
			RECTANGLE_WIDTH(BLACK_P2P, ROUND_U32(m_c / 6));
			RECTANGLE_WIDTH(BLACK_0, ROUND_U32(m_c / 6));
			RECTANGLE_WIDTH(BLACK_P4P, ROUND_U32(m_c / 6));
			RECTANGLE_WIDTH(BLACK_0, m_c);
			return GRAY_15;
		}
	}
	float m_ramp_frac;   ///< Fraction for ramp

private:
	uint32_t m_a, m_b, m_c, m_d; ///< a, b, c, d values in test pattern
};

/** Image element descriptor data structure */
class IEDescriptor
{
public:
	uint8_t descriptor;   ///< descriptor value
	bool h_subs = false;	///< If true, chroma is horizontally subsampled
	bool v_subs = false;	///< If true, chroma is vertically subsampled
};

/** Image element mapper */
class IEMapper
{
public:
	IEMapper() { }
	/** Constructor
		@param corder					Component order (as a string)
		@param chroma					Chroma type (444 => 4:4:4, 422 => 4:2:2, 420 => 4:2:0)
		@param planar					If true, write samples using individual planes rather than interleaved */
	IEMapper(std::string corder, int chroma, bool planar)
	{
		IEDescriptor desc;
		std::vector<Dpx::DatumLabel> comps;
		comps = GetLabelsFromString(corder);
		if (planar)
		{
			if (chroma == 444)    // RGB only supported - no planar YCbCr 4:4:4
			{
				for (auto c : comps)
				{
					std::vector<Dpx::DatumLabel> dlist;
					dlist.push_back(c);
					desc.descriptor = Dpx::DatumListToDescriptor(dlist);
					m_desc_list.push_back(desc);
				}
			}
			if (chroma == 422) // only 2 possibilities: w/ and w/o alpha
			{
				desc.descriptor = Dpx::eDescY;
				m_desc_list.push_back(desc);
				desc.descriptor = Dpx::eDescCbCr;
				desc.h_subs = true;
				m_desc_list.push_back(desc);
				if (corder == "YCbCrA")
				{
					desc.descriptor = Dpx::eDescA;
					desc.h_subs = false;
					m_desc_list.push_back(desc);
				}
			}
			if (chroma == 420) // only 2 possibilities: w/ and w/o alphas
			{
				desc.descriptor = Dpx::eDescY;
				m_desc_list.push_back(desc);
				desc.h_subs = desc.v_subs = true;
				desc.descriptor = Dpx::eDescCb;
				m_desc_list.push_back(desc);
				desc.descriptor = Dpx::eDescCr;
				m_desc_list.push_back(desc);
				if (corder == "YCbCrA")
				{
					desc.descriptor = Dpx::eDescA;
					desc.h_subs = desc.v_subs = false;
					m_desc_list.push_back(desc);
				}
			}
		}
		else if (!corder.compare("CbYCr"))
		{
			desc.descriptor = Dpx::eDescCbYCr;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("CbYCrA"))
		{
			desc.descriptor = Dpx::eDescCbYCrA;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("CbYCrY"))
		{
			desc.descriptor = Dpx::eDescCbYCrY;
			desc.h_subs = true;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("CbYACrYA"))
		{
			desc.descriptor = Dpx::eDescCbYACrYA;
			desc.h_subs = true;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("CYY"))
		{
			desc.descriptor = Dpx::eDescCYY;
			desc.h_subs = true;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("CYAYA"))
		{
			desc.descriptor = Dpx::eDescCYAYA;
			desc.h_subs = true;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("BGR"))
		{
			desc.descriptor = Dpx::eDescBGR;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("BGRA"))
		{
			desc.descriptor = Dpx::eDescBGRA;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("ARGB"))
		{
			desc.descriptor = Dpx::eDescARGB;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("RGB"))
		{
			desc.descriptor = Dpx::eDescRGB;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("RGBA"))
		{
			desc.descriptor = Dpx::eDescRGBA;
			m_desc_list.push_back(desc);
		}
		else if (!corder.compare("ABGR"))
		{
			desc.descriptor = Dpx::eDescABGR;
			m_desc_list.push_back(desc);
		}
	}
	/** Get the number of image elements to map to
		@return						Number of IEs to use */
	uint8_t GetNumberOfIEs()
	{
		return static_cast<uint8_t>(m_desc_list.size());
	}
	/** Get the descriptor to use for a specific image elemment 
		@param idx					Image element index
		@return						Descriptor data structure */
	IEDescriptor GetDescriptor(int idx)
	{
		return m_desc_list[idx];
	}

private:
	std::vector<IEDescriptor> m_desc_list;    ///< Descriptor for each IE
	/** Get a list of datum labels based on an input string 
		@param s					String with a componenet list (e.g., "RGB")
		@return						List of datum labels the correspond with component list */
	std::vector<Dpx::DatumLabel> GetLabelsFromString(std::string s)   
	{
		std::string subs = s;
		std::vector<Dpx::DatumLabel> ret;
		bool alpha_found = false, y_found = false;
		while (subs.length() > 0)
		{
			if (subs.substr(0, 1) == "R")
			{
				ret.push_back(Dpx::DATUM_R);
				subs = subs.substr(1);
			}
			if (subs.substr(0, 1) == "G")
			{
				ret.push_back(Dpx::DATUM_G);
				subs = subs.substr(1);
			}
			if (subs.substr(0, 1) == "B")
			{
				ret.push_back(Dpx::DATUM_B);
				subs = subs.substr(1);
			}
			if (subs.substr(0, 1) == "A" && !alpha_found)
			{
				ret.push_back(Dpx::DATUM_A);
				alpha_found = true;
				subs = subs.substr(1);
			}
			if (subs.substr(0, 1) == "A")
			{
				ret.push_back(Dpx::DATUM_A2);
				subs = subs.substr(1);
			}
			if (subs.substr(0, 1) == "Y" && !y_found)
			{
				ret.push_back(Dpx::DATUM_Y);
				alpha_found = true;
				subs = subs.substr(1);
			}
			if (subs.substr(0, 1) == "Y")
			{
				ret.push_back(Dpx::DATUM_Y2);
				subs = subs.substr(1);
			}
			if (subs.substr(0, 2) == "Cb")
			{
				ret.push_back(Dpx::DATUM_CB);
				subs = subs.substr(2);
			}
			else if (subs.substr(0, 2) == "Cr")
			{
				ret.push_back(Dpx::DATUM_CR);
				subs = subs.substr(2);
			}
			else if (subs.substr(0, 1) == "C")
			{
				ret.push_back(Dpx::DATUM_C);
				subs = subs.substr(1);
			}
		}
		return ret;
	}

};


// Permitted descriptor strings:
//   CbYCr
//   CbYCrA
//   CbYCrY422
//   CbYACrYA422
//   YCbCr422p
//   YCbCrA422p
//   CYY420
//   CYAYA420
//   YCbCr420p
//   YCbCrA420p
//   BGR
//   BGRA
//   ARGB
//   RGB
//   RGBA
//   ABGR

// Example calling sequence for HDR DPX write (single image element case):
/** Main program 
	@param argc				Number of arguments
	@param argv				Arguments */
int main(int argc, char *argv[])
{
	Dpx::ErrorObject err;
	std::string fname = "colorbar.dpx";
	uint32_t width = 1920, height = 1080;
	comp_type_e ctype;
	tf_type_e tftype = TF_PQ;
	bool usefullrange = false;
	uint8_t bpc = 10;
	std::string corder = "CbYCr";
	bool planar = false;
	int chroma = 444;
	int32_t alphaval;
	Dpx::HdrDpxDatumMappingDirection datum_mapping_direction = Dpx::eDatumMappingDirectionL2R; 
	Dpx::HdrDpxByteOrder byte_order = Dpx::eNativeByteOrder;  
	Dpx::HdrDpxPacking packing = Dpx::ePackingPacked;   // 0 => packed, 1 => method A, 2 => Method B
	Dpx::HdrDpxEncoding rle_encoding = Dpx::eEncodingNoEncoding;

	if ((argc % 2) != 1)
	{
		std::cerr << "Expected even number of command line parameters\n";
		return 1;
	}

	if (argc == 1)
	{
		std::cout << "Usage: " << argv[0];
		std::cout << " -o <outfilename>";
		std::cout << " -tf <BT709|HLG|PQ>";
		std::cout << " -corder <corder>";
		std::cout << " -userfr <1|0>";
		std::cout << " -bpc <8|10|12>";
		std::cout << " -planar <1|0>";
		std::cout << " -chroma <444|422|420>";
		std::cout << " -w <width>";
		std::cout << " -h <height>";
		std::cout << " -dmd <l2r|r2l>";
		std::cout << " -order <msbf|lsbf>";
		std::cout << " -packing <packed|ma|mb>";
		std::cout << " -encoding <1|0>\n";
		std::cout << "\n\nSupported list of formats:  CbYCr, CbYCrA, CbYCrY422, CbYACrYA422, YCbCr422p, YCbCrA422p, CYY420, CYAYA420, YCbCr420p, YCbCrA420p, BGR, BGRA, ARGB, RGB, RGBA, ABGR\n";
		return 0;
	}

	// Note that command-line argument error checking is absent
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-o"))
			fname = argv[++i];
		else if (!strcmp(argv[i], "-tf"))
		{
			if (!strcmp(argv[i + 1], "BT709"))
				tftype = TF_SDR;
			else if (!strcmp(argv[i + 1], "HLG"))
				tftype = TF_HLG;
			else
				tftype = TF_PQ;
			i++;
		}
		else if (!strcmp(argv[i], "-corder"))
			corder = argv[++i];
		else if (!strcmp(argv[i], "-usefr"))
			usefullrange = atoi(argv[++i]) == 1;
		else if (!strcmp(argv[i], "-bpc"))
			bpc = static_cast<uint8_t>(atoi(argv[++i]));
		else if (!strcmp(argv[i], "-planar"))
			planar = atoi(argv[++i]) > 0;
		else if (!strcmp(argv[i], "-chroma"))
			chroma = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-w"))
			width = static_cast<uint32_t>(atoi(argv[++i]));
		else if (!strcmp(argv[i], "-h"))
			height = static_cast<uint32_t>(atoi(argv[++i]));
		else if (!strcmp(argv[i], "-dmd"))
			datum_mapping_direction = (strcmp(argv[++i], "r2l") == 0) ? Dpx::eDatumMappingDirectionR2L : Dpx::eDatumMappingDirectionL2R;
		else if (!strcmp(argv[i], "-order"))
		{
			byte_order = (strcmp(argv[i + 1], "msbf") == 0) ? Dpx::eMSBF : ((strcmp(argv[i + 1], "lsbf") == 0) ? Dpx::eLSBF : Dpx::eNativeByteOrder);
			i++;
		}
		else if (!strcmp(argv[i], "-packing"))
		{
			packing = (strcmp(argv[i + 1], "ma") == 0) ? Dpx::ePackingFilledMethodA : ((strcmp(argv[i + 1], "mb") == 0) ? Dpx::ePackingFilledMethodB : Dpx::ePackingPacked);
			i++;
		}
		else if (!strcmp(argv[i], "-encoding"))
			rle_encoding = (atoi(argv[++i]) == 1) ? Dpx::eEncodingRLE : Dpx::eEncodingNoEncoding;
		else
		{
			std::cerr << "Unrecognized parameter: " << argv[i] << "\n";
			return 1;
		}
	}

	cout << "Output file name: " << fname << endl;
	cout << "Transfer function: " << static_cast<int>(tftype) << endl;
	cout << "Component order: " << corder << endl;
	cout << "Use full range?  " << usefullrange << endl;
	cout << "Bit depth:  " << static_cast<int>(bpc) << endl;
	cout << "Use planar format?  " << planar << endl;
	cout << "Picture width:  " << width << endl;
	cout << "Picture hieght:  " << height << endl;
	cout << "Datum mapping direction:  " << static_cast<int>(datum_mapping_direction) << endl;
	cout << "Byte order:  " << static_cast<int>(byte_order) << endl;
	cout << "Packing:  " << static_cast<int>(packing) << endl;
	cout << "RLE encoding:  " << static_cast<int>(rle_encoding) << endl;

	alphaval = (1 << bpc) - 1;   // Always use max alpha

	if (chroma != 444 && chroma != 422 && chroma != 420)
	{
		std::cerr << "invalid chroma option (needs to be 444, 422, or 420)\n";
		return 1;
	}
	if (corder.find("R") != std::string::npos)
		ctype = CT_RGB;
	else
		ctype = CT_YCBCR;

	ColorBarGenerator cbgen(width, height);

	CBColor colormap(bpc, ctype, tftype, usefullrange);
	if (!colormap.m_isvalid)
	{
		std::cerr << "Invalid color test configuration\n";
		return 1;
	}
	IEMapper iemap(corder, chroma, planar);

	uint8_t ie_idx = 0;
	// DPX library calls start here:
	//  Create DPX file object
	Dpx::HdrDpxFile dpxf;

	// Initialization can be in any order, but changes must be frozen before OpenForWriting() call
	dpxf.SetHeader(Dpx::ePixelsPerLine, width);
	dpxf.SetHeader(Dpx::eLinesPerImageElement, height);
	dpxf.SetHeader(Dpx::eDatumMappingDirection, datum_mapping_direction);
	dpxf.SetHeader(Dpx::eByteOrder, byte_order);

	for(ie_idx=0; ie_idx < iemap.GetNumberOfIEs(); ++ie_idx)
	{
		Dpx::HdrDpxImageElement *ie = dpxf.GetImageElement(ie_idx);

		if (bpc == 8)
			ie->SetHeader(Dpx::eBitDepth, Dpx::eBitDepth8); // Bit depth needs to be set before low/high code values
		else if (bpc == 10)
			ie->SetHeader(Dpx::eBitDepth, Dpx::eBitDepth10); // Bit depth needs to be set before low/high code values
		else if (bpc == 12)
			ie->SetHeader(Dpx::eBitDepth, Dpx::eBitDepth12); // Bit depth needs to be set before low/high code values
		else
		{
			std::cerr << "Invalid bit depth\n";
			return 1;
		}

		if (usefullrange)
		{
			ie->SetHeader(Dpx::eReferenceLowDataCode, 0);
			ie->SetHeader(Dpx::eReferenceHighDataCode, static_cast<float>((1 << bpc) - 1));
		}
		else  // Limited range
		{
			ie->SetHeader(Dpx::eReferenceLowDataCode, static_cast<float>(16 << (bpc - 8)));
			ie->SetHeader(Dpx::eReferenceHighDataCode, static_cast<float>(235 << (bpc - 8)));
		}

		ie->SetHeader(Dpx::eDescriptor, static_cast<Dpx::HdrDpxDescriptor>(iemap.GetDescriptor(ie_idx).descriptor));
		ie->SetHeader(Dpx::eEncoding, rle_encoding);
		ie->SetHeader(Dpx::ePacking, packing);

		// Metadata fields can be set at any point
		if (tftype == TF_SDR)
		{
			ie->SetHeader(Dpx::eColorimetricSpecification, Dpx::eColorimetricBT_709);
			ie->SetHeader(Dpx::eTransferCharacteristic, Dpx::eTransferBT_709);
		}
		else if (tftype == TF_PQ)
		{
			ie->SetHeader(Dpx::eColorimetricSpecification, Dpx::eColorimetricBT_2020);
			ie->SetHeader(Dpx::eTransferCharacteristic, Dpx::eTransferBT_2100_PQ_NCL);
		}
		else if (tftype == TF_HLG)
		{
			ie->SetHeader(Dpx::eColorimetricSpecification, Dpx::eColorimetricBT_2020);
			ie->SetHeader(Dpx::eTransferCharacteristic, Dpx::eTransferBT_2100_HLG_NCL);
		}
		else
		{
			std::cerr << "Invalid tf type\n";
			return 1;
		}
	}


	// Set header values as desired (defaults assumed based on image element structure)
	dpxf.SetHeader(Dpx::eRightToUseOrCopyright, "(C) 20XX Not a real copyright");  // Key is a string, value matches data type
	dpxf.SetHeader(Dpx::eDatumMappingDirection, Dpx::eDatumMappingDirectionL2R);


	// Start writing file
	dpxf.OpenForWriting(fname);
	if (dpxf.Validate())
	{
		dump_error_log("Validation errors:\n", dpxf);
	}
	if (!dpxf.IsOk())
	{
		dump_error_log("File write message log:\n", dpxf);
	}

	// If RLE enabled, rows must be written sequentially
	// If RLE disabled, rows can be written in any order
	for (ie_idx = 0; ie_idx < iemap.GetNumberOfIEs(); ++ie_idx)
	{
		Dpx::HdrDpxImageElement *ie = dpxf.GetImageElement(ie_idx);
		std::vector<int32_t> datum_row;
		IEDescriptor desc = iemap.GetDescriptor(ie_idx);
		datum_row.resize(ie->GetRowSizeInDatums());
		for (uint32_t row = 0; row < ie->GetHeight(); ++row)
		{
			uint32_t datum_idx = 0;
			for (uint32_t column = 0; column < ie->GetWidth(); ++column)
			{
				int32_t cbcomps[3];
				bar_colors_e color = cbgen.GetPixelColor(column * (desc.h_subs ? 2 : 1), row * (desc.v_subs ? 2 : 1));
				colormap.GetComponents(color, cbcomps, cbgen.m_ramp_frac);
				for (auto dl : Dpx::DescriptorToDatumList(desc.descriptor))
				{
					if (dl == Dpx::DATUM_A || dl == Dpx::DATUM_A2)
						datum_row[datum_idx++] = alphaval;
					else if (dl == Dpx::DATUM_R || dl == Dpx::DATUM_Y)
						datum_row[datum_idx++] = cbcomps[0];
					else if (dl == Dpx::DATUM_G || dl == Dpx::DATUM_CB)
						datum_row[datum_idx++] = cbcomps[1];
					else if (dl == Dpx::DATUM_B || dl == Dpx::DATUM_CR)
						datum_row[datum_idx++] = cbcomps[2];
					else if (dl == Dpx::DATUM_C)
					{
						if (row & 1)   // odd row (CR)
							datum_row[datum_idx++] = cbcomps[2];
						else           // Even row (CB)
							datum_row[datum_idx++] = cbcomps[1];
					}
					else if (dl == Dpx::DATUM_Y2)
					{
						color = cbgen.GetPixelColor(column * (desc.h_subs ? 2 : 1) + 1, row * (desc.v_subs ? 2 : 1));
						colormap.GetComponents(color, cbcomps, cbgen.m_ramp_frac);
						datum_row[datum_idx++] = cbcomps[0];
					}
				}
			}
			if (datum_idx != (width * Dpx::DescriptorToDatumList(desc.descriptor).size() / (desc.h_subs ? 2 : 1)))
			{
				printf("Unexpected datum index\n");
				getchar();
			}
			ie->App2DpxPixels(row, datum_row.data());
		}
	}

	// Close() automatically called if HdrDpxFile goes out of scope
	dpxf.Close();
	return 0;
}
