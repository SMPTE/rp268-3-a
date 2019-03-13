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

#if !defined(LOGGING_H)
#define LOGGING_H 1

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

extern int global_verbose_level;

#if !defined(__GNUC__)
#define __attribute__(...) /* erase */
#endif

void Err(char* format, ...) __attribute__((noreturn, format(printf,1,2)));  /* error, with trace */
void UErr(char* format, ...) __attribute__((noreturn, format(printf,1,2))); /* user-feedback message, no trace */
void CErr(char* format, ...) __attribute__((noreturn, format(printf,1,2))); /* programming error/assert, with trace */
void PErr(char* format, ...) __attribute__((noreturn, format(printf,1,2))); /* system perror(3), with trace */

#define Assert(condition) Assert_func(condition, #condition, __LINE__, __FILE__, __FUNCTION__)
void Assert_func(int condition, const char* const condition_str,
                 int cline, const char* const cfile, const char* const cfunction);

#endif

