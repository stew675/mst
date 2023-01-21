# Tested that both gcc and clang-12 will successfully compile all executables
#
# clang-12 produces code that is slightly faster than gcc, but must use the
# -fno-inline flag when compiling s25
#
# Use gcc for consistent optimization behavior

all: mst

CC=gcc
CFLAGS=-g -Wall
LIBS=

mst: mst.c Makefile
	$(CC) $(CFLAGS) -o $@ mst.c $(LIBS)
