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

#ifndef DATUM_H
#define DATUM_H
#include <string>

namespace Dpx {

	enum DatumLabel {
		DATUM_UNSPEC, DATUM_R, DATUM_G, DATUM_B, DATUM_A, DATUM_Y, DATUM_CB, DATUM_CR, DATUM_Z, DATUM_COMPOSITE, DATUM_A2, DATUM_Y2, DATUM_C,
		DATUM_UNSPEC2, DATUM_UNSPEC3, DATUM_UNSPEC4, DATUM_UNSPEC5, DATUM_UNSPEC6, DATUM_UNSPEC7, DATUM_UNSPEC8, DATUM_SIZE
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
	};


}

#endif

