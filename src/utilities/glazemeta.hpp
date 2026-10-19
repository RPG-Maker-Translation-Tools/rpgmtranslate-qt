#pragma once

#include "Hasher.hpp"
#include "Utils.hpp"

#include <QDebug>
#include <QFile>

// Qt exports emit macro
#pragma push_macro("emit")
#undef emit
#include <glaze/glaze.hpp>
#pragma pop_macro("emit")

// glaze fills maps with `value[key]`, but `HashMap::operator[]` is lookup-only and silently yields nothing.
template <>
struct glz::meta<HashMap<FilenameArray, u64>> {
    static constexpr auto readHashes =
        [](HashMap<FilenameArray, u64>& out, const std::map<string, u64>& input, glz::context& ctx) -> void {
        out.reserve(input.size());

        for (const auto& [key, value] : input) {
            // Copying a longer or shorter name would write past the `FilenameArray`
            if (!fitsFilenameArray(key.size())) {
                ctx.error = glz::error_code::syntax_error;
                return;
            }

            FilenameArray filename{};
            copyFilenameArray(key.data(), key.size(), filename.data());
            out.insert({ filename, value });
        }
    };

    static constexpr auto writeHashes = [](const HashMap<FilenameArray, u64>& input) -> std::map<string, u64> {
        std::map<string, u64> out;

        for (const auto& [key, value] : input) {
            out.emplace(string(key.data()), value);
        }

        return out;
    };

    static constexpr auto value = glz::custom<readHashes, writeHashes>;
};

template <>
struct glz::meta<QString> {
    static constexpr auto value = glz::custom<[](QString& out, const string& input) -> void {
        out = QString::fromStdString(input);
    }, [](const QString& input) -> string { return input.toStdString(); }>;
};

// Persisted files must survive fields being added or removed between versions: unknown keys are skipped and missing
// ones keep their defaults. Use plain `glz::read_json` only where an unexpected key means malformed input.
template <typename T>
[[nodiscard]] auto readLenient(const string_view json) -> glz::expected<T, glz::error_ctx> {
    auto value = T{};
    const glz::error_ctx error = glz::read<glz::opts{ .error_on_unknown_keys = false }>(value, json);

    if (error) {
        return glz::unexpected(error);
    }

    return value;
}

// Reads `path` as `T`, falling back to `path.bak` (see `keepBackup`). An unusable file is kept as `path.corrupt`.
// `accept` rejects values that parse but this version can't use. Nothing usable: `nullopt`.
template <typename T, typename Accept>
[[nodiscard]] auto readJsonFile(const QString& path, Accept&& accept) -> optional<T> {
    const QString backupPath = path % BACKUP_SUFFIX;

    for (const QString* const candidate : { &path, &backupPath }) {
        auto file = QFile(*candidate);

        if (!file.open(QFile::ReadOnly)) {
            qWarning().noquote() << u"Failed to open %1: %2"_qsv.arg(*candidate, file.errorString());
            continue;
        }

        const QByteArray json = file.readAll();
        // Must not stay open: the file may be replaced below, and on Windows renaming over an open handle fails
        file.close();

        const auto view = string_view(json.data(), json.size());
        auto value = readLenient<T>(view);

        if (!value) {
            qWarning().noquote() << u"Parsing %1 failed: %2"_qsv.arg(
                *candidate,
                QString::fromStdString(glz::format_error(value.error(), view))
            );
            continue;
        }

        if (!accept(*value)) {
            qWarning().noquote() << u"%1 holds values that this version does not know."_qsv.arg(*candidate);
            continue;
        }

        if (candidate == &backupPath) {
            const QString corruptPath = path % CORRUPT_SUFFIX;
            QFile::remove(corruptPath);
            QFile::rename(path, corruptPath);
            QFile::copy(backupPath, path);

            qWarning().noquote() << u"Restored %1 from its backup. The unusable file is kept as %2."_qsv.arg(
                path,
                corruptPath
            );
        }

        return std::move(*value);
    }

    return nullopt;
}

template <typename T>
[[nodiscard]] auto readJsonFile(const QString& path) -> optional<T> {
    return readJsonFile<T>(path, [](const T&) -> bool { return true; });
}
