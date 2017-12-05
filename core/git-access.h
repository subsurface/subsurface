// SPDX-License-Identifier: GPL-2.0
#ifndef GITACCESS_H
#define GITACCESS_H

#include "git2.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

enum remote_transport { RT_OTHER, RT_HTTPS, RT_SSH };

// State needed for git accesses. Data must be freed with free_git_state()
struct git_state {
	const char *location;
	const char *branch;
	const char *user;
	bool is_remote;
	bool is_cloud;
	int auth_attempt;
};

struct git_oid;
struct git_repository;
#define dummy_git_repository ((git_repository *)3ul) /* Random bogus pointer, not NULL */
extern struct git_repository *is_git_repository(struct git_state *state);
extern int check_git_sha(struct git_state *loc, struct git_repository **git_p);
extern int sync_with_remote(struct git_repository *repo, struct git_state *state, enum remote_transport rt);
//extern int git_save_dives(struct git_repository *, struct git_state *state, bool select_only);
extern int git_load_dives(struct git_repository *, const char *);
extern const char *get_sha(git_repository *repo, const char *branch);
extern int do_git_save(git_repository *repo, struct git_state *state, bool select_only, bool create_empty);
extern const char *saved_git_id;
extern void clear_git_id(void);
extern void set_git_id(const struct git_oid *);
void set_git_update_cb(int(*)(const char *));
int git_storage_update_progress(const char *text);
char *get_local_dir(const struct git_state *state);
int git_create_local_repo(const char *directory);
void free_git_state(struct git_state *state);

#ifdef __cplusplus
}
#endif
#endif // GITACCESS_H

