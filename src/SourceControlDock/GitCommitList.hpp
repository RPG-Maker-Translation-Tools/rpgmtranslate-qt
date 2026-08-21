#pragma once

#include "Aliases.hpp"

#include <QAbstractListModel>
#include <QListView>
#include <QStyledItemDelegate>
#include <QTimer>
#include <deque>

struct GitCommit {
    QString hash;
    QString message;
    QString author;
    bool isRemote = false;
};

enum Role : u16 {
    Hash = Qt::UserRole + 1,
    Message,
    Author,
    IsRemote,
};

class GitCommitModel : public QAbstractListModel {
    Q_OBJECT

   public:
    explicit GitCommitModel(QObject* parent = nullptr);

    [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const -> i32 override;
    [[nodiscard]] auto data(const QModelIndex& index, i32 role = Qt::DisplayRole) const -> QVariant override;

    void prependCommit(GitCommit commit);
    void appendCommit(GitCommit commit);
    void clear();

    [[nodiscard]] auto commitAt(i32 row) const -> const GitCommit&;

   private:
    std::deque<GitCommit> commits;
};

class GitCommitDelegate : public QStyledItemDelegate {
    Q_OBJECT

   public:
    explicit GitCommitDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    [[nodiscard]] auto sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const -> QSize override;

};

class GitCommitList : public QListView {
    Q_OBJECT

   public:
    explicit GitCommitList(QWidget* parent = nullptr);

    [[nodiscard]] auto model() -> GitCommitModel* { return model_; };

    void appendCommit(GitCommit commit) { model_->appendCommit(std::move(commit)); };

    void prependCommit(GitCommit commit) { model_->prependCommit(std::move(commit)); };

    void clear() { model_->clear(); }

   signals:
    void commitHovered(const QModelIndex& index);

   protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

   private:
    GitCommitModel* const model_;

    QTimer hoverTimer;
    QModelIndex hoveredIndex;
    QPersistentModelIndex pendingIndex;
};
