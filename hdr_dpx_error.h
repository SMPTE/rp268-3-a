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
/** @file hdr_dpx_error.h
	@brief Defines error codes, severities, and ErrorObject. */
#include <string>
#include <vector>

namespace Dpx {

	/** Severity of errors */
	enum ErrorSeverity
	{
		eInformational,  ///< For information only
		eWarning,    ///< Error is warning (file can still be processed)
		eFatal     ///< Error is severe enough that file can no longer be read or written
	};

	/** Error code list */
	enum ErrorCode
	{
		eNoError = 0,  ///< Not an error
		eValidationError,   ///< Validation error
		eMissingCoreField,   ///< A core field is missing or undefined
		eUnexpectedNonzeroBit,   ///< A nonzero bit was detected where a zero bit was expected
		eFileOpenError,    ///< Error occurred while opening the file
		eFileReadError,    ///< Error occurred while reading the file
		eFileWriteError,   ///< Error occurred while writing the file
		eBadParameter,   ///< A bad parameter was passed
		eHeaderLocked   ///< The header was attempted to be modified while it was locked
	};

	/** Error object class */
	class ErrorObject
	{
	public:
		/** Function to make object testable
			@return					true if object has any logged errors or messages */
		explicit operator bool() const { 
			return (m_code.size() != 0);
		}
		/** Can use + operator to combine the errors from 2 objects
			@param obj			Object to combine */
		ErrorObject operator + (ErrorObject const &obj)  
		{
			ErrorObject err;
			Dpx::ErrorCode code;
			Dpx::ErrorSeverity severity;
			std::string message;
			unsigned int i;

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
		/** Can use += operator to append errors from 1 object to another
			@param obj			object to append */
		ErrorObject& operator += (ErrorObject const &obj)  
		{
			Dpx::ErrorCode code;
			Dpx::ErrorSeverity severity;
			std::string message;
			unsigned int i;

			for (i = 0; i < obj.GetNumErrors(); ++i)
			{
				obj.GetError(i, code, severity, message);
				this->LogError(code, severity, message);
			}
			return *this;
		}
		/** Log an error
			@param errorcode			Error code
			@param severity				Error severity
			@param errmsg				Error message */
		void LogError(Dpx::ErrorCode errorcode, Dpx::ErrorSeverity severity, std::string errmsg)
		{
			m_code.push_back(errorcode);
			m_severity.push_back(severity);
			m_message.push_back(errmsg);
			if (severity > m_worst_severity)
				m_worst_severity = severity;
		}
		/** Get the error at the specified index
			@param[in] index			index of the error (starting from 0)
			@param[out] errcode			returns the error code
			@param[out] severity		returns the error severity
			@param[out] errmsg			returns the error message */
		void GetError(unsigned int index, Dpx::ErrorCode &errcode, Dpx::ErrorSeverity &severity, std::string &errmsg) const
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
		/** Returns the number of errors that have been logged */
		unsigned int GetNumErrors() const
		{
			return static_cast<unsigned int>(m_severity.size());
		}
		/** Returns the worst severity level among the errors that have been logged */
		Dpx::ErrorSeverity GetWorstSeverity() const
		{
			return m_worst_severity;
		}
		/** Clear the error log */
		void Clear()
		{
			m_severity.clear();
			m_code.clear();
			m_message.clear();
			m_worst_severity = Dpx::eInformational;
		}
	private:
		std::vector<Dpx::ErrorSeverity> m_severity;   ///< list of severity levels for logged errors
		std::vector<Dpx::ErrorCode> m_code;	    ///< list of error codes for logged errors
		std::vector<std::string> m_message;		///< list of error messages for logged errors
		Dpx::ErrorSeverity m_worst_severity = Dpx::eInformational;   ///< track worst severity of the errors logged
	};

}