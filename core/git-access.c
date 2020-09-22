// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
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
#include <stdarg.h>
#include <git2.h>

#include "subsurface-string.h"
#include "membuffer.h"
#include "strndup.h"
#include "qthelper.h"
#include "file.h"
#include "errorhelper.h"
#include "git-access.h"
#include "gettext.h"
#include "sha1.h"

bool is_subsurface_cloud = false;

// the mobile app assumes that it shouldn't talk to the cloud
// the desktop app assumes that it should
#if defined(SUBSURFACE_MOBILE)
bool git_local_only = true;
#else
bool git_local_only = false;
#endif
bool git_remote_sync_successful = false;


int (*update_progress_cb)(const char *) = NULL;

static bool includes_string_caseinsensitive(const char *haystack, const char *needle)
{
	if (!needle)
		return 1; /* every string includes the NULL string */
	if (!haystack)
		return 0; /* nothing is included in the NULL string */
	int len = strlen(needle);
	while (*haystack) {
		if (strncasecmp(haystack, needle, len))
			return 1;
		haystack++;
	}
	return 0;
}

void set_git_update_cb(int(*cb)(const char *))
{
	update_progress_cb = cb;
}

// total overkill, but this allows us to get good timing in various scenarios;
// the various parts of interacting with the local and remote git repositories send
// us updates which indicate progress (and no, this is not smooth and definitely not
// proportional - some parts are based on compute performance, some on network speed)
// they also provide information where in the process we are so we can analyze the log
// to understand which parts of the process take how much time.
int git_storage_update_progress(const char *text)
{
	int ret = 0;
	if (update_progress_cb)
		ret = (*update_progress_cb)(text);
	return ret;
}

// the checkout_progress_cb doesn't allow canceling of the operation
// map the git progress to 20% of overall progress
static void progress_cb(const char *path, size_t completed_steps, size_t total_steps, void *payload)
{
	UNUSED(path);
	UNUSED(payload);
	char buf[80];
	snprintf(buf, sizeof(buf),  translate("gettextFromC", "Checkout from storage (%lu/%lu)"), completed_steps, total_steps);
	(void)git_storage_update_progress(buf);
}

// this randomly assumes that 80% of the time is spent on the objects and 20% on the deltas
// map the git progress to 20% of overall progress
// if the user cancels the dialog this is passed back to libgit2
static int transfer_progress_cb(const git_transfer_progress *stats, void *payload)
{
	UNUSED(payload);

	static int last_done = -1;
	char buf[80];
	int done = 0;
	int total = 0;

	if (stats->total_objects) {
		total = 60;
		done = 60 * stats->received_objects / stats->total_objects;
	}
	if (stats->total_deltas) {
		total += 20;
		done += 20 * stats->indexed_deltas / stats->total_deltas;
	}
	/* for debugging this is useful
	char buf[100];
	snprintf(buf, 100, "transfer cb rec_obj %d tot_obj %d idx_delta %d total_delta %d local obj %d", stats->received_objects, stats->total_objects, stats->indexed_deltas, stats->total_deltas, stats->local_objects);
	return git_storage_update_progress(buf);
	 */
	if (done > last_done) {
		last_done = done;
		snprintf(buf, sizeof(buf), translate("gettextFromC", "Transfer from storage (%d/%d)"), done, total);
		return git_storage_update_progress(buf);
	}
	return 0;
}

// the initial push to sync the repos is mapped to 10% of overall progress
static int push_transfer_progress_cb(unsigned int current, unsigned int total, size_t bytes, void *payload)
{
	UNUSED(bytes);
	UNUSED(payload);
	char buf[80];
	snprintf(buf, sizeof(buf), translate("gettextFromC", "Transfer to storage (%d/%d)"), current, total);
	return git_storage_update_progress(buf);
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
	UNUSED(payload);
	status &= ~GIT_STATUS_CURRENT | GIT_STATUS_IGNORED;
	if (!status)
		return 0;
	SSRF_INFO("git storage: local cache dir %s modified, git status 0x%04x", path, status);
	if (is_subsurface_cloud)
		report_error(translate("gettextFromC", "Local cache directory %s corrupted - can't sync with Subsurface cloud storage"), path);
	else
		report_error("WARNING: Git cache directory modified (path %s) status 0x%04x", path, status);
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
		SSRF_INFO("git storage: reset to remote\n");

	// If it's not checked out (bare or not HEAD), just update the reference */
	if (git_repository_is_bare(repo) || git_branch_is_head(local) != 1) {
		git_reference *out;

		if (git_reference_set_target(&out, local, new_id, "Update to remote")) {
			SSRF_INFO("git storage: could not update local cache to newer remote data");
			return report_error(translate("gettextFromC", "Could not update local cache to newer remote data"));
		}
		git_reference_free(out);

#ifdef DEBUG
		// Not really an error, just informational
		report_error("Updated local branch from remote");
#endif
		return 0;
	}

	if (git_object_lookup(&target, repo, new_id, GIT_OBJ_COMMIT)) {
		SSRF_INFO("git storage: could not look up remote commit");
		if (is_subsurface_cloud)
			return report_error(translate("gettextFromC", "Subsurface cloud storage corrupted"));
		else
			return report_error("Could not look up remote commit");
	}
	opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	if (git_reset(repo, target, GIT_RESET_HARD, &opts)) {
		SSRF_INFO("git storage: local head checkout failed after update");
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

static int auth_attempt = 0;
static const int max_auth_attempts = 2;

static bool exceeded_auth_attempts()
{
	if (auth_attempt++ > max_auth_attempts) {
		SSRF_INFO("git storage: authentication to cloud storage failed");
		report_error("Authentication to cloud storage failed.");
		return true;
	}
	return false;
}

int credential_ssh_cb(git_cred **out,
		  const char *url,
		  const char *username_from_url,
		  unsigned int allowed_types,
		  void *payload)
{
	UNUSED(url);
	UNUSED(payload);
	UNUSED(username_from_url);

	const char *username = prefs.cloud_storage_email_encoded;
	const char *passphrase = prefs.cloud_storage_password ? prefs.cloud_storage_password : "";

	// TODO: We need a way to differentiate between password and private key authentication
	if (allowed_types & GIT_CREDTYPE_SSH_KEY) {
		char *priv_key = format_string("%s/%s", system_default_directory(), "ssrf_remote.key");
		if (!access(priv_key, F_OK)) {
			if (exceeded_auth_attempts())
				return GIT_EUSER;
			int ret = git_cred_ssh_key_new(out, username, NULL, priv_key, passphrase);
			free(priv_key);
			return ret;
		}
		free(priv_key);
	}

	if (allowed_types & GIT_CREDTYPE_USERPASS_PLAINTEXT) {
		if (exceeded_auth_attempts())
			return GIT_EUSER;
		return git_cred_userpass_plaintext_new(out, username, passphrase);
	}

	if (allowed_types & GIT_CREDTYPE_USERNAME)
		return git_cred_username_new(out, username);

	report_error("No supported ssh authentication.");
	return GIT_EUSER;
}

int credential_https_cb(git_cred **out,
			const char *url,
			const char *username_from_url,
			unsigned int allowed_types,
			void *payload)
{
	UNUSED(url);
	UNUSED(username_from_url);
	UNUSED(payload);
	UNUSED(allowed_types);

	if (exceeded_auth_attempts())
		return GIT_EUSER;

	const char *username = prefs.cloud_storage_email_encoded;
	const char *password = prefs.cloud_storage_password ? prefs.cloud_storage_password : "";

	return git_cred_userpass_plaintext_new(out, username, password);
}

int certificate_check_cb(git_cert *cert, int valid, const char *host, void *payload)
{
	UNUSED(payload);
	if (verbose)
		SSRF_INFO("git storage: certificate callback for host %s with validity %d\n", host, valid);
	if (same_string(host, "cloud.subsurface-divelog.org") && cert->cert_type == GIT_CERT_X509) {
		// for some reason the LetsEncrypt certificate makes libgit2 throw up on some
		// platforms but not on others
		// if we are connecting to the cloud server we alrady called 'canReachCloudServer()'
		// which will fail if the SSL certificate isn't valid, so let's simply always
		// tell the caller that this certificate is valid
		return 1;
	}
	return valid;
}

static int update_remote(git_repository *repo, git_remote *origin, git_reference *local, git_reference *remote, enum remote_transport rt)
{
	UNUSED(repo);
	UNUSED(remote);

	git_push_options opts = GIT_PUSH_OPTIONS_INIT;
	git_strarray refspec;
	const char *name = git_reference_name(local);

	if (verbose)
		SSRF_INFO("git storage: update remote\n");

	refspec.count = 1;
	refspec.strings = (char **)&name;

	auth_attempt = 0;
	opts.callbacks.push_transfer_progress = &push_transfer_progress_cb;
	if (rt == RT_SSH)
		opts.callbacks.credentials = credential_ssh_cb;
	else if (rt == RT_HTTPS)
		opts.callbacks.credentials = credential_https_cb;
	opts.callbacks.certificate_check = certificate_check_cb;

	if (git_remote_push(origin, &refspec, &opts)) {
		const char *msg = giterr_last()->message;
		SSRF_INFO("git storage: unable to update remote with current local cache state, error: %s", msg);
		if (is_subsurface_cloud)
			return report_error(translate("gettextFromC", "Could not update Subsurface cloud storage, try again later"));
		else
			return report_error("Unable to update remote with current local cache state (%s)", msg);
	}
	return 0;
}

extern int update_git_checkout(git_repository *repo, git_object *parent, git_tree *tree);

static int try_to_git_merge(git_repository *repo, git_reference **local_p, git_reference *remote, git_oid *base, const git_oid *local_id, const git_oid *remote_id)
{
	UNUSED(remote);
	git_tree *local_tree, *remote_tree, *base_tree;
	git_commit *local_commit, *remote_commit, *base_commit;
	git_index *merged_index;
	git_merge_options merge_options;
	struct membuffer msg = { 0, 0, NULL};

	if (verbose) {
		char outlocal[41], outremote[41];
		outlocal[40] = outremote[40] = 0;
		git_oid_fmt(outlocal, local_id);
		git_oid_fmt(outremote, remote_id);
		SSRF_INFO("git storage: trying to merge local SHA %s remote SHA %s\n", outlocal, outremote);
	}

	git_merge_init_options(&merge_options, GIT_MERGE_OPTIONS_VERSION);
	merge_options.flags = GIT_MERGE_FIND_RENAMES;
	merge_options.file_favor = GIT_MERGE_FILE_FAVOR_UNION;
	merge_options.rename_threshold = 100;
	if (git_commit_lookup(&local_commit, repo, local_id)) {
		SSRF_INFO("git storage: remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&local_tree, local_commit)) {
		SSRF_INFO("git storage: remote storage and local data diverged. Error: failed local tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_lookup(&remote_commit, repo, remote_id)) {
		SSRF_INFO("git storage: remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&remote_tree, remote_commit)) {
		SSRF_INFO("git storage: remote storage and local data diverged. Error: failed local tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_lookup(&base_commit, repo, base)) {
		SSRF_INFO("git storage: remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&base_tree, base_commit)) {
		SSRF_INFO("git storage: remote storage and local data diverged. Error: failed base tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_merge_trees(&merged_index, repo, base_tree, local_tree, remote_tree, &merge_options)) {
		SSRF_INFO("git storage: remote storage and local data diverged. Error: merge failed (%s)", giterr_last()->message);
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
			SSRF_INFO("git storage: conflict in %s / %s / %s -- ",
				ours ? ours->path : "-",
				theirs ? theirs->path : "-",
				ancestor ? ancestor->path : "-");
			if ((!ours && theirs && ancestor) ||
			    (ours && !theirs && ancestor)) {
				// the file was removed on one side or the other - just remove it
				SSRF_INFO("git storage: looks like a delete on one side; removing the file from the index\n");
				error = git_index_remove(merged_index, ours ? ours->path : theirs->path, GIT_INDEX_STAGE_ANY);
			} else if (ancestor) {
				error = git_index_conflict_remove(merged_index, ours ? ours->path : theirs ? theirs->path : ancestor->path);
			}
			if (error) {
				SSRF_INFO("git storage: error at conflict resolution (%s)", giterr_last()->message);
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
	if (get_authorship(repo, &author) < 0)
		goto write_error;
	const char *user_agent = subsurface_user_agent();
	put_format(&msg, "Automatic merge\n\nCreated by %s\n", user_agent);
	free((void *)user_agent);
	if (git_commit_create_v(&commit_oid, repo, NULL, author, author, NULL, mb_cstring(&msg), merged_tree, 2, local_commit, remote_commit))
		goto write_error;
	if (git_commit_lookup(&commit, repo, &commit_oid))
		goto write_error;
	if (git_branch_is_head(*local_p) && !git_repository_is_bare(repo)) {
		git_object *parent;
		git_reference_peel(&parent, *local_p, GIT_OBJ_COMMIT);
		if (update_git_checkout(repo, parent, merged_tree)) {
			goto write_error;
		}
	}
	if (git_reference_set_target(local_p, *local_p, &commit_oid, "Subsurface merge event"))
		goto write_error;
	set_git_id(&commit_oid);
	git_signature_free(author);
	if (verbose)
		SSRF_INFO("git storage: successfully merged repositories");
	free_buffer(&msg);
	return 0;

diverged_error:
	return report_error(translate("gettextFromC", "Remote storage and local data diverged"));

write_error:
	free_buffer(&msg);
	return report_error(translate("gettextFromC", "Remote storage and local data diverged. Error: writing the data failed (%s)"), giterr_last()->message);
}

// if accessing the local cache of Subsurface cloud storage fails, we simplify things
// for the user and simply move the cache away (in case they want to try and extract data)
// and ask them to retry the operation (which will then refresh the data from the cloud server)
static int cleanup_local_cache(const char *remote_url, const char *branch)
{
	char *backup_path = move_local_cache(remote_url, branch);
	SSRF_INFO("git storage: problems with local cache, moved to %s", backup_path);
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
	int ret = 0;

	if (verbose)
		SSRF_INFO("git storage: try to update\n");

	if (!git_reference_cmp(local, remote))
		return 0;

	// Dirty modified state in the working tree? We're not going
	// to update either way
	if (git_status_foreach(repo, check_clean, NULL)) {
		SSRF_INFO("git storage: local cache is dirty, skipping update");
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("local cached copy is dirty, skipping update");
	}
	local_id = git_reference_target(local);
	remote_id = git_reference_target(remote);

	if (!local_id || !remote_id) {
		if (!local_id)
			SSRF_INFO("git storage: unable to get local SHA");
		if (!remote_id)
			SSRF_INFO("git storage: unable to get remote SHA");
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Unable to get local or remote SHA1");
	}
	if (git_merge_base(&base, repo, local_id, remote_id)) {
		// TODO:
		// if they have no merge base, they actually are different repos
		// so instead merge this as merging a commit into a repo - git_merge() appears to do that
		// but needs testing and cleanup afterwards
		//
		SSRF_INFO("git storage: no common commit between local and remote branches");
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Unable to find common commit of local and remote branches");
	}
	/* Is the remote strictly newer? Use it */
	if (git_oid_equal(&base, local_id)) {
		if (verbose)
			SSRF_INFO("git storage: remote is newer than local, update local");
		git_storage_update_progress(translate("gettextFromC", "Update local storage to match cloud storage"));
		return reset_to_remote(repo, local, remote_id);
	}

	/* Is the local repo the more recent one? See if we can update upstream */
	if (git_oid_equal(&base, remote_id)) {
		if (verbose)
			SSRF_INFO("git storage: local is newer than remote, update remote");
		git_storage_update_progress(translate("gettextFromC", "Push local changes to cloud storage"));
		return update_remote(repo, origin, local, remote, rt);
	}
	/* Merging a bare repository always needs user action */
	if (git_repository_is_bare(repo)) {
		SSRF_INFO("git storage: local is bare and has diverged from remote; user action needed");
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Local and remote have diverged, merge of bare branch needed");
	}
	/* Merging will definitely need the head branch too */
	if (git_branch_is_head(local) != 1) {
		SSRF_INFO("git storage: local branch is not HEAD, cannot merge");
		if (is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Local and remote do not match, local branch not HEAD - cannot update");
	}
	/* Ok, let's try to merge these */
	git_storage_update_progress(translate("gettextFromC", "Try to merge local changes into cloud storage"));
	ret = try_to_git_merge(repo, &local, remote, &base, local_id, remote_id);
	if (ret == 0)
		return update_remote(repo, origin, local, remote, rt);
	else
		return ret;

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
		SSRF_INFO("git storage: check remote status\n");

	if (git_branch_lookup(&local_ref, repo, branch, GIT_BRANCH_LOCAL)) {
		SSRF_INFO("git storage: branch %s is missing in local repo", branch);
		if (is_subsurface_cloud)
			return cleanup_local_cache(remote, branch);
		else
			return report_error("Git cache branch %s no longer exists", branch);
	}
	if (git_branch_upstream(&remote_ref, local_ref)) {
		/* so there is no upstream branch for our branch; that's a problem.
		 * let's push our branch */
		SSRF_INFO("git storage: branch %s is missing in remote, pushing branch", branch);
		git_strarray refspec;
		git_reference_list(&refspec, repo);
		git_push_options opts = GIT_PUSH_OPTIONS_INIT;
		opts.callbacks.transfer_progress = &transfer_progress_cb;
		auth_attempt = 0;
		if (rt == RT_SSH)
			opts.callbacks.credentials = credential_ssh_cb;
		else if (rt == RT_HTTPS)
			opts.callbacks.credentials = credential_https_cb;
		opts.callbacks.certificate_check = certificate_check_cb;
		git_storage_update_progress(translate("gettextFromC", "Store data into cloud storage"));
		error = git_remote_push(origin, &refspec, &opts);
	} else {
		error = try_to_update(repo, origin, local_ref, remote_ref, remote, branch, rt);
		git_reference_free(remote_ref);
	}
	git_reference_free(local_ref);
	git_remote_sync_successful = (error == 0);
	return error;
}

/* this is (so far) only used by the git storage tests to remove a remote branch
 * it will print out errors, but not return an error (as this isn't a function that
 * we test as part of the tests, it's a helper to not leave loads of dead branches on
 * the server)
 */
void delete_remote_branch(git_repository *repo, const char *remote, const char *branch)
{
	int error;
	char *proxy_string;
	git_remote *origin;
	git_config *conf;

	/* set up the config and proxy information in order to connect to the server */
	git_repository_config(&conf, repo);
	if (getProxyString(&proxy_string)) {
		git_config_set_string(conf, "http.proxy", proxy_string);
		free(proxy_string);
	} else {
		git_config_delete_entry(conf, "http.proxy");
	}
	if (git_remote_lookup(&origin, repo, "origin")) {
		SSRF_INFO("git storage: repository '%s' origin lookup failed (%s)", remote, giterr_last() ? giterr_last()->message : "(unspecified)");
		return;
	}
	/* fetch the remote state */
	git_fetch_options f_opts = GIT_FETCH_OPTIONS_INIT;
	auth_attempt = 0;
	f_opts.callbacks.credentials = credential_https_cb;
	error = git_remote_fetch(origin, NULL, &f_opts, NULL);
	if (error) {
		SSRF_INFO("git storage: remote fetch failed (%s)\n", giterr_last() ? giterr_last()->message : "authentication failed");
		return;
	}
	/* delete the remote branch by pushing to ":refs/heads/<branch>" */
	git_strarray refspec;
	char *branch_ref = format_string(":refs/heads/%s", branch);
	refspec.count = 1;
	refspec.strings = &branch_ref;
	git_push_options p_opts = GIT_PUSH_OPTIONS_INIT;
	auth_attempt = 0;
	p_opts.callbacks.credentials = credential_https_cb;
	error = git_remote_push(origin, &refspec, &p_opts);
	free(branch_ref);
	if (error) {
		SSRF_INFO("git storage: unable to delete branch '%s'", branch);
		SSRF_INFO("git storage: error was (%s)\n", giterr_last() ? giterr_last()->message : "(unspecified)");
	}
	git_remote_free(origin);
	return;
}

int sync_with_remote(git_repository *repo, const char *remote, const char *branch, enum remote_transport rt)
{
	int error;
	git_remote *origin;
	char *proxy_string;
	git_config *conf;

	if (git_local_only) {
		if (verbose)
			SSRF_INFO("git storage: don't sync with remote - read from cache only\n");
		return 0;
	}
	if (verbose)
		SSRF_INFO("git storage: sync with remote %s[%s]\n", remote, branch);
	git_storage_update_progress(translate("gettextFromC", "Sync with cloud storage"));
	git_repository_config(&conf, repo);
	if (rt == RT_HTTPS && getProxyString(&proxy_string)) {
		if (verbose)
			SSRF_INFO("git storage: set proxy to \"%s\"\n", proxy_string);
		git_config_set_string(conf, "http.proxy", proxy_string);
		free(proxy_string);
	} else {
		if (verbose)
			SSRF_INFO("git storage: delete proxy setting\n");
		git_config_delete_entry(conf, "http.proxy");
	}

	/*
	 * NOTE! Remote errors are reported, but are nonfatal:
	 * we still successfully return the local repository.
	 */
	error = git_remote_lookup(&origin, repo, "origin");
	if (error) {
		const char *msg = giterr_last()->message;
		SSRF_INFO("git storage: repo %s origin lookup failed with: %s", remote, msg);
		if (!is_subsurface_cloud)
			report_error("Repository '%s' origin lookup failed (%s)", remote, msg);
		return 0;
	}

	if (is_subsurface_cloud && !canReachCloudServer()) {
		// this is not an error, just a warning message, so return 0
		SSRF_INFO("git storage: cannot connect to remote server");
		report_error("Cannot connect to cloud server, working with local copy");
		git_storage_update_progress(translate("gettextFromC", "Can't reach cloud server, working with local data"));
		return 0;
	}
	if (verbose)
		SSRF_INFO("git storage: fetch remote\n");
	git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
	opts.callbacks.transfer_progress = &transfer_progress_cb;
	auth_attempt = 0;
	if (rt == RT_SSH)
		opts.callbacks.credentials = credential_ssh_cb;
	else if (rt == RT_HTTPS)
		opts.callbacks.credentials = credential_https_cb;
	opts.callbacks.certificate_check = certificate_check_cb;
	git_storage_update_progress(translate("gettextFromC", "Successful cloud connection, fetch remote"));
	error = git_remote_fetch(origin, NULL, &opts, NULL);
	// NOTE! A fetch error is not fatal, we just report it
	if (error) {
		if (is_subsurface_cloud)
			report_error("Cannot sync with cloud server, working with offline copy");
		else
			report_error("Unable to fetch remote '%s'", remote);
		// If we returned GIT_EUSER during authentication, giterr_last() returns NULL
		SSRF_INFO("git storage: remote fetch failed (%s)\n", giterr_last() ? giterr_last()->message : "authentication failed");
		// Since we failed to sync with online repository, enter offline mode
		git_local_only = true;
		error = 0;
	} else {
		error = check_remote_status(repo, origin, remote, branch, rt);
	}
	git_remote_free(origin);
	git_storage_update_progress(translate("gettextFromC", "Done syncing with cloud storage"));
	return error;
}

static git_repository *update_local_repo(const char *localdir, const char *remote, const char *branch, enum remote_transport rt)
{
	int error;
	git_repository *repo = NULL;
	git_reference *head;

	if (verbose)
		SSRF_INFO("git storage: update local repo\n");

	error = git_repository_open(&repo, localdir);
	if (error) {
		const char *msg = giterr_last()->message;
		SSRF_INFO("git storage: unable to open local cache at %s: %s", localdir, msg);
		if (is_subsurface_cloud)
			(void)cleanup_local_cache(remote, branch);
		else
			report_error("Unable to open git cache repository at %s: %s", localdir, msg);
		return NULL;
	}

	/* Check the HEAD being the right branch */
	if (!git_repository_head(&head, repo)) {
		const char *name;
		if (!git_branch_name(&name, head)) {
			if (strcmp(name, branch)) {
				char *branchref = format_string("refs/heads/%s", branch);
				SSRF_INFO("git storage: setting cache branch from '%s' to '%s'", name, branch);
				git_repository_set_head(repo, branchref);
				free(branchref);
			}
		}
		git_reference_free(head);
	}

	if (!git_local_only)
		sync_with_remote(repo, remote, branch, rt);

	return repo;
}

static int repository_create_cb(git_repository **out, const char *path, int bare, void *payload)
{
	UNUSED(payload);
	char *proxy_string;
	git_config *conf;

	int ret = git_repository_init(out, path, bare);
	if (ret != 0) {
		if (verbose)
			SSRF_INFO("git storage: initializing git repository failed\n");
		return ret;
	}

	git_repository_config(&conf, *out);
	if (getProxyString(&proxy_string)) {
		if (verbose)
			SSRF_INFO("git storage: set proxy to \"%s\"\n", proxy_string);
		git_config_set_string(conf, "http.proxy", proxy_string);
		free(proxy_string);
	} else {
		if (verbose)
			SSRF_INFO("git storage: delete proxy setting\n");
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
	char *variable_name, *head;

	if (verbose)
		SSRF_INFO("git storage: create and push remote\n");

	/* first make sure the directory for the local cache exists */
	subsurface_mkdir(localdir);

	head = format_string("refs/heads/%s", branch);

	/* set up the origin to point to our remote */
	git_repository_init_options init_opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
	init_opts.origin_url = remote;
	init_opts.initial_head = head;

	/* now initialize the repository with */
	git_repository_init_ext(&repo, localdir, &init_opts);

	/* create a config so we can set the remote tracking branch */
	git_repository_config(&conf, repo);
	variable_name = format_string("branch.%s.remote", branch);
	git_config_set_string(conf, variable_name, "origin");
	free(variable_name);

	variable_name = format_string("branch.%s.merge", branch);
	git_config_set_string(conf, variable_name, head);
	free(head);
	free(variable_name);

	/* finally create an empty commit and push it to the remote */
	if (do_git_save(repo, branch, remote, false, true))
		return NULL;
	return repo;
}

static git_repository *create_local_repo(const char *localdir, const char *remote, const char *branch, enum remote_transport rt)
{
	int error;
	git_repository *cloned_repo = NULL;
	git_clone_options opts = GIT_CLONE_OPTIONS_INIT;

	if (verbose)
		SSRF_INFO("git storage: create_local_repo\n");

	auth_attempt = 0;
	opts.fetch_opts.callbacks.transfer_progress = &transfer_progress_cb;
	if (rt == RT_SSH)
		opts.fetch_opts.callbacks.credentials = credential_ssh_cb;
	else if (rt == RT_HTTPS)
		opts.fetch_opts.callbacks.credentials = credential_https_cb;
	opts.repository_cb = repository_create_cb;
	opts.fetch_opts.callbacks.certificate_check = certificate_check_cb;

	opts.checkout_branch = branch;
	if (is_subsurface_cloud && !canReachCloudServer()) {
		SSRF_INFO("git storage: cannot reach remote server");
		return 0;
	}
	if (verbose > 1)
		SSRF_INFO("git storage: calling git_clone()\n");
	error = git_clone(&cloned_repo, remote, localdir, &opts);
	if (verbose > 1)
		SSRF_INFO("git storage: returned from git_clone() with return value %d\n", error);
	if (error) {
		SSRF_INFO("git storage: clone of %s failed", remote);
		char *msg = "";
		if (giterr_last()) {
			 msg = giterr_last()->message;
			 SSRF_INFO("git storage: error message was %s\n", msg);
		}
		char *pattern = format_string("reference 'refs/remotes/origin/%s' not found", branch);
		// it seems that we sometimes get 'Reference' and sometimes 'reference'
		if (includes_string_caseinsensitive(msg, pattern)) {
			/* we're trying to open the remote branch that corresponds
			 * to our cloud storage and the branch doesn't exist.
			 * So we need to create the branch and push it to the remote */
			if (verbose)
				SSRF_INFO("git storage: remote repo didn't include our branch\n");
			cloned_repo = create_and_push_remote(localdir, remote, branch);
#if !defined(DEBUG) && !defined(SUBSURFACE_MOBILE)
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

enum remote_transport url_to_remote_transport(const char *remote)
{
	/* figure out the remote transport */
	if (strncmp(remote, "ssh://", 6) == 0)
		return RT_SSH;
	else if (strncmp(remote, "https://", 8) == 0)
		return RT_HTTPS;
	else
		return RT_OTHER;
}

static struct git_repository *get_remote_repo(const char *localdir, const char *remote, const char *branch)
{
	struct stat st;
	enum remote_transport rt = url_to_remote_transport(remote);

	if (verbose > 1) {
		SSRF_INFO("git storage: accessing %s\n", remote);
	}
	git_storage_update_progress(translate("gettextFromC", "Synchronising data file"));
	/* Do we already have a local cache? */
	if (!subsurface_stat(localdir, &st)) {
		if (!S_ISDIR(st.st_mode)) {
			if (is_subsurface_cloud)
				(void)cleanup_local_cache(remote, branch);
			else
				report_error("local git cache at '%s' is corrupt", localdir);
			return NULL;
		}
		return update_local_repo(localdir, remote, branch, rt);
	} else {
		/* We have no local cache yet.
		 * Take us temporarly online to create a local and
		 * remote cloud repo.
		 */
		git_repository *ret;
		bool glo = git_local_only;
		git_local_only = false;
		ret = create_local_repo(localdir, remote, branch, rt);
		git_local_only = glo;
		if (ret)
			git_remote_sync_successful = true;
		return ret;
	}

	/* all normal cases are handled above */
	return 0;
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
	char *p = remote;

	while ((c = *p++) >= 'a' && c <= 'z')
		/* nothing */;
	if (c != ':')
		return NULL;
	if (*p++ != '/' || *p++ != '/')
		return NULL;

	/*
	 * Ok, we found "[a-z]*://" and we think we have a real
	 * "remote git" format. The "file://" case was handled
	 * in the calling function.
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
	 * has been extracted from the URL
	 */
	char *at = strchr(remote, '@');
	if (at) {
		/* was this the @ that denotes an account? that means it was before the
		 * first '/' after the protocol:// - so let's find a '/' after that and compare */
		char *slash = strchr(p, '/');
		if (slash && slash > at) {
			/* grab the part between "protocol://" and "@" as encoded email address
			 * (that's our username) and move the rest of the URL forward, remembering
			 * to copy the closing NUL as well */
			prefs.cloud_storage_email_encoded = strndup(p, at - p);
			memmove(p, at + 1, strlen(at + 1) + 1);
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

	/* we are looking at a new potential remote, but we haven't synced with it */
	git_remote_sync_successful = false;

	flen = strlen(filename);
	if (!flen || filename[--flen] != ']')
		return NULL;

	/*
	 * Special-case "file://", and treat it as a local
	 * repository since libgit2 is insanely slow for that.
	 */
	if (!strncmp(filename, "file://", 7)) {
		filename += 7;
		flen -= 7;
	}

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

	if (subsurface_stat(loc, &st) < 0 || !S_ISDIR(st.st_mode)) {
		if (verbose)
			SSRF_INFO("git storage: loc %s wasn't found or is not a directory\n", loc);
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

int git_create_local_repo(const char *filename)
{
	git_repository *repo;
	char *path = strdup(filename);
	char *branch = strchr(path, '[');
	if (branch)
		*branch = '\0';
	int ret = git_repository_init(&repo, path, false);
	free(path);
	if (ret != 0)
		(void)report_error("Create local repo failed with error code %d", ret);
	git_repository_free(repo);
	return ret;
}
