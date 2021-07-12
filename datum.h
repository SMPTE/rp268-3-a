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
/** @file datum.h
	@brief Defines datum labels used in DPX files. */

#ifndef DATUM_H
#define DATUM_H
#include <string>

namespace Dpx {

	/** List of datum labels */
	enum DatumLabel {
		DATUM_UNSPEC,		///< Unspecified component
		DATUM_R,			///< Red
		DATUM_G,			///< Green
		DATUM_B,			///< Blue
		DATUM_A,			///< Alpha
		DATUM_Y,			///< Luma
		DATUM_CB,			///< Cb
		DATUM_CR,			///< Cr
		DATUM_Z,			///< Z (depth)
		DATUM_COMPOSITE,	///< Composite
		DATUM_A2,			///< Alpha (2nd sample)
		DATUM_Y2,			///< Luma (2nd sample)
		DATUM_C,			///< Cb (even lines)/Cr (odd lines)
		DATUM_UNSPEC2,		///< Unspecified 2nd component
		DATUM_UNSPEC3,		///< Unspecified 3rd component
		DATUM_UNSPEC4,		///< Unspecified 4th component
		DATUM_UNSPEC5,		///< Unspecified 5th component
		DATUM_UNSPEC6,		///< Unspecified 6th component
		DATUM_UNSPEC7,		///< Unspecified 7th component
		DATUM_UNSPEC8,		///< Unspecified 8th component
		DATUM_SIZE			///< Not used
	}; 

	static std::string DatumLabelToName[] =
	{
		"Unspecified(1)",
		"R",
		"G",
		"B",
		"A",
		"Y",
		"Cb",
		"Cr",
		"Z",
		"Composite",
		"A2",
		"Y2",
		"C",
		"Unspecified(2)",
		"Unspecified(3)",
		"Unspecified(4)",
		"Unspecified(5)",
		"Unspecified(6)",
		"Unspecified(7)",
		"Unspecified(8)"
	};  ///< String representation of each datum label


}

#endif

