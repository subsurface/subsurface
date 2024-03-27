// SPDX-License-Identifier: GPL-2.0
#ifndef GITACCESS_H
#define GITACCESS_H

#include "git2.h"
#include "filterpreset.h"

struct dive_log;

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#define CLOUD_HOST_US "ssrf-cloud-us.subsurface-divelog.org"  // preferred (faster/bigger) server in the US
#define CLOUD_HOST_U2 "ssrf-cloud-u2.subsurface-divelog.org"  // secondary (older) server in the US
#define CLOUD_HOST_EU "ssrf-cloud-eu.subsurface-divelog.org"  // preferred (faster/bigger) server in Germany
#define CLOUD_HOST_E2 "ssrf-cloud-e2.subsurface-divelog.org"  // secondary (older) server in Germany
#define CLOUD_HOST_PATTERN "ssrf-cloud-..\\.subsurface-divelog\\.org"
#define CLOUD_HOST_GENERIC "cloud.subsurface-divelog.org"

enum remote_transport { RT_LOCAL, RT_HTTPS, RT_SSH, RT_OTHER };

extern bool git_local_only;
extern bool git_remote_sync_successful;
extern void clear_git_id(void);
extern void set_git_id(const struct git_oid *);
void set_git_update_cb(int(*)(const char *));
int git_storage_update_progress(const char *text);
int get_authorship(git_repository *repo, git_signature **authorp);

#ifdef __cplusplus
}

#include <string>

struct git_oid;
struct git_repository;
struct divelog;

struct git_info {
	std::string url;
	std::string branch;
	std::string username;
	std::string localdir;
	struct git_repository *repo;
	unsigned is_subsurface_cloud:1;
	enum remote_transport transport;
	git_info();
	~git_info();
};

extern std::string saved_git_id;
extern std::string get_sha(git_repository *repo, const std::string &branch);
extern std::string get_local_dir(const std::string &, const std::string &);
extern bool is_git_repository(const char *filename, struct git_info *info);
extern bool open_git_repository(struct git_info *info);
extern bool remote_repo_uptodate(const char *filename, struct git_info *info);
extern int sync_with_remote(struct git_info *);
extern int git_save_dives(struct git_info *, bool select_only);
extern int git_load_dives(struct git_info *, struct divelog *log);
extern int do_git_save(struct git_info *, bool select_only, bool create_empty);
extern int git_create_local_repo(const std::string &filename);

#endif
#endif // GITACCESS_H

