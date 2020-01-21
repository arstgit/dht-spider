CC=gcc

DEPS = core.h
BUILD = debug
CFLAGS_release = 
CFLAGS_debug = -g -O0
CFLAGS = ${CFLAGS_${BUILD}}

LIBS=

# EXAMPLE: 
#   $ make clean && make BUILD=debug

all: spider

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

spider: bencode.o core.o spider.o
	$(CC) -o $@ $^ $(LIBS)

test: all

up:
	mkdir data && docker-compose up --build

pretty:
	clang-format -i *.c *.h

install:
	cp spider /usr/local/bin/dhtspider

clean:
	rm -f *.o spider

.PHONY: clean pretty install
