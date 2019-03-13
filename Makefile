CC = gcc
GCCVER =
#CC = g++
DEFINES =
#JFLAGS = -std=c99 -g -Wall
JFLAGS = -std=c99 -O3 -Wall

# =================================================================================

dsc_DEFS = \
	dsc_codec.h \
	dsc_types.h \
	dsc_utils.h \
	cmd_parse.h \
	dpx.h \
	fifo.h \
	logging.h \
	multiplex.h \
	psnr.h \
	utl.h \
	vdo.h \

dsc_SRCS = \
	dsc_codec.c \
	dsc_utils.c \
	cmd_parse.c \
	codec_main.c \
	dpx.c \
	fifo.c \
	logging.c \
	multiplex.c \
	psnr.c \
	utl.c

dsc_OBJS = ${dsc_SRCS:.c=.o}

# ----------------------------------------------------------------

dsc: $(dsc_OBJS)
	$(CC) $(dsc_OBJS) -lm -o dsc

# ----------------------------------------------------------------
.c.o:
	$(CC) $(JFLAGS) $(DEFINES) -c $*.c 

.c.ln:
	lint -c $*.c 

% : %.c vdo.h utl.c utl.h dpx.h
	gcc -O -o $@ $@.c utl.c dpx.c -lm -W -Wall -std=c99

all: dsc

clean:
	rm -f *.o
	rm -f dsc
