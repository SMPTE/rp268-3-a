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

#ifndef CMD_PARSE_H
#define CMD_PARSE_H

int   has_ext(char const* path);
char* chop_ext(char const* path); // alloc space for return string - must free
char* chop_dir(char const* path); // alloc space for return string - must free
char* file_ext(char const* path); // alloc space for return string - must free
char* file_dir(char const* path); // alloc space for return string - must free

int   easy_mkdir(char const* dn);

int   strisnum(char* in);
int   strisrange(char* in);          // is str format #..# 
int   strisdim(char* in, char sep);  // is str format #<sep>#  aka #x# of #:#
char* lower_case(char* str);
void  change_ext(char* path, char* ext);
char* splitstring(char* in, char *sep);

typedef struct xy_dim_s {
  int   x;
  int   y;
} xy_dim_t;

typedef struct fxy_dim_s {
  float x;
  float y;
} fxy_dim_t;

typedef struct range_s {
  int   start;
  int   end;
} range_t;

typedef struct frange_s {
  float start;
  float end;
} frange_t;

// Parse "#" string into integer, unsigned, long, float, or double
int                    str2i( char* arg, const char* title);
int                    str2p( char* arg, const char* title);
long int               str2l( char* arg, const char* title);
long long int          str2ll(char* arg, const char* title);
unsigned int           str2ui( char* arg, const char* title);
unsigned long int      str2ul(char* arg, const char* title);
unsigned long long int str2ull(char* arg, const char* title);
float                  str2f( char* arg, const char* title);
double                 str2d( char* arg, const char* title);

// Parse "#..#" string into range structure
void str2prange(char* arg, const char* title,  range_t* result);
void str2range( char* arg, const char* title,  range_t* result);
void str2frange(char* arg, const char* title, frange_t* result);

// Parse "#<sep>#" (eg #:# or #x#) string into dim structure
void str2pdim(char* arg, const char* title, char sep, xy_dim_t* result);
void str2dim( char* arg, const char* title, char sep, xy_dim_t* result);
void str2fdim(char* arg, const char* title, char sep, fxy_dim_t* result);

// Parse "# # #" string into integer, unsigned, long, float, or double vector
void str2ivect(  char* arg, const char* title, int                result[], int vlng);
void str2pvect(  char* arg, const char* title, int                result[], int vlng);
void str2uivect( char* arg, const char* title, unsigned int       result[], int vlng);
void str2lvect(  char* arg, const char* title, long               result[], int vlng);
void str2ulvect( char* arg, const char* title, unsigned long      result[], int vlng);
void str2llvect( char* arg, const char* title, long long          result[], int vlng);
void str2ullvect(char* arg, const char* title, unsigned long long result[], int vlng);
void str2fvect(  char* arg, const char* title, float              result[], int vlng);
void str2dvect(  char* arg, const char* title, double             result[], int vlng);

typedef enum argtype_e{ NARG, NO_ARG, VARG, VALUE_ARG, SARG, STRING_ARG, 
                        IARG,  INT_ARG,   PARG,  PINT_ARG,  LARG,   LONG_ARG,   LLARG,   LONGLONG_ARG, 
                        IVARG, INT_VARG,  PVARG, PINT_VARG, LVARG,  LONG_VARG,  LLVARG,  LONGLONG_VARG, 
                        UARG,  UINT_ARG,                    ULARG,  ULONG_ARG,  ULLARG,  ULONGLONG_ARG,
                        UVARG, UINT_VARG,                   ULVARG, ULONG_VARG, ULLVARG, ULONGLONG_VARG,
                        IXIARG, INT_X_INT_ARG,     IMIARG, INT_COM_INT_ARG,     ICIARG, INT_COL_INT_ARG,     IDDIARG, INT_DOT_DOT_INT_ARG,  
                        PXPARG, PINT_X_PINT_ARG,   PMPARG, PINT_COM_PINT_ARG,   PCPARG, PINT_COL_PINT_ARG,   PDDPARG, PINT_DOT_DOT_PINT_ARG,  
                        FARG,  FLOAT_ARG,  DARG,  DOUBLE_ARG,  
                        FVARG, FLOAT_VARG, DVARG, DOUBLE_VARG,
                        FXFARG, FLOAT_X_FLOAT_ARG, FMFARG, FLOAT_COM_FLOAT_ARG, FCFARG, FLOAT_COL_FLOAT_ARG, FDDFARG, FLOAT_DOT_DOT_FLOAT_ARG} argtype_t;  
                        

typedef struct cmdarg_s {
  argtype_t   type;
  void       *var_ptr;
  const char *key;
  const char *cmd;
  int         value;
  int         vct_lng;
} cmdarg_t;

cmdarg_t* merge_cmd_args(cmdarg_t* args1, cmdarg_t *args2);

// identify key (and possible value) from arg[s] and assign the corresponding C variable
int   parse_cmd(char* arg0, char* arg1, cmdarg_t *cmdargs);
int   parse_cmd_strict(char* arg0, char* arg1, cmdarg_t *cmdargs);

// identify key (and possible value) fromt string and assign the corresponding C variable
int   parse_line(char* line, cmdarg_t *cmdargs);
void  parse_line_strict(char* line, cmdarg_t *cmdargs);

// print out list of possible keys 
void  parse_cmd_usage(cmdarg_t *cmdargs);
void  parse_key_usage(cmdarg_t *cmdargs);

#endif
