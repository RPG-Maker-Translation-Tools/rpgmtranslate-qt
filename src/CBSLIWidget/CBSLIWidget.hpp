#pragma once

#include "Aliases.hpp"
#include "UnfocusLineEdit.hpp"

#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSlider>
#include <QWidget>

// Label, slider and line edit for a whole number
class CBSLIWidget final : public QWidget {
    Q_OBJECT

   public:
    explicit CBSLIWidget(QWidget* parent = nullptr);

    void setLabel(const QString& label);

    void setRange(i32 minimum, i32 maximum);

    void setValue(i32 value);

    [[nodiscard]] auto value() const -> i32 { return slider_->value(); }

    [[nodiscard]] auto label() const -> QLabel*;
    [[nodiscard]] auto slider() const -> QSlider*;
    [[nodiscard]] auto lineEdit() const -> UnfocusLineEdit*;

   signals:
    void valueChanged(i32 value);

   private:
    void onSliderValueChanged(i32 value);
    void onEditCommitted();

    QLabel* const label_;
    QSlider* const slider_;
    UnfocusLineEdit* const lineEdit_;
};
