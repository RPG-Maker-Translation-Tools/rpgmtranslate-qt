#pragma once

#include <QByteArray>
#include <QString>
#include <archive.h>
#include <archive_entry.h>
#include <utility>
#include <vector>

// The libarchive calls that write the entries of a backup, replaceable by a test that needs one of them to fail
struct ArchiveWriter {
    int (*writeHeader)(archive*, archive_entry*) = archive_write_header;
    la_ssize_t (*writeData)(archive*, const void*, size_t) = archive_write_data;
    int (*close)(archive*) = archive_write_close;
};

// Compresses `files` into the `.tar.xz` at `archivePath`, writing it under a temporary name and renaming it at the
// end, so a failed write never leaves a broken archive among the backups and keeps the one that was there
[[nodiscard]] auto writeBackupArchive(
    const QString& archivePath,
    const std::vector<std::pair<QString, QByteArray>>& files,
    const ArchiveWriter& writer = {}
) -> bool;
