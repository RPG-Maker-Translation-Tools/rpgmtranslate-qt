#pragma once

#ifdef ENABLE_LIBGIT2
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <fstream>
#include <git2.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ezgit2 {
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = size_t;
using isize = ptrdiff_t;
using f32 = float;
using f64 = double;

using std::array;
using std::expected;
using std::nullopt;
using std::optional;
using std::string;
using std::string_view;
using std::unexpected;
using std::unique_ptr;
using std::vector;

template <typename T>
using Err = unexpected<T>;

struct Error {
    string message;
    i32 code;
};

template <typename Val>
using Result = expected<Val, Error>;

[[nodiscard]] inline auto makeError(const i32 code) -> Error {
    const git_error* const err = git_error_last();
    return Error{
        .message = (err != nullptr) ? string(err->message) : "unknown error",
        .code = code,
    };
}

struct CommitInfo {
    array<char, GIT_OID_HEXSZ + 1> oid;
    string message;
    string authorName;
    string authorEmail;
    i64 timestamp;
};

struct StashEntry {
    array<char, GIT_OID_HEXSZ + 1> oid;
    string message;
    usize index;
};

struct RemoteInfo {
    string name;
    string url;
    string pushUrl;
};

struct TagInfo {
    array<char, GIT_OID_HEXSZ + 1> oid;
    string name;
    string message;
    bool annotated;
};

struct DiffLine {
    string content;
    i32 oldLineno;
    i32 newLineno;
    char origin;
};

struct DiffHunk {
    string header;
    vector<DiffLine> lines;
};

struct FileDiff {
    string oldPath;
    string newPath;
    vector<DiffHunk> hunks;
};

struct BlameLine {
    array<char, GIT_OID_HEXSZ + 1> commitOid;
    string authorName;
    string authorEmail;
    string content;
    i64 timestamp;
    usize lineNo;
};

namespace detail {
template <auto DeleteFn>
struct HandleDeleter {
    template <typename T>
    auto operator()(T* const ptr) const noexcept -> void {
        if (ptr != nullptr) {
            DeleteFn(ptr);
        }
    }
};

template <typename T, auto DeleteFn>
using GitHandle = unique_ptr<T, HandleDeleter<DeleteFn>>;

using RepoHandle = GitHandle<git_repository, git_repository_free>;
using IndexHandle = GitHandle<git_index, git_index_free>;
using RevwalkHandle = GitHandle<git_revwalk, git_revwalk_free>;
using CommitHandle = GitHandle<git_commit, git_commit_free>;
using RemoteHandle = GitHandle<git_remote, git_remote_free>;
using TreeHandle = GitHandle<git_tree, git_tree_free>;
using DiffHandle = GitHandle<git_diff, git_diff_free>;
using RefHandle = GitHandle<git_reference, git_reference_free>;
using ObjHandle = GitHandle<git_object, git_object_free>;
using SigHandle = GitHandle<git_signature, git_signature_free>;
using AnnotatedHandle = GitHandle<git_annotated_commit, git_annotated_commit_free>;
using BranchIterHandle = GitHandle<git_branch_iterator, git_branch_iterator_free>;
using StatusListHandle = GitHandle<git_status_list, git_status_list_free>;
using ConfigHandle = GitHandle<git_config, git_config_free>;
using BlameHandle = GitHandle<git_blame, git_blame_free>;
using TagHandle = GitHandle<git_tag, git_tag_free>;

class StrArray final {
   public:
    explicit StrArray(const vector<string>& paths) {
        ptrs_.reserve(paths.size());

        for (const auto& path : paths) {
            ptrs_.push_back(path.c_str());
        }

        arr_.strings = const_cast<char**>(ptrs_.data());
        arr_.count = ptrs_.size();
    }

    [[nodiscard]] auto get() -> git_strarray* { return &arr_; }

   private:
    vector<const char*> ptrs_;
    git_strarray arr_ = {};
};

// RAII wrapper for a git_strarray *filled by* libgit2 (e.g. git_remote_list),
// as opposed to StrArray above, which wraps one we build to hand in.
class OwnedStrArray final {
   public:
    OwnedStrArray() = default;

    ~OwnedStrArray() { git_strarray_dispose(&arr_); }

    OwnedStrArray(const OwnedStrArray&) = delete;
    auto operator=(const OwnedStrArray&) -> OwnedStrArray& = delete;
    OwnedStrArray(OwnedStrArray&&) = delete;
    auto operator=(OwnedStrArray&&) -> OwnedStrArray& = delete;

    [[nodiscard]] auto get() -> git_strarray* { return &arr_; }

   private:
    git_strarray arr_ = {};
};
}  // namespace detail

class Repository final {
   public:
    Repository() { git_libgit2_init(); }

    ~Repository() { git_libgit2_shutdown(); }

    Repository(Repository&&) = default;
    auto operator=(Repository&&) -> Repository& = default;
    Repository(const Repository&) = delete;
    auto operator=(const Repository&) -> Repository& = delete;

    void close() { repo.reset(); }

    [[nodiscard]] auto setUserData(const string_view name = "", const string_view email = "") -> Result<void> {
        if (name.empty() && email.empty()) {
            git_signature* rawSig = nullptr;
            const i32 ret = git_signature_default(&rawSig, repo.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            const detail::SigHandle sig(rawSig);
            username = sig->name;
            userEmail = sig->email;
        } else {
            username = name;
            userEmail = email;
        }

        return {};
    }

    void userData(string_view& name, string_view& email) const {
        name = username;
        email = userEmail;
    }

    [[nodiscard]] auto open(const string_view path) -> Result<void> {
        git_repository* raw = nullptr;
        const i32 ret = git_repository_open(&raw, path.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        repo = detail::RepoHandle(raw);
        return {};
    }

    [[nodiscard]] auto init(const string_view path) -> Result<void> {
        git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
        opts.initial_head = "main";

        git_repository* raw = nullptr;
        const i32 ret = git_repository_init_ext(&raw, path.data(), &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        repo = detail::RepoHandle(raw);
        return {};
    }

    [[nodiscard]] auto cloneFrom(const string_view url, const string_view localPath) -> Result<void> {
        git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
        opts.fetch_opts.callbacks.credentials = &Repository::credentialsTrampoline;
        opts.fetch_opts.callbacks.transfer_progress = &Repository::transferProgressTrampoline;
        opts.fetch_opts.callbacks.payload = this;
        opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        opts.checkout_opts.progress_cb = checkoutProgressCallback;
        opts.checkout_opts.progress_payload = checkoutProgressPayload;

        git_repository* raw = nullptr;
        const i32 ret = git_clone(&raw, url.data(), localPath.data(), &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        repo = detail::RepoHandle(raw);
        return {};
    }

    auto setCredentialsCallback(const git_credential_acquire_cb callback, void* const payload = nullptr) -> void {
        credentialsCallback = callback;
        credentialsPayload = payload;
    }

    auto setTransferProgressCallback(const git_indexer_progress_cb callback, void* const payload = nullptr) -> void {
        transferProgressCallback = callback;
        transferProgressPayload = payload;
    }

    auto setPushProgressCallback(const git_push_transfer_progress_cb callback, void* const payload = nullptr) -> void {
        pushProgressCallback = callback;
        pushProgressPayload = payload;
    }

    auto setCheckoutProgressCallback(const git_checkout_progress_cb callback, void* const payload = nullptr) -> void {
        checkoutProgressCallback = callback;
        checkoutProgressPayload = payload;
    }

    [[nodiscard]] auto workdir() const -> const char* {
        const char* const workdir = git_repository_workdir(repo.get());
        return workdir;
    }

    [[nodiscard]] auto list(const git_status_t mask) const -> Result<vector<string>> {
        if (mask == 0) {  // "tracked" has no status flags — read straight
                          // from the index
            const auto indexResult = openIndex();
            if (!indexResult) {
                return Err(indexResult.error());
            }

            const auto& index = indexResult.value();
            const usize count = git_index_entrycount(index.get());

            vector<string> tracked;
            tracked.reserve(count);
            for (usize i = 0; i < count; i++) {
                const git_index_entry* const entry = git_index_get_byindex(index.get(), i);
                tracked.emplace_back(entry->path);
            }
            return tracked;
        }

        return collectStatusPaths(static_cast<u32>(mask));
    }

    struct FileStatus {
        string path;
        git_status_t status;
    };

    [[nodiscard]] auto listFiles(
        git_status_opt_t statusOptions =
            git_status_opt_t(GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS)
    ) const -> Result<vector<FileStatus>> {
        vector<FileStatus> results;

        git_status_options opts = GIT_STATUS_OPTIONS_INIT;
        opts.flags = statusOptions;

        git_status_list* rawList = nullptr;
        const i32 ret = git_status_list_new(&rawList, repo.get(), &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        const detail::StatusListHandle statusList(rawList);
        const usize count = git_status_list_entrycount(statusList.get());
        results.reserve(count);

        for (usize i = 0; i < count; i++) {
            const git_status_entry* const entry = git_status_byindex(statusList.get(), i);

            const char* const path = (entry->head_to_index != nullptr)      ? entry->head_to_index->old_file.path
                                     : (entry->index_to_workdir != nullptr) ? entry->index_to_workdir->old_file.path
                                                                            : nullptr;

            results.emplace_back(path, entry->status);
        }

        return results;
    }

    [[nodiscard]] auto commitHistory(const usize limit = 0) const -> Result<vector<CommitInfo>> {
        detail::RevwalkHandle walker;

        {
            git_revwalk* raw = nullptr;
            const i32 ret = git_revwalk_new(&raw, repo.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            walker = detail::RevwalkHandle(raw);
        }

        git_revwalk_sorting(walker.get(), GIT_SORT_TIME);

        {
            const i32 ret = git_revwalk_push_head(walker.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        vector<CommitInfo> commits;
        git_oid iterOid;
        usize count = 0;

        while (git_revwalk_next(&iterOid, walker.get()) == 0) {
            if (limit > 0 && count >= limit) {
                break;
            }

            git_commit* rawCommit = nullptr;
            const i32 ret = git_commit_lookup(&rawCommit, repo.get(), &iterOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            const detail::CommitHandle commitObj(rawCommit);

            array<char, GIT_OID_HEXSZ + 1> oidBuf;
            git_oid_tostr(oidBuf.data(), GIT_OID_HEXSZ + 1, &iterOid);

            const git_signature* const author = git_commit_author(commitObj.get());
            const char* const rawMsg = git_commit_message(commitObj.get());

            commits.push_back(
                CommitInfo{
                    .oid = oidBuf,
                    .message = (rawMsg != nullptr) ? string(rawMsg) : string(),
                    .authorName = (author != nullptr) ? string(author->name) : string(),
                    .authorEmail = (author != nullptr) ? string(author->email) : string(),
                    .timestamp = (author != nullptr) ? author->when.time : 0,
                }
            );

            count++;
        }

        return commits;
    }

    [[nodiscard]] auto pendingPushCommitsCount(
        const string_view remoteName = "origin",
        const string_view branchName = ""
    ) const -> Result<usize> {
        const auto localRef = getHeadRef();

        if (!localRef) {
            return Err(localRef.error());
        }

        const auto remoteRef = getRemoteTrackingRef(remoteName, branchName);

        if (!remoteRef) {
            return 0;
        }

        return countCommitsBetween(git_reference_target(remoteRef->get()), git_reference_target(localRef->get()));
    }

    [[nodiscard]] auto pendingPullCommitsCount(
        const string_view remoteName = "origin",
        const string_view branchName = ""
    ) const -> Result<usize> {
        const auto localRef = getHeadRef();

        if (!localRef) {
            return Err(localRef.error());
        }

        const auto remoteRef = getRemoteTrackingRef(remoteName, branchName);

        if (!remoteRef) {
            return 0;
        }

        return countCommitsBetween(git_reference_target(localRef->get()), git_reference_target(remoteRef->get()));
    }

    [[nodiscard]] auto stage(const vector<string>& paths) -> Result<void> {
        const auto idxResult = openIndex();

        if (!idxResult) {
            return Err(idxResult.error());
        }

        const auto& idx = idxResult.value();

        detail::StrArray arr(paths);
        const i32 ret = git_index_add_all(idx.get(), arr.get(), GIT_INDEX_ADD_DEFAULT, nullptr, nullptr);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        const i32 writeRet = git_index_write(idx.get());

        if (writeRet != 0) {
            return Err(makeError(writeRet));
        }

        return {};
    }

    [[nodiscard]] auto unstage(const vector<string>& paths) -> Result<void> {
        detail::ObjHandle headObj;

        {
            git_object* raw = nullptr;
            const i32 ret = git_revparse_single(&raw, repo.get(), "HEAD");

            if (ret != 0) {
                const auto idxResult = openIndex();

                if (!idxResult) {
                    return Err(idxResult.error());
                }

                const auto& idx = idxResult.value();
                detail::StrArray arr(paths);
                const i32 rmRet = git_index_remove_all(idx.get(), arr.get(), nullptr, nullptr);

                if (rmRet != 0) {
                    return Err(makeError(rmRet));
                }

                const i32 wrRet = git_index_write(idx.get());

                if (wrRet != 0) {
                    return Err(makeError(wrRet));
                }

                return {};
            }

            headObj = detail::ObjHandle(raw);
        }

        detail::StrArray arr(paths);
        const i32 ret = git_reset_default(repo.get(), headObj.get(), arr.get());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto commit(
        const string_view message,
        const vector<string>& stagePaths = {},
        const bool amend = false,
        const bool push = false
    ) -> Result<array<char, GIT_OID_HEXSZ + 1>> {
        if (!stagePaths.empty()) {
            const auto stageResult = stage(stagePaths);

            if (!stageResult) {
                return Err(stageResult.error());
            }
        }

        const auto idxResult = openIndex();

        if (!idxResult) {
            return Err(idxResult.error());
        }

        const auto& idx = idxResult.value();

        git_oid treeOid;

        {
            const i32 ret = git_index_write_tree(&treeOid, idx.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        detail::TreeHandle tree;

        {
            git_tree* raw = nullptr;
            const i32 ret = git_tree_lookup(&raw, repo.get(), &treeOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            tree = detail::TreeHandle(raw);
        }

        detail::CommitHandle headCommit;
        git_oid headOid;
        const bool hasHead = git_reference_name_to_id(&headOid, repo.get(), "HEAD") == 0;

        if (hasHead) {
            git_commit* raw = nullptr;
            const i32 ret = git_commit_lookup(&raw, repo.get(), &headOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            headCommit = detail::CommitHandle(raw);
        }

        if (amend && !hasHead) {
            return Err(makeError(GIT_ENOTFOUND));
        }

        git_signature* rawSig = nullptr;
        const i32 sigRet = git_signature_now(&rawSig, username.c_str(), userEmail.c_str());

        if (sigRet != 0) {
            return Err(makeError(sigRet));
        }

        const detail::SigHandle sig(rawSig);

        git_oid commitOid;
        i32 ret = 0;

        if (amend) {
            // Empty message means "keep the original commit message".
            const char* const msgPtr = message.empty() ? nullptr : message.data();

            ret = git_commit_amend(
                &commitOid,
                headCommit.get(),
                "HEAD",
                sig.get(),
                sig.get(),
                nullptr,
                msgPtr,
                tree.get()
            );
        } else {
            array<const git_commit*, 1> parentPtrs = { headCommit.get() };
            const usize parentCount = hasHead ? 1 : 0;

            ret = git_commit_create(
                &commitOid,
                repo.get(),
                "HEAD",
                sig.get(),
                sig.get(),
                nullptr,
                message.data(),
                tree.get(),
                parentCount,
                parentCount != 0 ? parentPtrs.data() : nullptr
            );
        }

        if (ret != 0) {
            return Err(makeError(ret));
        }

        if (push) {
            const auto result = this->push();
            //? Do something with this result? Commit has already succeeded,
            //  so it will be incorrect to return an error
        }

        array<char, GIT_OID_HEXSZ + 1> oidBuf;
        git_oid_tostr(oidBuf.data(), GIT_OID_HEXSZ + 1, &commitOid);
        return oidBuf;
    }

    [[nodiscard]] auto push(const string_view remoteName = "origin", const string_view branchName = "")
        -> Result<void> {
        detail::RemoteHandle remote;
        {
            git_remote* raw = nullptr;
            const i32 ret = git_remote_lookup(&raw, repo.get(), remoteName.data());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            remote = detail::RemoteHandle(raw);
        }

        string resolvedBranch;

        if (branchName.empty()) {
            auto branchResult = currentBranch();

            if (!branchResult) {
                return Err(branchResult.error());
            }

            resolvedBranch = std::move(branchResult.value());
        } else {
            resolvedBranch = string(branchName);
        }

        const string refspec = "refs/heads/" + resolvedBranch + ":refs/heads/" + resolvedBranch;
        const char* const refspecPtr = refspec.c_str();
        const git_strarray refspecs = { .strings = const_cast<char**>(&refspecPtr), .count = 1 };

        git_push_options opts = GIT_PUSH_OPTIONS_INIT;
        opts.callbacks.credentials = &Repository::credentialsTrampoline;
        opts.callbacks.push_transfer_progress = &Repository::pushProgressTrampoline;
        opts.callbacks.payload = this;

        const i32 ret = git_remote_push(remote.get(), &refspecs, &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto pull(const string_view remoteName = "origin", const string_view branchName = "")
        -> Result<void> {
        {
            detail::RemoteHandle remote;
            git_remote* raw = nullptr;
            const i32 ret = git_remote_lookup(&raw, repo.get(), remoteName.data());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            remote = detail::RemoteHandle(raw);

            git_fetch_options fetchOpts = GIT_FETCH_OPTIONS_INIT;
            fetchOpts.callbacks.credentials = &Repository::credentialsTrampoline;
            fetchOpts.callbacks.transfer_progress = &Repository::transferProgressTrampoline;
            fetchOpts.callbacks.payload = this;

            const i32 fetchRet = git_remote_fetch(remote.get(), nullptr, &fetchOpts, nullptr);

            if (fetchRet != 0) {
                return Err(makeError(fetchRet));
            }
        }

        string resolvedBranch;

        if (branchName.empty()) {
            auto branchResult = currentBranch();

            if (!branchResult) {
                return Err(branchResult.error());
            }

            resolvedBranch = std::move(branchResult.value());
        } else {
            resolvedBranch = string(branchName);
        }

        const string remoteRefSpec = string(remoteName) + "/" + resolvedBranch;

        detail::AnnotatedHandle annotated;
        {
            git_annotated_commit* raw = nullptr;
            const i32 ret = git_annotated_commit_from_revspec(&raw, repo.get(), remoteRefSpec.c_str());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            annotated = detail::AnnotatedHandle(raw);
        }

        array<const git_annotated_commit*, 1> mergeHeads = { annotated.get() };
        git_merge_analysis_t analysis = GIT_MERGE_ANALYSIS_NONE;
        git_merge_preference_t preference = GIT_MERGE_PREFERENCE_NONE;

        {
            const i32 ret = git_merge_analysis(&analysis, &preference, repo.get(), mergeHeads.data(), 1);

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        if ((analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) != 0) {
            return {};
        }

        if ((analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) != 0) {
            const git_oid* const targetOid = git_annotated_commit_id(annotated.get());
            detail::ObjHandle targetObj;
            {
                git_object* raw = nullptr;
                const i32 ret = git_object_lookup(&raw, repo.get(), targetOid, GIT_OBJECT_COMMIT);

                if (ret != 0) {
                    return Err(makeError(ret));
                }

                targetObj = detail::ObjHandle(raw);
            }

            return performFastForward(targetObj.get(), resolvedBranch);
        }

        const git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
        git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;
        checkoutOpts.progress_cb = checkoutProgressCallback;
        checkoutOpts.progress_payload = checkoutProgressPayload;

        {
            const i32 ret = git_merge(repo.get(), mergeHeads.data(), 1, &mergeOpts, &checkoutOpts);

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        const string mergeMsg = "Merge remote-tracking branch '" + remoteRefSpec + "'";
        const git_oid* const mergeHeadOid = git_annotated_commit_id(annotated.get());

        const auto commitResult = createMergeCommit(mergeMsg, mergeHeadOid);
        if (!commitResult) {
            return Err(commitResult.error());
        }

        git_repository_state_cleanup(repo.get());
        return {};
    }

    [[nodiscard]] auto stashPush(const string_view message = "") -> Result<void> {
        git_signature* rawSig = nullptr;
        const i32 sigRet = git_signature_now(&rawSig, username.c_str(), userEmail.c_str());

        if (sigRet != 0) {
            return Err(makeError(sigRet));
        }

        const detail::SigHandle sig(rawSig);
        const char* const msgCstr = message.data();

        git_oid stashOid;
        const i32 ret = git_stash_save(&stashOid, repo.get(), sig.get(), msgCstr, GIT_STASH_DEFAULT);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto stashPop(const usize stashIndex = 0) -> Result<void> {
        const git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
        const i32 ret = git_stash_pop(repo.get(), stashIndex, &opts);
        if (ret != 0) {
            return Err(makeError(ret));
        }
        return {};
    }

    [[nodiscard]] auto stashApply(const usize stashIndex = 0) -> Result<void> {
        const git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
        const i32 ret = git_stash_apply(repo.get(), stashIndex, &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto stashDrop(const usize stashIndex = 0) -> Result<void> {
        const i32 ret = git_stash_drop(repo.get(), stashIndex);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto listStashes() const -> Result<vector<StashEntry>> {
        vector<StashEntry> entries;

        const i32 ret = git_stash_foreach(
            repo.get(),
            [](const usize idx, const char* const msg, const git_oid* const stashId, void* const payload) -> i32 {
            auto* const out = static_cast<vector<StashEntry>*>(payload);

            array<char, GIT_OID_HEXSZ + 1> buf;
            git_oid_tostr(buf.data(), GIT_OID_HEXSZ + 1, stashId);

            out->push_back(
                StashEntry{
                    .oid = buf,
                    .message = (msg != nullptr) ? string(msg) : string(),
                    .index = idx,
                }
            );

            return 0;
        },
            &entries
        );

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return entries;
    }

    [[nodiscard]] auto listStashFiles(const usize stashIndex) const -> Result<vector<string>> {
        const auto stashesResult = listStashes();
        if (!stashesResult) {
            return Err(stashesResult.error());
        }

        const auto& stashes = stashesResult.value();
        if (stashIndex >= stashes.size()) {
            return Err(Error{ .message = "stash index out of range", .code = -1 });
        }

        git_oid stashOid;

        {
            const i32 ret = git_oid_fromstr(&stashOid, stashes[stashIndex].oid.data());

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        detail::CommitHandle stashCommit;

        {
            git_commit* raw = nullptr;
            const i32 ret = git_commit_lookup(&raw, repo.get(), &stashOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            stashCommit = detail::CommitHandle(raw);
        }

        detail::TreeHandle stashTree;

        {
            git_tree* raw = nullptr;
            const i32 ret = git_commit_tree(&raw, stashCommit.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            stashTree = detail::TreeHandle(raw);
        }

        detail::CommitHandle parentCommit;
        detail::TreeHandle parentTree;

        if (git_commit_parentcount(stashCommit.get()) != 0) {
            git_commit* raw = nullptr;
            const i32 ret = git_commit_parent(&raw, stashCommit.get(), 0);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            parentCommit = detail::CommitHandle(raw);

            git_tree* treeRaw = nullptr;
            const i32 treeRet = git_commit_tree(&treeRaw, parentCommit.get());

            if (treeRet != 0) {
                return Err(makeError(treeRet));
            }

            parentTree = detail::TreeHandle(treeRaw);
        }

        detail::DiffHandle diff;
        {
            git_diff* raw = nullptr;
            const i32 ret = git_diff_tree_to_tree(&raw, repo.get(), parentTree.get(), stashTree.get(), nullptr);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            diff = detail::DiffHandle(raw);
        }

        vector<string> filePaths;
        const i32 ret = git_diff_foreach(
            diff.get(),
            [](const git_diff_delta* const delta, const f32 /*progress*/, void* const payload) -> i32 {
            auto* const out = static_cast<vector<string>*>(payload);

            if (delta->new_file.path != nullptr) {
                out->emplace_back(delta->new_file.path);
            }

            return 0;
        },
            nullptr,
            nullptr,
            nullptr,
            &filePaths
        );

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return filePaths;
    }

    struct Branch {
        string name;
        git_branch_t type;
    };

    [[nodiscard]] auto changeBranch(const string& branch, const git_branch_t type = GIT_BRANCH_LOCAL) -> Result<void> {
        detail::RefHandle branchRef;
        {
            git_reference* raw = nullptr;
            const i32 ret = git_branch_lookup(&raw, repo.get(), branch.c_str(), type);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            branchRef = detail::RefHandle(raw);
        }

        {
            const i32 ret = git_repository_set_head(repo.get(), git_reference_name(branchRef.get()));

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        opts.progress_cb = checkoutProgressCallback;
        opts.progress_payload = checkoutProgressPayload;
        const i32 ret = git_checkout_head(repo.get(), &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    };

    [[nodiscard]] auto listBranches(git_branch_t type = GIT_BRANCH_ALL) const -> Result<vector<Branch>> {
        git_branch_iterator* iter = nullptr;
        i32 result = git_branch_iterator_new(&iter, repo.get(), type);

        if (result != 0) {
            return Err(makeError(result));
        }

        const detail::BranchIterHandle iterHandle(iter);

        vector<Branch> branches;
        git_reference* ref = nullptr;
        git_branch_t outType = type;

        while (true) {
            result = git_branch_next(&ref, &outType, iter);

            if (result == GIT_ITEROVER) {
                break;
            }

            if (result != 0) {
                return Err(makeError(result));
            }

            const detail::RefHandle refHandle(ref);

            const char* name = nullptr;
            result = git_branch_name(&name, refHandle.get());

            if (result == 0) {
                branches.emplace_back(name, outType);
            }
        }

        if (git_repository_head_unborn(repo.get()) == 1) {
            git_reference* headRef = nullptr;

            if (git_reference_lookup(&headRef, repo.get(), "HEAD") == 0) {
                const detail::RefHandle headHandle(headRef);
                const char* const target = git_reference_symbolic_target(headHandle.get());

                if (target != nullptr) {
                    const auto full = string_view(target);  // e.g. "refs/heads/master"
                    static constexpr string_view prefix = "refs/heads/";

                    if (full.starts_with(prefix)) {
                        branches.emplace_back(string(full.substr(prefix.size())), GIT_BRANCH_LOCAL);
                    }
                }
            }
        }

        return branches;
    }

    [[nodiscard]] auto diffWorkdirToIndex(const vector<string>& pathspec = {}) const -> Result<vector<FileDiff>> {
        const auto idxResult = openIndex();

        if (!idxResult) {
            return Err(idxResult.error());
        }

        detail::StrArray pathArr(pathspec);
        git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
        opts.pathspec = *pathArr.get();

        detail::DiffHandle diff;
        {
            git_diff* raw = nullptr;
            const i32 ret = git_diff_index_to_workdir(&raw, repo.get(), idxResult.value().get(), &opts);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            diff = detail::DiffHandle(raw);
        }

        return collectDiff(diff.get());
    }

    [[nodiscard]] auto diffIndexToHead(const vector<string>& pathspec = {}) const -> Result<vector<FileDiff>> {
        const auto idxResult = openIndex();

        if (!idxResult) {
            return Err(idxResult.error());
        }

        const auto headTreeResult = resolveTree("HEAD");

        if (!headTreeResult) {
            return Err(headTreeResult.error());
        }

        detail::StrArray pathArr(pathspec);
        git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
        opts.pathspec = *pathArr.get();

        detail::DiffHandle diff;
        {
            git_diff* raw = nullptr;
            const i32 ret =
                git_diff_tree_to_index(&raw, repo.get(), headTreeResult.value().get(), idxResult.value().get(), &opts);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            diff = detail::DiffHandle(raw);
        }

        return collectDiff(diff.get());
    }

    [[nodiscard]] auto diffCommits(
        const string_view oldCommitish,
        const string_view newCommitish,
        const vector<string>& pathspec = {}
    ) const -> Result<vector<FileDiff>> {
        const auto oldTreeResult = resolveTree(oldCommitish);

        if (!oldTreeResult) {
            return Err(oldTreeResult.error());
        }

        const auto newTreeResult = resolveTree(newCommitish);

        if (!newTreeResult) {
            return Err(newTreeResult.error());
        }

        detail::StrArray pathArr(pathspec);
        git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
        opts.pathspec = *pathArr.get();

        detail::DiffHandle diff;
        {
            git_diff* raw = nullptr;
            const i32 ret = git_diff_tree_to_tree(
                &raw,
                repo.get(),
                oldTreeResult.value().get(),
                newTreeResult.value().get(),
                &opts
            );

            if (ret != 0) {
                return Err(makeError(ret));
            }

            diff = detail::DiffHandle(raw);
        }

        return collectDiff(diff.get());
    }

    [[nodiscard]] auto reset(const string_view commitish, const git_reset_t type = GIT_RESET_MIXED) -> Result<void> {
        const auto objResult = resolveObject(commitish);

        if (!objResult) {
            return Err(objResult.error());
        }

        git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;
        checkoutOpts.progress_cb = checkoutProgressCallback;
        checkoutOpts.progress_payload = checkoutProgressPayload;

        const i32 ret = git_reset(repo.get(), objResult.value().get(), type, &checkoutOpts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto checkoutPaths(const vector<string>& paths, const string_view commitish = "HEAD")
        -> Result<void> {
        const auto objResult = resolveObject(commitish);

        if (!objResult) {
            return Err(objResult.error());
        }

        detail::StrArray pathArr(paths);
        git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        opts.checkout_strategy = GIT_CHECKOUT_FORCE;
        opts.paths = *pathArr.get();
        opts.progress_cb = checkoutProgressCallback;
        opts.progress_payload = checkoutProgressPayload;

        const i32 ret = git_checkout_tree(repo.get(), objResult.value().get(), &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto listRemotes() const -> Result<vector<RemoteInfo>> {
        detail::OwnedStrArray names;
        const i32 ret = git_remote_list(names.get(), repo.get());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        vector<RemoteInfo> remotes;
        remotes.reserve(names.get()->count);

        for (usize i = 0; i < names.get()->count; i++) {
            const char* const remoteName = names.get()->strings[i];

            detail::RemoteHandle remote;
            {
                git_remote* raw = nullptr;
                const i32 lookupRet = git_remote_lookup(&raw, repo.get(), remoteName);

                if (lookupRet != 0) {
                    return Err(makeError(lookupRet));
                }

                remote = detail::RemoteHandle(raw);
            }

            const char* const url = git_remote_url(remote.get());
            const char* const pushUrl = git_remote_pushurl(remote.get());

            remotes.push_back(
                RemoteInfo{
                    .name = string(remoteName),
                    .url = (url != nullptr) ? string(url) : string(),
                    .pushUrl = (pushUrl != nullptr) ? string(pushUrl) : ((url != nullptr) ? string(url) : string()),
                }
            );
        }

        return remotes;
    }

    [[nodiscard]] auto addRemote(const string_view name, const string_view url) -> Result<void> {
        git_remote* raw = nullptr;
        const i32 ret = git_remote_create(&raw, repo.get(), name.data(), url.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        const detail::RemoteHandle remote(raw);
        return {};
    }

    [[nodiscard]] auto removeRemote(const string_view name) -> Result<void> {
        const i32 ret = git_remote_delete(repo.get(), name.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto setRemoteUrl(const string_view name, const string_view url) -> Result<void> {
        const i32 ret = git_remote_set_url(repo.get(), name.data(), url.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto listTags() const -> Result<vector<TagInfo>> {
        detail::OwnedStrArray names;
        const i32 ret = git_tag_list(names.get(), repo.get());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        vector<TagInfo> tags;
        tags.reserve(names.get()->count);

        for (usize i = 0; i < names.get()->count; i++) {
            const char* const tagName = names.get()->strings[i];
            const string refPath = string("refs/tags/") + tagName;

            detail::RefHandle ref;
            {
                git_reference* raw = nullptr;
                const i32 refRet = git_reference_lookup(&raw, repo.get(), refPath.c_str());

                if (refRet != 0) {
                    return Err(makeError(refRet));
                }

                ref = detail::RefHandle(raw);
            }

            detail::ObjHandle target;
            {
                git_object* raw = nullptr;
                const i32 peelRet = git_reference_peel(&raw, ref.get(), GIT_OBJECT_ANY);

                if (peelRet != 0) {
                    return Err(makeError(peelRet));
                }

                target = detail::ObjHandle(raw);
            }

            array<char, GIT_OID_HEXSZ + 1> oidBuf;

            if (git_object_type(target.get()) == GIT_OBJECT_TAG) {
                detail::TagHandle tagHandle;
                {
                    git_tag* raw = nullptr;
                    const i32 tagRet = git_tag_lookup(&raw, repo.get(), git_object_id(target.get()));

                    if (tagRet != 0) {
                        return Err(makeError(tagRet));
                    }

                    tagHandle = detail::TagHandle(raw);
                }

                git_oid_tostr(oidBuf.data(), GIT_OID_HEXSZ + 1, git_tag_target_id(tagHandle.get()));
                const char* const message = git_tag_message(tagHandle.get());

                tags.push_back(
                    TagInfo{
                        .oid = oidBuf,
                        .name = string(tagName),
                        .message = (message != nullptr) ? string(message) : string(),
                        .annotated = true,
                    }
                );
            } else {
                git_oid_tostr(oidBuf.data(), GIT_OID_HEXSZ + 1, git_object_id(target.get()));

                tags.push_back(
                    TagInfo{
                        .oid = oidBuf,
                        .name = string(tagName),
                        .message = string(),
                        .annotated = false,
                    }
                );
            }
        }

        return tags;
    }

    [[nodiscard]] auto
    createTag(const string_view name, const string_view message = "", const string_view target = "HEAD")
        -> Result<void> {
        const auto objResult = resolveObject(target);

        if (!objResult) {
            return Err(objResult.error());
        }

        git_oid tagOid;

        if (message.empty()) {
            const i32 ret = git_tag_create_lightweight(&tagOid, repo.get(), name.data(), objResult.value().get(), 0);

            if (ret != 0) {
                return Err(makeError(ret));
            }
        } else {
            git_signature* rawSig = nullptr;
            const i32 sigRet = git_signature_now(&rawSig, username.c_str(), userEmail.c_str());

            if (sigRet != 0) {
                return Err(makeError(sigRet));
            }

            const detail::SigHandle sig(rawSig);
            const i32 ret =
                git_tag_create(&tagOid, repo.get(), name.data(), objResult.value().get(), sig.get(), message.data(), 0);

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        return {};
    }

    [[nodiscard]] auto deleteTag(const string_view name) -> Result<void> {
        const i32 ret = git_tag_delete(repo.get(), name.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto configGet(const string_view key) const -> Result<string> {
        detail::ConfigHandle cfg;
        {
            git_config* raw = nullptr;
            const i32 ret = git_repository_config(&raw, repo.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            cfg = detail::ConfigHandle(raw);
        }

        git_buf buf = GIT_BUF_INIT;
        const i32 ret = git_config_get_string_buf(&buf, cfg.get(), key.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        string value(buf.ptr, buf.size);
        git_buf_dispose(&buf);
        return value;
    }

    [[nodiscard]] auto configSet(const string_view key, const string_view value) -> Result<void> {
        detail::ConfigHandle cfg;
        {
            git_config* raw = nullptr;
            const i32 ret = git_repository_config(&raw, repo.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            cfg = detail::ConfigHandle(raw);
        }

        const i32 ret = git_config_set_string(cfg.get(), key.data(), value.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return {};
    }

    [[nodiscard]] auto blameFile(const string_view path) const -> Result<vector<BlameLine>> {
        detail::BlameHandle blame;
        {
            git_blame* raw = nullptr;
            git_blame_options opts = GIT_BLAME_OPTIONS_INIT;
            const i32 ret = git_blame_file(&raw, repo.get(), string(path).c_str(), &opts);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            blame = detail::BlameHandle(raw);
        }

        // git_blame's line numbers describe the file as it exists in the
        // working directory, so blame content is read from there rather
        // than from any blob.
        const string fullPath = string(workdir()) + string(path);
        std::ifstream file(fullPath);
        vector<string> lines;
        string line;

        while (std::getline(file, line)) {
            lines.push_back(line);
        }

        vector<BlameLine> result;
        const u32 hunkCount = git_blame_get_hunk_count(blame.get());
        result.reserve(hunkCount);

        for (u32 i = 0; i < hunkCount; i++) {
            const git_blame_hunk* const hunk = git_blame_get_hunk_byindex(blame.get(), i);

            if (hunk == nullptr) {
                continue;
            }

            array<char, GIT_OID_HEXSZ + 1> oidBuf;
            git_oid_tostr(oidBuf.data(), GIT_OID_HEXSZ + 1, &hunk->final_commit_id);

            for (u32 lineOffset = 0; lineOffset < hunk->lines_in_hunk; lineOffset++) {
                const usize lineNo = hunk->final_start_line_number + lineOffset;

                result.push_back(
                    BlameLine{
                        .commitOid = oidBuf,
                        .authorName =
                            (hunk->final_signature != nullptr) ? string(hunk->final_signature->name) : string(),
                        .authorEmail =
                            (hunk->final_signature != nullptr) ? string(hunk->final_signature->email) : string(),
                        .content = (lineNo - 1 < lines.size()) ? lines[lineNo - 1] : string(),
                        .timestamp = (hunk->final_signature != nullptr) ? hunk->final_signature->when.time : 0,
                        .lineNo = lineNo,
                    }
                );
            }
        }

        return result;
    }

    [[nodiscard]] auto repoHandle() -> git_repository* { return repo.get(); }

   private:
    detail::RepoHandle repo;

    string username;
    string userEmail;

    git_credential_acquire_cb credentialsCallback = nullptr;
    void* credentialsPayload = nullptr;

    git_indexer_progress_cb transferProgressCallback = nullptr;
    void* transferProgressPayload = nullptr;

    git_push_transfer_progress_cb pushProgressCallback = nullptr;
    void* pushProgressPayload = nullptr;

    git_checkout_progress_cb checkoutProgressCallback = nullptr;
    void* checkoutProgressPayload = nullptr;

    static auto credentialsTrampoline(
        git_credential** const out,
        const char* const url,
        const char* const usernameFromUrl,
        const u32 allowedTypes,
        void* const payload
    ) -> i32 {
        auto* const self = static_cast<Repository*>(payload);

        if (self->credentialsCallback == nullptr) {
            return GIT_PASSTHROUGH;
        }

        return self->credentialsCallback(out, url, usernameFromUrl, allowedTypes, self->credentialsPayload);
    }

    static auto transferProgressTrampoline(const git_indexer_progress* const stats, void* const payload) -> i32 {
        auto* const self = static_cast<Repository*>(payload);

        if (self->transferProgressCallback == nullptr) {
            return 0;
        }

        return self->transferProgressCallback(stats, self->transferProgressPayload);
    }

    static auto pushProgressTrampoline(const u32 current, const u32 total, const usize bytes, void* const payload)
        -> i32 {
        auto* const self = static_cast<Repository*>(payload);

        if (self->pushProgressCallback == nullptr) {
            return 0;
        }

        return self->pushProgressCallback(current, total, bytes, self->pushProgressPayload);
    }

    [[nodiscard]] auto resolveObject(const string_view spec) const -> Result<detail::ObjHandle> {
        git_object* raw = nullptr;
        const i32 ret = git_revparse_single(&raw, repo.get(), spec.data());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return detail::ObjHandle(raw);
    }

    [[nodiscard]] auto resolveTree(const string_view spec) const -> Result<detail::TreeHandle> {
        const auto objResult = resolveObject(spec);

        if (!objResult) {
            return Err(objResult.error());
        }

        git_tree* rawTree = nullptr;

        // git_object_peel's out-parameter is declared as git_object** purely
        // for genericity; when target_type is GIT_OBJECT_TREE the object it
        // fills in is always a git_tree, and this cast is libgit2's own
        // documented usage pattern for consuming peel results.
        const i32 ret =
            git_object_peel(reinterpret_cast<git_object**>(&rawTree), objResult.value().get(), GIT_OBJECT_TREE);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return detail::TreeHandle(rawTree);
    }

    [[nodiscard]] auto collectDiff(git_diff* const diff) const -> Result<vector<FileDiff>> {
        vector<FileDiff> results;

        const i32 ret = git_diff_foreach(
            diff,
            [](const git_diff_delta* const delta, const f32 /*progress*/, void* const payload) -> i32 {
            auto* const out = static_cast<vector<FileDiff>*>(payload);
            out->push_back(
                FileDiff{
                    .oldPath = (delta->old_file.path != nullptr) ? string(delta->old_file.path) : string(),
                    .newPath = (delta->new_file.path != nullptr) ? string(delta->new_file.path) : string(),
                    .hunks = {},
                }
            );
            return 0;
        },
            nullptr,
            [](const git_diff_delta* const /*delta*/, const git_diff_hunk* const hunk, void* const payload) -> i32 {
            auto* const out = static_cast<vector<FileDiff>*>(payload);
            out->back().hunks.push_back(
                DiffHunk{
                    .header = string(hunk->header, hunk->header_len),
                    .lines = {},
                }
            );
            return 0;
        },
            [](const git_diff_delta* const /*delta*/,
               const git_diff_hunk* const /*hunk*/,
               const git_diff_line* const line,
               void* const payload) -> i32 {
            auto* const out = static_cast<vector<FileDiff>*>(payload);
            out->back().hunks.back().lines.push_back(
                DiffLine{
                    .content = string(line->content, line->content_len),
                    .oldLineno = line->old_lineno,
                    .newLineno = line->new_lineno,
                    .origin = line->origin,
                }
            );
            return 0;
        },
            &results
        );

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return results;
    }

    [[nodiscard]] auto collectStatusPaths(const u32 flags) const -> Result<vector<string>> {
        git_status_options opts = GIT_STATUS_OPTIONS_INIT;
        opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
        opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS |
                     GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

        git_status_list* rawList = nullptr;
        const i32 ret = git_status_list_new(&rawList, repo.get(), &opts);

        if (ret != 0) {
            return Err(makeError(ret));
        }

        const detail::StatusListHandle statusList(rawList);
        const usize count = git_status_list_entrycount(statusList.get());

        vector<string> paths;
        for (usize idx = 0; idx < count; idx++) {
            const git_status_entry* const entry = git_status_byindex(statusList.get(), idx);

            if (entry == nullptr) {
                continue;
            }

            if ((entry->status & flags) == 0) {
                continue;
            }

            const char* filePath = nullptr;

            if (entry->index_to_workdir != nullptr) {
                filePath = entry->index_to_workdir->new_file.path;
            }

            if (filePath == nullptr && entry->head_to_index != nullptr) {
                filePath = entry->head_to_index->new_file.path;
            }

            if (filePath != nullptr) {
                paths.emplace_back(filePath);
            }
        }

        return paths;
    }

    [[nodiscard]] auto openIndex() const -> Result<detail::IndexHandle> {
        git_index* raw = nullptr;
        const i32 ret = git_repository_index(&raw, repo.get());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        detail::IndexHandle idx(raw);

        const i32 readRet = git_index_read(idx.get(), 1);

        if (readRet != 0) {
            return Err(makeError(readRet));
        }

        return idx;
    }

    [[nodiscard]] auto currentBranch() const -> Result<string> {
        detail::RefHandle headRef;
        {
            git_reference* raw = nullptr;
            const i32 ret = git_repository_head(&raw, repo.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            headRef = detail::RefHandle(raw);
        }

        const char* branchCstr = nullptr;
        const i32 ret = git_branch_name(&branchCstr, headRef.get());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return string(branchCstr);
    }

    [[nodiscard]] auto getHeadRef() const -> Result<detail::RefHandle> {
        git_reference* raw = nullptr;
        const i32 ret = git_repository_head(&raw, repo.get());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return detail::RefHandle(raw);
    }

    [[nodiscard]] auto getRemoteTrackingRef(const string_view remoteName, const string_view branchName) const
        -> Result<detail::RefHandle> {
        string resolvedBranch;

        if (branchName.empty()) {
            auto branchResult = currentBranch();

            if (!branchResult) {
                return Err(branchResult.error());
            }

            resolvedBranch = std::move(branchResult.value());
        } else {
            resolvedBranch = branchName;
        }

        git_reference* raw = nullptr;
        const string refPath = "refs/remotes/" + string(remoteName) + '/' + resolvedBranch;
        const i32 ret = git_reference_lookup(&raw, repo.get(), refPath.c_str());

        if (ret != 0) {
            return Err(makeError(ret));
        }

        return detail::RefHandle(raw);
    }

    [[nodiscard]] auto countCommitsBetween(const git_oid* const fromOid, const git_oid* const toOid) const
        -> Result<usize> {
        if (fromOid == nullptr || toOid == nullptr) {
            return 0;
        }

        detail::RevwalkHandle walker;

        {
            git_revwalk* raw = nullptr;
            const i32 ret = git_revwalk_new(&raw, repo.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            walker = detail::RevwalkHandle(raw);
        }

        {
            const i32 ret = git_revwalk_push(walker.get(), toOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        {
            const i32 ret = git_revwalk_hide(walker.get(), fromOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        usize count = 0;
        git_oid iterOid;

        while (git_revwalk_next(&iterOid, walker.get()) == 0) {
            count++;
        }

        return count;
    }

    [[nodiscard]] auto performFastForward(const git_object* const targetObj, const string& targetBranch)
        -> Result<void> {
        git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;
        checkoutOpts.progress_cb = checkoutProgressCallback;
        checkoutOpts.progress_payload = checkoutProgressPayload;

        {
            const i32 ret = git_checkout_tree(repo.get(), targetObj, &checkoutOpts);

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        const string refPath = "refs/heads/" + targetBranch;
        const git_oid* const newOid = git_object_id(targetObj);

        detail::RefHandle branchRef;

        {
            git_reference* raw = nullptr;
            const i32 ret = git_reference_lookup(&raw, repo.get(), refPath.c_str());

            if (ret != 0) {
                return Err(makeError(ret));
            }

            branchRef = detail::RefHandle(raw);
        }

        {
            git_reference* raw = nullptr;
            const i32 ret = git_reference_set_target(&raw, branchRef.get(), newOid, "Fast-forward");

            if (ret != 0) {
                return Err(makeError(ret));
            }

            const detail::RefHandle updatedRef(raw);
        }

        return {};
    }

    [[nodiscard]] auto createMergeCommit(const string& message, const git_oid* const mergeHeadOid)
        -> Result<array<char, GIT_OID_HEXSZ + 1>> {
        const auto idxResult = openIndex();

        if (!idxResult) {
            return Err(idxResult.error());
        }

        const auto& idx = idxResult.value();

        if (git_index_has_conflicts(idx.get()) != 0) {
            return Err(
                Error{
                    .message = "merge conflicts must be resolved before committing",
                    .code = -1,
                }
            );
        }

        git_oid treeOid;

        {
            const i32 ret = git_index_write_tree(&treeOid, idx.get());

            if (ret != 0) {
                return Err(makeError(ret));
            }
        }

        detail::TreeHandle tree;

        {
            git_tree* raw = nullptr;
            const i32 ret = git_tree_lookup(&raw, repo.get(), &treeOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            tree = detail::TreeHandle(raw);
        }

        git_oid headOid;
        detail::CommitHandle headCommit;

        {
            const i32 ret = git_reference_name_to_id(&headOid, repo.get(), "HEAD");

            if (ret != 0) {
                return Err(makeError(ret));
            }

            git_commit* raw = nullptr;
            const i32 lookupRet = git_commit_lookup(&raw, repo.get(), &headOid);

            if (lookupRet != 0) {
                return Err(makeError(lookupRet));
            }

            headCommit = detail::CommitHandle(raw);
        }

        detail::CommitHandle mergeCommitObj;

        {
            git_commit* raw = nullptr;
            const i32 ret = git_commit_lookup(&raw, repo.get(), mergeHeadOid);

            if (ret != 0) {
                return Err(makeError(ret));
            }

            mergeCommitObj = detail::CommitHandle(raw);
        }

        array<const git_commit*, 2> parents = { headCommit.get(), mergeCommitObj.get() };

        git_signature* rawSig = nullptr;
        const i32 sigRet = git_signature_now(&rawSig, username.c_str(), userEmail.c_str());

        if (sigRet != 0) {
            return Err(makeError(sigRet));
        }

        const detail::SigHandle sig(rawSig);

        git_oid newCommitOid;
        const i32 ret = git_commit_create(
            &newCommitOid,
            repo.get(),
            "HEAD",
            sig.get(),
            sig.get(),
            nullptr,
            message.c_str(),
            tree.get(),
            2,
            parents.data()
        );

        if (ret != 0) {
            return Err(makeError(ret));
        }

        array<char, GIT_OID_HEXSZ + 1> oidBuf;
        git_oid_tostr(oidBuf.data(), GIT_OID_HEXSZ + 1, &newCommitOid);
        return oidBuf;
    }
};
}  // namespace ezgit2
#endif
