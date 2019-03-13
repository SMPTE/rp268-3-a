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

#if (defined(__APPLE__) && defined(__GNUC__))
#include <execinfo.h> /* for backtrace */
#include <sys/wait.h> /* for bactrace printing, WEXITSTATUS macro */
#include <unistd.h> /* for readlink */
#include <limits.h> /* for PATH_MAX */
#include <dirent.h>
#include <sys/wait.h>
#elif (defined(__linux__) && defined(__GNUC__))
#include <execinfo.h> /* for backtrace */
#include <sys/wait.h> /* for bactrace printing, WEXITSTATUS macro */
#include <unistd.h> /* for readlink */
#include <linux/limits.h> /* for PATH_MAX */
#include <dirent.h>
#include <sys/wait.h>
#else
#include <direct.h> /* directory functions */
#define PATH_MAX 256
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include "vdo.h"
#include "utl.h"
#include "logging.h"
#include "cmd_parse.h"

#if ((!defined(LLONG_MAX) || !defined(LLONG_MIN) || !defined(ULLONG_MAX)) && defined(__GNUC__))
#error "Please compile with gcc -std=c99 flag."
#endif

// determine if path ends with .<ext>
int has_ext(const char * path) {
  int   path_len  = strlen(path);
  char* dot_pos   = strrchr(path, '.');
  char* slash_pos = strrchr(path, '/');

  if (    dot_pos   == NULL 
      ||  path_len  == 1
      || (path_len  == 2    && path[0] == '.' && path[1] == '.')
      || (slash_pos != NULL && dot_pos < slash_pos)) { // no extension
    return 0;  
  }

  return 1;
}

// copy path name after locating and removing extension (full path name if no ext)
//   - example ../abc/def.006.ghi => ../abc/def.006
//   - must free return value: result is malloced
char* chop_ext(const char * path) {
  int   path_len  = strlen(path);
  char* dot_pos   = strrchr(path, '.');
  char* slash_pos = strrchr(path, '/');
  char* ret_string;
  int ret_len;

  if (    dot_pos   == NULL 
      ||  path_len  == 1
      || (path_len  == 2    && path[0] == '.' && path[1] == '.')
      || (slash_pos != NULL && dot_pos < slash_pos)) { // no extension
    ret_string = (char*) malloc((path_len+1)*sizeof(char));
    return strcpy(ret_string, path);  
  }

  ret_len = dot_pos-path;
  ret_string  = (char*) malloc((ret_len+1)*sizeof(char));
  strncpy(ret_string, path, ret_len);  
  ret_string[ret_len] = '\0';
  return ret_string;  
}

// copy file name after locating and removing the dir (full path name if no dir)
//   - example "../abc/def.006.ghi" => "def.006.ghi"
//             "."   => ""
//             ".."  => ""
//             "./"  => ""
//             "../" => ""
//             "/dev" => "dev"
//   - must free return value: result is malloced
char* chop_dir(const char * path) {
  int   path_len  = strlen(path);
  char* ret_string;
  char *slash_pos;
  int ret_len;

  if (   (path_len  == 1 && path[0] == '.')
      || (path_len  == 2 && path[0] == '.' && path[1] == '.')) { // no extension
    ret_string = (char*) malloc(sizeof(char));
    ret_string[0] = '\0';  
    return ret_string;  
  }
  
  slash_pos = strrchr(path, '/');

  if (slash_pos == NULL) { // no extension
    ret_string = (char*) malloc((path_len+1)*sizeof(char));
    return strcpy(ret_string, path);  
  }

  slash_pos += 1;  // skip slash
  ret_len = path+path_len-slash_pos;
  ret_string  = (char*) malloc((ret_len+1)*sizeof(char));
  return strcpy(ret_string, slash_pos);  
}

// locate and copy extension in path name (NULL is no extension)
//   - example ../abc/def.006.ghi => ghi
//   - must free non-NULL return value: result is malloced
char* file_ext(const char * path) {
  int   path_len  = strlen(path);
  char* dot_pos   = strrchr(path, '.');
  char* slash_pos = strrchr(path, '/');
  char* ret_string;
  int ret_len;

  if (    dot_pos   == NULL 
      ||  path_len  == 1
      || (path_len  == 2    && path[0] == '.' && path[1] == '.')
      || (slash_pos != NULL && dot_pos < slash_pos)) { // no extension
    return NULL;  
  }

  dot_pos++;
  ret_len = path+path_len-dot_pos;
  ret_string = (char*) malloc((ret_len+1)*sizeof(char));
  return strcpy(ret_string, dot_pos);
}

// locate and copy directory (directory defined by text preceding /)
//   - example "../abc/def.006.ghi" => "../abc"
//             "."   => "."
//             ".."  => ".."
//             "./"  => "."
//             "../" => ".."
//             "/dev" => "/"
//   - must free return value: result is malloced
char* file_dir(const char * path) {
  int   path_len  = strlen(path);
  char* ret_string;
  char *slash_pos;
  int ret_len;

  if (   (path_len  == 1 && path[0] == '.')
      || (path_len  == 2 && path[0] == '.' && path[1] == '.')) { // no extension
    ret_string = (char*) malloc((path_len+1)*sizeof(char));
    return strcpy(ret_string, path);  
  }
  
  slash_pos = strrchr(path, '/');

  if (slash_pos == NULL) { // no path
    ret_string = (char*) malloc(2*sizeof(char));
    return strcpy(ret_string, ".");  
  }

  ret_len = slash_pos-path;
  if (0 == ret_len) {  // root dir special condition
    ret_string  = (char*) malloc(2*sizeof(char));
    return strcpy(ret_string, "/");
  }
  ret_string  = (char*) malloc((ret_len+1)*sizeof(char));
  strncpy(ret_string, path, ret_len);  
  ret_string[ret_len] = '\0';
  return ret_string;  
}

// returns 1 on success; 0 on empty input; fatal exit on error
int easy_mkdir (const char * dn) {
  char* justdir = NULL;
  int   isolate_path;
  int ret;
#ifndef WIN32
  DIR* dd;
#endif

  if ((NULL == dn) || (0 == dn[0])) {
    return 0;
  }
  isolate_path = has_ext(dn);
  if (isolate_path) {
    justdir = file_dir(dn);
  }
  
#ifdef WIN32
  ret = _chdir(isolate_path ? justdir : dn);
#else
  dd = opendir(isolate_path? justdir: dn);
  ret = (dd == NULL);
#endif
  if (ret) { // cannot open it, need to create it
	char cmd[PATH_MAX+200+1] = "mkdir -p ";
#ifdef WIN32
	ret = _mkdir(isolate_path?justdir:dn);
	if(ret!=0)
		PErr("mkdir failed '%s'\n", isolate_path ? justdir:dn);
#else
    strncat(cmd, isolate_path? justdir: dn, PATH_MAX+200);
    ret = system(cmd); // simplest way
    if (-1 == ret) {
      PErr("call to system failed executing '%s'", cmd);
    }
    if (WEXITSTATUS(ret) != 0) {
      PErr("cannot create directory '%s'", isolate_path? justdir: dn);
    }
#endif
  }
#ifndef WIN32
  closedir(dd);
#endif
  if (isolate_path) {
    free(justdir);
  }
  
  return 1;
}

// int strisnum(char* in)
//  determines if string is a number
//  Return int - (0) not number; (2) integer; (3) neg integer; (4) floating point; (5) floating point

int strisnum(char* in)
{
  int lng      = strlen(in);
  int neg_num  = ('-' == in[0]);
  int found_pt = 0;
  int i;
  for (i=neg_num ; i<lng; i++) {
    if ('.' == in[i]) {
      if (found_pt++) {
        return 0;
      }
    }
    else if (!isdigit(in[i])) {
      return 0;
    }
  }
  return found_pt? 4+neg_num
                 : 2+neg_num;
}

// int isrange(char* in)
//  determines if string is a number
//  Return int - (0) not number; (1) integer; (2) neg integer

int strisrange(char* in)
{
  int lng      = strlen(in);
  int neg_num  = ('-' == in[0]);
  int found_dd = 0;
  int found_st = 0;
  int found_ed = 0;
  int i;
  for (i=neg_num ; i<lng; i++) {
    if ('.' == in[i] && '.' == in[i+1]) {
      if (found_dd++) {
        return 0;
      }
      i++;
      if ((i+1<lng) && ('-' == in[i+1])){
        neg_num = 1;
        i++;
      }
    }
    else if (!isdigit(in[i])) {
      return 0;
    }
    else if (found_dd) {
      found_ed = 1;
    }
    else {
      found_st = 1;
    }
  }
  return (found_ed&&found_st) ? 1 + neg_num: 0;
}

// int isdim(char* in)
//  determines if string is a number
//  Return int - (0) not number; (1) integer; (2) neg integer

int strisdim(char* in, char sep)
{
  int lng     = strlen(in);
  int neg_num = ('-' == in[0]);
  int found_x = 0;
  int found_h = 0;
  int found_v = 0;
  int i;
  for (i=neg_num ; i<lng; i++) {
    if (sep == in[i]) {
      if (found_x++) {
        return 0;
      }
      if ('-' == in[i+1]){
        neg_num = 1;
        i++;
      }
    }
    else if (!isdigit(in[i])) {
      return 0;
    }
    else if (found_x) {
      found_v = 1;
    }
    else {
      found_h = 1;
    }
  }
  return (found_v&&found_h) ? 1 + neg_num: 0;
}

// char* lower_case(char* str)
//  Convert str into lowercase characters
//
char* lower_case(char *str)
{
  int i;
  for (i=0; str[i]!='\0'; i++) {
    str[i] = tolower(str[i]);
  }
  return str;
}

void change_ext(char* path, char* ext)
{
  strtok(path, ".");
  sprintf(path, "%s.%s", path, ext);
}


// char *splitstring(char* in, char *sep)
//  Split "in" string at first occurance of char sequence consisting of chars
//    listed in sep array.
//  In is shorted (adding \0) at first occurance. 
//  Return char* - (NULL) no occurence of sep string; (!NULL) ptr to suffix

char *splitstring(char* in, char *sep)
{
  int   pref_lng;
  char *suff;
  if ((NULL == in) || (0 == in[0])) { // empty input
    return NULL;
  }
  
  pref_lng = strcspn(in, sep);
  suff     = in+pref_lng;
  if (0 == *suff) {
    return NULL;
  }
  suff += strspn(suff, sep);
  in[pref_lng] = '\0';

  return suff;
}

// char *splitstring_exact(char* in, char *substr)
//  Split "in" string at first occurance of substr.
//  In is shorted (adding \0) at first occurance. 
//  Return char* - (NULL) no occurence of substr string; (!NULL) ptr to suffix after substr

char *splitstring_exact(char* in, char *substr)
{
  char*  suff;
  if ((NULL == in) || (0 == in[0])) { // empty input
    return NULL;
  }
  
  suff = strstr(in, substr);
  if (NULL == suff) {
    return NULL;
  }
  suff[0] = '\0';
  suff += strlen(substr);
  if (0 == suff[0]) {
    return NULL;
  }

  return suff;
}

// char *splitstring_exact_strict(char* in, char *substr, const char *arg)
//   Wrap splitstring_exact in an error feedback
char *splitstring_exact_strict(char* in, char *substr, const char *arg)
{
  char* suff = splitstring_exact(in, substr);
  if ((NULL == suff) || (0 == suff[0])) {
    UErr("Cannot locate '%s' in %s\n", substr, arg);
  }

  return suff;
}


// int str2i(char* arg, const char* title)
//   parse # into int
int str2i(char* arg, const char* title) {
  long int res = str2l(arg, title);
#if (INT_MAX != LONG_MAX) 
  if ((res > INT_MAX) || (res < INT_MIN)) {
    if (NULL == title) { title = "Argument";}
    Err("%s value (%s) exceeds integer range", title, arg);
  }
#endif
  return (int) res;
}

// int str2p(char* arg, const char* title)
//   parse # into (positive) int
int str2p(char* arg, const char* title) {
  long int res = str2l(arg, title);
#if (INT_MAX != LONG_MAX) 
  if ((res > INT_MAX) || (res < INT_MIN)) {
    if (NULL == title) { title = "Argument";}
    Err("%s value (%s) exceeds integer range", title, arg);
  }
#endif
  if (res < 0) {
    if (NULL == title) { title = "Argument";}
    Err("%s positive value (%s) is negative", title, arg);
  }
  return (int) res;
}

// long int str2l(char* arg, const char* title)
//   parse # into long int
long int str2l(char* arg, const char* title) {
  long long int res = str2ll(arg, title);
#if (LLONG_MAX != LONG_MAX) 
  if ((res > LONG_MAX) || (res < LONG_MIN)) {
    if (NULL == title) { title = "Argument";}
    Err("%s value (%s) exceeds long integer range", title, arg);
  }
#endif
  return (long int) res;
}

// long long int str2ll(char* arg, const char* title)
//   parse # into long long int
long long int str2ll(char* arg, const char* title) {
  char*         eptr;
  long long int res;
  if (NULL == title) { title = "Argument";}
  
  eptr = NULL;
  res  = strtoll(arg, &eptr, 0);
  if ((NULL != eptr) && ('\0' != eptr[0])) {
    Err("%s (%s) is not an integer number", title, arg);
  }
  if ((res == LLONG_MAX || res == LLONG_MIN) && (ERANGE == errno)) {
    Err("%s value (%s) exceeds long long integer range", title, arg);
  }
  return res;
}

// unsigned int str2ui(char* arg, const char* title)
//   parse # into unsigned int
unsigned int str2ui(char* arg, const char* title) {
  unsigned long int res = str2ul(arg, title);
#if (UINT_MAX != ULONG_MAX) 
  if (res > UINT_MAX) { // if cond is platform specific
    if (NULL == title) { title = "Argument";}
    Err("%s value (%s) exceeds unsigned integer range", title, arg);
  }
#endif
  return (unsigned int) res;
}

// unsigned long int str2ul(char* arg, const char* title)
//   parse # into long unsigned int
unsigned long int str2ul(char* arg, const char* title) {
  unsigned long long int res = str2ull(arg, title);
#if (ULLONG_MAX != ULONG_MAX) 
  if (res > ULONG_MAX) { // if cond is platform specific
    if (NULL == title) { title = "Argument";}
    Err("%s value (%s) exceeds unsigned long integer range", title, arg);
  }
#endif
  return (unsigned long int) res;
}

// unsigned long long int str2ull(char* arg, const char* title)
//   parse # into long long unsigned int
unsigned long long int str2ull(char* arg, const char* title) {
  char*                  eptr;
  unsigned long long int res;
  if (NULL == title) { title = "Argument";}
  
  eptr = NULL;
  res  = strtoull(arg, &eptr, 0);
  if ((NULL != eptr) && ('\0' != eptr[0])) {
    Err("%s (%s) is not an integer number", title, arg);
  }
  if ((res == ULLONG_MAX) && (ERANGE == errno)) {
    Err("%s value (%s) exceeds unsigned long long integer range", title, arg);
  }
  return res;
}


// float str2f(char* arg, const char* title)
//   parse # into float
float str2f(char* arg, const char* title) {
  double res = str2d(arg, title);
  return (float) res;
}

// double str2d(char* arg, const char* title)
//   parse # into double
double str2d(char* arg, const char* title) {
  char*  eptr;
  double res;
  if (NULL == title) { title = "Argument";}
  
  eptr = NULL;
  res  = strtod(arg, &eptr);
  if ((NULL != eptr) && ('\0' != eptr[0])) {
    Err("%s (%s) is not a floating point number", title, arg);
  }
  return res;
}


// void str2prange(char* arg, const char* title, range_t* result)
//   parse #..# into range structure
void str2prange(char* arg, const char* title, range_t* result) {
  char* end = splitstring_exact_strict(arg, "..", title);
  result->start = str2p(arg, title);
  result->end   = str2p(end, title);
}

// void str2range(char* arg, const char* title, range_t* result)
//   parse signed #..# into range structure
void str2range(char* arg, const char* title, range_t* result) {
  char* end     = splitstring_exact_strict(arg, "..", title);
  result->start = str2i(arg, title);
  result->end   = str2i(end, title);
}

// void str2range(char* arg, const char* title, range_t* result)
//   parse floating point #..# into range structure
void str2frange(char* arg, const char* title, frange_t* result) {
  char* end     = splitstring_exact_strict(arg, "..", title);
  result->start = str2f(arg, title);
  result->end   = str2f(end, title);
}

// void str2pdim(char* arg, const char* title, range_t* result)
//   parse #<sep># (eg #:# or #x#) into dim structure
void str2pdim(char* arg, const char* title, char sep, xy_dim_t* result) {
  char* end = splitstring_exact_strict(lower_case(arg), &sep, title);
  result->x = str2p(arg, title);
  result->y = str2p(end, title);
}

// void str2dim(char* arg, const char* title, range_t* result)
//   parse signed #<sep># (eg #:# or #x#) into dim structure
void str2dim(char* arg, const char* title, char sep, xy_dim_t* result) {
  char* end = splitstring_exact_strict(lower_case(arg), &sep, title);
  result->x = str2i(arg, title);
  result->y = str2i(end, title);
}

// void str2dim(char* arg, const char* title, range_t* result)
//   parse signed #<sep># (eg #:# or #x#) into dim structure
void str2fdim(char* arg, const char* title, char sep, fxy_dim_t* result) {
  char* end = splitstring_exact_strict(lower_case(arg), &sep, title);
  result->x = str2f(arg, title);
  result->y = str2f(end, title);
}

// void str2ivect(char* arg, const char* title, int result[], int vlng)
//   parse # # # into integer vector
void str2ivect(char* arg, const char* title, int result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,");
    result[elem] = str2i(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2pvect(char* arg, const char* title, int result[], int vlng)
//   parse # # # into integer vector
void str2pvect(char* arg, const char* title, int result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2p(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2uvect(char* arg, const char* title, unsigned int result[], int vlng)
//   parse # # # into integer vector
void str2uivect(char* arg, const char* title, unsigned int result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2ui(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2lvect(char* arg, const char* title, long result[], int vlng)
//   parse # # # into integer vector
void str2lvect(char* arg, const char* title, long result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2l(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2ulvect(char* arg, const char* title, unsigned long result[], int vlng)
//   parse # # # into integer vector
void str2ulvect(char* arg, const char* title, unsigned long result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2ul(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2llvect(char* arg, const char* title, long long result[], int vlng)
//   parse # # # into integer vector
void str2llvect(char* arg, const char* title, long long result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2ll(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2ullvect(char* arg, const char* title, unsigned long long result[], int vlng)
//   parse # # # into integer vector
void str2ullvect(char* arg, const char* title, unsigned long long result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2ull(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2fvect(char* arg, const char* title, float result[], int vlng)
//   parse # # # into integer vector
void str2fvect(char* arg, const char* title, float result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2f(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}

// void str2ufvect(char* arg, const char* title, double result[], int vlng)
//   parse # # # into integer vector
void str2dvect(char* arg, const char* title, double result[], int vlng) {
  int  elem = 0;
  char* next;
  while ((NULL != arg) && (0 != arg[0])) {
    if (elem >= vlng) {
      UErr("Number of array arguments for %s exceed the defined array size (%d)", title, vlng);
    }
    next = splitstring(arg, " ,\t");
    result[elem] = str2d(arg, title);
    arg = next;
    elem++;
  }
  if (elem < vlng) {
    UErr("Number of array arguments (%d) for %s is less than defined array size (%d)", elem, title, vlng);
  }
}



static int assign_val(const cmdarg_t cmdarg, char* v, int cmd)
{
  const char *arg = cmd? cmdarg.cmd: cmdarg.key;
  if ((cmdarg.type == NARG) || (cmdarg.type == NO_ARG) ||
      (cmdarg.type == VARG) || (cmdarg.type == VALUE_ARG)) {
    if (NULL != v){
      UErr("argument %s specifed with value %s", arg, v);
    }
    Assert(NULL == v);  
    *((int*) cmdarg.var_ptr) = cmdarg.value;
    return 0;
  }

  if (NULL == v){
    UErr("argument %s specifed without value", arg);
  }
  
  switch(cmdarg.type) {
    case STRING_ARG:
    case SARG:
      strcpy(cmdarg.var_ptr, v);
      return 0;
    case NO_ARG:    // just listed to avoid warning
    case NARG:      // "
    case VALUE_ARG: // "
    case VARG:      // "
    case INT_ARG:
    case IARG:
      *((int*)                 cmdarg.var_ptr) = str2i(v, arg);
      return 0;
    case PINT_ARG:
    case PARG:
      *((int*)                 cmdarg.var_ptr) = str2p(v, arg);
      return 0;
    case UINT_ARG:
    case UARG:
      *((unsigned int*)        cmdarg.var_ptr) = str2ui(v, arg);
      return 0;
    case LONG_ARG:
    case LARG:
      *((long*)                cmdarg.var_ptr) = str2l(v, arg);
      return 0;
    case LONGLONG_ARG:
    case LLARG:
      *((long long*)           cmdarg.var_ptr) = str2ll(v, arg);
      return 0;
    case ULONG_ARG:
    case ULARG:
      *((unsigned long*)       cmdarg.var_ptr) = str2ul(v, arg);
      return 0;
    case ULONGLONG_ARG:
    case ULLARG:
      *((unsigned long long*)  cmdarg.var_ptr) = str2ull(v, arg);
      return 0;
    case FLOAT_ARG:
    case FARG:
      *((float*)               cmdarg.var_ptr) = str2f(v, arg);
      return 0;
    case DOUBLE_ARG:
    case DARG:
      *((double*)              cmdarg.var_ptr) = str2d(v, arg);
      return 0;

    case INT_COL_INT_ARG:
    case ICIARG:
      str2dim( v, arg, ':', (xy_dim_t*) cmdarg.var_ptr);
      return 0;
    case PINT_COL_PINT_ARG:
    case PCPARG:
      str2pdim(v, arg, ':', (xy_dim_t*) cmdarg.var_ptr);
      return 0;
    case FLOAT_COL_FLOAT_ARG:
    case FCFARG:
      str2fdim(v, arg, ':', (fxy_dim_t*)cmdarg.var_ptr);
      return 0;
    case INT_X_INT_ARG:
    case IXIARG:
      str2dim( v, arg, 'x', (xy_dim_t*) cmdarg.var_ptr);
      return 0;
    case PINT_X_PINT_ARG:
    case PXPARG:
      str2pdim(v, arg, 'x', (xy_dim_t*) cmdarg.var_ptr);
      return 0;
    case FLOAT_X_FLOAT_ARG:
    case FXFARG:
      str2fdim(v, arg, 'x', (fxy_dim_t*)cmdarg.var_ptr);
      return 0;
    case INT_COM_INT_ARG:
    case IMIARG:
      str2dim( v, arg, ',', (xy_dim_t*) cmdarg.var_ptr);
      return 0;
    case PINT_COM_PINT_ARG:
    case PMPARG:
      str2pdim(v, arg, ',', (xy_dim_t*) cmdarg.var_ptr);
      return 0;
    case FLOAT_COM_FLOAT_ARG:
    case FMFARG:
      str2fdim(v, arg, ',', (fxy_dim_t*)cmdarg.var_ptr);
      return 0;
    case INT_DOT_DOT_INT_ARG:
    case IDDIARG:
      str2range( v, arg, (range_t*) cmdarg.var_ptr);
      return 0;
    case PINT_DOT_DOT_PINT_ARG:
    case PDDPARG:
      str2prange(v, arg, (range_t*) cmdarg.var_ptr);
      return 0;
    case FLOAT_DOT_DOT_FLOAT_ARG:
    case FDDFARG:
      str2frange(v, arg, (frange_t*) cmdarg.var_ptr);
      return 0;

    case INT_VARG:
    case IVARG:
      str2ivect(  v, arg, (int*)                cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case PINT_VARG:
    case PVARG:
      str2pvect(  v, arg, (int*)                cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case UINT_VARG:
    case UVARG:
      str2uivect( v, arg, (unsigned int*)       cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case LONG_VARG:
    case LVARG:
      str2lvect(  v, arg, (long*)               cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case LONGLONG_VARG:
    case LLVARG:
      str2llvect( v, arg, (long long*)          cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case ULONG_VARG:
    case ULVARG:
      str2ulvect( v, arg, (unsigned long*)      cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case ULONGLONG_VARG:
    case ULLVARG:
      str2ullvect(v, arg, (unsigned long long*) cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case FLOAT_VARG:
    case FVARG:
      str2fvect(  v, arg, (float*)               cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
    case DOUBLE_VARG:
    case DVARG:
      str2dvect(  v, arg, (double*)             cmdarg.var_ptr, cmdarg.vct_lng);
      return 0;
  }
  return 1;

}

static void error_check(cmdarg_t *cmdargs, const int num_args, const int cmd)
{
  int i;
  int j;
  for (i=0; i<num_args; i++) {
    const char *arg = cmd? cmdargs[i].cmd: cmdargs[i].key;

    if ((NARG == cmdargs[i].type) || (NO_ARG == cmdargs[i].type)) {
      cmdargs[i].value = 1;
    }

    if ((IVARG   == cmdargs[i].type) || (INT_VARG       == cmdargs[i].type) ||
        (LVARG   == cmdargs[i].type) || (LONG_VARG      == cmdargs[i].type) ||
        (LLVARG  == cmdargs[i].type) || (LONGLONG_VARG  == cmdargs[i].type) ||
        (UVARG   == cmdargs[i].type) || (UINT_VARG      == cmdargs[i].type) ||
        (ULVARG  == cmdargs[i].type) || (ULONG_VARG     == cmdargs[i].type) ||
        (ULLVARG == cmdargs[i].type) || (ULONGLONG_VARG == cmdargs[i].type) ||
        (FVARG   == cmdargs[i].type) || (FLOAT_VARG     == cmdargs[i].type) ||
        (DVARG   == cmdargs[i].type) || (DOUBLE_VARG    == cmdargs[i].type)) {
      if ((cmdargs[i].vct_lng > 1) && (cmdargs[i].vct_lng < 101)) {
        continue;
      }
      if (0 == cmdargs[i].vct_lng) {
        CErr("Argument %s is defined as a vector but lacks defined size", arg);
      }
      else if (cmdargs[i].vct_lng < 0) {
        CErr("Argument %s is defined as a vector with an illegal size %d", arg, cmdargs[i].vct_lng);
      }
      else {
        CErr("Argument %s is defined as a vector with a size %d outside of acceptable range (2..100)", arg, cmdargs[i].vct_lng);
      }
    }
      
    if (cmd) {
      if ((NULL != cmdargs[i].cmd) && (0 != cmdargs[i].cmd[0])) {
        for (j=0; j<num_args; j++) {
          if ((i!=j) && (NULL != cmdargs[j].cmd) && (0 != cmdargs[j].cmd[0])) { 
            if (0 == strcmp(cmdargs[i].cmd, cmdargs[j].cmd)) {
              UErr("Two command-line arguments (%d and %d) have matching patterns (%s)", 
                   i, j, cmdargs[i].cmd);
            }
          }
        }
      }
    }
    else {
      if ((NULL != cmdargs[i].key) && (0 != cmdargs[i].key[0])) {
        for (j=0; j<num_args; j++) {
          if ((i!=j) && (NULL != cmdargs[j].key) && (0 != cmdargs[j].key[0])) { 
            if (0 == strcmp(cmdargs[i].key, cmdargs[j].key)) {
              UErr("Two command-line arguments (%d and %d) have matching patterns (%s)", 
                   i, j, cmdargs[i].key);
            }
          }
        }
      }
    }
  }
}

static int* order_keys(cmdarg_t *cmdargs, int num_args)
{
  int* order;
  int init_order;
  int i;
  Assert((NULL != cmdargs) && (0 < num_args));

  order  = calloc(num_args, sizeof(int));

  init_order = 0;
  for (i=0; i<num_args; i++) {
    if ('\0' == cmdargs[i].key[0]) {
      order[init_order++] = i;
    }
  }
  for (i=0; i<num_args; i++) {
    if ('\0' != cmdargs[i].key[0]) {
      int num = init_order;
      int j;
      for (j=0; j<num_args; j++) {
        if ((j != i) && ('\0' != cmdargs[j].key[0])) { 
          if (0 < strcmp(cmdargs[i].cmd, cmdargs[j].cmd)) {
            num += 1;
          }
        }
      }
      order[num] = i;
    }
  }

  return order;
}

static int* order_cmds(cmdarg_t *cmdargs, int num_args)
{
  int* order;
  int init_order = 0;
  int i;
  Assert((NULL != cmdargs) && (0 < num_args));

  order  = calloc(num_args, sizeof(int));

  for (i=0; i<num_args; i++) {
    if ('\0' == cmdargs[i].cmd[0]) {
      order[init_order++] = i;
    }
  }
  for (i=0; i<num_args; i++) {
    if ('\0' != cmdargs[i].cmd[0]) {
      int num = init_order;
      int j;
      for (j=0; j<num_args; j++) {
        if ((j != i) && ('\0' != cmdargs[j].cmd[0])) { 
          if (0 > strcmp(cmdargs[i].cmd, cmdargs[j].cmd)) {
            num += 1;
          }
        }
      }
      order[num] = i;
    }
  }

  return order;
}

// void distinguish_dist(cmdarg_t *cmdargs, int numargs, int verbose)
//  Review list of cmdargs (array of numarg elements)
//  Foreach cmdarg, determine how many charaters of cmd are required to
//     distinguish from other cmds
//  verbose - (0) no feedback; (1) list arguments alphabetically indicating 
//     optional charaters

static int* distinguish_dist(cmdarg_t *cmdargs, int *order, int num_args)
{
  int* distinguish;
  int i;
  int c;
  Assert((NULL != cmdargs) && (0 < num_args)  && (NULL != order));
      
  distinguish = calloc(num_args, sizeof(int));

  distinguish[order[0]] = 0;
  for (i=0; i<num_args-1; i++) {
    int minlng = MIN(strlen(cmdargs[order[i]].cmd), strlen(cmdargs[order[i+1]].cmd));
    distinguish[order[i+1]] = 0;
    for (c=0; c<minlng+1; c+=1) {
      if (cmdargs[order[i]].cmd[c] != cmdargs[order[i+1]].cmd[c]) {
        int trunc_case = ('\0' == cmdargs[order[i+1]].cmd[c]); 
        int c1 = (trunc_case)? c-1: c;
        distinguish[order[i  ]] = MAX(c, distinguish[order[i]]);
        distinguish[order[i+1]] = c1;
        //printf("%d: %s vs %s (%d): %d %d trunc %d\n",c, cmdargs[order[i]].cmd, cmdargs[order[i+1]].cmd,minlng,distinguish[order[i  ]],distinguish[order[i+1]], trunc_case);
        break;
      }
    }
  }
  return distinguish;
}

// int test_cmd(char *arg, char **v, cmdarg_t cmdarg)
//  Determine if arg matches required prefix in cmdarg.cmd
//  Argument v points to remaining charaters of arg (NULL if no remaining chars)
//  Return int - (0) match; (1) no match

static int test_cmd(char *arg, char **v, const cmdarg_t cmdarg, const int distinguish)
{
  int i;
  *v = NULL;
  for (i=0; 1; i+=1) {
    if ('\0' == arg[i]) {
      int passed_test = distinguish < i;
      return passed_test;
    }
    else if (cmdarg.cmd[i] != arg[i]) {
      int string = (SARG == cmdarg.type) || (STRING_ARG == cmdarg.type);
      int passed_test;
	  if (string && ('\0' != cmdarg.cmd[i])) {
        return 0;  
      }
      passed_test = distinguish < i;
      if (passed_test) {
        *v = &(arg[i]);
      }
      return passed_test;
    }
  }
}

// parse a key-value line; comments are C++-style
// returns (1) identifies of non-empty key and value; (0) empty line; fatal error otherwise
static int parse_kvline (char* in, char** key_r, char** val_r) {
  char* slash_slash;
  char* past_key;
  Assert((NULL != key_r) && (NULL != val_r));
  *key_r = NULL;
  *val_r = NULL;
  if ((NULL == in) || (0 == in[0])) { // empty input
    return 0;
  }
  slash_slash = strstr(in, "//");
  if (NULL != slash_slash) {
    slash_slash[0] = 0; // terminate at comment
  };
  
  *key_r = in + strspn(in, " \t\v\r\n");
  if (0 == (*key_r)[0]) { // empty line or just spaces
    return 0;
  }
  past_key = *key_r + strcspn(*key_r, " \t\v\r\n");
  if (*key_r == past_key) {
    UErr("empty parameter name in line '%s'.\n", in);
  }

  if (0 != past_key[0]) {
    *val_r = past_key + strspn(past_key, " \t\v\r\n");
    if (0 == (*val_r)[0]) {
      *val_r = NULL;
    }
    else {
      char* past_val = *val_r + strcspn(*val_r, " \t\v\r\n");
      *past_val = 0; // terminate after value
    }
  }
  *past_key = 0; // terminate after key

  return 1;
}

cmdarg_t* merge_cmd_args(cmdarg_t* args1, cmdarg_t *args2)
{
  int lng1 = 0;
  int lng2 = 0;
  cmdarg_t* args_tot;
  int ptr;

  while (NULL != args1[lng1].var_ptr) {
    lng1++;
  }
  while (NULL != args2[lng2].var_ptr) {
    lng2++;
  }
  
  args_tot = calloc(lng1+lng2+1, sizeof(cmdarg_t));
  for (ptr = 0; ptr < lng1; ptr++) {
    args_tot[ptr].type    = args1[ptr].type;
    args_tot[ptr].var_ptr = args1[ptr].var_ptr;
    args_tot[ptr].key     = args1[ptr].key;
    args_tot[ptr].cmd     = args1[ptr].cmd;
    args_tot[ptr].value   = args1[ptr].value;
    args_tot[ptr].vct_lng = args1[ptr].vct_lng;
  }
  for (ptr = 0; ptr <= lng2; ptr++) { // go past lng2 to get NULL
    args_tot[lng1+ptr].type    = args2[ptr].type;
    args_tot[lng1+ptr].var_ptr = args2[ptr].var_ptr;
    args_tot[lng1+ptr].key     = args2[ptr].key;
    args_tot[lng1+ptr].cmd     = args2[ptr].cmd;
    args_tot[lng1+ptr].value   = args2[ptr].value;
    args_tot[lng1+ptr].vct_lng = args2[ptr].vct_lng;
  }
  return args_tot;
}

// int parse_line(char* k, char* v, cmdarg_t *cmdargs)
//  Determine if k matches any key paramter for an element in cmdargs array
//  Argument v used for decoding value if one is required (if not, variable set to 1)
//  Return int - (0) no match; (1) match or empty line

int parse_line(char* line, cmdarg_t *cmdargs)
{
  int num_args=0;
  char* k = NULL;
  char* v = NULL;
  Assert((NULL != line) && (NULL != cmdargs));
  while (NULL != cmdargs[num_args].var_ptr) {num_args++;}
  error_check(cmdargs, num_args, 0);

  if (parse_kvline(line, &k, &v)) { // errors are fatal in this function
    int i;
    for (i=0; i < num_args; i++) {
      if ((NULL != cmdargs[i].key) && (0 != cmdargs[i].key[0])) {
        if (0 == strcmp(k, cmdargs[i].key)) { // key match
          if (assign_val(cmdargs[i], v, 0)) {
            CErr("Unknown arg type (%d) specified for arg %s", 
                 cmdargs[i].type, cmdargs[i].key);
          }
          return 1;
        }
      }
    } 
    return 0;
  } 
  return 1;
}


// int parse_cmd(char* arg0, char* arg1, cmdarg_t *cmdargs)
//  Determine if arg0 matches any cmd paramter for an element in cmdargs array
//    
//  If value is required, remainder of arg0 is used, otherwise arg1 is used.
//   + if no value is require, arg0 must contain no excess characters, arg1 is not used.
//   + arg1 is set to NULL if out of arguments.
//  Return int - (0) no match; (1) arg0 is used; (2) arg0 and arg1 used.

int parse_cmd(char* arg0, char* arg1, cmdarg_t *cmdargs)
{
  int arg0_lng;
  int num_args=0;
  int *order;
  int *distinguish;
  int count = 0;
  int i;

  Assert(NULL != arg0);
  arg0_lng = strlen(arg0);
  while ((arg0_lng>1) && ('-' == arg0[0]) && ('-' == arg0[1])) {
    arg0      = &arg0[1];
    arg0_lng -= 1;
  }

  while (NULL != cmdargs[num_args].var_ptr) {num_args++;}
  error_check(cmdargs, num_args, 1);
  order       = order_cmds(cmdargs, num_args);
  distinguish = distinguish_dist(cmdargs, order, num_args);
  
  for (i=0; i<num_args; i++) {
    int ind = order[i];
    char* v;
    if (0 == cmdargs[ind].cmd[0]) {
      continue;
    }
    if (test_cmd(arg0, &v, cmdargs[ind], distinguish[ind])){
      int no_arg = (NARG == cmdargs[ind].type) || (NO_ARG    == cmdargs[ind].type) ||
                   (VARG == cmdargs[ind].type) || (VALUE_ARG == cmdargs[ind].type);

      if ((NULL != v) || no_arg) {
        count = 1;
      }
      else {
        count = 2;
        v     = arg1;
      }
      if (assign_val(cmdargs[ind], v, 1)) {
        CErr("Unknown arg type (%d) specified for arg %s", 
             cmdargs[i].type, cmdargs[i].cmd);
      }
      break;
    }
  } 

  free(distinguish);
  free(order);
  return count;
}


int parse_cmd_strict(char* arg0, char* arg1, cmdarg_t *cmdargs) {
  int args = parse_cmd(arg0, arg1, cmdargs);
  if (!args) {
    printf("Error: unknown command line argument '%s'", arg0);
    printf("Valid list includes:");
    parse_cmd_usage(cmdargs);
    exit(1);
  }
  return args;
}

void  parse_line_strict(char* line, cmdarg_t *cmdargs) {
  if (!parse_line(line, cmdargs)) {
    printf("Error: unknown key specified in '%s' command", line);
    printf("Valid keys includes:");
    parse_key_usage(cmdargs);
    exit(1);
  }
}


void appendarg(char* string, const cmdarg_t cmdarg) {
  switch(cmdarg.type) {
    case NO_ARG:
    case NARG:
    case VALUE_ARG:
    case VARG:
      return;  // no change
    case STRING_ARG:
    case SARG:
      strcat(string, "<s>");
      return;
    case INT_ARG:
    case IARG:
    case PINT_ARG:
    case PARG:
    case LONG_ARG:
    case LARG:
    case LONGLONG_ARG:
    case LLARG:
    case UINT_ARG:
    case UARG:
    case ULONG_ARG:
    case ULARG:
    case ULONGLONG_ARG:
    case ULLARG:
      strcat(string, "#");
      return;
    case INT_VARG:
    case IVARG:
    case PINT_VARG:
    case PVARG:
    case LONG_VARG:
    case LVARG:
    case LONGLONG_VARG:
    case LLVARG:
    case UINT_VARG:
    case UVARG:
    case ULONG_VARG:
    case ULVARG:
    case ULONGLONG_VARG:
    case ULLVARG:
      strcat(string, "#, .., #");
      return;
    case INT_COL_INT_ARG:
    case ICIARG:
    case PINT_COL_PINT_ARG:
    case PCPARG:
      strcat(string, "#:#");
      return;
    case FLOAT_COL_FLOAT_ARG:
    case FCFARG:
      strcat(string, "#.#:#.#");
      return;
    case INT_X_INT_ARG:
    case IXIARG:
    case PINT_X_PINT_ARG:
    case PXPARG:
      strcat(string, "#x#");
      return;
    case FLOAT_X_FLOAT_ARG:
    case FXFARG:
      strcat(string, "#.#x#.#");
      return;
    case INT_COM_INT_ARG:
    case IMIARG:
    case PINT_COM_PINT_ARG:
    case PMPARG:
      strcat(string, "#,#");
      return;
    case FLOAT_COM_FLOAT_ARG:
    case FMFARG:
      strcat(string, "#.#,#.#");
      return;
    case INT_DOT_DOT_INT_ARG:
    case IDDIARG:
    case PINT_DOT_DOT_PINT_ARG:
    case PDDPARG:
    case FLOAT_DOT_DOT_FLOAT_ARG:
    case FDDFARG:
      strcat(string, "#..#");
      return;
    case FLOAT_ARG:
    case FARG:
    case DOUBLE_ARG:
    case DARG:
      strcat(string, "#.#");
      return;
    case FLOAT_VARG:
    case FVARG:
    case DOUBLE_VARG:
    case DVARG:
      strcat(string, "#.#, .., #.#");
      return;
  } 
} 

// void parse_cmd_usage(cmdarg_t *cmdargs)
//  List arguments alphabetically indicating optional charaters

void parse_cmd_usage(cmdarg_t *cmdargs)
{
  int num_args = 0;
  int maxlng   = 0;
  int  *order;
  int  *distinguish;
  char *cmd;
  int debug = 0;
  int i;

  while (NULL != cmdargs[num_args].var_ptr) {
    int lng = strlen(cmdargs[num_args].cmd);
    maxlng  = MAX(maxlng, lng);
    num_args++;
  }
  order       = order_cmds(cmdargs, num_args);
  distinguish = distinguish_dist(cmdargs, order, num_args);
  cmd         = calloc(maxlng+3+12, sizeof(char));

  for (i=num_args-1; i>=0; i--) {
    int   len = strlen(cmdargs[order[i]].cmd);
    if (len-1 <= distinguish[order[i]]) {
      strcpy(cmd, cmdargs[order[i]].cmd);
    }
    else {
      int j;
      for (j=0; j<len; j+=1) {
        int k = (j<=distinguish[order[i]])? j: j+1;
        cmd[k] = cmdargs[order[i]].cmd[j];
      }
      cmd[distinguish[order[i]]+1] = '[';
      cmd[len+1] = ']';
      cmd[len+2] = '\0';
    }
    
    appendarg(cmd, cmdargs[order[i]]);
    
    if (debug) {
      printf("%3d (%3d): %s - %d v %d\n", i, order[i], cmd, len-1, distinguish[order[i]]);
    }
    else if (distinguish[order[i]]) {
      printf("  %s\n", cmd);
    }
  }
  free(cmd); 

  free(distinguish);
  free(order);
}

// void parse_key_usage(cmdarg_t *cmdargs)
//  List keys alphabetically from list os key/value pairs 

void parse_key_usage(cmdarg_t *cmdargs)
{
  int num_args = 0;
  int maxlng   = 0;
  int  *order;
  char *key;
  int i;
  
  while (NULL != cmdargs[num_args].var_ptr) {
    int lng = strlen(cmdargs[num_args].cmd);
    maxlng  = MAX(maxlng, lng);
    num_args++;
  }
  order = order_keys(cmdargs, num_args);
  key   = calloc(maxlng+4+12, sizeof(char));

  for (i=0; i<num_args; i++) {
    if ((NULL == cmdargs[order[i]].key) || (0 == cmdargs[order[i]].key[0])) continue;
    strcpy(key, cmdargs[order[i]].key);
    strcat(key, " = ");
    appendarg(key, cmdargs[order[i]]);
    printf("  %s\n", key);
  }

  free(order);
}


void* retrieve_keys_var(char* key, cmdarg_t *cmdargs)
{
  int i;
  Assert((NULL != key) && ('\0' != key[0]) && (NULL != cmdargs));
  for (i=0; NULL != cmdargs[i].var_ptr; i+=1) {
    if (0 == strcmp(key, cmdargs[i].key)) { // key match
       return cmdargs[i].var_ptr;
    }
  }
  return NULL;
}


void* retrieve_cmds_var(char* cmd, cmdarg_t *cmdargs)
{
  int i;
  Assert((NULL != cmd) && ('\0' != cmd[0]) && (NULL != cmdargs));
  for (i=0; NULL != cmdargs[i].var_ptr; i+=1) {
    if (0 == strcmp(cmd, cmdargs[i].cmd)) { // key match
       return cmdargs[i].var_ptr;
    }
  }
  return NULL;
}
