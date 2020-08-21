#CC = gcc
GCCVER =
CC = g++
DEFINES =
#JFLAGS = -std=c99 -g -Wall
JFLAGS = -D_GNU_SOURCE -O3 -Wall
CFLAGS = -g D_GNU_SOURCE -Wall
CPPFLAGS = -g -D_GNU_SOURCE -Wall -std=gnu++11 -Wc++11-extensions

# =================================================================================

convert_descriptor_DEFS = \
	datum.h \
	fifo.h \
	file_map.h \
	hdr_dpx.h \
	hdr_dpx_error.h

convert_descriptor_SRCS = \
	convert_descriptor.cpp \
	fifo.cpp \
	file_map.cpp \
	hdr_dpx_file.cpp \
	hdr_dpx_image_element.cpp

convert_descriptor_OBJS = ${convert_descriptor_SRCS:.cpp=.o}

dump_dpx_DEFS = \
	datum.h \
	fifo.h \
	file_map.h \
	hdr_dpx.h \
	hdr_dpx_error.h

dump_dpx_SRCS = \
	dump_dpx.cpp \
	fifo.cpp \
	file_map.cpp \
	hdr_dpx_file.cpp \
	hdr_dpx_image_element.cpp

dump_dpx_OBJS = ${dump_dpx_SRCS:.cpp=.o}

generate_color_test_DEFS = \
	datum.h \
	fifo.h \
	file_map.h \
	hdr_dpx.h \
	hdr_dpx_error.h

generate_color_test_SRCS = \
	generate_color_test.cpp \
	fifo.cpp \
	file_map.cpp \
	hdr_dpx_file.cpp \
	hdr_dpx_image_element.cpp

generate_color_test_OBJS = ${generate_color_test_SRCS:.c=.o}


# ----------------------------------------------------------------

convert_descriptor: $(convert_descriptor_OBJS)
	$(CC) $(convert_descriptor_OBJS) -lm -o convert_descriptor

dump_dpx: $(dump_dpx_OBJS)
	$(CC) $(dump_dpx_OBJS) -lm -o dump_dpx

generate_color_test: $(generate_color_test_OBJS)
	$(CC) $(generate_color_test_OBJS) -lm -o generate_color_test

# ----------------------------------------------------------------
# .c.o:
# 	$(CC) $(JFLAGS) $(DEFINES) -c $*.c
#
# .c.ln:
# 	lint -c $*.c

%.o: %.c
	$(CC) $(CFLAGS) $(INCPATH) -c $< -o $@

%.o: %.cpp
	$(CC) $(CPPFLAGS) $(INCPATH) -c $< -o $@

all: convert_descriptor dump_dpx generate_color_test

clean:
	rm -f *.o
	rm -f convert_descriptor dump_dpx generate_color_test

