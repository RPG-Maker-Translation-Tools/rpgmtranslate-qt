#pragma once

#include "Aliases.hpp"
#include "BatchMenu.hpp"
#include "Enums.hpp"
#include "ProjectSettings.hpp"
#include "SearchMenu.hpp"
#include "SearchPanelDock.hpp"
#include "Settings.hpp"
#include "Types.hpp"
#include "rpgmtranslate.h"

#include <QObject>
#include <QThread>

struct FileLines {
    QString content;
    QSVList lines;
};

auto fileLines(
    QLatin1StringView filename,
    const HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings
) -> result<FileLines, QString>;

template <class F>
auto modifyFile(
    QLatin1StringView filename,
    HashMap<u16, QString>& mapSections,
    const shared_ptr<ProjectSettings>& projectSettings,
    F&& func
) -> bool;

class TaskWorker final : public QObject {
    Q_OBJECT

   public:
    enum class Task : u8 {
        Search,
        Replace,
        Put,
        BatchTrim,
        BatchTranslate,
        BatchWrap
    };

    explicit TaskWorker(QObject* parent = nullptr);
    ~TaskWorker() override;

    void start();
    void stop();

    void extractArchive(const QString& archivePath, const QString& folder);

    void read(
        const QString& projectPath,
        const QString& sourcePath,
        const QString& translationPath,
        ReadMode readMode,
        EngineType engineType,
        DuplicateMode duplicateMode,
        Selected selected,
        BaseFlags flags,
        bool mapEvents,
        ByteBuffer hashes
    );

    void write(const QString& gameTitle, Selected selected);

    void purge(const QString& gameTitle, Selected selected);

    void search(
        SearchMenu::Action action,
        Selected selected,
        const QString& searchText,
        SearchLocation searchLocation,
        i8 columnIndex,
        SearchFlags searchFlags,
        u16 tabCount
    );

    void performBatchAction(
        Selected selected,
        BatchAction action,
        u8 columnIndex,
        const std::variant<
            BatchMenu::TrimFlags,
            std::tuple<TranslationEndpoint, QString>,
            u8>& variant,
        const Glossary& glossary
    );

    void replace(
        HashMap<array<char, 13>, vector<CellMatch>> searchMatches,
        Selected selected,
        SearchMenu::Action action,
        const QString& searchText,
        const QString& replaceText,
        SearchLocation searchLocation,
        i8 columnIndex,
        SearchFlags searchFlags
    );

    void translateSingle(
        const QString& filename,
        const QString& text,
        const Glossary& glossary
    );

    void replaceSingle(
        const QString& replaceText,
        SearchPanelDock::Action action,
        const QString& filename,
        i32 rowIndex,
        u8 columnIndex,
        span<const TextMatch> matches
    );

    void init(
        const shared_ptr<Settings>& settings,
        const shared_ptr<ProjectSettings>& projectSettings,
        HashMap<u16, QString>* const mapSections
    ) {
        this->settings = settings;
        this->projectSettings = projectSettings;
        this->mapSections = mapSections;
    };

   signals:
    void lockFile(const QString& file);
    void message(const QString& message);
    void progressChanged(Task task, u32 progress, u32 total);

    void readFinished(std::tuple<FFIString, ByteBuffer> results);
    void extractFinished(FFIString error);
    void writeFinished(std::tuple<FFIString, f32> results);
    void purgeFinished(FFIString error);
    void searchFinished(HashMap<array<char, 13>, vector<CellMatch>> results);
    void singleTranslateFinished(
        const array<QString, TRANSLATION_ENDPOINT_COUNT>& translations
    );
    void singleReplaceFinished(const std::tuple<QString, TextMatch*>& results);
    void translateFinished(
        result<std::tuple<ByteBuffer, ByteBuffer>, FFIString> results
    );

   private:
    QThread thread;

    shared_ptr<Settings> settings;
    shared_ptr<ProjectSettings> projectSettings;

    HashMap<u16, QString>* mapSections;
};