#include "CBSLIWidget.hpp"

CBSLIWidget::CBSLIWidget(const bool checkable, QWidget* const parent) :
    QWidget(parent),
    label_(new QLabel(QString(), this)),
    checkbox_(checkable ? new QCheckBox(tr("Custom"), this) : nullptr),
    slider_(new QSlider(Qt::Horizontal, this)),
    lineEdit_(new UnfocusLineEdit(this)) {
    lineEdit_->setAlignment(Qt::AlignRight);

    auto* const layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label_);

    if (checkable) {
        layout->addWidget(checkbox_);
    }

    layout->addWidget(slider_, 1);
    layout->addWidget(lineEdit_);

    connect(slider_, &QSlider::valueChanged, this, &CBSLIWidget::onSliderValueChanged);

    connect(lineEdit_, &UnfocusLineEdit::editingFinished, this, &CBSLIWidget::onEditCommitted);

    connect(lineEdit_, &UnfocusLineEdit::returnPressed, this, &CBSLIWidget::onEditCommitted);
}

void CBSLIWidget::setCheckable(const bool checkable) {
    if (checkable) {
        if (checkbox_ != nullptr) {
            return;
        }

        checkbox_ = new QCheckBox(tr("Custom"), this);
        scast<QHBoxLayout*>(layout())->insertWidget(1, checkbox_);
    } else {
        delete checkbox_;
        checkbox_ = nullptr;
    }
}

void CBSLIWidget::setLabel(const QString& label) {
    label_->setText(label);
}

void CBSLIWidget::setChecked(const bool checked) {
    checkbox_->setChecked(checked);
}

void CBSLIWidget::setRange(const f32 floatMin, const f32 floatMax) {
    const i32 minimum = scast<i32>(floatMin * FLOAT_FACTOR);
    const i32 maximum = scast<i32>(floatMax * FLOAT_FACTOR);

    floatFormat = true;
    slider_->setRange(minimum, maximum);

    lineEdit_->setValidator(new QDoubleValidator(floatMin, floatMax, 3, lineEdit_));
    lineEdit_->setMaxLength(intLen(scast<i32>(floatMax)) + 1 + 3);

    setValue(clamp(minimum, value<i32>(), maximum));
}

void CBSLIWidget::setRange(const i32 minimum, const i32 maximum) {
    floatFormat = false;
    slider_->setRange(minimum, maximum);

    lineEdit_->setValidator(new QIntValidator(minimum, maximum, lineEdit_));
    lineEdit_->setMaxLength(intLen(maximum));

    setValue(clamp(minimum, value<i32>(), maximum));
}

void CBSLIWidget::setValue(i32 value) {
    value = clamp(slider_->minimum(), value, slider_->maximum());

    {
        const QSignalBlocker blocker(slider_);
        slider_->setValue(value);
    }

    {
        const QSignalBlocker blocker(lineEdit_);
        lineEdit_->setText(QString(floatFormat ? ftos(scast<f32>(value) / FLOAT_FACTOR, 3).qsv() : itos(value).qsv()));
    }

    emit valueChanged(value);
}

void CBSLIWidget::setValue(f32 floatVal) {
    i32 value = scast<i32>(floatVal * FLOAT_FACTOR);

    value = clamp(slider_->minimum(), value, slider_->maximum());

    {
        const QSignalBlocker blocker(slider_);
        slider_->setValue(value);
    }

    {
        const QSignalBlocker blocker(lineEdit_);
        lineEdit_->setText(QString(floatFormat ? ftos(scast<f32>(value) / FLOAT_FACTOR, 3).qsv() : itos(value).qsv()));
    }

    emit valueChanged(value);
}

[[nodiscard]] auto CBSLIWidget::isChecked() const -> bool {
    return checkbox_ == nullptr ? true : checkbox_->isChecked();
}

[[nodiscard]] auto CBSLIWidget::label() const -> QLabel* {
    return label_;
}

[[nodiscard]] auto CBSLIWidget::slider() const -> QSlider* {
    return slider_;
}

[[nodiscard]] auto CBSLIWidget::lineEdit() const -> UnfocusLineEdit* {
    return lineEdit_;
}

void CBSLIWidget::onSliderValueChanged(const i32 value) {
    lineEdit_->blockSignals(true);
    lineEdit_->setText(QString(floatFormat ? ftos(scast<f32>(value) / FLOAT_FACTOR, 3).qsv() : itos(value).qsv()));
    lineEdit_->blockSignals(false);
    emit valueChanged(value);
}

void CBSLIWidget::onEditCommitted() {
    bool correct = false;
    i32 value;

    if (floatFormat) {
        const f32 flt = stoa<f32>(lineEdit_->text(), 10, &correct);
        value = scast<i32>(flt * FLOAT_FACTOR);
    } else {
        value = stoa<i32>(lineEdit_->text(), 10, &correct);
    }

    if (!correct) {
        const QSignalBlocker blocker(lineEdit_);
        lineEdit_->setText(QString(
            floatFormat ? ftos(scast<f32>(slider_->value()) / FLOAT_FACTOR, 3).qsv() : itos(slider_->value()).qsv()
        ));
        return;
    }

    value = clamp(slider_->minimum(), value, slider_->maximum());
    setValue(value);
}