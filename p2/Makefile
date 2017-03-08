CC=gcc
CXX=g++
OPTIMIZE=-Os
CFLAGS=-o

all: rdps rdpr

debug:
	make all OPTIMIZE='-g'

	
rdps: rdps.c
	$(CC) $(CFLAGS) rdps rdps.c helpers.c helpers.h
	
rdpr: rdpr.c
	$(CC) $(CFLAGS) rdpr rdpr.c helpers.c helpers.h

clean:
	rm -rf *.o
