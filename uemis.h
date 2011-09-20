#ifndef UEMIS_H
#define UEMIS_H

/*
 * defines and prototypes for the uemis Zurich SDA file parser
 * we add this to the list of dive computers that is supported
 * in libdivecomputer by using a negative value for the type enum
 */
#define DEVICE_TYPE_UEMIS (-1)

void uemis_import();

#endif /* DIVE_H */
