CC=gcc
CXX=g++
OPTIMIZE=-Os
CFLAGS=$(OPTIMIZE) -Wall -o
SC=$(CC)

all: rdps.out rdpr.out

debug:
	make all OPTIMIZE='-g'


rdps.out: rdps.c  helpers.c helpers.h
	$(SC) $(CFLAGS) rdps.out rdps.c helpers.c helpers.h

rdpr.out: rdpr.c  helpers.c helpers.h
	$(SC) $(CFLAGS) rdpr.out rdpr.c helpers.c helpers.h

clean:
	rm -rf *.o
	rm *.out
