#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <git2.h>

#include "dive.h"
#include "membuffer.h"

/*
 * The libgit2 people are incompetent at making libraries. They randomly change
 * the interfaces, often just renaming things without any sane way to know which
 * version you should check for etc etc. It's a disgrace.
 */
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR < 22
  #define git_remote_lookup(res, repo, name) git_remote_load(res, repo, name)
  #define git_remote_fetch(remote, refspecs, signature, reflog) git_remote_fetch(remote, signature, reflog)
#endif

static char *get_local_dir(const char *remote, const char *branch)
{
	SHA_CTX ctx;
	unsigned char hash[20];

	// That zero-byte update is so that we don't get hash
	// collisions for "repo1 branch" vs "repo 1branch".
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, remote, strlen(remote));
	SHA1_Update(&ctx, "", 1);
	SHA1_Update(&ctx, branch, strlen(branch));
	SHA1_Final(hash, &ctx);

	return format_string("%s/%02x%02x%02x%02x%02x%02x%02x%02x",
			system_default_directory(),
			hash[0], hash[1], hash[2], hash[3],
			hash[4], hash[5], hash[6], hash[7]);
}

static int check_clean(const char *path, unsigned int status, void *payload)
{
	status &= ~GIT_STATUS_CURRENT | GIT_STATUS_IGNORED;
	if (!status)
		return 0;
	report_error("WARNING: Git cache directory modified (path %s)", path);
	return 1;
}

static int try_to_update(git_repository *rep, git_reference *local, git_reference *remote)
{
	if (!git_reference_cmp(local, remote))
		return 0;
	return report_error("Local and remote do not match, not updating");
}

static git_repository *update_local_repo(const char *localdir, const char *remote, const char *branch)
{
	int error;
	git_repository *repo = NULL;
	git_remote *origin;
	git_reference *local_ref, *remote_ref;

	error = git_repository_open(&repo, localdir);
	if (error) {
		report_error("Unable to open git cache repository at %s: %s",
			localdir, giterr_last()->message);
		return NULL;
	}

	/*
	 * NOTE! Remote errors are reported, but are nonfatal:
	 * we still successfully return the local repository.
	 */
	error = git_remote_lookup(&origin, repo, "origin");
	if (error) {
		report_error("Repository '%s' origin lookup failed (%s)",
			remote, giterr_last()->message);
		return repo;
	}

	// NOTE! A fetch error is not fatal, we just report it
	error = git_remote_fetch(origin, NULL, NULL, NULL);
	git_remote_free(origin);
	if (error) {
		report_error("Unable to update cache for remote '%s'", remote);
		return repo;
	}

	// Dirty modified state in the working tree? We're not going
	// to tru to update
	if (git_status_foreach(repo, check_clean, NULL))
		return repo;

	if (git_branch_lookup(&local_ref, repo, branch, GIT_BRANCH_LOCAL)) {
		report_error("Git cache branch %s no longer exists", branch);
		return repo;
	}

	if (git_branch_upstream(&remote_ref, local_ref)) {
		report_error("Git cache branch %s no longer has an upstream branch", branch);
		git_reference_free(local_ref);
		return repo;
	}

	try_to_update(repo, local_ref, remote_ref);
	git_reference_free(local_ref);
	git_reference_free(remote_ref);
	return repo;
}

static git_repository *create_local_repo(const char *localdir, const char *remote, const char *branch)
{
	int error;
	git_repository *cloned_repo = NULL;
	git_clone_options opts = GIT_CLONE_OPTIONS_INIT;

	opts.checkout_branch = branch;
	error = git_clone(&cloned_repo, remote, localdir, &opts);
	if (error) {
		report_error("git clone of %s failed (%s)", remote, giterr_last()->message);
		return NULL;
	}
	return cloned_repo;
}

static struct git_repository *get_remote_repo(const char *localdir, const char *remote, const char *branch)
{
	struct stat st;

	/* Do we already have a local cache? */
	if (!stat(localdir, &st)) {
		if (!S_ISDIR(st.st_mode)) {
			report_error("local git cache at '%s' is corrupt");
			return NULL;
		}
		return update_local_repo(localdir, remote, branch);
	}
	return create_local_repo(localdir, remote, branch);
}

/*
 * This turns a remote repository into a local one if possible.
 *
 * The recognized formats are
 *    git://host/repo[branch]
 *    ssh://host/repo[branch]
 *    http://host/repo[branch]
 *    https://host/repo[branch]
 *    file://repo[branch]
 */
static struct git_repository *is_remote_git_repository(const char *remote, const char *branch)
{
	char c, *localdir;
	const char *p = remote;

	while ((c = *p++) >= 'a' && c <= 'z')
		/* nothing */;
	if (c != ':')
		return NULL;
	if (*p++ != '/' || *p++ != '/')
		return NULL;

	/* Special-case "file://", since it's already local */
	if (!strncmp(remote, "file://", 7))
		remote += 7;

	/*
	 * Ok, we found "[a-z]*://", we've simplified the
	 * local repo case (because libgit2 is insanely slow
	 * for that), and we think we have a real "remote
	 * git" format.
	 *
	 * We now create the SHA1 hash of the whole thing,
	 * including the branch name. That will be our unique
	 * unique local repository name.
	 *
	 * NOTE! We will create a local repository per branch,
	 * because
	 *
	 *  (a) libgit2 remote tracking branch support seems to
	 *      be a bit lacking
	 *  (b) we'll actually check the branch out so that we
	 *      can do merges etc too.
	 *
	 * so even if you have a single remote git repo with
	 * multiple branches for different people, the local
	 * caches will sadly force that to split into multiple
	 * individual repositories.
	 */
	localdir = get_local_dir(remote, branch);
	if (!localdir)
		return NULL;

	return get_remote_repo(localdir, remote, branch);
}

/*
 * If it's not a git repo, return NULL. Be very conservative.
 */
struct git_repository *is_git_repository(const char *filename, const char **branchp)
{
	int flen, blen, ret;
	struct stat st;
	git_repository *repo;
	char *loc, *branch;

	flen = strlen(filename);
	if (!flen || filename[--flen] != ']')
		return NULL;

	/* Find the matching '[' */
	blen = 0;
	while (flen && filename[--flen] != '[')
		blen++;

	/* Ignore slashes at the end of the repo name */
	while (flen && filename[flen-1] == '/')
		flen--;

	if (!flen)
		return NULL;

	/*
	 * This is the "point of no return": the name matches
	 * the git repository name rules, and we will no longer
	 * return NULL.
	 *
	 * We will either return "dummy_git_repository" and the
	 * branch pointer will have the _whole_ filename in it,
	 * or we will return a real git repository with the
	 * branch pointer being filled in with just the branch
	 * name.
	 *
	 * The actual git reading/writing routines can use this
	 * to generate proper error messages.
	 */
	*branchp = filename;
	loc = format_string("%.*s", flen, filename);
	if (!loc)
		return dummy_git_repository;

	branch = format_string("%.*s", blen, filename+flen+1);
	if (!branch) {
		free(loc);
		return dummy_git_repository;
	}

	repo = is_remote_git_repository(loc, branch);
	if (repo) {
		free(loc);
		*branchp = branch;
		return repo;
	}

	if (stat(loc, &st) < 0 || !S_ISDIR(st.st_mode)) {
		free(loc);
		free(branch);
		return dummy_git_repository;
	}

	ret = git_repository_open(&repo, loc);
	free(loc);
	if (ret < 0) {
		free(branch);
		return dummy_git_repository;
	}
	*branchp = branch;
	return repo;
}
