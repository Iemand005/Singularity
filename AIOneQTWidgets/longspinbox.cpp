#include "longspinbox.h"

#include <QLineEdit>

LongSpinBox::LongSpinBox() {}

LongSpinBox::LongSpinBox(QWidget *parent)
    : QAbstractSpinBox(parent) {
    setRange(0, 100); // Sensible default
    connect(lineEdit(), &QLineEdit::textEdited,
            this, [this]() { lineEdit()->setText(lineEdit()->text().trimmed()); });
}

qint64 LongSpinBox::value() const { return m_value; }
qint64 LongSpinBox::minimum() const { return m_minimum; }
qint64 LongSpinBox::maximum() const { return m_maximum; }

void LongSpinBox::setRange(qint64 min, qint64 max) {
    m_minimum = min;
    m_maximum = max;
    if (m_value < min) setValue(min);
    if (m_value > max) setValue(max);
    updateEdit();
}

void LongSpinBox::setMinimum(qint64 min) { setRange(min, m_maximum); }
void LongSpinBox::setMaximum(qint64 max) { setRange(m_minimum, max); }

void LongSpinBox::setValue(qint64 val) {
    val = qBound(m_minimum, val, m_maximum);
    if (m_value != val) {
        m_value = val;
        updateEdit();
        emit valueChanged(m_value);
        emit valueChanged(textFromValue(m_value));
    }
}

void LongSpinBox::stepBy(int steps) {
    setValue(m_value + steps);
}

QAbstractSpinBox::StepEnabled LongSpinBox::stepEnabled() const {
    StepEnabled se = StepNone;
    if (m_value > m_minimum) se |= StepDownEnabled;
    if (m_value < m_maximum) se |= StepUpEnabled;
    return se;
}

QValidator::State LongSpinBox::validate(QString &input, int &) const {
    if (input.isEmpty()) return QValidator::Intermediate;

    bool ok;
    qint64 val = input.toLongLong(&ok);
    if (!ok) return QValidator::Invalid;

    if (val < m_minimum || val > m_maximum)
        return QValidator::Intermediate;

    return QValidator::Acceptable;
}

void LongSpinBox::fixup(QString &input) const {
    bool ok;
    qint64 val = input.toLongLong(&ok);
    if (!ok) val = m_value;
    input = textFromValue(qBound(m_minimum, val, m_maximum));
}

qint64 LongSpinBox::valueFromText(const QString &text) const {
    return text.toLongLong();
}

QString LongSpinBox::textFromValue(qint64 value) const {
    return QString::number(value);
}

void LongSpinBox::updateEdit() {
    lineEdit()->setText(textFromValue(m_value));
}
