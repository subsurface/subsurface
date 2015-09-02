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
#include "strndup.h"
#include "qthelperfromc.h"
#include "git-access.h"
#include "gettext.h"

/*
 * The libgit2 people are incompetent at making libraries. They randomly change
 * the interfaces, often just renaming things without any sane way to know which
 * version you should check for etc etc. It's a disgrace.
 */
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR < 22
  #define git_remote_lookup(res, repo, name) git_remote_load(res, repo, name)
  #if LIBGIT2_VER_MINOR <= 20
    #define git_remote_fetch(remote, refspecs, signature, reflog) git_remote_fetch(remote)
  #else
    #define git_remote_fetch(remote, refspecs, signature, reflog) git_remote_fetch(remote, signature, reflog)
  #endif
#endif

#if !USE_LIBGIT23_API && !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR == 22
  #define git_remote_push(remote,refspecs,opts) git_remote_push(remote,refspecs,opts,NULL,NULL)
  #define git_reference_set_target(out,ref,id,log_message) git_reference_set_target(out,ref,id,NULL,log_message)
  #define git_reset(repo,target,reset_type,checkout_opts) git_reset(repo,target,reset_type,checkout_opts,NULL,NULL)
#endif

/*
 * api break introduced in libgit2 master after 0.22 - let's guess this is the v0.23 API
 */
#if USE_LIBGIT23_API || (!LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR >= 23)
  #define git_branch_create(out, repo, branch_name, target, force, signature, log_message) \
	git_branch_create(out, repo, branch_name, target, force)
#endif

bool is_subsurface_cloud = false;

int (*update_progress_cb)(int) = NULL;

void set_git_update_cb(int(*cb)(int))
{
	update_progress_cb = cb;
}

static int update_progress(int percent)
{
	static int last_percent = -10;
	int ret = 0;
	if (update_progress_cb)
		ret = (*update_progress_cb)(percent);
	if (verbose && percent - 10 >= last_percent) {
		last_percent = percent;
		fprintf(stderr, "git progress %d%%\n", percent);
	}
	return ret;
}

// the checkout_progress_cb doesn't allow canceling of the operation
static void progress_cb(const char *path, size_t completed_steps, size_t total_steps, void *payload)
{
	int percent = 0;
	if (total_steps)
		percent = 100 * completed_steps / total_steps;
	(void)update_progress(percent);
}

// this randomly assumes that 80% of the time is spent on the objects and 20% on the deltas
// if the user cancels the dialog this is passed back to libgit2
static int transfer_progress_cb(const git_transfer_progress *stats, void *payload)
{
	int percent = 0;
	if (stats->total_objects)
		percent = 80 * stats->received_objects / stats->total_objects;
	if (stats->total_deltas)
		percent += 20 * stats->indexed_deltas / stats->total_deltas;
	return update_progress(percent);
}

static int push_transfer_progress_cb(unsigned int current, unsigned int total, size_t bytes, void *payload)
{
	int percent = 0;
	if (total != 0)
		percent = 100 * current / total;
	return update_progress(percent);
}

char *get_local_dir(const char *remote, const char *branch)
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

	return format_string("%s/cloudstorage/%02x%02x%02x%02x%02x%02x%02x%02x",
			system_default_directory(),
			hash[0], hash[1], hash[2], hash[3],
			hash[4], hash[5], hash[6], hash[7]);
}

static char *move_local_cache(const char *remote, const char *branch)
{
	char *old_path = get_local_dir(remote, branch);
	return move_away(old_path);
}

static int check_clean(const char *path, unsigned int status, void *payload)
{
	status &= ~GIT_STATUS_CURRENT | GIT_STATUS_IGNORED;
	if (!status)
		return 0;
	if (is_subsurface_cloud)
		report_error(translate("gettextFromC", "Local cache directory %s corrupted - can't sync with Subsurface cloud storage"), path);
	else
		report_error("WARNING: Git cache directory modified (path %s) status %0x", path, status);
	return 1;
}

/*
 * The remote is strictly newer than the local branch.
 */
static int reset_to_remote(git_repository *repo, git_reference *local, const git_oid *new_id)
{
	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
	opts.progress_cb = &progress_cb;
	git_object *target;

	if (verbose)
		fprintf(stderr, "git storage: reset to remote\n");

	// If it's not checked out (bare or not HEAD), just update the reference */
	if (git_repository_is_bare(repo) || git_branch_is_head(local) != 1) {
		git_reference *out;

		if (git_reference_set_target(&out, local, new_id, "Update to remote"))
			return report_error(translate("gettextFromC", "Could not update local cache to newer remote data"));

		git_reference_free(out);

#ifdef DEBUG
		// Not really an error, just informational
		report_error("Updated local branch from remote");
#endif
		return 0;
	}

	if (git_object_lookup(&target, repo, new_id, GIT_OBJ_COMMIT)) {
		if (is_subsurface_cloud)
			return report_error(translate("gettextFromC", "Subsurface cloud storage corrupted"));
		else
			return report_error("Could not look up remote commit");
	}
	opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	if (git_reset(repo, target, GIT_RESET_HARD, &opts)) {
		if (is_subsurface_cloud)
			return report_error(translate("gettextFromC", "Could not update local cache to newer remote data"));
		else
			return report_error("Local head checkout failed after update");
	}
	// Not really an error, just informational
#ifdef DEBUG
	report_error("Updated local information from remote");
#endif
	return 0;
}

#if USE_LIBGIT23_API
int credential_ssh_cb(git_cred **out,
		  const char *url,
		  const char *username_from_url,
		  unsigned int allowed_types,
		  void *payload)
{
	const char *priv_key = format_string("%s/%s", system_default_directory(), "ssrf_remote.key");
	const char *passphrase = prefs.cloud_storage_password ? strdup(prefs.cloud_storage_password) : strdup("");
	return git_cred_ssh_key_new(out, username_from_url, NULL, priv_key, passphrase);
}

int credential_https_cb(git_cred **out,
			const char *url,
			const char *username_from_url,
			unsigned int allowed_types,
			void *payload)
{
	const char *username = prefs.cloud_storage_email_encoded;
	const char *password = prefs.cloud_storage_password ? strdup(prefs.cloud_storage_password) : strdup("");
	return git_cred_userpass_plaintext_new(out, username, password);
}

#define KNOWN_CERT "\xfd\xb8\xf7\x73\x76\xe2\x75\x53\x93\x37\xdc\xfe\x1e\x55\x43\x3d\xf2\x2c\x18\x2c"
int certificate_check_cb(git_cert *cert, int valid, const char *host, void *payload)
{
	if (same_string(host, "cloud.subsurface-divelog.org") && cert->cert_type == GIT_CERT_X509) {
		SHA_CTX ctx;
		unsigned char hash[21];
		git_cert_x509 *cert509 = (git_cert_x509 *)cert;
		SHA1_Init(&ctx);
		SHA1_Update(&ctx, cert509->data, cert509->len);
		SHA1_Final(hash, &ctx);
		hash[20] = 0;
		if (verbose > 1)
			if (same_string((char *)hash, KNOWN_CERT)) {
				fprintf(stderr, "cloud certificate considered %s, forcing it valid\n",
					valid ? "valid" : "not valid");
				return 1;
			}
	}
	return valid;
}

#endif

static int update_remote(git_repository *repo, git_remote *origin, git_reference *local, git_reference *remote, enum remote_transport rt)
{
	git_push_options opts = GIT_PUSH_OPTIONS_INIT;
	git_strarray refspec;
	const char *name = git_reference_name(local);

	if (verbose)
		fprintf(stderr, "git storage: update remote\n");

	refspec.count = 1;
	refspec.strings = (char **)&name;

#if USE_LIBGIT23_API
	opts.callbacks.push_transfer_progress = &push_transfer_progress_cb;
	if (rt == RT_SSH)
		opts.callbacks.credentials = credential_ssh_cb;
	else if (rt == RT_HTTPS)
		opts.callbacks.credentials = credential_https_cb;
	opts.callbacks.certificate_check = certificate_check_cb;
#endif
	if (git_remote_push(origin, &refspec, &opts)) {
		if (is_subsurface_cloud)
			return report_error(translate("gettextFromC", "Could not update Subsurface cloud storage, try again later"));
		else
			return report_error("Unable to update remote with current local cache state (%s)", giterr_last()->message);
	}
	return 0;
}

extern int update_git_checkout(git_repository *repo, git_object *parent, git_tree *tree);

static int try_to_git_merge(git_repository *repo, git_reference *local, git_reference *remote, git_oid *base, const git_oid *local_id, const git_oid *remote_id)
{
	git_tree *local_tree, *remote_tree, *base_tree;
	git_commit *local_commit, *remote_commit, *base_commit;
	git_index *merged_index;
	git_merge_options merge_options;

	if (verbose) {
		char outlocal[41], outremote[41];
		outlocal[40] = outremote[40] = 0;
		git_oid_fmt(outlocal, local_id);
		git_oid_fmt(outremote, remote_id);
		fprintf(stderr, "trying to merge local SHA %s remote SHA %s\n", outlocal, outremote);
	}

	git_merge_init_options(&merge_options, GIT_MERGE_OPTIONS_VERSION);
#ifdef USE_LIBGIT23_API
	merge_options.tree_flags = GIT_MERGE_TREE_FIND_RENAMES;
#else
	merge_options.flags = GIT_MERGE_TREE_FIND_RENAMES;
#endif
	merge_options.file_favor = GIT_MERGE_FILE_FAVOR_UNION;
	merge_options.rename_threshold = 100;
	if (git_commit_lookup(&local_commit, repo, local_id)) {
		fprintf(stderr, "Remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&local_tree, local_commit)) {
		fprintf(stderr, "Remote storage and local data diverged. Error: failed local tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_lookup(&remote_commit, repo, remote_id)) {
		fprintf(stderr, "Remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&remote_tree, remote_commit)) {
		fprintf(stderr, "Remote storage and local data diverged. Error: failed local tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_lookup(&base_commit, repo, base)) {
		fprintf(stderr, "Remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&base_tree, base_commit)) {
		fprintf(stderr, "Remote storage and local data diverged. Error: failed base tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_merge_trees(&merged_index, repo, base_tree, local_tree, remote_tree, &merge_options)) {
		fprintf(stderr, "Remote storage and local data diverged. Error: merge failed (%s)", giterr_last()->message);
		// this is the one where I want to report more detail to the user - can't quite explain why
		return report_error(translate("gettextFromC", "Remote storage and local data diverged. Error: merge failed (%s)"), giterr_last()->message);
	}
	if (git_index_has_conflicts(merged_index)) {
		int error;
		const git_index_entry *ancestor = NULL,
				*ours = NULL,
				*theirs = NULL;
		git_index_conflict_iterator *iter = NULL;
		error = git_index_conflict_iterator_new(&iter, merged_index);
		while (git_index_conflict_next(&ancestor, &ours, &theirs, iter)
		       != GIT_ITEROVER) {
			/* Mark this conflict as resolved */
			fprintf(stderr, "conflict in %s / %s / %s -- ",
				ours ? ours->path : "-",
				theirs ? theirs->path : "-",
				ancestor ? ancestor->path : "-");
			if ((!ours && theirs && ancestor) ||
			    (ours && !theirs && ancestor)) {
				// the file was removed on one side or the other - just remove it
				fprintf(stderr, "looks like a delete on one side; removing the file from the index\n");
				error = git_index_remove(merged_index, ours ? ours->path : theirs->path, GIT_INDEX_STAGE_ANY);
			} else if (ancestor) {
				error = git_index_conflict_remove(merged_index, ours ? ours->path : theirs ? theirs->path : ancestor->path);
			}
			if (error) {
				fprintf(stderr, "error at conflict resplution (%s)", giterr_last()->message);
			}
		}
		git_index_conflict_cleanup(merged_index);
		git_index_conflict_iterator_free(iter);
		report_error(translate("gettextFromC", "Remote storage and local data diverged. Cannot combine local and remote changes"));
	}
	git_oid merge_oid, commit_oid;
	git_tree *merged_tree;
	git_signature *author;
	git_commit *commit;

	if (git_index_write_tree_to(&merge_oid, merged_index, repo))
		goto write_error;
	if (git_tree_lookup(&merged_tree, repo, &merge_oid))
		goto write_error;
	if (git_signature_default(&author, repo) < 0)
		if (git_signature_now(&author, "Subsurface", "noemail@given") < 0)
			goto write_error;
	if (git_commit_create_v(&commit_oid, repo, NULL, author, author, NULL, "automatic merge", merged_tree, 2, local_commit, remote_commit))
		goto write_error;
	if (git_commit_lookup(&commit, repo, &commit_oid))
		goto write_error;
	if (git_branch_is_head(local) && !git_repository_is_bare(repo)) {
		git_object *parent;
		git_reference_peel(&parent, local, GIT_OBJ_COMMIT);
		if (update_git_checkout(repo, parent, merged_tree)) {
			goto write_error;
		}
	}
	if (git_reference_set_target(&local, local, &commit_oid, "Subsurface merge event"))
		goto write_error;
	set_git_id(&commit_oid);
	git_signature_free(author);

	return 0;

diverged_error:
	return report_error(translate("gettextFromC", "Remote storage and local data diverged"));

write_error:
	return report_error(translate("gettextFromC", "Remote storage and local data diverged. Error: writing the data failed (%s)"), giterr_last()->message);
}

// if accessing the local cache of Subsurface cloud storage fails, we simplify things
// for the user and simply move the cache away (in case they want to try and extract data)
// and ask them to retry the operation (which will then refresh the data from the cloud server)
static int cleanup_local_cache(const char *remote_url, const char *branch)
{
	char *backup_path = move_local_cache(remote_url, branch);
	report_error(translate("gettextFromC", "Problems with local cache of Subsurface cloud data"));
	report_error(translate("gettextFromC", "Moved cache data to %s. Please try the operation again."), backup_path);
	free(backup_path);
	return -1;
}
static int try_to_update(git_repository *repo, git_remote *origin, git_reference *local, git_reference *remote,
			 const char *remote_url, const char *branch, enum remote_transport rt)
{
	git_oid base;
	const git_oid *local_id, *remote_id;

	if (verbose)
		fprintf(stderr, "git storage: try to update\n");

	if (!git_reference_cmp(local, remote))
		return 0;

	// Dirty modified state in the working tree? We're not going
	// to update either way
	if (git_status_foreach(repo, check_clean, NULL)) {
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("local cached copy is dirty, skipping update");
	}
	local_id = git_reference_target(local);
	remote_id = git_reference_target(remote);

	if (!local_id || !remote_id) {
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Unable to get local or remote SHA1");
	}
	if (git_merge_base(&base, repo, local_id, remote_id)) {
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Unable to find common commit of local and remote branches");
	}
	/* Is the remote strictly newer? Use it */
	if (git_oid_equal(&base, local_id))
		return reset_to_remote(repo, local, remote_id);

	/* Is the local repo the more recent one? See if we can update upstream */
	if (git_oid_equal(&base, remote_id))
		return update_remote(repo, origin, local, remote, rt);

	/* Merging a bare repository always needs user action */
	if (git_repository_is_bare(repo)) {
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Local and remote have diverged, merge of bare branch needed");
	}
	/* Merging will definitely need the head branch too */
	if (git_branch_is_head(local) != 1) {
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Local and remote do not match, local branch not HEAD - cannot update");
	}
	/* Ok, let's try to merge these */
	return try_to_git_merge(repo, local, remote, &base, local_id, remote_id);

cloud_data_error:
	// since we are working with Subsurface cloud storage we want to make the user interaction
	// as painless as possible. So if something went wrong with the local cache, tell the user
	// about it an move it away
	return cleanup_local_cache(remote_url, branch);
}

static int check_remote_status(git_repository *repo, git_remote *origin, const char *remote, const char *branch, enum remote_transport rt)
{
	int error = 0;

	git_reference *local_ref, *remote_ref;

	if (verbose)
		fprintf(stderr, "git storage: check remote status\n");

	if (git_branch_lookup(&local_ref, repo, branch, GIT_BRANCH_LOCAL)) {
		if (is_subsurface_cloud)
			return cleanup_local_cache(remote, branch);
		else
			return report_error("Git cache branch %s no longer exists", branch);
	}
	if (git_branch_upstream(&remote_ref, local_ref)) {
		/* so there is no upstream branch for our branch; that's a problem.
		 * let's push our branch */
		git_strarray refspec;
		git_reference_list(&refspec, repo);
#if USE_LIBGIT23_API
		git_push_options opts = GIT_PUSH_OPTIONS_INIT;
		opts.callbacks.transfer_progress = &transfer_progress_cb;
		if (rt == RT_SSH)
			opts.callbacks.credentials = credential_ssh_cb;
		else if (rt == RT_HTTPS)
			opts.callbacks.credentials = credential_https_cb;
		opts.callbacks.certificate_check = certificate_check_cb;
		error = git_remote_push(origin, &refspec, &opts);
#else
		error = git_remote_push(origin, &refspec, NULL);
#endif
	} else {
		error = try_to_update(repo, origin, local_ref, remote_ref, remote, branch, rt);
		git_reference_free(remote_ref);
	}
	git_reference_free(local_ref);
	return error;
}

int sync_with_remote(git_repository *repo, const char *remote, const char *branch, enum remote_transport rt)
{
	int error;
	git_remote *origin;
	char *proxy_string;
	git_config *conf;

	if (verbose)
		fprintf(stderr, "sync with remote %s[%s]\n", remote, branch);

	git_repository_config(&conf, repo);
	if (rt == RT_HTTPS && getProxyString(&proxy_string)) {
		if (verbose)
			fprintf(stderr, "set proxy to \"%s\"\n", proxy_string);
		git_config_set_string(conf, "http.proxy", proxy_string);
		free(proxy_string);
	} else {
		if (verbose)
			fprintf(stderr, "delete proxy setting\n");
		git_config_delete_entry(conf, "http.proxy");
	}

	/*
	 * NOTE! Remote errors are reported, but are nonfatal:
	 * we still successfully return the local repository.
	 */
	error = git_remote_lookup(&origin, repo, "origin");
	if (error) {
		if (!is_subsurface_cloud)
			report_error("Repository '%s' origin lookup failed (%s)", remote, giterr_last()->message);
		return 0;
	}

	if (rt == RT_HTTPS && !canReachCloudServer()) {
		// this is not an error, just a warning message, so return 0
		report_error("Cannot connect to cloud server, working with local copy");
		return 0;
	}
	if (verbose)
		fprintf(stderr, "git storage: fetch remote\n");
#if USE_LIBGIT23_API
	git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
	opts.callbacks.transfer_progress = &transfer_progress_cb;
	if (rt == RT_SSH)
		opts.callbacks.credentials = credential_ssh_cb;
	else if (rt == RT_HTTPS)
		opts.callbacks.credentials = credential_https_cb;
	opts.callbacks.certificate_check = certificate_check_cb;
	error = git_remote_fetch(origin, NULL, &opts, NULL);
#else
	error = git_remote_fetch(origin, NULL, NULL, NULL);
#endif
	// NOTE! A fetch error is not fatal, we just report it
	if (error) {
		if (is_subsurface_cloud)
			report_error("Cannot sync with cloud server, working with offline copy");
		else
			report_error("Unable to fetch remote '%s'", remote);
		if (verbose)
			fprintf(stderr, "remote fetch failed (%s)\n", giterr_last()->message);
		error = 0;
	} else {
		error = check_remote_status(repo, origin, remote, branch, rt);
	}
	git_remote_free(origin);
	return error;
}

static git_repository *update_local_repo(const char *localdir, const char *remote, const char *branch, enum remote_transport rt)
{
	int error;
	git_repository *repo = NULL;

	if (verbose)
		fprintf(stderr, "git storage: update local repo\n");

	error = git_repository_open(&repo, localdir);
	if (error) {
		if (is_subsurface_cloud)
			(void)cleanup_local_cache(remote, branch);
		else
			report_error("Unable to open git cache repository at %s: %s", localdir, giterr_last()->message);
		return NULL;
	}
	sync_with_remote(repo, remote, branch, rt);
	return repo;
}

static int repository_create_cb(git_repository **out, const char *path, int bare, void *payload)
{
	char *proxy_string;
	git_config *conf;

	int ret = git_repository_init(out, path, bare);

	git_repository_config(&conf, *out);
	if (getProxyString(&proxy_string)) {
		if (verbose)
			fprintf(stderr, "set proxy to \"%s\"\n", proxy_string);
		git_config_set_string(conf, "http.proxy", proxy_string);
		free(proxy_string);
	} else {
		if (verbose)
			fprintf(stderr, "delete proxy setting\n");
		git_config_delete_entry(conf, "http.proxy");
	}
	return ret;
}

/* this should correctly initialize both the local and remote
 * repository for the Subsurface cloud storage */
static git_repository *create_and_push_remote(const char *localdir, const char *remote, const char *branch)
{
	git_repository *repo;
	git_config *conf;
	int len;
	char *variable_name, *merge_head;

	if (verbose)
		fprintf(stderr, "git storage: create and push remote\n");

	/* first make sure the directory for the local cache exists */
	subsurface_mkdir(localdir);

	/* set up the origin to point to our remote */
	git_repository_init_options init_opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
	init_opts.origin_url = remote;

	/* now initialize the repository with */
	git_repository_init_ext(&repo, localdir, &init_opts);

	/* create a config so we can set the remote tracking branch */
	git_repository_config(&conf, repo);
	len = sizeof("branch..remote") + strlen(branch);
	variable_name = malloc(len);
	snprintf(variable_name, len, "branch.%s.remote", branch);
	git_config_set_string(conf, variable_name, "origin");
	/* we know this is shorter than the previous one, so we reuse the variable*/
	snprintf(variable_name, len, "branch.%s.merge", branch);
	len = sizeof("refs/heads/") + strlen(branch);
	merge_head = malloc(len);
	snprintf(merge_head, len, "refs/heads/%s", branch);
	git_config_set_string(conf, variable_name, merge_head);

	/* finally create an empty commit and push it to the remote */
	if (do_git_save(repo, branch, remote, false, true))
		return NULL;
	return(repo);
}

static git_repository *create_local_repo(const char *localdir, const char *remote, const char *branch, enum remote_transport rt)
{
	int error;
	git_repository *cloned_repo = NULL;
	git_clone_options opts = GIT_CLONE_OPTIONS_INIT;

	if (verbose)
		fprintf(stderr, "git storage: create_local_repo\n");

#if USE_LIBGIT23_API
	opts.fetch_opts.callbacks.transfer_progress = &transfer_progress_cb;
	if (rt == RT_SSH)
		opts.fetch_opts.callbacks.credentials = credential_ssh_cb;
	else if (rt == RT_HTTPS)
		opts.fetch_opts.callbacks.credentials = credential_https_cb;
	opts.repository_cb = repository_create_cb;
	opts.fetch_opts.callbacks.certificate_check = certificate_check_cb;
#endif
	opts.checkout_branch = branch;
	if (rt == RT_HTTPS && !canReachCloudServer())
		return 0;
	if (verbose > 1)
		fprintf(stderr, "git storage: calling git_clone()\n");
	error = git_clone(&cloned_repo, remote, localdir, &opts);
	if (verbose > 1)
		fprintf(stderr, "git storage: returned from git_clone() with error %d\n", error);
	if (error) {
		char *msg = giterr_last()->message;
		int len = sizeof("Reference 'refs/remotes/origin/' not found") + strlen(branch);
		char *pattern = malloc(len);
		snprintf(pattern, len, "Reference 'refs/remotes/origin/%s' not found", branch);
		if (strstr(remote, prefs.cloud_git_url) && strstr(msg, pattern)) {
			/* we're trying to open the remote branch that corresponds
			 * to our cloud storage and the branch doesn't exist.
			 * So we need to create the branch and push it to the remote */
			cloned_repo = create_and_push_remote(localdir, remote, branch);
#if !defined(DEBUG)
		} else if (is_subsurface_cloud) {
			report_error(translate("gettextFromC", "Error connecting to Subsurface cloud storage"));
#endif
		} else {
			report_error(translate("gettextFromC", "git clone of %s failed (%s)"), remote, msg);
		}
		free(pattern);
	}
	return cloned_repo;
}

static struct git_repository *get_remote_repo(const char *localdir, const char *remote, const char *branch)
{
	struct stat st;
	enum remote_transport rt;

	/* figure out the remote transport */
	if (strncmp(remote, "ssh://", 6) == 0)
		rt = RT_SSH;
	else if (strncmp(remote, "https://", 8) == 0)
		rt = RT_HTTPS;
	else
		rt = RT_OTHER;

	if (verbose > 1) {
		fprintf(stderr, "git storage: accessing %s\n", remote);
	}
	/* Do we already have a local cache? */
	if (!stat(localdir, &st)) {
		if (!S_ISDIR(st.st_mode)) {
			if (is_subsurface_cloud)
				(void)cleanup_local_cache(remote, branch);
			else
				report_error("local git cache at '%s' is corrupt");
			return NULL;
		}
		return update_local_repo(localdir, remote, branch, rt);
	}
	return create_local_repo(localdir, remote, branch, rt);
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
static struct git_repository *is_remote_git_repository(char *remote, const char *branch)
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

	/*
	 * next we need to make sure that any encoded username
	 * has been extracted from an https:// based URL
	 */
	if  (!strncmp(remote, "https://", 8)) {
		char *at = strchr(remote, '@');
		if (at) {
			/* was this the @ that denotes an account? that means it was before the
			 * first '/' after the https:// - so let's find a '/' after that and compare */
			char *slash = strchr(remote + 8, '/');
			if (slash && slash > at) {
				/* grab the part between "https://" and "@" as encoded email address
				 * (that's our username) and move the rest of the URL forward, remembering
				 * to copy the closing NUL as well */
				prefs.cloud_storage_email_encoded = strndup(remote + 8, at - remote - 8);
				memmove(remote + 8, at + 1, strlen(at + 1) + 1);
			}
		}
	}
	localdir = get_local_dir(remote, branch);
	if (!localdir)
		return NULL;

	/* remember if the current git storage we are working on is our cloud storage
	 * this is used to create more user friendly error message and warnings */
	is_subsurface_cloud = strstr(remote, prefs.cloud_git_url) != NULL;

	return get_remote_repo(localdir, remote, branch);
}

/*
 * If it's not a git repo, return NULL. Be very conservative.
 */
struct git_repository *is_git_repository(const char *filename, const char **branchp, const char **remote, bool dry_run)
{
	int flen, blen, ret;
	int offset = 1;
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
	while (flen && filename[flen-1] == '/') {
		flen--;
		offset++;
	}

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

	branch = format_string("%.*s", blen, filename + flen + offset);
	if (!branch) {
		free(loc);
		return dummy_git_repository;
	}

	if (dry_run) {
		*branchp = branch;
		*remote = loc;
		return dummy_git_repository;
	}
	repo = is_remote_git_repository(loc, branch);
	if (repo) {
		if (remote)
			*remote = loc;
		else
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
	if (remote)
		*remote = NULL;
	*branchp = branch;
	return repo;
}
