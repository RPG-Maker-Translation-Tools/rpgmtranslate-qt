#ifdef ENABLE_LIBGIT2
#pragma once

#include <git2/index.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <git2.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// TODO: Mostly vibe-coded by Claude. Needs actual polishing, then move to the
// separate repository

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
    using std::vector;

    template <typename T>
    using Err = unexpected<T>;

    struct Error {
        string message;
        i32 code;
    };

    template <typename Val>
    using Result = expected<Val, Error>;

    [[nodiscard]] inline auto make_error(const i32 code) -> Error {
        const git_error* const err = git_error_last();
        return Error{
            .message =
                (err != nullptr) ? string(err->message) : "unknown error",
            .code = code,
        };
    }

    struct CommitInfo {
        string oid;
        string message;
        string author_name;
        string author_email;
        i64 timestamp;
    };

    struct StashEntry {
        string message;
        string oid;
        usize index;
    };

    namespace detail {
        template <typename T, auto DelFn>
        class GitHandle final {
           public:
            GitHandle() = default;

            explicit GitHandle(T* const ptr) : ptr_(ptr) {}

            ~GitHandle() { reset(); }

            GitHandle(GitHandle&& other) noexcept : ptr_(other.ptr_) {
                other.ptr_ = nullptr;
            }

            auto operator=(GitHandle&& other) noexcept -> GitHandle& {
                if (this != &other) {
                    reset();
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                }

                return *this;
            }

            GitHandle(const GitHandle&) = delete;
            auto operator=(const GitHandle&) -> GitHandle& = delete;

            [[nodiscard]] auto get() const -> T* { return ptr_; }

            [[nodiscard]] explicit operator bool() const {
                return ptr_ != nullptr;
            }

            auto reset() -> void {
                if (ptr_ != nullptr) {
                    DelFn(ptr_);
                    ptr_ = nullptr;
                }
            }

           private:
            T* ptr_ = nullptr;
        };

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
        using AnnotatedHandle =
            GitHandle<git_annotated_commit, git_annotated_commit_free>;

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
    }  // namespace detail

    class Repository final {
       public:
        [[nodiscard]] static auto open(const string_view path)
            -> Result<Repository> {
            git_repository* raw = nullptr;
            const i32 ret = git_repository_open(&raw, string(path).c_str());

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return Repository(raw);
        }

        [[nodiscard]] static auto init(
            const string_view path,
            const bool bare = false
        ) -> Result<Repository> {
            git_repository* raw = nullptr;
            const i32 ret =
                git_repository_init(&raw, string(path).c_str(), bare ? 1 : 0);

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return Repository(raw);
        }

        [[nodiscard]] static auto clone_from(
            const string_view url,
            const string_view local_path
        ) -> Result<Repository> {
            git_repository* raw = nullptr;
            const i32 ret = git_clone(
                &raw,
                string(url).c_str(),
                string(local_path).c_str(),
                nullptr
            );

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return Repository(raw);
        }

        Repository() = default;
        Repository(Repository&&) = default;
        auto operator=(Repository&&) -> Repository& = default;
        Repository(const Repository&) = delete;
        auto operator=(const Repository&) -> Repository& = delete;
        ~Repository() = default;

        [[nodiscard]] auto workdir() const -> const char* {
            const char* const workdir = git_repository_workdir(repo.get());
            return workdir;
        }

        [[nodiscard]] auto list_tracked() const -> Result<vector<string>> {
            const auto index_result = open_index();

            if (!index_result) {
                return Err(index_result.error());
            }

            const auto& index = index_result.value();
            const usize count = git_index_entrycount(index.get());

            vector<string> tracked;
            tracked.reserve(count);

            for (usize i = 0; i < count; i++) {
                const git_index_entry* entry =
                    git_index_get_byindex(index.get(), i);
                tracked.emplace_back(entry->path);
            }

            return tracked;
        }

        [[nodiscard]] auto list_untracked() const -> Result<vector<string>> {
            return collect_status_paths(GIT_STATUS_WT_NEW);
        }

        [[nodiscard]] auto list_unstaged() const -> Result<vector<string>> {
            constexpr u32 mask =
                GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_DELETED |
                GIT_STATUS_WT_TYPECHANGE | GIT_STATUS_WT_RENAMED;
            return collect_status_paths(mask);
        }

        [[nodiscard]] auto list_staged() const -> Result<vector<string>> {
            constexpr u32 mask =
                GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED |
                GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_TYPECHANGE |
                GIT_STATUS_INDEX_RENAMED;
            return collect_status_paths(mask);
        }

        struct FileStatus {
            string path;
            git_delta_t status;
        };

        [[nodiscard]] auto list_files_status(
            git_diff_option_t diff_options = git_diff_option_t(
                GIT_DIFF_NORMAL | GIT_DIFF_INCLUDE_UNTRACKED |
                GIT_DIFF_RECURSE_UNTRACKED_DIRS
            )
        ) -> Result<vector<FileStatus>> {
            vector<FileStatus> results;

            git_object* head_obj = nullptr;
            git_tree* head_tree = nullptr;
            git_diff* diff = nullptr;

            i32 result =
                git_revparse_single(&head_obj, repo.get(), "HEAD^{tree}");

            if (result != 0) {
                return Err(make_error(result));
            }

            head_tree = reinterpret_cast<git_tree*>(head_obj);

            git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
            diff_opts.flags = diff_options;

            result = git_diff_tree_to_workdir_with_index(
                &diff,
                repo.get(),
                head_tree,
                &diff_opts
            );

            if (result != 0) {
                git_object_free(head_obj);
                return Err(make_error(result));
            }

            const usize count = git_diff_num_deltas(diff);
            results.reserve(count);

            for (usize i = 0; i < count; i++) {
                const git_diff_delta* delta = git_diff_get_delta(diff, i);

                results.emplace_back(
                    delta->status == GIT_DELTA_DELETED ? delta->old_file.path
                                                       : delta->new_file.path,
                    delta->status
                );
            }

            git_diff_free(diff);
            git_object_free(head_obj);  // frees head_tree as well

            return results;
        }

        [[nodiscard]] auto get_commit_history(
            optional<usize> limit = nullopt
        ) const -> Result<vector<CommitInfo>> {
            detail::RevwalkHandle walker;

            {
                git_revwalk* raw = nullptr;
                const i32 ret = git_revwalk_new(&raw, repo.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                walker = detail::RevwalkHandle(raw);
            }

            git_revwalk_sorting(walker.get(), GIT_SORT_TIME);

            {
                const i32 ret = git_revwalk_push_head(walker.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            vector<CommitInfo> commits;
            git_oid iter_oid;
            usize count = 0;

            while (git_revwalk_next(&iter_oid, walker.get()) == 0) {
                if (limit && count >= limit.value()) {
                    break;
                }

                git_commit* raw_commit = nullptr;
                const i32 ret =
                    git_commit_lookup(&raw_commit, repo.get(), &iter_oid);

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                detail::CommitHandle commit_obj(raw_commit);

                array<char, GIT_OID_HEXSZ + 1> oid_buf;
                git_oid_tostr(oid_buf.data(), GIT_OID_HEXSZ + 1, &iter_oid);

                const git_signature* const author =
                    git_commit_author(commit_obj.get());
                const char* const raw_msg =
                    git_commit_message(commit_obj.get());

                commits.push_back(
                    CommitInfo{
                        .oid = string(oid_buf.data()),
                        .message =
                            (raw_msg != nullptr) ? string(raw_msg) : string(),
                        .author_name = (author != nullptr)
                                           ? string(author->name)
                                           : string(),
                        .author_email = (author != nullptr)
                                            ? string(author->email)
                                            : string(),
                        .timestamp =
                            (author != nullptr) ? author->when.time : 0,
                    }
                );

                count++;
            }

            return commits;
        }

        [[nodiscard]] auto count_unpushed_commits(
            const string_view remote_name = "origin",
            const string_view branch_name = ""
        ) const -> Result<usize> {
            const auto local_ref = get_head_ref();

            if (!local_ref) {
                return Err(local_ref.error());
            }

            const auto remote_ref =
                get_remote_tracking_ref(remote_name, branch_name);

            if (!remote_ref) {
                return 0;
            }

            return count_commits_between(
                git_reference_target(remote_ref->get()),
                git_reference_target(local_ref->get())
            );
        }

        [[nodiscard]] auto count_unpulled_commits(
            const string_view remote_name = "origin",
            const string_view branch_name = ""
        ) const -> Result<usize> {
            const auto local_ref = get_head_ref();

            if (!local_ref) {
                return Err(local_ref.error());
            }

            const auto remote_ref =
                get_remote_tracking_ref(remote_name, branch_name);

            if (!remote_ref) {
                return 0;
            }

            return count_commits_between(
                git_reference_target(local_ref->get()),
                git_reference_target(remote_ref->get())
            );
        }

        [[nodiscard]] auto track_files(const vector<string>& paths)
            -> Result<void> {
            return stage_files(paths);
        }

        [[nodiscard]] auto stage_files(const vector<string>& paths)
            -> Result<void> {
            const auto idx_result = open_index();

            if (!idx_result) {
                return Err(idx_result.error());
            }

            const auto& idx = idx_result.value();

            detail::StrArray arr(paths);
            const i32 ret = git_index_add_all(
                idx.get(),
                arr.get(),
                GIT_INDEX_ADD_DEFAULT,
                nullptr,
                nullptr
            );

            if (ret != 0) {
                return Err(make_error(ret));
            }

            const i32 write_ret = git_index_write(idx.get());

            if (write_ret != 0) {
                return Err(make_error(write_ret));
            }

            return {};
        }

        [[nodiscard]] auto unstage_files(const vector<string>& paths)
            -> Result<void> {
            detail::ObjHandle head_obj;
            {
                git_object* raw = nullptr;
                const i32 ret = git_revparse_single(&raw, repo.get(), "HEAD");

                if (ret != 0) {
                    const auto idx_result = open_index();

                    if (!idx_result) {
                        return Err(idx_result.error());
                    }

                    const auto& idx = idx_result.value();
                    detail::StrArray arr(paths);
                    const i32 rm_ret = git_index_remove_all(
                        idx.get(),
                        arr.get(),
                        nullptr,
                        nullptr
                    );

                    if (rm_ret != 0) {
                        return Err(make_error(rm_ret));
                    }

                    const i32 wr_ret = git_index_write(idx.get());

                    if (wr_ret != 0) {
                        return Err(make_error(wr_ret));
                    }

                    return {};
                }

                head_obj = detail::ObjHandle(raw);
            }

            detail::StrArray arr(paths);
            const i32 ret =
                git_reset_default(repo.get(), head_obj.get(), arr.get());

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return {};
        }

        [[nodiscard]] auto commit(
            const string_view message,
            const vector<string>& stage_paths = {}
        ) -> Result<string> {
            if (!stage_paths.empty()) {
                auto stage_result = stage_files(stage_paths);

                if (!stage_result) {
                    return Err(stage_result.error());
                }
            }

            const auto idx_result = open_index();

            if (!idx_result) {
                return Err(idx_result.error());
            }

            const auto& idx = idx_result.value();

            git_oid tree_oid;

            {
                const i32 ret = git_index_write_tree(&tree_oid, idx.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            detail::TreeHandle tree;

            {
                git_tree* raw = nullptr;
                const i32 ret = git_tree_lookup(&raw, repo.get(), &tree_oid);

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                tree = detail::TreeHandle(raw);
            }

            detail::SigHandle sig;

            {
                git_signature* raw = nullptr;
                const i32 ret = git_signature_default(&raw, repo.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                sig = detail::SigHandle(raw);
            }

            detail::CommitHandle head_commit;
            git_oid head_oid;
            const bool has_head =
                git_reference_name_to_id(&head_oid, repo.get(), "HEAD") == 0;

            if (has_head) {
                git_commit* raw = nullptr;
                const i32 ret = git_commit_lookup(&raw, repo.get(), &head_oid);

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                head_commit = detail::CommitHandle(raw);
            }

            array<const git_commit*, 1> parent_ptrs = { head_commit.get() };
            const usize parent_count = has_head ? 1 : 0;

            git_oid commit_oid;
            const i32 ret = git_commit_create(
                &commit_oid,
                repo.get(),
                "HEAD",
                sig.get(),
                sig.get(),
                nullptr,
                string(message).c_str(),
                tree.get(),
                parent_count,
                parent_count != 0 ? parent_ptrs.data() : nullptr
            );

            if (ret != 0) {
                return Err(make_error(ret));
            }

            array<char, GIT_OID_HEXSZ + 1> oid_buf;
            git_oid_tostr(oid_buf.data(), GIT_OID_HEXSZ + 1, &commit_oid);
            return string(oid_buf.data());
        }

        [[nodiscard]] auto push(
            const string_view remote_name = "origin",
            const string_view branch_name = ""
        ) -> Result<void> {
            detail::RemoteHandle remote;
            {
                git_remote* raw = nullptr;
                const i32 ret = git_remote_lookup(
                    &raw,
                    repo.get(),
                    string(remote_name).c_str()
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                remote = detail::RemoteHandle(raw);
            }

            string resolved_branch;

            if (branch_name.empty()) {
                auto branch_result = get_current_branch_name();

                if (!branch_result) {
                    return Err(branch_result.error());
                }

                resolved_branch = std::move(branch_result.value());
            } else {
                resolved_branch = string(branch_name);
            }

            const string refspec = "refs/heads/" + resolved_branch +
                                   ":refs/heads/" + resolved_branch;
            const char* refspec_ptr = refspec.c_str();
            const git_strarray refspecs = {
                .strings = const_cast<char**>(&refspec_ptr),
                .count = 1
            };

            const git_push_options opts = GIT_PUSH_OPTIONS_INIT;
            const i32 ret = git_remote_push(remote.get(), &refspecs, &opts);

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return {};
        }

        [[nodiscard]] auto pull(
            const string_view remote_name = "origin",
            const string_view branch_name = ""
        ) -> Result<void> {
            {
                detail::RemoteHandle remote;
                git_remote* raw = nullptr;
                const i32 ret = git_remote_lookup(
                    &raw,
                    repo.get(),
                    string(remote_name).c_str()
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                remote = detail::RemoteHandle(raw);

                git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
                const i32 fetch_ret = git_remote_fetch(
                    remote.get(),
                    nullptr,
                    &fetch_opts,
                    nullptr
                );

                if (fetch_ret != 0) {
                    return Err(make_error(fetch_ret));
                }
            }

            string resolved_branch;

            if (branch_name.empty()) {
                auto branch_result = get_current_branch_name();

                if (!branch_result) {
                    return Err(branch_result.error());
                }

                resolved_branch = std::move(branch_result.value());
            } else {
                resolved_branch = string(branch_name);
            }

            const string remote_ref_spec =
                string(remote_name) + "/" + resolved_branch;

            detail::AnnotatedHandle annotated;
            {
                git_annotated_commit* raw = nullptr;
                const i32 ret = git_annotated_commit_from_revspec(
                    &raw,
                    repo.get(),
                    remote_ref_spec.c_str()
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                annotated = detail::AnnotatedHandle(raw);
            }

            array<const git_annotated_commit*, 1> merge_heads = {
                annotated.get()
            };
            git_merge_analysis_t analysis = GIT_MERGE_ANALYSIS_NONE;
            git_merge_preference_t preference = GIT_MERGE_PREFERENCE_NONE;

            {
                const i32 ret = git_merge_analysis(
                    &analysis,
                    &preference,
                    repo.get(),
                    merge_heads.data(),
                    1
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            if ((analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) != 0) {
                return {};
            }

            if ((analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) != 0) {
                const git_oid* target_oid =
                    git_annotated_commit_id(annotated.get());
                detail::ObjHandle target_obj;
                {
                    git_object* raw = nullptr;
                    const i32 ret = git_object_lookup(
                        &raw,
                        repo.get(),
                        target_oid,
                        GIT_OBJECT_COMMIT
                    );

                    if (ret != 0) {
                        return Err(make_error(ret));
                    }

                    target_obj = detail::ObjHandle(raw);
                }

                return perform_fast_forward(target_obj.get(), resolved_branch);
            }

            const git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

            {
                const i32 ret = git_merge(
                    repo.get(),
                    merge_heads.data(),
                    1,
                    &merge_opts,
                    &checkout_opts
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            const string merge_msg =
                "Merge remote-tracking branch '" + remote_ref_spec + "'";
            const git_oid* merge_head_oid =
                git_annotated_commit_id(annotated.get());

            const auto commit_result =
                create_merge_commit(merge_msg, merge_head_oid);
            if (!commit_result) {
                return Err(commit_result.error());
            }

            git_repository_state_cleanup(repo.get());
            return {};
        }

        [[nodiscard]] auto stash_push(const string_view message = "")
            -> Result<void> {
            detail::SigHandle sig;
            {
                git_signature* raw = nullptr;
                const i32 ret = git_signature_default(&raw, repo.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                sig = detail::SigHandle(raw);
            }

            const string msg_str = string(message);
            const char* msg_cstr = message.empty() ? nullptr : msg_str.c_str();

            git_oid stash_oid;
            const i32 ret = git_stash_save(
                &stash_oid,
                repo.get(),
                sig.get(),
                msg_cstr,
                GIT_STASH_DEFAULT
            );

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return {};
        }

        [[nodiscard]] auto stash_pop(const usize stash_index = 0)
            -> Result<void> {
            const git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
            const i32 ret = git_stash_pop(repo.get(), stash_index, &opts);
            if (ret != 0) {
                return Err(make_error(ret));
            }
            return {};
        }

        [[nodiscard]] auto stash_apply(const usize stash_index = 0)
            -> Result<void> {
            const git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
            const i32 ret = git_stash_apply(repo.get(), stash_index, &opts);

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return {};
        }

        [[nodiscard]] auto stash_drop(const usize stash_index = 0)
            -> Result<void> {
            const i32 ret = git_stash_drop(repo.get(), stash_index);

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return {};
        }

        [[nodiscard]] auto list_stashes() const -> Result<vector<StashEntry>> {
            vector<StashEntry> entries;

            const i32 ret = git_stash_foreach(
                repo.get(),
                [](const usize idx,
                   const char* const msg,
                   const git_oid* const stash_id,
                   void* const payload) -> i32 {
                auto* const out = static_cast<vector<StashEntry>*>(payload);

                array<char, GIT_OID_HEXSZ + 1> buf;
                git_oid_tostr(buf.data(), GIT_OID_HEXSZ + 1, stash_id);

                out->push_back(
                    StashEntry{
                        .message = (msg != nullptr) ? string(msg) : string(),
                        .oid = string(buf.data()),
                        .index = idx,
                    }
                );

                return 0;
            },
                &entries
            );

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return entries;
        }

        [[nodiscard]] auto list_stash_files(const usize stash_index) const
            -> Result<vector<string>> {
            const auto stashes_result = list_stashes();
            if (!stashes_result) {
                return Err(stashes_result.error());
            }

            const auto& stashes = stashes_result.value();
            if (stash_index >= stashes.size()) {
                return Err(
                    Error{ .message = "stash index out of range", .code = -1 }
                );
            }

            git_oid stash_oid;

            {
                const i32 ret = git_oid_fromstr(
                    &stash_oid,
                    stashes[stash_index].oid.c_str()
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            detail::CommitHandle stash_commit;

            {
                git_commit* raw = nullptr;
                const i32 ret = git_commit_lookup(&raw, repo.get(), &stash_oid);
                if (ret != 0) {
                    return Err(make_error(ret));
                }
                stash_commit = detail::CommitHandle(raw);
            }

            detail::TreeHandle stash_tree;

            {
                git_tree* raw = nullptr;
                const i32 ret = git_commit_tree(&raw, stash_commit.get());
                if (ret != 0) {
                    return Err(make_error(ret));
                }
                stash_tree = detail::TreeHandle(raw);
            }

            detail::CommitHandle parent_commit;
            detail::TreeHandle parent_tree;

            if (git_commit_parentcount(stash_commit.get()) != 0) {
                git_commit* raw = nullptr;
                const i32 ret = git_commit_parent(&raw, stash_commit.get(), 0);

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                parent_commit = detail::CommitHandle(raw);

                git_tree* tree_raw = nullptr;
                const i32 tree_ret =
                    git_commit_tree(&tree_raw, parent_commit.get());

                if (tree_ret != 0) {
                    return Err(make_error(tree_ret));
                }

                parent_tree = detail::TreeHandle(tree_raw);
            }

            detail::DiffHandle diff;
            {
                git_diff* raw = nullptr;
                const i32 ret = git_diff_tree_to_tree(
                    &raw,
                    repo.get(),
                    parent_tree.get(),
                    stash_tree.get(),
                    nullptr
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                diff = detail::DiffHandle(raw);
            }

            vector<string> file_paths;
            const i32 ret = git_diff_foreach(
                diff.get(),
                [](const git_diff_delta* const delta,
                   const f32 /*progress*/,
                   void* const payload) -> i32 {
                auto* out = static_cast<vector<string>*>(payload);

                if (delta->new_file.path != nullptr) {
                    out->emplace_back(delta->new_file.path);
                }

                return 0;
            },
                nullptr,
                nullptr,
                nullptr,
                &file_paths
            );

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return file_paths;
        }

        struct Branch {
            string name;
            git_branch_t type;
        };

        [[nodiscard]] auto change_branch(
            const string& branch,
            git_branch_t type = GIT_BRANCH_LOCAL
        ) -> Result<void> {
            git_reference* branch_ref;
            i32 result = git_branch_lookup(
                &branch_ref,
                repo.get(),
                branch.c_str(),
                GIT_BRANCH_LOCAL
            );

            if (result < 0) {
                // TODO
            }

            result = git_repository_set_head(
                repo.get(),
                git_reference_name(branch_ref)
            );

            if (result < 0) {
                // TODO
            }

            git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
            opts.checkout_strategy = GIT_CHECKOUT_SAFE;
            result = git_checkout_head(repo.get(), &opts);

            if (result < 0) {
                // TODO
            }

            git_reference_free(branch_ref);
            return {};
        };

        [[nodiscard]] auto list_branches(
            git_branch_t type = GIT_BRANCH_ALL
        ) const -> Result<vector<Branch>> {
            git_branch_iterator* iter;
            i32 result = git_branch_iterator_new(&iter, repo.get(), type);

            if (result != 0) {
                return Err(make_error(result));
            }

            vector<Branch> branches;
            git_reference* ref;

            while (true) {
                result = git_branch_next(&ref, &type, iter);

                if (result == GIT_ITEROVER) {
                    break;
                }

                if (result != 0) {
                    return Err(make_error(result));
                }

                const char* name = nullptr;
                result = git_branch_name(&name, ref);

                if (result == 0) {
                    branches.emplace_back(name, type);
                }
            }

            return branches;
        }

       private:
        explicit Repository(git_repository* const repo) : repo(repo) {}

        detail::RepoHandle repo;

        [[nodiscard]] auto collect_status_paths(const u32 flags) const
            -> Result<vector<string>> {
            git_status_options opts = GIT_STATUS_OPTIONS_INIT;
            opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
            opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                         GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS |
                         GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

            git_status_list* raw_list = nullptr;
            const i32 ret = git_status_list_new(&raw_list, repo.get(), &opts);

            if (ret != 0) {
                return Err(make_error(ret));
            }

            detail::GitHandle<git_status_list, git_status_list_free>
                status_list(raw_list);
            const usize count = git_status_list_entrycount(status_list.get());

            vector<string> paths;
            for (usize idx = 0; idx < count; idx++) {
                const git_status_entry* entry =
                    git_status_byindex(status_list.get(), idx);

                if (entry == nullptr) {
                    continue;
                }

                if ((entry->status & flags) == 0) {
                    continue;
                }

                const char* file_path = nullptr;

                if (entry->index_to_workdir != nullptr) {
                    file_path = entry->index_to_workdir->new_file.path;
                }

                if (file_path == nullptr && entry->head_to_index != nullptr) {
                    file_path = entry->head_to_index->new_file.path;
                }

                if (file_path != nullptr) {
                    paths.emplace_back(file_path);
                }
            }

            return paths;
        }

        [[nodiscard]] auto open_index() const -> Result<detail::IndexHandle> {
            git_index* raw = nullptr;
            const i32 ret = git_repository_index(&raw, repo.get());

            if (ret != 0) {
                return Err(make_error(ret));
            }

            detail::IndexHandle idx(raw);

            const i32 read_ret = git_index_read(idx.get(), 1);

            if (read_ret != 0) {
                return Err(make_error(read_ret));
            }

            return idx;
        }

        [[nodiscard]] auto get_current_branch_name() const -> Result<string> {
            detail::RefHandle head_ref;
            {
                git_reference* raw = nullptr;
                const i32 ret = git_repository_head(&raw, repo.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                head_ref = detail::RefHandle(raw);
            }

            const char* branch_cstr = nullptr;
            const i32 ret = git_branch_name(&branch_cstr, head_ref.get());

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return string(branch_cstr);
        }

        [[nodiscard]] auto get_head_ref() const -> Result<detail::RefHandle> {
            git_reference* raw = nullptr;
            const i32 ret = git_repository_head(&raw, repo.get());

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return detail::RefHandle(raw);
        }

        [[nodiscard]] auto get_remote_tracking_ref(
            const string_view remote_name,
            const string_view branch_name
        ) const -> Result<detail::RefHandle> {
            string resolved_branch;

            if (branch_name.empty()) {
                auto branch_result = get_current_branch_name();

                if (!branch_result) {
                    return Err(branch_result.error());
                }

                resolved_branch = std::move(branch_result.value());
            } else {
                resolved_branch = string(branch_name);
            }

            git_reference* raw = nullptr;
            const string ref_path =
                "refs/remotes/" + string(remote_name) + '/' + resolved_branch;
            const i32 ret =
                git_reference_lookup(&raw, repo.get(), ref_path.c_str());

            if (ret != 0) {
                return Err(make_error(ret));
            }

            return detail::RefHandle(raw);
        }

        [[nodiscard]] auto count_commits_between(
            const git_oid* const from_oid,
            const git_oid* const to_oid
        ) const -> Result<usize> {
            if (from_oid == nullptr || to_oid == nullptr) {
                return 0;
            }

            detail::RevwalkHandle walker;

            {
                git_revwalk* raw = nullptr;
                const i32 ret = git_revwalk_new(&raw, repo.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                walker = detail::RevwalkHandle(raw);
            }

            {
                const i32 ret = git_revwalk_push(walker.get(), to_oid);

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            {
                const i32 ret = git_revwalk_hide(walker.get(), from_oid);

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            usize count = 0;
            git_oid iter_oid;

            while (git_revwalk_next(&iter_oid, walker.get()) == 0) {
                count++;
            }

            return count;
        }

        [[nodiscard]] auto perform_fast_forward(
            const git_object* const target_obj,
            const string& target_branch
        ) -> Result<void> {
            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

            {
                const i32 ret =
                    git_checkout_tree(repo.get(), target_obj, &checkout_opts);

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            const string ref_path = "refs/heads/" + target_branch;
            const git_oid* new_oid = git_object_id(target_obj);

            detail::RefHandle branch_ref;

            {
                git_reference* raw = nullptr;
                const i32 ret =
                    git_reference_lookup(&raw, repo.get(), ref_path.c_str());

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                branch_ref = detail::RefHandle(raw);
            }

            detail::RefHandle updated_ref;

            {
                git_reference* raw = nullptr;
                const i32 ret = git_reference_set_target(
                    &raw,
                    branch_ref.get(),
                    new_oid,
                    "Fast-forward"
                );

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                updated_ref = detail::RefHandle(raw);
            }

            return {};
        }

        [[nodiscard]] auto create_merge_commit(
            const string& message,
            const git_oid* const merge_head_oid
        ) -> Result<string> {
            const auto idx_result = open_index();

            if (!idx_result) {
                return Err(idx_result.error());
            }

            const auto& idx = idx_result.value();

            if (git_index_has_conflicts(idx.get()) != 0) {
                return Err(
                    Error{
                        .message =
                            "merge conflicts must be resolved before committing",
                        .code = -1,
                    }
                );
            }

            git_oid tree_oid;

            {
                const i32 ret = git_index_write_tree(&tree_oid, idx.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }
            }

            detail::TreeHandle tree;

            {
                git_tree* raw = nullptr;
                const i32 ret = git_tree_lookup(&raw, repo.get(), &tree_oid);

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                tree = detail::TreeHandle(raw);
            }

            detail::SigHandle sig;

            {
                git_signature* raw = nullptr;
                const i32 ret = git_signature_default(&raw, repo.get());

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                sig = detail::SigHandle(raw);
            }

            git_oid head_oid;
            detail::CommitHandle head_commit;

            {
                const i32 ret =
                    git_reference_name_to_id(&head_oid, repo.get(), "HEAD");

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                git_commit* raw = nullptr;
                const i32 lookup_ret =
                    git_commit_lookup(&raw, repo.get(), &head_oid);

                if (lookup_ret != 0) {
                    return Err(make_error(lookup_ret));
                }

                head_commit = detail::CommitHandle(raw);
            }

            detail::CommitHandle merge_commit_obj;

            {
                git_commit* raw = nullptr;
                const i32 ret =
                    git_commit_lookup(&raw, repo.get(), merge_head_oid);

                if (ret != 0) {
                    return Err(make_error(ret));
                }

                merge_commit_obj = detail::CommitHandle(raw);
            }

            array<const git_commit*, 2> parents = { head_commit.get(),
                                                    merge_commit_obj.get() };

            git_oid new_commit_oid;
            const i32 ret = git_commit_create(
                &new_commit_oid,
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
                return Err(make_error(ret));
            }

            array<char, GIT_OID_HEXSZ + 1> oid_buf;
            git_oid_tostr(oid_buf.data(), GIT_OID_HEXSZ + 1, &new_commit_oid);
            return string(oid_buf.data());
        }
    };

    class ezgit2 final {
       public:
        ezgit2() { git_libgit2_init(); }

        ~ezgit2() { git_libgit2_shutdown(); }

        ezgit2(const ezgit2&) = delete;
        auto operator=(const ezgit2&) -> ezgit2& = delete;
        ezgit2(ezgit2&&) = delete;
        auto operator=(ezgit2&&) -> ezgit2& = delete;
    };
}  // namespace ezgit2
#endif