CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g

parse: main.o parse.o
	$(CC) $(LDLAGS) -o parse main.o parse.o `xml2-config --libs` \
		`pkg-config --libs gtk+-2.0`

parse.o: parse.c dive.h
	$(CC) $(CFLAGS) -c `xml2-config --cflags` parse.c

main.o: main.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c main.c
