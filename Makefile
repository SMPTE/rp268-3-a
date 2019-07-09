CC = gcc
GCCVER =
#CC = g++
DEFINES =
#JFLAGS = -std=c99 -g -Wall
JFLAGS = -std=c99 -D_GNU_SOURCE -O3 -Wall

# =================================================================================

conv_format_DEFS = \
	cmd_parse.h \
	dpx.h \
	hdr_dpx.h \
	fifo.h \
	logging.h \
	psnr.h \
	utl.h \
	vdo.h

conv_format_SRCS = \
	cmd_parse.c \
	conv_main.c \
	dpx.c \
	hdr_dpx.c \
	fifo.c \
	logging.c \
	print_header.c \
	utl.c

conv_format_OBJS = ${conv_format_SRCS:.c=.o}

# ----------------------------------------------------------------

conv_format: $(conv_format_OBJS)
	$(CC) $(conv_format_OBJS) -lm -o conv_format

# ----------------------------------------------------------------
.c.o:
	$(CC) $(JFLAGS) $(DEFINES) -c $*.c 

.c.ln:
	lint -c $*.c 

% : %.c vdo.h utl_brcm.c utl.h dpx.h
	gcc -O -o $@ $@.c utl_brcm.c dpx.c -lm -W -Wall -std=c99

all: conv_format

clean:
	rm -f *.o
	rm -f conv_format

