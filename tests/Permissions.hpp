#pragma once

#include <QFile>
#include <QTemporaryFile>

// Root (and some file systems) read what a file refuses, which leaves the failures below out of reach
[[nodiscard]] inline auto permissionsAreIgnored() -> bool {
    QTemporaryFile probe;

    if (!probe.open()) {
        return true;
    }

    QFile::setPermissions(probe.fileName(), QFileDevice::Permissions());
    return QFile(probe.fileName()).open(QFile::ReadOnly);
}

// Kept under the name the refusal tests use: a refusal that does not bind (root, Windows read-only attributes, wine)
// leaves them out of reach in the same way
[[nodiscard]] inline auto runsAsRoot() -> bool {
    static const bool ignored = permissionsAreIgnored();
    return ignored;
}
