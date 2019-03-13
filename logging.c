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

#if (defined(__linux__) && defined(__GNUC__))
#include <execinfo.h> /* for backtrace */
#include <sys/wait.h> /* for bactrace printing, WEXITSTATUS macro */
#include <unistd.h> /* for readlink */
#include <linux/limits.h> /* for PATH_MAX */
#endif
#if defined(ERROR_IS_ASSERT)
#include <assert.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <errno.h>
#include "logging.h"

#ifdef WIN32
#define STRTOI64 _strtoi64
#define STRTOUI64 _strtoui64
#else
#define STRTOI64 strtoll
#define STRTOUI64 strtoull
#endif

#if (defined(__linux__) && defined(__GNUC__))
void err_backtrace_func (void);
#endif

#if ((!defined(LLONG_MAX) || !defined(LLONG_MIN) || !defined(ULLONG_MAX)) && defined(__GNUC__))
#error "Please compile with gcc -std=c99 flag."
#endif

int global_verbose_level;



#if defined(ERROR_IS_ASSERT)
#define BIG_EXIT assert(0)
#else
#define BIG_EXIT exit(2) /* give a chance to the application to use 1 for signalling */
#endif

/* prints "Error" on stderr, then the given argument list with
   printf-line syntax, then a call trace from main to the current
   function in which Err was called. This is supposed to be used for
   generic errors, but see variants below [UCP]Err. */
void Err(char* format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "\nError: ");
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
#if (defined(__linux__) && defined(__GNUC__))
  err_backtrace_func();
#endif
  BIG_EXIT;
}

/* a variant of Err() function, doesn't print a stack trace. Should be
   used when a stack trace is not needed/desired, for example to avoid
   confusing the user when reporting a configuration or invocation error. */
void UErr(char* format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "\nError: ");
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
  BIG_EXIT;
}

/* a variant of the Err() function, differs in printing "Programming
   error" as a prefix to the given message. Use this for places where
   you'd otherwise call assert(3). */
void CErr(char* format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "\nProgramming error: ");
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
#if (defined(__linux__) && defined(__GNUC__))
  err_backtrace_func();
#endif
  BIG_EXIT;
}

/* a variant of the Err() function, it prefixes a call to perror(3)
   with "Error" string and caller-passed string (with printf-like
   syntax). Use this after catching a system error, for example failure
   to open a file. */
void PErr(char* format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "\nError: ");
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, ": ");
  perror("");
#if (defined(__linux__) && defined(__GNUC__))
  err_backtrace_func();
#endif
  BIG_EXIT;
}

/* replacement for assert(3) function: prints also the stack call trace */
void Assert_func(int condition, const char* const condition_str,
                 int cline, const char* const cfile, const char* const cfunction) {
  if (condition) {
    return;
  }
  fprintf(stderr, "\nAssertion failure in function %s (%s:%d): %s",
          cfunction, cfile, cline, condition_str);
#if (defined(__linux__) && defined(__GNUC__))
  err_backtrace_func();
#endif
  BIG_EXIT;
}


#if (defined(__linux__) && defined(__GNUC__))
/* Called from the Err-variety macros, to print the call trace at the
   moment of the error. It's not the neatest code. From backtrace(3)
   and backtrace_symbols(3) it obtains the stack of addresses, and
   feeds them to addr2line(1) and, through that, to a perl script for
   folding pairs of lines (function name and invocation location).

   The alternative is to fork, exec a shell with the addr2line|perl,
   open a one-way pipe to the child, and fprintf the addresses to that
   pipe, close the pipe-fd, wait(2) for the child to finish, then
   return. */
void err_backtrace_func (void) {
  void* a[40];
  int n = backtrace(a, 40);
  if (n > 0) {

    fprintf(stderr, "\nBacktrace:\n");
    char** c = backtrace_symbols(a, n);
    if (NULL != c) {
      for (int i=2; i<n; i++) { /* skip this function and the caller (one of the Err-variants) */
	/* ./vdec(err_backtrace_func+0x1c) [0x448938] */
	char* addr = strrchr(c[i], 'x');
	if (NULL != addr) addr++; /* skip 'x' */
	char* closing_bracket = strchr(addr, ']');
	/* forcefully terminate the address string to not include ']': */
	if (NULL != closing_bracket) {
	  *closing_bracket = 0;
	}
	if (NULL == addr) {
	  fprintf(stderr, "  %s\n", c[i]); /* we don't have anything better */
	} else {
	  /* addr2line -s -e vdec -f 448978 4488a8 |
	     perl -ane '{chomp; if (length $b) { print "$b ($_)\n"; $b=""; } else { $b=$_; } }' */
	  /* obtain the full-path of the executable from
	     /proc/self/exe. This works only on Linux, but we're
	     already under that #ifdef */
	  char program_full_path[PATH_MAX+1] = "";
///extern int readlink (char* path, char* buf, size_t len); /* somehow cannot find it in unistd */ // Changing to std=gnu99 fixes
	  int len = readlink("/proc/self/exe", program_full_path, (size_t)PATH_MAX);
	  if (-1 == len) {
	    PErr("cannot follow symlink /proc/self/exe");
	  }
	  program_full_path[len] = 0; /* readlink(2) doesn't zero-terminate */
	  char aline[1024] = "";
	  sprintf(aline, "addr2line -s -e %s -f %s | "
		  "perl -ane '{chomp; if (length $b) {"
		                     "  print qq[  $b ($_)\n]; if ($b eq q(main)){exit 2;} $b=q(); "
		                     "} else { $b=$_;} };' 1>&2",
		  program_full_path, addr);
	  int ret = system(aline);
	  if ((NULL != strstr(c[i], "main+")) || (2 == WEXITSTATUS(ret))) {
	    break; /* climb only up to main */
	  }
	}
      }
      free(c);
    }
  }
}
#endif /* if(__linux__,__GNUC__) */



