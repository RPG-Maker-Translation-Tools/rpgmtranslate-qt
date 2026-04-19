#pragma once

#ifdef ENABLE_LIBGIT2
#include "Aliases.hpp"
#include "Utils.hpp"

#include <git2/diff.h>

#include <QAbstractListModel>
#include <QApplication>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFontMetrics>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QStyledItemDelegate>

struct GitChangeItem final {
    QString filePath;
    git_delta_t changeType;
    bool staged = false;

    [[nodiscard]] auto fileName() const -> QStringView {
        return lastPathComponent(filePath);
    }

    [[nodiscard]] auto relativeDir(const QString& repoRootPath) const
        -> QString {
        // TODO: Manual relative making?
        const QString dir =
            QDir(repoRootPath)
                .relativeFilePath(QFileInfo(filePath).absolutePath());
        return (dir == u'.') ? QString() : dir;
    }
};

class GitChangesModel final : public QAbstractListModel {
    Q_OBJECT

   public:
    enum Roles : u16 {
        GitChangeItemRole = Qt::UserRole + 1,
    };

    explicit GitChangesModel(QObject* const parent = nullptr) :
        QAbstractListModel(parent) {}

    [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const
        -> i32 override {
        return i32(items.size());
    }

    [[nodiscard]] auto data(
        const QModelIndex& index,
        const i32 role = Qt::DisplayRole
    ) const -> QVariant override {
        if (!index.isValid()) {
            return {};
        }

        if (role == GitChangeItemRole) {
            return QVariant::fromValue(items.at(index.row()));
        }

        return {};
    }

    void setItems(const QList<GitChangeItem>& newItems) {
        beginResetModel();
        this->items = newItems;
        endResetModel();
    }

    void appendItem(const GitChangeItem& item) {
        beginInsertRows({}, i32(items.size()), i32(items.size()));
        items.append(item);
        endInsertRows();
    }

    void removeItem(const i32 row) {
        beginRemoveRows({}, row, row);
        items.removeAt(row);
        endRemoveRows();
    }

    void setStaged(const i32 row, const bool staged) {
        items[row].staged = staged;
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, { GitChangeItemRole });
    }

    void clear() {
        beginResetModel();
        items.clear();
        endResetModel();
    }

    QString repoRootPath;

   private:
    QList<GitChangeItem> items;
};

class GitChangesDelegate final : public QStyledItemDelegate {
    Q_OBJECT

    static constexpr i32 ITEM_HEIGHT = 24;
    static constexpr i32 ICON_SIZE = 16;
    static constexpr i32 H_PADDING = 4;
    static constexpr i32 ICON_TEXT_GAP = 4;
    static constexpr i32 NAME_PATH_GAP = 8;
    static constexpr i32 BADGE_RIGHT_PAD = 4;
    static constexpr i32 CHECKBOX_SIZE = 20;
    static constexpr i32 CHECKBOX_PAD = 4;

   public:
    explicit GitChangesDelegate(
        GitChangesModel* const model,
        QObject* const parent = nullptr
    ) :
        QStyledItemDelegate(parent),
        model(model) {}

    [[nodiscard]] constexpr auto sizeHint(
        const QStyleOptionViewItem& /* opt */,
        const QModelIndex& /* idx */
    ) const -> QSize override {
        return { 0, ITEM_HEIGHT };
    }

    void paint(
        QPainter* const painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override {
        QStyledItemDelegate::paint(painter, option, index);

        const auto item = index.data(GitChangesModel::GitChangeItemRole)
                              .value<GitChangeItem>();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setClipRect(option.rect);

        const QRect& rect = option.rect;
        const i32 midY = rect.y() + (rect.height() / 2);
        const bool selected = (option.state & QStyle::State_Selected) != 0;
        const QPalette& palette = option.palette;

        const QRect checkboxRect = checkboxHitRect(rect);

        QStyleOptionButton checkOpt;
        checkOpt.rect = checkboxRect;
        checkOpt.state = QStyle::State_Enabled;
        checkOpt.state |= item.staged ? QStyle::State_On : QStyle::State_Off;

        qApp->style()
            ->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkOpt, painter);

        const i32 checkboxReserved =
            CHECKBOX_PAD + CHECKBOX_SIZE + CHECKBOX_PAD;

        QChar badgeLetter;
        QColor badgeColor;
        resolveBadge(item.changeType, badgeLetter, badgeColor);

        QFont font = option.font;
        font.setBold(true);

        const auto fontMetrics = QFontMetrics(font);
        const i32 badgeWidth = fontMetrics.horizontalAdvance(badgeLetter);
        const i32 badgeX =
            rect.right() - checkboxReserved - BADGE_RIGHT_PAD - badgeWidth;

        const QColor primaryColor = selected ? palette.highlightedText().color()
                                             : palette.text().color();
        const QColor dimColor = selected ? palette.highlightedText().color()
                                         : palette.placeholderText().color();

        i32 xPos = rect.x() + H_PADDING;

        const QIcon icon = fileIcon(item.filePath);
        const QRect
            iconRect(xPos, midY - (ICON_SIZE / 2), ICON_SIZE, ICON_SIZE);
        icon.paint(painter, iconRect);
        xPos += ICON_SIZE + ICON_TEXT_GAP;

        const QString fileName = item.fileName().toString();
        const i32 nameWidth = fontMetrics.horizontalAdvance(fileName);
        const i32 totalTextAvail = badgeX - xPos - NAME_PATH_GAP;
        const i32 actualNameWidth = min(nameWidth, totalTextAvail);

        font.setStrikeOut(badgeLetter == u'D');
        painter->setFont(font);
        painter->setPen(primaryColor);
        painter->drawText(
            QRect(xPos, rect.y(), actualNameWidth, rect.height()),
            Qt::AlignVCenter | Qt::AlignLeft,
            fileName
        );

        xPos += actualNameWidth + NAME_PATH_GAP;

        const QString relativeDir = item.relativeDir(model->repoRootPath);
        const i32 pathAvail = badgeX - xPos;

        if (!relativeDir.isEmpty() && pathAvail > 0) {
            painter->setFont(font);
            painter->setPen(dimColor);
            painter->drawText(
                QRect(xPos, rect.y(), pathAvail, rect.height()),
                Qt::AlignVCenter | Qt::AlignLeft,
                relativeDir
            );
        }

        font.setStrikeOut(false);
        painter->setFont(font);
        painter->setPen(badgeColor);
        painter->drawText(
            QRect(badgeX, rect.y(), badgeWidth, rect.height()),
            Qt::AlignVCenter | Qt::AlignLeft,
            QChar(badgeLetter)
        );

        painter->restore();
    }

    auto editorEvent(
        QEvent* const event,
        QAbstractItemModel* const /* abstractModel */,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) -> bool override {
        if (event->type() != QEvent::MouseButtonRelease) {
            return false;
        }

        const auto* const mouseEvent = as<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton) {
            return false;
        }

        const QRect hitRect = checkboxHitRect(option.rect);
        if (!hitRect.contains(mouseEvent->pos())) {
            return false;
        }

        const auto item = index.data(GitChangesModel::GitChangeItemRole)
                              .value<GitChangeItem>();
        model->setStaged(index.row(), !item.staged);

        emit staged(item.filePath);

        return true;
    }

   signals:
    void staged(const QString& path);

   private:
    [[nodiscard]] static auto checkboxHitRect(const QRect& itemRect) -> QRect {
        const i32 midY = itemRect.y() + (itemRect.height() / 2);
        return { itemRect.right() - CHECKBOX_PAD - CHECKBOX_SIZE,
                 midY - (CHECKBOX_SIZE / 2),
                 CHECKBOX_SIZE,
                 CHECKBOX_SIZE };
    }

    [[nodiscard]] auto fileIcon(const QString& absoluteFilePath) const
        -> QIcon {
        QIcon icon = iconProvider.icon(QFileInfo(absoluteFilePath));

        if (icon.isNull()) {
            icon = iconProvider.icon(QFileIconProvider::File);
        }

        return icon;
    }

    static void
    resolveBadge(const git_delta_t type, QChar& letter, QColor& color) {
        switch (type) {
            case GIT_DELTA_MODIFIED:
                letter = u'M';
                color = qRgb(0xE2, 0xC0, 0x8D);
                return;
            case GIT_DELTA_UNTRACKED:
                letter = u'U';
                color = qRgb(0x73, 0xC9, 0x91);
                return;
            case GIT_DELTA_ADDED:
                letter = u'A';
                color = qRgb(0x73, 0xC9, 0x91);
                return;
            case GIT_DELTA_DELETED:
                letter = u'D';
                color = qRgb(0xF1, 0x4C, 0x4C);
                return;
            case GIT_DELTA_RENAMED:
                letter = u'R';
                color = qRgb(0xE2, 0xC0, 0x8D);
                return;
            case GIT_DELTA_COPIED:
                letter = u'C';
                color = qRgb(0x73, 0xC9, 0x91);
                return;
            case GIT_DELTA_CONFLICTED:
                letter = u'!';
                color = qRgb(0xF1, 0x4C, 0x4C);
                return;
            case GIT_DELTA_IGNORED:
                letter = u'I';
                color = qRgb(0x8C, 0x8C, 0x8C);
                return;
            case GIT_DELTA_TYPECHANGE:
                // TODO;
                return;
            case GIT_DELTA_UNMODIFIED:
                // TODO;
                return;
            case GIT_DELTA_UNREADABLE:
                // TODO;
                return;
        }
    }

    mutable QFileIconProvider iconProvider;
    GitChangesModel* model;
};

class GitChangesList final : public QListView {
    Q_OBJECT

   public:
    explicit GitChangesList(QWidget* const parent = nullptr) :
        QListView(parent),
        model_(new GitChangesModel(this)),
        delegate(new GitChangesDelegate(model_, this)) {
        setModel(model_);
        setItemDelegate(delegate);

        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSelectionMode(QListView::ExtendedSelection);
        setUniformItemSizes(true);
        setMouseTracking(true);

        connect(
            delegate,
            &GitChangesDelegate::staged,
            this,
            &GitChangesList::staged
        );
    }

    [[nodiscard]] auto model() const -> GitChangesModel* { return model_; }

   signals:
    void staged(const QString& path);

   private:
    GitChangesModel* const model_;
    GitChangesDelegate* const delegate;
};
#else
#include <QListView>

class GitChangesList final : public QListView {
    Q_OBJECT

   public:
    using QListView::QListView;
};
#endif
