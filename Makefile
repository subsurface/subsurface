CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -g

LIBDIVECOMPUTERDIR = /usr/local
LIBDIVECOMPUTERINCLUDES = $(LIBDIVECOMPUTERDIR)/include/libdivecomputer
LIBDIVECOMPUTERARCHIVE = $(LIBDIVECOMPUTERDIR)/lib/libdivecomputer.a

OBJS =	main.o dive.o profile.o info.o equipment.o divelist.o \
	parse-xml.o save-xml.o libdivecomputer.o print.o uemis.o \
	gtk-gui.o

subsurface: $(OBJS)
	$(CC) $(LDFLAGS) -o subsurface $(OBJS) \
		`xml2-config --libs` \
		`pkg-config --libs gtk+-2.0 glib-2.0 gconf-2.0` \
		$(LIBDIVECOMPUTERARCHIVE) -lpthread

parse-xml.o: parse-xml.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c `xml2-config --cflags`  parse-xml.c

save-xml.o: save-xml.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c save-xml.c

dive.o: dive.c dive.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0` -c dive.c

main.o: main.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags glib-2.0 gconf-2.0` \
		-c main.c

profile.o: profile.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c profile.c

info.o: info.c dive.h display.h display-gtk.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c info.c

equipment.o: equipment.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c equipment.c

divelist.o: divelist.c dive.h display.h divelist.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c divelist.c

print.o: print.c dive.h display.h display-gtk.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c print.c

libdivecomputer.o: libdivecomputer.c dive.h display.h display-gtk.h libdivecomputer.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` \
			-I$(LIBDIVECOMPUTERINCLUDES) \
			-c libdivecomputer.c

gtk-gui.o: gtk-gui.c dive.h display.h divelist.h display-gtk.h libdivecomputer.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0 gconf-2.0` \
			-I$(LIBDIVECOMPUTERINCLUDES) \
			-c gtk-gui.c

uemis.o: uemis.c uemis.h
	$(CC) $(CFLAGS) `pkg-config --cflags gtk+-2.0 glib-2.0` -c uemis.c
