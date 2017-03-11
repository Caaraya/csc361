CC=gcc
CXX=g++
OPTIMIZE=-Os
CFLAGS=-Wall -o

all: rdps rdpr

debug:
	make all OPTIMIZE='-g'

	
rdps: rdps.c  helpers.c helpers.h
	$(CC) $(CFLAGS) rdps rdps.c helpers.c helpers.h
	
rdpr: rdpr.c  helpers.c helpers.h
	$(CC) $(CFLAGS) rdpr rdpr.c helpers.c helpers.h

clean:
	rm -rf *.o
