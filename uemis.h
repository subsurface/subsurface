#ifndef UEMIS_H
#define UEMIS_H

/*
 * defines and prototypes for the uemis Zurich SDA file parser
 * we add this to the list of dive computers that is supported
 * in libdivecomputer by using a negative value for the type enum
 */
#define DEVICE_TYPE_UEMIS (-1)

void uemis_import();
void uemis_parse_divelog_binary(char *base64, struct dive **divep);

#endif /* DIVE_H */
