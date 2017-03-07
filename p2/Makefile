CC=gcc
CXX=g++
OPTIMIZE=-Os
CFLAGS=-o

all: rdps rdpr

debug:
	make all OPTIMIZE='-g'

	
rdps: rdps.c
	$(CC) $(CFLAGS) rdps rdps.c
	
rdpr: rdpr.c
	$(CC) $(CFLAGS) rdpr rdpr.c

clean:
	rm -rf *.o
