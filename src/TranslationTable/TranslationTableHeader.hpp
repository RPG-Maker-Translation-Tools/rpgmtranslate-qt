#pragma once

#include "Aliases.hpp"

#include <QHeaderView>

class TranslationTableHeader final : public QHeaderView {
    Q_OBJECT

   public:
    explicit TranslationTableHeader(QWidget* parent = nullptr);
    ~TranslationTableHeader() override;

   signals:
    void addButtonClicked();

   protected:
    void paintSection(
        QPainter* painter,
        const QRect& rect,
        i32 logicalIndex
    ) const override;
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void leaveEvent(QEvent* event) override;

    [[nodiscard]] auto sizeHint() const -> QSize override;

   private:
    [[nodiscard]] auto getButtonRect() const -> QRect;

    void beginEditSection(i32 logicalIndex);
    void commitEdit();
    void cancelEdit();

    QLineEdit* labelInput = nullptr;
    i32 editingSection = -1;

    bool addButtonHovered = false;
    bool addButtonPressed = false;
};
