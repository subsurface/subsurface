CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g

OBJS=main.o dive.o profile.o info.o cylinders.o divelist.o parse-xml.o save-xml.o

divelog: $(OBJS)
	$(CC) $(LDFLAGS) -o divelog $(OBJS) \
		`xml2-config --libs` \
		`pkg-config --libs gtk+-2.0 glib-2.0 gconf-2.0`

parse-xml.o: parse-xml.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c `xml2-config --cflags`  parse-xml.c

save-xml.o: save-xml.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c save-xml.c

dive.o: dive.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c dive.c

main.o: main.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0 gconf-2.0` \
		-c main.c

profile.o: profile.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c profile.c

info.o: info.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c info.c

cylinders.o: cylinders.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c cylinders.c

divelist.o: divelist.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c divelist.c
