CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g

OBJS=main.o profile.o divelist.o parse.o

parse: $(OBJS)
	$(CC) $(LDLAGS) -o parse $(OBJS) `xml2-config --libs` \
		`pkg-config --libs gtk+-2.0`

parse.o: parse.c dive.h
	$(CC) $(CFLAGS) -c `xml2-config --cflags` parse.c

main.o: main.c dive.h display.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c main.c

profile.o: profile.c dive.h display.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c profile.c

divelist.o: divelist.c dive.h display.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c divelist.c
