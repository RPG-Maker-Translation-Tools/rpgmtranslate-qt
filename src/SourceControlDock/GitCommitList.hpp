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

    [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const
        -> i32 override;
    [[nodiscard]] auto data(
        const QModelIndex& index,
        i32 role = Qt::DisplayRole
    ) const -> QVariant override;

    void setCommits(vector<GitCommit> commits);
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

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

    [[nodiscard]] auto sizeHint(
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const -> QSize override;

   private:
    static constexpr u8 ROW_HEIGHT = 58;
    static constexpr u8 PADDING_HORIZONTAL = 12;
    static constexpr u8 PADDING_VERTICAL = 6;
    static constexpr u8 CIRCLE_RADIUS = 6;
    static constexpr u8 CIRCLE_TO_TEXT = 10;
    static constexpr u8 HASH_MIN_WIDTH = 56;

    static constexpr QRgb COLOR_REMOTE = 0xFF4A90D9;
    static constexpr QRgb COLOR_LOCAL = 0xFF3DBE6E;
};

class GitCommitList : public QListView {
    Q_OBJECT

   public:
    static constexpr u16 HOVER_DELAY_MS = 600;

    explicit GitCommitList(QWidget* parent = nullptr);

    [[nodiscard]] auto model() -> GitCommitModel* { return model_; };

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
