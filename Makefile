parse: parse.c
	gcc -g -o parse `xml2-config --cflags` parse.c `xml2-config --libs`
