#ifndef VERSION_H
#define VERSION_H

/* this is used for both git and xml format */
static constexpr int dataformat_version = 3;

const char *subsurface_git_version();
const char *subsurface_canonical_version();
int get_min_datafile_version();
void report_datafile_version(int version);
int clear_min_datafile_version();

#endif
