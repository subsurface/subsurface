CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g

parse: parse.c dive.h
	$(CC) $(CFLAGS) -o parse `xml2-config --cflags` parse.c `xml2-config --libs`
