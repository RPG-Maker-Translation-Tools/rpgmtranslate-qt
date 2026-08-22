#pragma once

#include "Aliases.hpp"
#include "UnfocusLineEdit.hpp"
#include "Utils.hpp"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSlider>
#include <QWidget>

// CheckBox, Slider and Line Edit
class CBSLIWidget final : public QWidget {
    Q_OBJECT

   public:
    explicit CBSLIWidget(bool checkable = false, QWidget* parent = nullptr);

    void setCheckable(bool checkable);
    void setLabel(const QString& label);
    void setChecked(bool checked);

    void setRange(f32 floatMin, f32 floatMax);
    void setRange(i32 minimum, i32 maximum);

    void setValue(i32 value);
    void setValue(f32 floatVal);

    [[nodiscard]] auto isChecked() const -> bool;

    template <typename T = i32>
        requires std::is_arithmetic_v<T>
    [[nodiscard]] auto value() const -> T {
        if constexpr (std::is_floating_point_v<T>) {
            return scast<f32>(slider_->value()) / FLOAT_FACTOR;

        } else {
            return slider_->value();
        }
    }

    [[nodiscard]] auto label() const -> QLabel*;
    [[nodiscard]] auto slider() const -> QSlider*;
    [[nodiscard]] auto lineEdit() const -> UnfocusLineEdit*;

   signals:
    void valueChanged(i32 value);

   private:
    void onSliderValueChanged(i32 value);
    void onEditCommitted();

    static constexpr i32 FLOAT_FACTOR = 1000;

    QLabel* const label_;
    QCheckBox* checkbox_;
    QSlider* const slider_;
    UnfocusLineEdit* const lineEdit_;

    bool floatFormat = false;
};
