#ifdef ENABLE_LIBGIT2
#include "SourceControlDock.hpp"

#include "GitChangesList.hpp"
#include "GitCommitList.hpp"
#include "ezgit2.hpp"

#include <QComboBox>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>

void SourceControlDock::init(
    QComboBox* const branchSelect,
    GitChangesList* const changesList,
    GitCommitList* const commitList,

    QPushButton* const commitButton,
    QToolButton* const commitOptionsButton,

    QPlainTextEdit* const commitMessageInput,

    QPushButton* const copyTranslationButton,
    QToolButton* const refreshChangesButton
) {
    this->branchSelect = branchSelect;
    this->changesList = changesList;
    this->commitList = commitList;

    this->commitButton = commitButton;
    this->commitOptionsButton = commitOptionsButton;
    this->commitMessageInput = commitMessageInput;

    connect(this->changesList, &GitChangesList::staged, this, [this] -> void {
        // TODO
    });

    connect(
        this->branchSelect,
        &QComboBox::currentTextChanged,
        this,
        [this](const QString& branch) -> void {
        // TODO
    }
    );

    connect(this->commitButton, &QPushButton::pressed, this, [this] -> void {
        const QString message = this->commitMessageInput->toPlainText();
        const auto result = repo.commit(message.toStdString());

        if (!result) {
            // TODO
            return;
        }

        const auto& value = result.value();
        // TODO
    });

    connect(
        this->commitOptionsButton,
        &QToolButton::pressed,
        this,
        [this] -> void {
        QMenu menu;

        const QAction* const commitAmendAction =
            menu.addAction(tr("Commit (Amend)"));
        const QAction* const commitPushAction =
            menu.addAction(tr("Commit and push"));

        const QAction* const selectedAction =
            menu.exec(this->commitOptionsButton->mapToGlobal(
                QPoint(0, this->commitOptionsButton->height())
            ));

        if (selectedAction == commitAmendAction) {
        } else if (selectedAction == commitPushAction) {
        }
    }
    );

    connect(copyTranslationButton, &QPushButton::pressed, this, [this] -> void {
        // TODO
    });

    connect(refreshChangesButton, &QToolButton::pressed, this, [this] -> void {
        // TODO
    });
}

auto SourceControlDock::setProjectPath(const QString& projectPath)
    -> result<void, QString> {
    // TODO: Clean up the dock

    // TODO: If project is not opened, display button to create the repository.

    auto result = ezgit2::Repository::open(projectPath.toStdString());

    if (!result) {
        const auto& error = result.error();
        this->changesList->model()->clear();
        this->branchSelect->clear();
        return Err("Failed to open repository '%1': %2"_L1
                       .arg(projectPath, QString::fromUtf8(error.message)));
    }

    repo = std::move(result.value());

    const auto filesStatus = repo.list_files_status();
    const auto branches = repo.list_branches();

    if (!filesStatus) {
        return Err(QString::fromUtf8(filesStatus.error().message));
    }

    if (!branches) {
        return Err(QString::fromUtf8(branches.error().message));
    }

    for (const auto& status : filesStatus.value()) {
        this->changesList->model()->appendItem(
            GitChangeItem{ .filePath = QString::fromUtf8(status.path),
                           .changeType = status.status }
        );
    }

    for (const auto& branch : branches.value()) {
        branchSelect->addItem(
            QIcon(
                branch.type == GIT_BRANCH_LOCAL ? u":/icons/home.svg"_s
                                                : u":/icons/cloud.svg"_s
            ),
            QString::fromUtf8(branch.name)
        );
    }

    for (const auto& commit : repo.get_commit_history().value()) {
        commitList->model()->appendCommit(
            { .hash = QString::fromUtf8(commit.oid),
              .message = QString::fromUtf8(commit.message),
              .author = QString::fromUtf8(commit.author_name),
              .isRemote = false }
        );
    }

    return {};
}
#endif
