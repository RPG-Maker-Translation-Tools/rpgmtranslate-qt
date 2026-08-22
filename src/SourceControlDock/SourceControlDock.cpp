#ifdef ENABLE_LIBGIT2
#include "SourceControlDock.hpp"

#include "GitCommitList.hpp"
#include "GitFileList.hpp"
#include "Notice.hpp"
#include "Settings.hpp"
#include "ezgit2.hpp"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QDirIterator>
#include <QLayout>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>

namespace {
constexpr QL1SV GITIGNORE_CONTENT = "/backups\n/baseline-data\n/img/source\n"_L1;

void logErr(QWidget* const parent, const ezgit2::Error& err) {
    present(
        parent,
        Notice(
            NOTICE_IN("SourceControlDock", "libgit2 returned error code %1: %2"),
            Severity::Warning,
            Delivery::Modal,
            { itos(err.code).qsv(), err.message }
        )
    );
}
}  // namespace

void SourceControlDock::init(const Ui::MainWindow& ui_) {
    sourceControlDockContent = ui_.sourceControlDockContent;

    branchSelect = ui_.branchSelect;
    fileList = ui_.gitFileList;
    commitList = ui_.commitList;

    commitButton = ui_.commitButton;
    commitOptionsButton = ui_.commitOptionsButton;
    commitMessageInput = ui_.commitMessageInput;

    copyTranslationButton = ui_.copyTranslationButton;
    refreshChangesButton = ui_.refreshChangesButton;

    connect(fileList, &GitFileList::staged, this, [this](const QString& path) -> void {
        const auto& items = fileList->items();
        const auto item = ranges::find_if(items, [&path](const GitFileItem& item) -> bool { return item.filePath == path; });

        if (item == items.end()) {
            return;
        }

        const auto result = item->staged ? repo.stage({ path.toStdString() }) : repo.unstage({ path.toStdString() });

        if (!result) {
            logErr(this, result.error());
        }
    });

    connect(branchSelect, &QComboBox::currentTextChanged, this, [this](const QString& branch) -> void {
        const auto res = repo.changeBranch(branch.toStdString());

        if (!res) {
            logErr(this, res.error());
        }
    });

    connect(commitButton, &QPushButton::pressed, this, [this] -> void { commit(); });

    connect(commitOptionsButton, &QToolButton::pressed, this, [this] -> void {
        QMenu menu;

        const QAction* const commitAmendAction = menu.addAction(tr("Commit (Amend)"));
        const QAction* const commitPushAction = menu.addAction(tr("Commit and push"));

        const QAction* const selectedAction =
            menu.exec(commitOptionsButton->mapToGlobal(QPoint(0, commitOptionsButton->height())));

        if (selectedAction == commitAmendAction) {
            commit(true);
        } else if (selectedAction == commitPushAction) {
            commit(false, true);
        }
    });

    connect(copyTranslationButton, &QPushButton::pressed, this, [this] -> void {
        const QString workdir = QString::fromUtf8(repo.workdir());

        auto dir = QDir(workdir);
        dir.cdUp();

        const QByteArray src = (workdir % u"/translation"_qsv).toUtf8();
        const QByteArray dest = (dir.path() % u"/translation"_qsv).toUtf8();

        fs::copy(
            path(string_view(src.data(), src.size())),
            path(string_view(dest.data(), dest.size())),
            fs::copy_options::recursive | fs::copy_options::overwrite_existing
        );
    });

    connect(refreshChangesButton, &QToolButton::pressed, this, [this] -> void {
        fileList->clear();
        const auto filesStatus = repo.listFiles();

        if (filesStatus) {
            for (const auto& status : filesStatus.value()) {
                fileList->appendItem(
                    GitFileItem{ .filePath = QString::fromUtf8(status.path), .status = status.status }
                );
            }
        } else {
            logErr(this, filesStatus.error());
        }
    });
}

void SourceControlDock::loadUI(const GitSettings& gitSettings) {
    updateSettings(gitSettings);

    const auto filesStatus = repo.listFiles();

    if (filesStatus) {
        for (const auto& status : filesStatus.value()) {
            fileList->appendItem(GitFileItem{ .filePath = QString::fromUtf8(status.path), .status = status.status });
        }
    } else {
        logErr(this, filesStatus.error());
    }

    const auto branches = repo.listBranches();

    if (branches) {
        for (const auto& branch : branches.value()) {
            branchSelect->addItem(
                QIcon(branch.type == GIT_BRANCH_LOCAL ? u":/icons/home.svg"_s : u":/icons/cloud.svg"_s),
                QString::fromUtf8(branch.name)
            );
        }
    } else {
        logErr(this, branches.error());
    }

    const auto commitHistory = repo.commitHistory();

    if (commitHistory) {
        for (const auto& commit : repo.commitHistory().value()) {
            commitList->appendCommit(
                { .hash = QString::fromLatin1(commit.oid),
                  .message = QString::fromUtf8(commit.message),
                  .author = QString::fromUtf8(commit.authorName),
                  .isRemote = false }
            );
        }
    } else {
        logErr(this, commitHistory.error());
    }
}

auto SourceControlDock::openRepository(const QString& dataPath, const GitSettings& gitSettings)
    -> result<void, QString> {
    clear();
    sourceControlDockContent->show();

    const string dataPathStd = dataPath.toStdString();
    auto result = repo.open(dataPathStd);

    if (!result) {
        const auto& error = result.error();

        if (error.code == GIT_ENOTFOUND) {
            for (auto* const child : sourceControlDockContent->findChildren<QWidget*>()) {
                child->hide();
            }

            auto* const createRepositoryButton = new QPushButton(tr("Create Repository"), sourceControlDockContent);

            sourceControlDockContent->layout()->addWidget(createRepositoryButton);

            connect(
                createRepositoryButton,
                &QPushButton::pressed,
                this,
                [this, createRepositoryButton, dataPathStd, dataPath, &gitSettings] -> void {
                auto gitignoreFile = QFile(dataPath % u"/.gitignore");

                if (!gitignoreFile.exists() && gitignoreFile.open(QFile::WriteOnly)) {
                    gitignoreFile.write(GITIGNORE_CONTENT.data(), GITIGNORE_CONTENT.size());
                }

                gitignoreFile.close();

                const auto initResult = repo.init(dataPathStd);

                if (!initResult) {
                    logErr(this, initResult.error());
                    return;
                }

                loadUI(gitSettings);

                for (auto* const widget : sourceControlDockContent->findChildren<QWidget*>()) {
                    widget->show();
                }

                delete createRepositoryButton;
            }
            );

            return {};
        }

        for (auto* const child : sourceControlDockContent->findChildren<QWidget*>()) {
            child->hide();
        }

        return Err(u"Failed to open repository '%1': %2"_qsv.arg(dataPath, QString::fromUtf8(error.message)));
    }

    loadUI(gitSettings);
    return {};
}

void SourceControlDock::updateSettings(const GitSettings& gitSettings) {
    ezgit2::Result<void> res;

    if (gitSettings.preferGitConfig) {
        res = repo.setUserData({}, {});
    } else {
        res = repo.setUserData(gitSettings.username.toStdString(), gitSettings.useremail.toStdString());
    }

    if (!res) {
        logErr(this, res.error());
    }
};

void SourceControlDock::commit(const bool amend, const bool force) {
    QString message = commitMessageInput->toPlainText();

    const auto items = fileList->selectedItems();
    auto fileNames = ranges::to<vector>(views::transform(items, [](const auto& item) -> string {
        return item->filePath.toStdString();
    }));

    const auto result = repo.commit(message.toStdString(), fileNames, amend, force);

    if (!result) {
        logErr(this, result.error());
        return;
    }

    fileList->removeSelectedItems();

    const auto& hash = result.value();

    string_view username;
    string_view userEmail;
    repo.userData(username, userEmail);

    commitList->prependCommit(
        { .hash = QString::fromLatin1(hash),
          .message = std::move(message),
          .author = QString::fromUtf8(username),
          .isRemote = false }
    );
}

void SourceControlDock::clear() {
    sourceControlDockContent->hide();
    repo.close();
    commitList->clear();
    fileList->clear();
    branchSelect->clear();
    commitMessageInput->clear();
}
#endif
