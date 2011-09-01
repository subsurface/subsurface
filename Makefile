CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g

OBJS=main.o profile.o info.o divelist.o parse-xml.o save-xml.o

divelog: $(OBJS)
	$(CC) $(LDLAGS) -o divelog $(OBJS) \
		`xml2-config --libs` \
		`pkg-config --libs gtk+-2.0`

parse-xml.o: parse-xml.c dive.h
	$(CC) $(CFLAGS) -c `xml2-config --cflags` parse-xml.c

save-xml.o: save-xml.c dive.h
	$(CC) $(CFLAGS) -c save-xml.c

main.o: main.c dive.h display.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c main.c

profile.o: profile.c dive.h display.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c profile.c

info.o: info.c dive.h display.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c info.c

divelist.o: divelist.c dive.h display.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0` -c divelist.c
