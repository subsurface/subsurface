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
#include <QString>
#include <QRegularExpression>
#include <QNetworkProxy>

#include "subsurface-string.h"
#include "format.h"
#include "membuffer.h"
#include "qthelper.h"
#include "file.h"
#include "errorhelper.h"
#include "git-access.h"
#include "gettext.h"
#include "sha1.h"

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

extern "C" void set_git_update_cb(int(*cb)(const char *))
{
	update_progress_cb = cb;
}

// total overkill, but this allows us to get good timing in various scenarios;
// the various parts of interacting with the local and remote git repositories send
// us updates which indicate progress (and no, this is not smooth and definitely not
// proportional - some parts are based on compute performance, some on network speed)
// they also provide information where in the process we are so we can analyze the log
// to understand which parts of the process take how much time.
extern "C" int git_storage_update_progress(const char *text)
{
	int ret = 0;
	if (update_progress_cb)
		ret = (*update_progress_cb)(text);
	return ret;
}

// the checkout_progress_cb doesn't allow canceling of the operation
// map the git progress to 20% of overall progress
static void progress_cb(const char *, size_t completed_steps, size_t total_steps, void *)
{
	char buf[80];
	snprintf(buf, sizeof(buf),  translate("gettextFromC", "Checkout from storage (%lu/%lu)"), completed_steps, total_steps);
	(void)git_storage_update_progress(buf);
}

// this randomly assumes that 80% of the time is spent on the objects and 20% on the deltas
// map the git progress to 20% of overall progress
// if the user cancels the dialog this is passed back to libgit2
static int transfer_progress_cb(const git_transfer_progress *stats, void *)
{
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
static int push_transfer_progress_cb(unsigned int current, unsigned int total, size_t, void *)
{
	std::string buf = casprintf_loc(translate("gettextFromC", "Transfer to storage (%d/%d)"), current, total);
	return git_storage_update_progress(buf.c_str());
}

std::string normalize_cloud_name(const std::string &remote_in)
{
	// replace ssrf-cloud-XX.subsurface... names with cloud.subsurface... names
	// that trailing '/' is to match old code
	QString ri = QString::fromStdString(remote_in);
	ri.replace(QRegularExpression(CLOUD_HOST_PATTERN), CLOUD_HOST_GENERIC "/");
	return ri.toStdString();
}

std::string get_local_dir(const std::string &url, const std::string &branch)
{
	SHA_CTX ctx;
	unsigned char hash[20];

	// this optimization could in theory lead to odd things happening if the
	// cloud backend servers ever get out of sync - but when a user switches
	// between those servers (either because one is down, or because the algorithm
	// which server to pick changed, or because the user is on a different continent),
	// then the hash and therefore the local directory would change. To prevent that
	// from happening, normalize the cloud string to always use the old default name.
	std::string remote = normalize_cloud_name(url);

	// That zero-byte update is so that we don't get hash
	// collisions for "repo1 branch" vs "repo 1branch".
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, remote.c_str(), remote.size());
	SHA1_Update(&ctx, "", 1);
	SHA1_Update(&ctx, branch.c_str(), branch.size());
	SHA1_Final(hash, &ctx);
	return format_string_std("%s/cloudstorage/%02x%02x%02x%02x%02x%02x%02x%02x",
			system_default_directory(),
			hash[0], hash[1], hash[2], hash[3],
			hash[4], hash[5], hash[6], hash[7]);
}

static std::string move_local_cache(struct git_info *info)
{
	std::string old_path = get_local_dir(info->url, info->branch);
	std::string new_path = move_away(old_path);
	return new_path;
}

static int check_clean(const char *path, unsigned int status, void *payload)
{
	struct git_info *info = (struct git_info *)payload;
	status &= ~GIT_STATUS_CURRENT | GIT_STATUS_IGNORED;
	if (!status)
		return 0;
	report_info("git storage: local cache dir %s modified, git status 0x%04x", path, status);
	if (info->is_subsurface_cloud)
		report_error(translate("gettextFromC", "Local cache directory %s corrupted - can't sync with Subsurface cloud storage"), path);
	else
		report_error("WARNING: Git cache directory modified (path %s) status 0x%04x", path, status);
	return 1;
}

/*
 * The remote is strictly newer than the local branch.
 */
static int reset_to_remote(struct git_info *info, git_reference *local, const git_oid *new_id)
{
	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
	opts.progress_cb = &progress_cb;
	git_object *target;

	if (verbose)
		report_info("git storage: reset to remote\n");

	/* If it's not checked out (bare or not HEAD), just update the reference */
	if (git_repository_is_bare(info->repo) || git_branch_is_head(local) != 1) {
		git_reference *out;

		if (git_reference_set_target(&out, local, new_id, "Update to remote")) {
			report_info("git storage: could not update local cache to newer remote data");
			return report_error("%s", translate("gettextFromC", "Could not update local cache to newer remote data"));
		}
		git_reference_free(out);

#ifdef DEBUG
		// Not really an error, just informational
		report_error("Updated local branch from remote");
#endif
		return 0;
	}

	if (git_object_lookup(&target, info->repo, new_id, GIT_OBJ_COMMIT)) {
		report_info("git storage: could not look up remote commit");
		if (info->is_subsurface_cloud)
			return report_error("%s", translate("gettextFromC", "Subsurface cloud storage corrupted"));
		else
			return report_error("Could not look up remote commit");
	}
	opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	if (git_reset(info->repo, target, GIT_RESET_HARD, &opts)) {
		report_info("git storage: local head checkout failed after update");
		if (info->is_subsurface_cloud)
			return report_error("%s", translate("gettextFromC", "Could not update local cache to newer remote data"));
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
		report_info("git storage: authentication to cloud storage failed");
		report_error("Authentication to cloud storage failed.");
		return true;
	}
	return false;
}

extern "C" int credential_ssh_cb(git_cred **out,
		  const char *,
		  const char *,
		  unsigned int allowed_types,
		  void *)
{
	const char *username = prefs.cloud_storage_email_encoded;
	const char *passphrase = prefs.cloud_storage_password ? prefs.cloud_storage_password : "";

	// TODO: We need a way to differentiate between password and private key authentication
	if (allowed_types & GIT_CREDTYPE_SSH_KEY) {
		std::string priv_key = std::string(system_default_directory()) + "/ssrf_remote.key";
		if (!access(priv_key.c_str(), F_OK)) {
			if (exceeded_auth_attempts())
				return GIT_EUSER;
			return git_cred_ssh_key_new(out, username, NULL, priv_key.c_str(), passphrase);
		}
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

extern "C" int credential_https_cb(git_cred **out,
			const char *,
			const char *,
			unsigned int,
			void *)
{
	if (exceeded_auth_attempts())
		return GIT_EUSER;

	const char *username = prefs.cloud_storage_email_encoded;
	const char *password = prefs.cloud_storage_password ? prefs.cloud_storage_password : "";

	return git_cred_userpass_plaintext_new(out, username, password);
}

extern "C" int certificate_check_cb(git_cert *cert, int valid, const char *host, void *)
{
	if (verbose)
		report_info("git storage: certificate callback for host %s with validity %d\n", host, valid);
	if ((same_string(host, CLOUD_HOST_GENERIC) ||
	     same_string(host, CLOUD_HOST_US) ||
	     same_string(host, CLOUD_HOST_U2) ||
	     same_string(host, CLOUD_HOST_EU) ||
	     same_string(host, CLOUD_HOST_E2)) &&
			cert->cert_type == GIT_CERT_X509) {
		// for some reason the LetsEncrypt certificate makes libgit2 throw up on some
		// platforms but not on others
		// if we are connecting to the cloud server we alrady called 'canReachCloudServer()'
		// which will fail if the SSL certificate isn't valid, so let's simply always
		// tell the caller that this certificate is valid
		return 0;
	}
	return valid ? 0 : -1;
}

static int update_remote(struct git_info *info, git_remote *origin, git_reference *local, git_reference *)
{
	git_push_options opts = GIT_PUSH_OPTIONS_INIT;
	git_strarray refspec;
	const char *name = git_reference_name(local);

	if (verbose)
		report_info("git storage: update remote\n");

	refspec.count = 1;
	refspec.strings = (char **)&name;

	auth_attempt = 0;
	opts.callbacks.push_transfer_progress = &push_transfer_progress_cb;
	if (info->transport == RT_SSH)
		opts.callbacks.credentials = credential_ssh_cb;
	else if (info->transport == RT_HTTPS)
		opts.callbacks.credentials = credential_https_cb;
	opts.callbacks.certificate_check = certificate_check_cb;

	if (git_remote_push(origin, &refspec, &opts)) {
		const char *msg = giterr_last()->message;
		report_info("git storage: unable to update remote with current local cache state, error: %s", msg);
		if (info->is_subsurface_cloud)
			return report_error("%s", translate("gettextFromC", "Could not update Subsurface cloud storage, try again later"));
		else
			return report_error("Unable to update remote with current local cache state (%s)", msg);
	}
	return 0;
}

extern "C" int update_git_checkout(git_repository *repo, git_object *parent, git_tree *tree);

static int try_to_git_merge(struct git_info *info, git_reference **local_p, git_reference *, git_oid *base, const git_oid *local_id, const git_oid *remote_id)
{
	git_tree *local_tree, *remote_tree, *base_tree;
	git_commit *local_commit, *remote_commit, *base_commit;
	git_index *merged_index;
	git_merge_options merge_options;
	struct membufferpp msg;

	if (verbose) {
		char outlocal[41], outremote[41];
		outlocal[40] = outremote[40] = 0;
		git_oid_fmt(outlocal, local_id);
		git_oid_fmt(outremote, remote_id);
		report_info("git storage: trying to merge local SHA %s remote SHA %s\n", outlocal, outremote);
	}

	git_merge_init_options(&merge_options, GIT_MERGE_OPTIONS_VERSION);
	merge_options.flags = GIT_MERGE_FIND_RENAMES;
	merge_options.file_favor = GIT_MERGE_FILE_FAVOR_UNION;
	merge_options.rename_threshold = 100;
	if (git_commit_lookup(&local_commit, info->repo, local_id)) {
		report_info("git storage: remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&local_tree, local_commit)) {
		report_info("git storage: remote storage and local data diverged. Error: failed local tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_lookup(&remote_commit, info->repo, remote_id)) {
		report_info("git storage: remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&remote_tree, remote_commit)) {
		report_info("git storage: remote storage and local data diverged. Error: failed local tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_lookup(&base_commit, info->repo, base)) {
		report_info("git storage: remote storage and local data diverged. Error: can't get commit (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_commit_tree(&base_tree, base_commit)) {
		report_info("git storage: remote storage and local data diverged. Error: failed base tree lookup (%s)", giterr_last()->message);
		goto diverged_error;
	}
	if (git_merge_trees(&merged_index, info->repo, base_tree, local_tree, remote_tree, &merge_options)) {
		report_info("git storage: remote storage and local data diverged. Error: merge failed (%s)", giterr_last()->message);
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
			report_info("git storage: conflict in %s / %s / %s -- ",
				ours ? ours->path : "-",
				theirs ? theirs->path : "-",
				ancestor ? ancestor->path : "-");
			if ((!ours && theirs && ancestor) ||
			    (ours && !theirs && ancestor)) {
				// the file was removed on one side or the other - just remove it
				report_info("git storage: looks like a delete on one side; removing the file from the index\n");
				error = git_index_remove(merged_index, ours ? ours->path : theirs->path, GIT_INDEX_STAGE_ANY);
			} else if (ancestor) {
				error = git_index_conflict_remove(merged_index, ours ? ours->path : theirs ? theirs->path : ancestor->path);
			}
			if (error) {
				report_info("git storage: error at conflict resolution (%s)", giterr_last()->message);
			}
		}
		git_index_conflict_cleanup(merged_index);
		git_index_conflict_iterator_free(iter);
		report_error("%s", translate("gettextFromC", "Remote storage and local data diverged. Cannot combine local and remote changes"));
	}
	{
		git_oid merge_oid, commit_oid;
		git_tree *merged_tree;
		git_signature *author;
		git_commit *commit;

		if (git_index_write_tree_to(&merge_oid, merged_index, info->repo))
			goto write_error;
		if (git_tree_lookup(&merged_tree, info->repo, &merge_oid))
			goto write_error;
		if (get_authorship(info->repo, &author) < 0)
			goto write_error;
		std::string user_agent = subsurface_user_agent();
		put_format(&msg, "Automatic merge\n\nCreated by %s\n", user_agent.c_str());
		if (git_commit_create_v(&commit_oid, info->repo, NULL, author, author, NULL, mb_cstring(&msg), merged_tree, 2, local_commit, remote_commit))
			goto write_error;
		if (git_commit_lookup(&commit, info->repo, &commit_oid))
			goto write_error;
		if (git_branch_is_head(*local_p) && !git_repository_is_bare(info->repo)) {
			git_object *parent;
			git_reference_peel(&parent, *local_p, GIT_OBJ_COMMIT);
			if (update_git_checkout(info->repo, parent, merged_tree)) {
				goto write_error;
			}
		}
		if (git_reference_set_target(local_p, *local_p, &commit_oid, "Subsurface merge event"))
			goto write_error;
		set_git_id(&commit_oid);
		git_signature_free(author);
		if (verbose)
			report_info("git storage: successfully merged repositories");
		return 0;
	}

diverged_error:
	return report_error("%s", translate("gettextFromC", "Remote storage and local data diverged"));

write_error:
	return report_error(translate("gettextFromC", "Remote storage and local data diverged. Error: writing the data failed (%s)"), giterr_last()->message);
}

// if accessing the local cache of Subsurface cloud storage fails, we simplify things
// for the user and simply move the cache away (in case they want to try and extract data)
// and ask them to retry the operation (which will then refresh the data from the cloud server)
static int cleanup_local_cache(struct git_info *info)
{
	std::string backup_path = move_local_cache(info);
	report_info("git storage: problems with local cache, moved to %s", backup_path.c_str());
	report_error("%s", translate("gettextFromC", "Problems with local cache of Subsurface cloud data"));
	report_error(translate("gettextFromC", "Moved cache data to %s. Please try the operation again."), backup_path.c_str());
	return -1;
}

static int try_to_update(struct git_info *info, git_remote *origin, git_reference *local, git_reference *remote)
{
	git_oid base;
	const git_oid *local_id, *remote_id;
	int ret = 0;

	if (verbose)
		report_info("git storage: try to update\n");

	if (!git_reference_cmp(local, remote))
		return 0;

	// Dirty modified state in the working tree? We're not going
	// to update either way
	if (git_status_foreach(info->repo, check_clean, (void *)info)) {
		report_info("git storage: local cache is dirty, skipping update");
		if (info->is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("local cached copy is dirty, skipping update");
	}
	local_id = git_reference_target(local);
	remote_id = git_reference_target(remote);

	if (!local_id || !remote_id) {
		if (!local_id)
			report_info("git storage: unable to get local SHA");
		if (!remote_id)
			report_info("git storage: unable to get remote SHA");
		if (info->is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Unable to get local or remote SHA1");
	}
	if (git_merge_base(&base, info->repo, local_id, remote_id)) {
		// TODO:
		// if they have no merge base, they actually are different repos
		// so instead merge this as merging a commit into a repo - git_merge() appears to do that
		// but needs testing and cleanup afterwards
		//
		report_info("git storage: no common commit between local and remote branches");
		if (info->is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Unable to find common commit of local and remote branches");
	}
	/* Is the remote strictly newer? Use it */
	if (git_oid_equal(&base, local_id)) {
		if (verbose)
			report_info("git storage: remote is newer than local, update local");
		git_storage_update_progress(translate("gettextFromC", "Update local storage to match cloud storage"));
		return reset_to_remote(info, local, remote_id);
	}

	/* Is the local repo the more recent one? See if we can update upstream */
	if (git_oid_equal(&base, remote_id)) {
		if (verbose)
			report_info("git storage: local is newer than remote, update remote");
		git_storage_update_progress(translate("gettextFromC", "Push local changes to cloud storage"));
		return update_remote(info, origin, local, remote);
	}
	/* Merging a bare repository always needs user action */
	if (git_repository_is_bare(info->repo)) {
		report_info("git storage: local is bare and has diverged from remote; user action needed");
		if (info->is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Local and remote have diverged, merge of bare branch needed");
	}
	/* Merging will definitely need the head branch too */
	if (git_branch_is_head(local) != 1) {
		report_info("git storage: local branch is not HEAD, cannot merge");
		if (info->is_subsurface_cloud)
			goto cloud_data_error;
		else
			return report_error("Local and remote do not match, local branch not HEAD - cannot update");
	}
	/* Ok, let's try to merge these */
	git_storage_update_progress(translate("gettextFromC", "Try to merge local changes into cloud storage"));
	ret = try_to_git_merge(info, &local, remote, &base, local_id, remote_id);
	if (ret == 0)
		return update_remote(info, origin, local, remote);
	else
		return ret;

cloud_data_error:
	// since we are working with Subsurface cloud storage we want to make the user interaction
	// as painless as possible. So if something went wrong with the local cache, tell the user
	// about it an move it away
	return cleanup_local_cache(info);
}

static int check_remote_status(struct git_info *info, git_remote *origin)
{
	int error = 0;

	git_reference *local_ref, *remote_ref;

	if (verbose)
		report_info("git storage: check remote status\n");

	if (git_branch_lookup(&local_ref, info->repo, info->branch.c_str(), GIT_BRANCH_LOCAL)) {
		report_info("git storage: branch %s is missing in local repo", info->branch.c_str());
		if (info->is_subsurface_cloud)
			return cleanup_local_cache(info);
		else
			return report_error("Git cache branch %s no longer exists", info->branch.c_str());
	}
	if (git_branch_upstream(&remote_ref, local_ref)) {
		/* so there is no upstream branch for our branch; that's a problem.
		 * let's push our branch */
		report_info("git storage: branch %s is missing in remote, pushing branch", info->branch.c_str());
		git_strarray refspec;
		git_reference_list(&refspec, info->repo);
		git_push_options opts = GIT_PUSH_OPTIONS_INIT;
		opts.callbacks.transfer_progress = &transfer_progress_cb;
		auth_attempt = 0;
		if (info->transport == RT_SSH)
			opts.callbacks.credentials = credential_ssh_cb;
		else if (info->transport == RT_HTTPS)
			opts.callbacks.credentials = credential_https_cb;
		opts.callbacks.certificate_check = certificate_check_cb;
		git_storage_update_progress(translate("gettextFromC", "Store data into cloud storage"));
		error = git_remote_push(origin, &refspec, &opts);
	} else {
		error = try_to_update(info, origin, local_ref, remote_ref);
		git_reference_free(remote_ref);
	}
	git_reference_free(local_ref);
	git_remote_sync_successful = (error == 0);
	return error;
}

static std::string getProxyString()
{
	if (prefs.proxy_type == QNetworkProxy::HttpProxy) {
		if (prefs.proxy_auth)
			return format_string_std("http://%s:%s@%s:%d", prefs.proxy_user, prefs.proxy_pass,
					prefs.proxy_host, prefs.proxy_port);
		else
			return format_string_std("http://%s:%d", prefs.proxy_host, prefs.proxy_port);
	}
	return std::string();
}

/* this is (so far) only used by the git storage tests to remove a remote branch
 * it will print out errors, but not return an error (as this isn't a function that
 * we test as part of the tests, it's a helper to not leave loads of dead branches on
 * the server)
 */
void delete_remote_branch(git_repository *repo, const std::string &remote, const std::string &branch)
{
	git_remote *origin;
	git_config *conf;

	/* set up the config and proxy information in order to connect to the server */
	git_repository_config(&conf, repo);
	std::string proxy_string = getProxyString();
	if (!proxy_string.empty()) {
		git_config_set_string(conf, "http.proxy", proxy_string.c_str());
	} else {
		git_config_delete_entry(conf, "http.proxy");
	}
	if (git_remote_lookup(&origin, repo, "origin")) {
		report_info("git storage: repository '%s' origin lookup failed (%s)", remote.c_str(), giterr_last() ? giterr_last()->message : "(unspecified)");
		return;
	}
	/* fetch the remote state */
	git_fetch_options f_opts = GIT_FETCH_OPTIONS_INIT;
	auth_attempt = 0;
	f_opts.callbacks.credentials = credential_https_cb;
	if (git_remote_fetch(origin, NULL, &f_opts, NULL)) {
		report_info("git storage: remote fetch failed (%s)\n", giterr_last() ? giterr_last()->message : "authentication failed");
		return;
	}
	/* delete the remote branch by pushing to ":refs/heads/<branch>" */
	git_strarray refspec;
	std::string branch_ref = std::string(":refs/heads/") + branch;
	char *dummy = branch_ref.data();
	refspec.count = 1;
	refspec.strings = &dummy;
	git_push_options p_opts = GIT_PUSH_OPTIONS_INIT;
	auth_attempt = 0;
	p_opts.callbacks.credentials = credential_https_cb;
	if (git_remote_push(origin, &refspec, &p_opts)) {
		report_info("git storage: unable to delete branch '%s'", branch.c_str());
		report_info("git storage: error was (%s)\n", giterr_last() ? giterr_last()->message : "(unspecified)");
	}
	git_remote_free(origin);
	return;
}

int sync_with_remote(struct git_info *info)
{
	int error;
	git_remote *origin;
	git_config *conf;

	if (git_local_only) {
		if (verbose)
			report_info("git storage: don't sync with remote - read from cache only\n");
		return 0;
	}
	if (verbose)
		report_info("git storage: sync with remote %s[%s]\n", info->url.c_str(), info->branch.c_str());
	git_storage_update_progress(translate("gettextFromC", "Sync with cloud storage"));
	git_repository_config(&conf, info->repo);
	std::string proxy_string = getProxyString();
	if (info->transport == RT_HTTPS && !proxy_string.empty()) {
		if (verbose)
			report_info("git storage: set proxy to \"%s\"\n", proxy_string.c_str());
		git_config_set_string(conf, "http.proxy", proxy_string.c_str());
	} else {
		if (verbose)
			report_info("git storage: delete proxy setting\n");
		git_config_delete_entry(conf, "http.proxy");
	}

	/*
	 * NOTE! Remote errors are reported, but are nonfatal:
	 * we still successfully return the local repository.
	 */
	error = git_remote_lookup(&origin, info->repo, "origin");
	if (error) {
		const char *msg = giterr_last()->message;
		report_info("git storage: repo %s origin lookup failed with: %s", info->url.c_str(), msg);
		if (!info->is_subsurface_cloud)
			report_error("Repository '%s' origin lookup failed (%s)", info->url.c_str(), msg);
		return 0;
	}

	// we know that we already checked for the cloud server, but to give a decent warning message
	// here in case none of them are reachable, let's check one more time
	if (info->is_subsurface_cloud && !canReachCloudServer(info)) {
		// this is not an error, just a warning message, so return 0
		report_info("git storage: cannot connect to remote server");
		report_error("Cannot connect to cloud server, working with local copy");
		git_storage_update_progress(translate("gettextFromC", "Can't reach cloud server, working with local data"));
		return 0;
	}

	if (verbose)
		report_info("git storage: fetch remote %s\n", git_remote_url(origin));
	git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
	opts.callbacks.transfer_progress = &transfer_progress_cb;
	auth_attempt = 0;
	if (info->transport == RT_SSH)
		opts.callbacks.credentials = credential_ssh_cb;
	else if (info->transport == RT_HTTPS)
		opts.callbacks.credentials = credential_https_cb;
	opts.callbacks.certificate_check = certificate_check_cb;
	git_storage_update_progress(translate("gettextFromC", "Successful cloud connection, fetch remote"));
	error = git_remote_fetch(origin, NULL, &opts, NULL);
	// NOTE! A fetch error is not fatal, we just report it
	if (error) {
		if (info->is_subsurface_cloud)
			report_error("Cannot sync with cloud server, working with offline copy");
		else
			report_error("Unable to fetch remote '%s'", info->url.c_str());
		// If we returned GIT_EUSER during authentication, giterr_last() returns NULL
		report_info("git storage: remote fetch failed (%s)\n", giterr_last() ? giterr_last()->message : "authentication failed");
		// Since we failed to sync with online repository, enter offline mode
		git_local_only = true;
		error = 0;
	} else {
		error = check_remote_status(info, origin);
	}
	git_remote_free(origin);
	git_storage_update_progress(translate("gettextFromC", "Done syncing with cloud storage"));
	return error;
}

static bool update_local_repo(struct git_info *info)
{
	git_reference *head;

	/* Check the HEAD being the right branch */
	if (!git_repository_head(&head, info->repo)) {
		const char *name;
		if (!git_branch_name(&name, head)) {
			if (info->branch != name) {
				std::string branchref = "refs/heads/" + info->branch;
				report_info("git storage: setting cache branch from '%s' to '%s'", name, info->branch.c_str());
				git_repository_set_head(info->repo, branchref.c_str());
			}
		}
		git_reference_free(head);
	}
	/* make sure we have the correct origin - the cloud server URL could have changed */
	if (git_remote_set_url(info->repo, "origin", info->url.c_str())) {
		report_info("git storage: failed to update origin to '%s'", info->url.c_str());
		return false;
	}

	if (!git_local_only)
		sync_with_remote(info);

	return true;
}

static int repository_create_cb(git_repository **out, const char *path, int bare, void *)
{
	git_config *conf;

	int ret = git_repository_init(out, path, bare);
	if (ret != 0) {
		if (verbose)
			report_info("git storage: initializing git repository failed\n");
		return ret;
	}

	git_repository_config(&conf, *out);
	std::string proxy_string = getProxyString();
	if (!proxy_string.empty()) {
		if (verbose)
			report_info("git storage: set proxy to \"%s\"\n", proxy_string.c_str());
		git_config_set_string(conf, "http.proxy", proxy_string.c_str());
	} else {
		if (verbose)
			report_info("git storage: delete proxy setting\n");
		git_config_delete_entry(conf, "http.proxy");
	}
	return ret;
}

/* this should correctly initialize both the local and remote
 * repository for the Subsurface cloud storage */
static bool create_and_push_remote(struct git_info *info)
{
	git_config *conf;

	if (verbose)
		report_info("git storage: create and push remote\n");

	/* first make sure the directory for the local cache exists */
	subsurface_mkdir(info->localdir.c_str());

	std::string head = "refs/heads/" + info->branch;

	/* set up the origin to point to our remote */
	git_repository_init_options init_opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
	init_opts.origin_url = info->url.c_str();
	init_opts.initial_head = head.c_str();

	/* now initialize the repository with */
	git_repository_init_ext(&info->repo, info->localdir.c_str(), &init_opts);

	/* create a config so we can set the remote tracking branch */
	git_repository_config(&conf, info->repo);
	std::string variable_name = "branch." + info->branch + ".remote";
	git_config_set_string(conf, variable_name.c_str(), "origin");

	variable_name = "branch." + info->branch + ".merge";
	git_config_set_string(conf, variable_name.c_str(), head.c_str());

	/* finally create an empty commit and push it to the remote */
	if (do_git_save(info, false, true))
		return false;

	return true;
}

static bool create_local_repo(struct git_info *info)
{
	int error;
	git_clone_options opts = GIT_CLONE_OPTIONS_INIT;

	if (verbose)
		report_info("git storage: create_local_repo\n");

	auth_attempt = 0;
	opts.fetch_opts.callbacks.transfer_progress = &transfer_progress_cb;
	if (info->transport == RT_SSH)
		opts.fetch_opts.callbacks.credentials = credential_ssh_cb;
	else if (info->transport == RT_HTTPS)
		opts.fetch_opts.callbacks.credentials = credential_https_cb;
	opts.repository_cb = repository_create_cb;
	opts.fetch_opts.callbacks.certificate_check = certificate_check_cb;

	opts.checkout_branch = info->branch.c_str();
	if (info->is_subsurface_cloud && !canReachCloudServer(info)) {
		report_info("git storage: cannot reach remote server");
		return false;
	}
	if (verbose > 1)
		report_info("git storage: calling git_clone()\n");
	error = git_clone(&info->repo, info->url.c_str(), info->localdir.c_str(), &opts);
	if (verbose > 1)
		report_info("git storage: returned from git_clone() with return value %d\n", error);
	if (error) {
		report_info("git storage: clone of %s failed", info->url.c_str());
		const char *msg = "";
		if (giterr_last()) {
			 msg = giterr_last()->message;
			 report_info("git storage: error message was %s\n", msg);
		} else {
			 report_info("git storage: giterr_last() is null\n");
		}
		std::string pattern = format_string_std("reference 'refs/remotes/origin/%s' not found", info->branch.c_str());
		// it seems that we sometimes get 'Reference' and sometimes 'reference'
		if (includes_string_caseinsensitive(msg, pattern.c_str())) {
			/* we're trying to open the remote branch that corresponds
			 * to our cloud storage and the branch doesn't exist.
			 * So we need to create the branch and push it to the remote */
			if (verbose)
				report_info("git storage: remote repo didn't include our branch\n");
			if (create_and_push_remote(info))
				error = 0;
#if !defined(DEBUG) && !defined(SUBSURFACE_MOBILE)
		} else if (info->is_subsurface_cloud) {
			report_error("%s", translate("gettextFromC", "Error connecting to Subsurface cloud storage"));
#endif
		} else {
			report_error(translate("gettextFromC", "git clone of %s failed (%s)"), info->url.c_str(), msg);
		}
	}
	return !error;
}

static enum remote_transport url_to_remote_transport(const std::string &remote)
{
	/* figure out the remote transport */
	if (starts_with(remote, "ssh://"))
		return RT_SSH;
	else if (starts_with(remote.c_str(), "https://"))
		return RT_HTTPS;
	else
		return RT_OTHER;
}

static bool get_remote_repo(struct git_info *info)
{
	struct stat st;

	if (verbose > 1) {
		report_info("git storage: accessing %s\n", info->url.c_str());
	}
	git_storage_update_progress(translate("gettextFromC", "Synchronising data file"));
	/* Do we already have a local cache? */
	if (!subsurface_stat(info->localdir.c_str(), &st)) {
		int error;

		if (verbose)
			report_info("git storage: update local repo\n");

		error = git_repository_open(&info->repo, info->localdir.c_str());
		if (error) {
			const char *msg = giterr_last()->message;
			report_info("git storage: unable to open local cache at %s: %s", info->localdir.c_str(), msg);
			if (info->is_subsurface_cloud)
				(void)cleanup_local_cache(info);
			else
				report_error("Unable to open git cache repository at %s: %s", info->localdir.c_str(), msg);
			return false;
		}

		return update_local_repo(info);
	} else {
		/* We have no local cache yet.
		 * Take us temporarly online to create a local and
		 * remote cloud repo.
		 */
		bool ret;
		bool glo = git_local_only;
		git_local_only = false;
		ret = create_local_repo(info);
		git_local_only = glo;
		if (ret)
			git_remote_sync_successful = true;
		return ret;
	}

	/* all normal cases are handled above */
	return false;
}

/*
 * Remove the user name from the url if it exists, and
 * save it in 'info->username'.
 */
std::string extract_username(struct git_info *info, const std::string &url)
{
	char c;
	const char *p = url.c_str();

	while ((c = *p++) >= 'a' && c <= 'z')
		/* nothing */;
	if (c != ':')
		return url;
	if (*p++ != '/' || *p++ != '/')
		return url;

	/*
	 * Ok, we found "[a-z]*://" and we think we have a real
	 * "remote git" format. The "file://" case was handled
	 * in the calling function.
	 */
	info->transport = url_to_remote_transport(url);

	const char *at = strchr(p, '@');
	if (!at)
		return url;

	/* was this the @ that denotes an account? that means it was before the
	 * first '/' after the protocol:// - so let's find a '/' after that and compare */
	const char *slash = strchr(p, '/');
	if (!slash || at > slash)
		return url;

	/* grab the part between "protocol://" and "@" as encoded email address
	 * (that's our username) and move the rest of the URL forward, remembering
	 * to copy the closing NUL as well */
	info->username = std::string(p, at - p);

	/*
	 * Ugly, ugly. Parsing the remote repo user name also sets
	 * it in the preferences. We should do this somewhere else!
	 */
	prefs.cloud_storage_email_encoded = strdup(info->username.c_str());

	return url.substr(at + 1 - url.c_str());
}

git_info::git_info() : repo(nullptr), is_subsurface_cloud(0), transport(RT_LOCAL)
{
}

git_info::~git_info()
{
	if (repo)
		git_repository_free(repo);
}

/*
 * If it's not a git repo, return NULL. Be very conservative.
 *
 * The recognized formats are
 *    git://host/repo[branch]
 *    ssh://host/repo[branch]
 *    http://host/repo[branch]
 *    https://host/repo[branch]
 *    file://repo[branch]
 */
bool is_git_repository(const char *filename, struct git_info *info)
{
	int flen, blen;
	int offset = 1;

	/* we are looking at a new potential remote, but we haven't synced with it */
	git_remote_sync_successful = false;

	info->transport = RT_LOCAL;
	flen = strlen(filename);
	if (!flen || filename[--flen] != ']')
		return false;

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
		return false;

	/*
	 * This is the "point of no return": the name matches
	 * the git repository name rules, and we will no longer
	 * return NULL.
	 *
	 * We will either return with NULL git repo and the
	 * branch pointer will have the _whole_ filename in it,
	 * or we will return a real git repository with the
	 * branch pointer being filled in with just the branch
	 * name.
	 *
	 * The actual git reading/writing routines can use this
	 * to generate proper error messages.
	 */
	std::string url(filename, flen);
	std::string branch(filename + flen + offset, blen);

	/* Extract the username from the url string */
	url = extract_username(info, url);

	info->url = url;
	info->branch = branch;

	/*
	 * We now create the SHA1 hash of the whole thing,
	 * including the branch name. That will be our unique
	 * local repository name.
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
	switch (info->transport) {
	case RT_LOCAL:
		info->localdir = url;
		break;
	default:
		info->localdir = get_local_dir(info->url.c_str(), info->branch).c_str();
		break;
	}

	/*
	 * Remember if the current git storage we are working on is our
	 * cloud storage.
	 *
	 * This is used to create more user friendly error message and warnings.
	 */
	info->is_subsurface_cloud = (strstr(info->url.c_str(), prefs.cloud_base_url) != NULL);

	return true;
}

bool open_git_repository(struct git_info *info)
{
	/*
	 * If the repository is local, just open it. There's nothing
	 * else to do.
	 */
	if (info->transport == RT_LOCAL) {
		const char *url = info->localdir.c_str();

		if (git_repository_open(&info->repo, url)) {
			if (verbose)
				report_info("git storage: loc %s couldn't be opened (%s)\n", url, giterr_last()->message);
			return false;
		}
		return true;
	}

	/* if we are planning to access the server, make sure it's available and try to
	 * pick one of the alternative servers if necessary */
	if (info->is_subsurface_cloud && !git_local_only) {
		// since we know that this is Subsurface cloud storage, we don't have to
		// worry about the local directory name changing if we end up with a different
		// cloud_base_url... the algorithm normalizes those URLs
		(void)canReachCloudServer(info);
	}
	return get_remote_repo(info);
}

int git_create_local_repo(const std::string &filename)
{
	git_repository *repo;

	auto idx = filename.find('[');
	std::string path = filename.substr(0, idx);
	int ret = git_repository_init(&repo, path.c_str(), false);
	if (ret != 0)
		(void)report_error("Create local repo failed with error code %d", ret);
	git_repository_free(repo);
	return ret;
}
