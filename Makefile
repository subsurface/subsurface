parse: parse.c
	gcc -Wall -g -o parse `xml2-config --cflags` parse.c `xml2-config --libs`
