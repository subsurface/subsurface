// SPDX-License-Identifier: GPL-2.0
#ifndef GITACCESS_H
#define GITACCESS_H

#include "git2.h"
#include "filterpreset.h"

struct dive_table;
struct dive_site_table;
struct trip_table;

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

enum remote_transport { RT_OTHER, RT_HTTPS, RT_SSH };

struct git_oid;
struct git_repository;
struct device_table;
#define dummy_git_repository ((git_repository *)3ul) /* Random bogus pointer, not NULL */
extern struct git_repository *is_git_repository(const char *filename, const char **branchp, const char **remote, bool dry_run);
extern int check_git_sha(const char *filename, git_repository **git_p, const char **branch_p);
extern int sync_with_remote(struct git_repository *repo, const char *remote, const char *branch, enum remote_transport rt);
extern int git_save_dives(struct git_repository *, const char *, const char *remote, bool select_only);
extern int git_load_dives(struct git_repository *repo, const char *branch, struct dive_table *table, struct trip_table *trips,
			  struct dive_site_table *sites, struct device_table *devices,
			  struct filter_preset_table *filter_presets);
extern const char *get_sha(git_repository *repo, const char *branch);
extern int do_git_save(git_repository *repo, const char *branch, const char *remote, bool select_only, bool create_empty);
extern const char *saved_git_id;
extern bool git_local_only;
extern bool git_remote_sync_successful;
extern void clear_git_id(void);
extern void set_git_id(const struct git_oid *);
extern enum remote_transport url_to_remote_transport(const char *remote);
void set_git_update_cb(int(*)(const char *));
int git_storage_update_progress(const char *text);
char *get_local_dir(const char *remote, const char *branch);
int git_create_local_repo(const char *filename);
int get_authorship(git_repository *repo, git_signature **authorp);

#ifdef __cplusplus
}
#endif
#endif // GITACCESS_H

