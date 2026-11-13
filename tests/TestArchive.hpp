#pragma once

#include "Aliases.hpp"

#include <QByteArray>
#include <archive.h>
#include <archive_entry.h>
#include <set>
#include <string>
#include <vector>

struct Entry {
    string name;
    string content;
};

// An archive of the kind the release workflow publishes: 7z on Windows, tar.xz elsewhere
[[nodiscard]] inline auto makeArchive(const vector<Entry>& entries) -> QByteArray {
    QByteArray buffer(1 << 20, '\0');
    usize used = 0;

    archive* const out = archive_write_new();
#ifdef Q_OS_WINDOWS
    archive_write_set_format_7zip(out);
#else
    archive_write_set_format_pax_restricted(out);
    archive_write_add_filter_xz(out);
#endif
    archive_write_open_memory(out, buffer.data(), static_cast<usize>(buffer.size()), &used);

    // Like the release archives, folders come before what they hold
    std::set<string> folders;

    for (const Entry& item : entries) {
        for (usize slash = item.name.find('/'); slash != string::npos; slash = item.name.find('/', slash + 1)) {
            const string folder = item.name.substr(0, slash + 1);

            if (!folders.insert(folder).second) {
                continue;
            }

            archive_entry* const dirEntry = archive_entry_new();
            archive_entry_set_pathname(dirEntry, folder.c_str());
            archive_entry_set_filetype(dirEntry, AE_IFDIR);
            archive_entry_set_perm(dirEntry, 0755);
            archive_write_header(out, dirEntry);
            archive_entry_free(dirEntry);
        }

        archive_entry* const entry = archive_entry_new();
        archive_entry_set_pathname(entry, item.name.c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, static_cast<la_int64_t>(item.content.size()));
        archive_write_header(out, entry);
        archive_write_data(out, item.content.data(), item.content.size());
        archive_entry_free(entry);
    }

    archive_write_close(out);
    archive_write_free(out);
    buffer.resize(static_cast<qsizetype>(used));
    return buffer;
}
