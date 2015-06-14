#ifndef GITACCESS_H
#define GITACCESS_H

#include "git2.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

enum remote_transport { RT_OTHER, RT_HTTPS, RT_SSH };

struct git_oid;
struct git_repository;
#define dummy_git_repository ((git_repository *)3ul) /* Random bogus pointer, not NULL */
extern struct git_repository *is_git_repository(const char *filename, const char **branchp, const char **remote);
extern int sync_with_remote(struct git_repository *repo, const char *remote, const char *branch, enum remote_transport rt);
extern int git_save_dives(struct git_repository *, const char *, const char *remote, bool select_only);
extern int git_load_dives(struct git_repository *, const char *);
extern int do_git_save(git_repository *repo, const char *branch, const char *remote, bool select_only, bool create_empty);
extern const char *saved_git_id;
extern void clear_git_id(void);
extern void set_git_id(const struct git_oid *);

#ifdef __cplusplus
}
#endif
#endif // GITACCESS_H

