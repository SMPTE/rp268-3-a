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
#include <vector>

namespace Dpx {

	enum ErrorSeverity
	{
		eInformational,
		eWarning,
		eFatal
	};

	enum ErrorCode
	{
		eNoError = 0,
		eValidationError,
		eMissingCoreField,
		eUnexpectedNonzeroBit,
		eFileOpenError,
		eFileReadError,
		eFileWriteError,
		eBadParameter,
		eHeaderLocked
	};

	class ErrorObject
	{
	public:
		explicit operator bool() const {  // Makes return object testable
			return (m_code.size() != 0);
		}
		ErrorObject operator + (ErrorObject const &obj)   // Combine the errors from 2 objects
		{
			ErrorObject err;
			Dpx::ErrorCode code;
			Dpx::ErrorSeverity severity;
			std::string message;
			int i;

			for (i = 0; i < GetNumErrors(); ++i)
			{
				GetError(i, code, severity, message);
				err.LogError(code, severity, message);
			}
			for (i = 0; i < obj.GetNumErrors(); ++i)
			{
				obj.GetError(i, code, severity, message);
				err.LogError(code, severity, message);
			}
			return err;
		}
		ErrorObject& operator += (ErrorObject const &obj)
		{
			Dpx::ErrorCode code;
			Dpx::ErrorSeverity severity;
			std::string message;
			int i;

			for (i = 0; i < obj.GetNumErrors(); ++i)
			{
				obj.GetError(i, code, severity, message);
				this->LogError(code, severity, message);
			}
			return *this;
		}
		void LogError(Dpx::ErrorCode errorcode, Dpx::ErrorSeverity severity, std::string errmsg)
		{
			m_code.push_back(errorcode);
			m_severity.push_back(severity);
			m_message.push_back(errmsg);
			if (severity > m_worst_severity)
				m_worst_severity = severity;
		}
		void GetError(int index, Dpx::ErrorCode &errcode, Dpx::ErrorSeverity &severity, std::string &errmsg) const
		{
			if (index >= m_code.size())
			{
				errcode = eNoError;
				errmsg = "";
				return;
			}
			errcode = m_code[index];
			severity = m_severity[index];
			errmsg = m_message[index];
		}
		int GetNumErrors() const
		{
			return static_cast<int>(m_severity.size());
		}
		Dpx::ErrorSeverity GetWorstSeverity() const
		{
			return m_worst_severity;
		}
		void Clear()
		{
			m_severity.clear();
			m_code.clear();
			m_message.clear();
			m_worst_severity = Dpx::eInformational;
		}
	private:
		std::vector<Dpx::ErrorSeverity> m_severity;
		std::vector<Dpx::ErrorCode> m_code;
		std::vector<std::string> m_message;
		Dpx::ErrorSeverity m_worst_severity = Dpx::eInformational;
	};

}