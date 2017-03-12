CC=gcc
CXX=g++
OPTIMIZE=-Os
CFLAGS=$(OPTIMIZE) -Wall -o
SC=$(CC)

all: rdps rdpr

debug:
	make all OPTIMIZE='-g'

	
rdps: rdps.c  helpers.c helpers.h
	$(SC) $(CFLAGS) rdps rdps.c helpers.c helpers.h
	
rdpr: rdpr.c  helpers.c helpers.h
	$(SC) $(CFLAGS) rdpr rdpr.c helpers.c helpers.h

clean:
	rm -rf *.o
