#include "CBSLIWidget.hpp"

#include "QtNumbers.hpp"

CBSLIWidget::CBSLIWidget(QWidget* const parent) :
    QWidget(parent),
    label_(new QLabel(QString(), this)),
    slider_(new QSlider(Qt::Horizontal, this)),
    lineEdit_(new UnfocusLineEdit(this)) {
    lineEdit_->setAlignment(Qt::AlignRight);

    auto* const layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label_);
    layout->addWidget(slider_, 1);
    layout->addWidget(lineEdit_);

    connect(slider_, &QSlider::valueChanged, this, &CBSLIWidget::onSliderValueChanged);

    connect(lineEdit_, &UnfocusLineEdit::editingFinished, this, &CBSLIWidget::onEditCommitted);
}

void CBSLIWidget::setLabel(const QString& label) {
    label_->setText(label);
}

void CBSLIWidget::setRange(const i32 minimum, const i32 maximum) {
    slider_->setRange(minimum, maximum);

    lineEdit_->setValidator(new QIntValidator(minimum, maximum, lineEdit_));
    lineEdit_->setMaxLength(scast<i32>(itos(maximum).size()));

    setValue(value());
}

void CBSLIWidget::setValue(i32 value) {
    value = clamp(slider_->minimum(), value, slider_->maximum());

    {
        const QSignalBlocker blocker(slider_);
        slider_->setValue(value);
    }

    {
        const QSignalBlocker blocker(lineEdit_);
        lineEdit_->setText(QString(qsv(qitos(value))));
    }

    emit valueChanged(value);
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
    lineEdit_->setText(QString(qsv(qitos(value))));
    lineEdit_->blockSignals(false);
    emit valueChanged(value);
}

void CBSLIWidget::onEditCommitted() {
    const optional<i32> parsed = stoa<i32>(lineEdit_->text());

    if (!parsed) {
        const QSignalBlocker blocker(lineEdit_);
        lineEdit_->setText(QString(qsv(qitos(slider_->value()))));
        return;
    }

    setValue(*parsed);
}
