#include "BatchMenu.hpp"
#include "BookmarkMenu.hpp"
#include "Constants.hpp"
#include "GlossaryMenu.hpp"
#include "LintMenu.hpp"
#include "MainWindow.hpp"
#include "Notice.hpp"
#include "TranslationTableModel.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"
#include "ui_MainWindow.h"

#include <QDir>
#include <QSaveFile>
#include <QStyleHints>
#include <QUuid>
#include <QtConcurrent>
#include <archive.h>
#include <archive_entry.h>

namespace {
constexpr i32 ARCHIVE_ENTRY_PERM = 0644;
constexpr QStringView SETTINGS_PATH = u"/settings.json";

// LanguageTool has one shared API key (not per-endpoint), so it gets a fixed keyring account.
constexpr QByteArrayView LANGUAGETOOL_CREDENTIAL_ACCOUNT = "languagetool";

// Compresses on a worker thread; level 6 is much lighter on memory than 9 for a size that differs little on text.
// Written under a temporary name and renamed at the end, so a failed or interrupted write never leaves a broken
// archive among the backups.
[[nodiscard]] auto writeBackupArchive(const QString& archivePath, const vector<pair<QString, QByteArray>>& files)
    -> bool {
    const QString partPath = archivePath + u".part"_qsv;

    struct archive* arch = archive_write_new();
    archive_write_add_filter_xz(arch);
    archive_write_set_format_pax_restricted(arch);

    archive_write_set_filter_option(arch, "xz", "compression-level", "6");

#ifdef Q_OS_WINDOWS
    // The narrow variant takes the ANSI code page, which mangles a path that holds other characters
    const i32 openResult = archive_write_open_filename_w(arch, partPath.toStdWString().c_str());
#else
    const i32 openResult = archive_write_open_filename(arch, partPath.toStdString().c_str());
#endif

    if (openResult != ARCHIVE_OK) {
        qWarning().noquote() << u"Failed to open archive %1: %2"_qsv.arg(
            partPath,
            svtostr(QUtf8SV(archive_error_string(arch)))
        );
        archive_write_free(arch);
        return false;
    }

    bool success = true;

    for (const auto& [name, buffer] : files) {
        archive_entry* const aentry = archive_entry_new();

        archive_entry_set_pathname_utf8(aentry, name.toUtf8().constData());
        archive_entry_set_filetype(aentry, AE_IFREG);
        archive_entry_set_perm(aentry, ARCHIVE_ENTRY_PERM);
        archive_entry_set_size(aentry, buffer.size());

        if (archive_write_header(arch, aentry) != ARCHIVE_OK) {
            qWarning().noquote() << u"Failed to write archive header of %1"_qsv.arg(name);
            success = false;
        } else if (archive_write_data(arch, buffer.constData(), buffer.size()) != buffer.size()) {
            qWarning().noquote() << u"Failed to write %1 to the backup"_qsv.arg(name);
            success = false;
        }

        archive_entry_free(aentry);

        if (!success) {
            break;
        }
    }

    // Closing flushes the compressor, which is where a full disk shows up
    success = archive_write_close(arch) == ARCHIVE_OK && success;
    archive_write_free(arch);

    if (success) {
        QFile::remove(archivePath);
        success = QFile::rename(partPath, archivePath);
    }

    if (!success) {
        QFile::remove(partPath);
    }

    return success;
}
}  // namespace

auto MainWindow::saveCurrentTab(QString tabName) -> bool {
    if (tabName.isEmpty()) {
        tabName = ui->tabPanel->currentTabName();

        if (tabName.isEmpty()) {
            return true;
        }
    }

    const bool isMap = tabName.startsWith(u"map");

    // A map is built apart and swapped into `mapSections` at once, as the workers read it
    QString mapText;
    unique_ptr<QSaveFile> file;
    unique_ptr<QTextStream> stream;

    if (!isMap) {
        QString filePath = projectSettings->translationPath() % u'/' % tabName % u".txt";

        const QString suffix = u'/' % tabName % u".txt";
        auto opened = openForWriting(this, filePath, suffix);

        if (!opened) {
            return false;
        }

        file = std::move(opened.value());

        if (file == nullptr) {
            return true;
        }

        stream = make_unique<QTextStream>(file.get());
        stream->setEncoding(QStringConverter::Utf8);
    } else {
        stream = make_unique<QTextStream>(&mapText);
    }

    TranslationTableModel* const model = ui->translationTable->model();

    for (i32 row = 0; row < model->rowCount(); row++) {
        if ((model->flags(model->index(row, 1)) & Qt::ItemIsEditable) == 0) {
            *stream << *model->item(row, 0);
        } else {
            auto fields = QStringList(model->columnCount());

            for (i32 column = 0; column < model->columnCount(); column++) {
                const QString* item = model->item(row, column);

                if (item->isNull()) {
                    qWarning().noquote() << u"Item at row %1 and column %2 is nullptr."_qsv.arg(
                        qsv(qitos(row)),
                        qsv(qitos(column))
                    );
                    continue;
                }

                fields[column] = qsvReplace(QStringView(item->data(), item->size()), LINE_FEED, NEW_LINE);
            }

            *stream << fields.join(SEPARATOR);
        }

        *stream << '\n';
    }

    // Only a tab that had a title on load gets one back: otherwise this would add a junk line to the file on every save
    if (tabName == gameTitleTab) {
        const QString placeholder = ui->gameTitleInput->placeholderText();
        const QString text = ui->gameTitleInput->text();

        *stream << placeholder;
        *stream << SEPARATOR;

        if (text != placeholder) {
            *stream << text;
        }
    }

    stream->flush();

    if (isMap) {
        const std::scoped_lock lock(mapSectionsMutex);

        // Always there: a map tab exists only for a section that `openProject` put in `mapSections`
        mapSections.find(stoa<u16>(QStringView(tabName).sliced(3)).value_or(0))->second = std::move(mapText);
    } else if (!commitFile(this, *file)) {
        return false;
    }

    present(this, NOTICE("Tab %1 saved.", Info, Status, tabName));

    return true;
}

auto MainWindow::saveMaps() -> bool {
    QString mapsPath = projectSettings->translationPath() + u"/maps.txt";

    auto opened = openForWriting(this, mapsPath, u"/maps.txt");

    if (!opened) {
        return false;
    }

    auto mapsFile = std::move(opened.value());

    if (mapsFile == nullptr) {
        return true;
    }

    auto stream = QTextStream(mapsFile.get());

    {
        const std::scoped_lock lock(mapSectionsMutex);

        auto keys = ranges::to<vector>(views::keys(mapSections));
        ranges::sort(keys);

        for (const auto key : keys) {
            stream << mapSections[key];
        }
    }

    stream.flush();

    if (!commitFile(this, *mapsFile)) {
        return false;
    }

    present(this, NOTICE("maps.txt saved.", Info, Status));

    return true;
}

auto MainWindow::saveTranslations() -> bool {
    return saveCurrentTab() && saveMaps();
}

void MainWindow::saveBackup() {
    // The previous one is still being compressed
    if (backupFuture.isRunning()) {
        return;
    }

    if (!saveTranslations()) {
        return;
    }

    const QString backupPath = projectSettings->backupPath();

    // Left by a write that a crash interrupted; the previous write is over, so none is in progress
    for (const QFileInfo& stale : QDir(backupPath).entryInfoList({ u"*.tar.xz.part"_s }, QDir::Files)) {
        QFile::remove(stale.filePath());
    }

    const auto date = QDate::currentDate();
    const auto time = QTime::currentTime();

    const QString archivePath = u"%1/%2-%3-%4_%5-%6-%7.tar.xz"_qsv.arg(
        backupPath,
        qsv(qitos(date.day(), 2, '0')),
        qsv(qitos(date.month(), 2, '0')),
        qsv(qitos(date.year())),
        qsv(qitos(time.hour(), 2, '0')),
        qsv(qitos(time.minute(), 2, '0')),
        qsv(qitos(time.second(), 2, '0'))
    );

    // The files are read here, as they are not touched by anything meanwhile; compressing them is what takes time
    vector<pair<QString, QByteArray>> files;

    auto listing =
        QDirListing(projectSettings->translationPath(), { u"*.txt"_s }, QDirListing::IteratorFlag::FilesOnly);

    for (const auto& entry : listing) {
        const QString filePath = entry.filePath();

        // Read first: the header carries the size, and a file that can't be read must not leave an entry that is
        // announced but empty
        auto file = QFile(filePath);

        if (!file.open(QFile::ReadOnly)) {
            qWarning().noquote() << u"Failed to open file %1: %2"_qsv.arg(filePath, file.errorString());
            continue;
        }

        files.emplace_back(entry.fileName(), file.readAll());
    }

    saveGlossary();
    saveProjectSettings();

    const isize maxBackups = settings->core.backup.max;

    backupFuture = QtConcurrent::run([archivePath, files = std::move(files)] -> auto {
        return writeBackupArchive(archivePath, files);
    });

    backupFuture.then(this, [this, archivePath, backupPath, maxBackups](const bool written) -> void {
        if (!written) {
            present(this, NOTICE("Failed to create the backup %1.", Warning, Status, lastPathComponent(archivePath)));
            return;
        }

        // The earlier archives, the oldest first, are dropped only now that the new one is there
        const QList<QFileInfo> backups =
            QDir(backupPath).entryInfoList({ u"*.tar.xz"_s }, QDir::Files, QDir::Time | QDir::Reversed);

        for (isize idx = 0; idx < backups.size() - maxBackups; idx++) {
            if (!QFile::remove(backups[idx].filePath())) {
                qWarning().noquote() << u"Failed to remove the old backup %1"_qsv.arg(backups[idx].filePath());
            }
        }

        present(this, NOTICE("Backup %1 created.", Info, Status, lastPathComponent(archivePath)));
    });
}

auto MainWindow::saveSettings() -> bool {
    QString path = dataLocation() + SETTINGS_PATH;

    auto opened = openForWriting(this, path, SETTINGS_PATH);

    if (!opened) {
        return false;
    }

    auto settingsFile = std::move(opened.value());

    if (settingsFile == nullptr) {
        return true;
    }

    // API keys are moved into the OS credential store and stripped from the written copy. A key the store refuses
    // (no keyring service...) stays in the file, since losing it silently would be worse.
    auto redacted = *settings;
    bool storeFailed = false;
    bool deleteFailed = false;

    for (auto& endpoint : redacted.translation.endpoints) {
        // A key the user cleared must not come back from the store at the next start
        if (endpoint.apiKey.isEmpty()) {
            if (!rpgm_delete_credential(strtoffi(endpoint.id.toUtf8()))) {
                deleteFailed = true;
            }
            continue;
        }

        if (rpgm_store_credential(strtoffi(endpoint.id.toUtf8()), strtoffi(endpoint.apiKey.toUtf8()))) {
            endpoint.apiKey.clear();
        } else {
            storeFailed = true;
        }
    }

    if (redacted.translation.languageTool.apiKey.isEmpty()) {
        if (!rpgm_delete_credential(strtoffi(LANGUAGETOOL_CREDENTIAL_ACCOUNT))) {
            deleteFailed = true;
        }
    } else {
        if (rpgm_store_credential(
                strtoffi(LANGUAGETOOL_CREDENTIAL_ACCOUNT),
                strtoffi(redacted.translation.languageTool.apiKey.toUtf8())
            )) {
            redacted.translation.languageTool.apiKey.clear();
        } else {
            storeFailed = true;
        }
    }

    if (storeFailed) {
        present(
            this,
            NOTICE(
                "The system credential store refused the API keys: %1. They are saved in settings.json instead.",
                Warning,
                Status,
                ffitostr(rpgm_error())
            )
        );
    }

    if (deleteFailed) {
        present(
            this,
            NOTICE(
                "The system credential store refused to delete an API key: %1. The key may come back at the next start.",
                Warning,
                Status,
                ffitostr(rpgm_error())
            )
        );
    }

    const auto result = glz::write_json(redacted);

    if (!result) {
        present(
            this,
            NOTICE(
                "Serializing settings failed: %1",
                Warning,
                Modal,
                QString::fromStdString(glz::format_error(result.error()))
            )
        );
        return false;
    }

    keepBackup(path);
    settingsFile->write(result->data(), scast<i64>(result->size()));
    return commitFile(this, *settingsFile);
}

auto MainWindow::saveGlossary() -> bool {
    QString path = projectSettings->glossaryPath();

    auto opened = openForWriting(this, path, GLOSSARY_FILE);

    if (!opened) {
        return false;
    }

    auto glossaryFile = std::move(opened.value());

    if (glossaryFile == nullptr) {
        return true;
    }

    const auto result = glz::write_json(glossaryMenu->glossary());

    if (!result) {
        present(
            this,
            NOTICE(
                "Serializing glossary failed: %1",
                Warning,
                Modal,
                QString::fromStdString(glz::format_error(result.error()))
            )
        );
        return false;
    }

    keepBackup(path);
    glossaryFile->write(result->data(), scast<i64>(result->size()));
    return commitFile(this, *glossaryFile);
}

auto MainWindow::saveProjectSettings() -> bool {
    QString path = projectSettings->projectSettingsPath();

    auto opened = openForWriting(this, path, PROJECT_SETTINGS_FILE);

    if (!opened) {
        return false;
    }

    if (const auto& projectSettingsFile = opened.value(); projectSettingsFile != nullptr) {
        const auto result = glz::write_json(projectSettings);

        if (!result) {
            present(
                this,
                NOTICE(
                    "Serializing project settings failed: %1",
                    Warning,
                    Modal,
                    QString::fromStdString(glz::format_error(result.error()))
                )
            );
            return false;
        }

        keepBackup(path);
        projectSettingsFile->write(result->data(), scast<i64>(result->size()));

        if (!commitFile(this, *projectSettingsFile)) {
            return false;
        }
    }

    QString metadataPath = projectSettings->translationPath() + RVPACKER_METADATA_FILE;

    auto openedMetadata = openForWriting(this, metadataPath, RVPACKER_METADATA_FILE);

    if (!openedMetadata) {
        return false;
    }

    auto metadataFile = std::move(openedMetadata.value());

    if (metadataFile == nullptr) {
        return true;
    }

    auto metadata =
        Metadata{ .hashes = projectSettings->hashes, .duplicateMode = scast<u8>(projectSettings->duplicateMode) };

    if (!projectSettings->readEncoding.isEmpty()) {
        metadata.readEncoding = projectSettings->readEncoding.toStdString();
    }

    string json;

    if (glz::write<glz::opts{ .skip_null_members = false }>(metadata, json)) {
        present(this, NOTICE("Serializing project metadata failed.", Warning, Modal));
        return false;
    }

    metadataFile->write(json.data(), scast<qint64>(json.size()));

    return commitFile(this, *metadataFile);
}

auto MainWindow::flushTranslations() -> bool {
    // Saves the tab and empties the table, so that nothing of it is saved later. The tab stays open if saving failed.
    ui->tabPanel->changeTab(QString());

    // Maps go after the tab: a map tab is saved into `mapSections`
    return ui->tabPanel->currentTabName().isEmpty() && saveMaps();
}

auto MainWindow::saveEverything(const bool skipTranslations) -> bool {
    if (!saveSettings()) {
        return false;
    }

    if (projectSettings != nullptr) {
        if (!skipTranslations && (!saveCurrentTab() || !saveMaps())) {
            return false;
        }

        if (!saveProjectSettings()) {
            return false;
        }

        if (!saveGlossary()) {
            return false;
        }

        if (!batchMenu->save()) {
            return false;
        }
    }

    return true;
}

void MainWindow::loadBackup(const QString& backupPath) {
    auto file = QFile(backupPath);

    if (!file.open(QFile::ReadOnly)) {
        present(this, NOTICE("Opening backup %1 failed: %2", Warning, Modal, backupPath, file.errorString()));
        return;
    }

    QByteArray content = file.readAll();

    archive* const arch = archive_read_new();
    archive_read_support_format_tar(arch);
    archive_read_support_filter_xz(arch);

    if (archive_read_open_memory(arch, content.data(), content.size()) != ARCHIVE_OK) {
        present(this, NOTICE("Reading archive failed with %1", Warning, Modal, QUtf8SV(archive_error_string(arch))));
        archive_read_free(arch);
        return;
    }

    archive_entry* entry = nullptr;
    i32 headerResult;

    while ((headerResult = archive_read_next_header(arch, &entry)) == ARCHIVE_OK) {
        const char* const pathStr = archive_entry_pathname(entry);

        const QString name = QString::fromUtf8(pathStr);

        // Backups hold flat files only (see saveBackup); a name with a separator or `..` could escape the translation
        // directory, and anything but a regular file would be written out as an empty file
        if (archive_entry_filetype(entry) != AE_IFREG || name.contains(u'/') || name.contains(u'\\') ||
            name == u".."_qsv || name == u"."_qsv) {
            qWarning().noquote() << u"Skipping backup entry with an unsafe name: %1"_qsv.arg(name);
            archive_read_data_skip(arch);
            continue;
        }

        const QString path = projectSettings->translationPath() % u'/' % name;

        // The entry is read completely before the destination is touched, and QSaveFile only replaces the existing
        // file on commit - a corrupt archive must never destroy the translation it was meant to restore.
        const i64 size = archive_entry_size(entry);
        auto data = QByteArray(size, Qt::Uninitialized);

        if (archive_read_data(arch, data.data(), size) != size) {
            present(
                this,
                NOTICE(
                    "Reading entry %1 failed: %2",
                    Warning,
                    Modal,
                    name,
                    QUtf8SV(archive_error_string(arch) != nullptr ? archive_error_string(arch) : "")
                )
            );
            continue;
        }

        auto entryFile = QSaveFile(path);
        entryFile.setDirectWriteFallback(true);

        if (!entryFile.open(QFile::WriteOnly)) {
            present(this, NOTICE("Writing entry %1 failed: %2", Warning, Modal, path, entryFile.errorString()));
            archive_read_data_skip(arch);
            continue;
        }

        entryFile.write(data);

        if (!entryFile.commit()) {
            present(this, NOTICE("Writing entry %1 failed: %2", Warning, Modal, path, entryFile.errorString()));
        }
    }

    // Not the end of the archive: it is damaged, and what came before the damage is all that was restored
    if (headerResult != ARCHIVE_EOF) {
        present(
            this,
            NOTICE(
                "The backup is damaged, so it was restored only in part: %1",
                Warning,
                Modal,
                QUtf8SV(archive_error_string(arch) != nullptr ? archive_error_string(arch) : "")
            )
        );
    }

    archive_read_close(arch);
    archive_read_free(arch);
}

void MainWindow::loadSettings() {
    qApp->setStyle(settings->appearance.style);
    qApp->styleHints()->setColorScheme(settings->appearance.theme);

    QFont tableFont = font();

    if (!settings->appearance.translationTableFont.isEmpty()) {
        tableFont.setFamily(settings->appearance.translationTableFont);
    }

    if (settings->appearance.translationTableFontSize != 0) {
        tableFont.setPointSize(settings->appearance.translationTableFontSize);
    }

    ui->translationTable->setFont(tableFont);

    ui->tabPanel->setProgressDisplay(settings->appearance.displayPercents);
    bookmarkMenu->setShowEntryNames(settings->appearance.displayEntryNames);
    ui->lintMenu->settingsChanged();
    updateCounts();

    // The backup settings can change while the project is open, and the timer got them when it opened
    if (projectSettings != nullptr) {
        if (settings->core.backup.enabled) {
            backupTimer.start(settings->core.backup.period * SECOND_MS);
        } else {
            backupTimer.stop();
        }
    }

    actionTabPanel->setShortcut(settings->controls.tabPanel);
    actionSearchPanel->setShortcut(settings->controls.searchPanel);
    actionGoToRow->setShortcut(settings->controls.goToRow);
    actionBatchMenu->setShortcut(settings->controls.batchMenu);
    actionBookmarkMenu->setShortcut(settings->controls.bookmarkMenu);
    actionLintMenu->setShortcut(settings->controls.lintMenu);
    actionGlossaryMenu->setShortcut(settings->controls.glossaryMenu);
    actionTranslationsMenu->setShortcut(settings->controls.translationsMenu);
    actionSingleTrim->setShortcut(settings->controls.singleTrim);
    actionSingleWrap->setShortcut(settings->controls.singleWrap);
    actionSingleLatinize->setShortcut(settings->controls.singleLatinize);

#ifdef ENABLE_NUSPELL
    if (projectSettings != nullptr) {
        if (const auto dictionary = initDictionary(); !dictionary) {
            present(this, NOTICE("Failed to load the spellcheck dictionary: %1", Warning, Modal, dictionary.error()));
        }
    }
#endif

    retranslate(settings->appearance.language);
}

void MainWindow::initializeSettings() {
    // Falls back to the backup every save keeps, and keeps an unusable file aside instead of overwriting it
    if (auto loaded = readJsonFile<Settings>(dataLocation() + SETTINGS_PATH)) {
        settings = make_unique<Settings>(std::move(*loaded));
    } else {
        settings = make_unique<Settings>();
    }

    // Up to v1.1.x the Translations menu defaulted to Ctrl+S, which Save also uses, so neither shortcut worked
    if (settings->controls.translationsMenu == u"Ctrl+S"_qsv) {
        settings->controls.translationsMenu = ControlSettings{}.translationsMenu;
        qInfo().noquote() << u"Reset the Translations menu hotkey, which clashed with Save, to %1."_qsv.arg(
            settings->controls.translationsMenu
        );
    }

    // Endpoints saved by v1.1.x with an LLM backend (indices past DeepL in the old enum) no longer exist. `Script`
    // reuses one of those indices but always has a path, so a pathless one is a leftover as well.
    const usize endpointCount = settings->translation.endpoints.size();

    std::erase_if(settings->translation.endpoints, [](const EndpointSettings& endpoint) -> bool {
        const bool unsupported = endpoint.endpoint > TranslationEndpoint::Script ||
                                 (endpoint.endpoint == TranslationEndpoint::Script && endpoint.scriptPath.isEmpty());

        // Its API key would stay in the credential store with nothing left to use or delete it
        if (unsupported && !endpoint.id.isEmpty() && !rpgm_delete_credential(strtoffi(endpoint.id.toUtf8()))) {
            qWarning().noquote() << u"Failed to delete the API key of an unsupported endpoint: %1"_qsv.arg(
                svtostr(ffitostr(rpgm_error()))
            );
        }

        return unsupported;
    });

    if (settings->translation.endpoints.size() != endpointCount) {
        qInfo().noquote() << u"Removed %1 translation endpoint(s) that are no longer supported."_qsv.arg(
            qsv(qitos(endpointCount - settings->translation.endpoints.size()))
        );
    }

    // API keys live in the OS credential store, not settings.json - pull them back in here.
    // A missing entry means the key was never saved to the store; keep the `apiKey` parsed from JSON as the fallback.
    for (auto& endpoint : settings->translation.endpoints) {
        if (endpoint.id.isEmpty()) {
            endpoint.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }

        FFIString storedKey;

        if (rpgm_get_credential(strtoffi(endpoint.id.toUtf8()), &storedKey)) {
            endpoint.apiKey = QString::fromUtf8(storedKey.ptr, storedKey.len);
            rpgm_string_free(storedKey);
        }
    }

    FFIString storedLtKey;

    if (rpgm_get_credential(strtoffi(LANGUAGETOOL_CREDENTIAL_ACCOUNT), &storedLtKey)) {
        settings->translation.languageTool.apiKey = QString::fromUtf8(storedLtKey.ptr, storedLtKey.len);
        rpgm_string_free(storedLtKey);
    }

    loadSettings();
}