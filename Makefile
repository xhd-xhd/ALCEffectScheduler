CC=gcc
CFLAGS=-O2 -Wall -Iinclude -Isrc
LDFLAGS=-lm
SRCS=$(wildcard src/*.c src/*/*.c src/*/*/*.c)

all: alceffect

alceffect: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f alceffect *.o
