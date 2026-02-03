#ifndef LONGSPINBOX_H
#define LONGSPINBOX_H

#include <QAbstractSpinBox>
#include <QValidator>

class LongSpinBox : public QAbstractSpinBox
{
    Q_OBJECT
public:
    LongSpinBox();
    LongSpinBox(QWidget *parent = nullptr);

    qint64 value() const;
    qint64 minimum() const;
    qint64 maximum() const;

    void setRange(qint64 min, qint64 max);
    void setMinimum(qint64 min);
    void setMaximum(qint64 max);

public slots:
    void setValue(qint64 val);

signals:
    void valueChanged(qint64);
    void valueChanged(const QString &);

protected:
    void stepBy(int steps) override;
    StepEnabled stepEnabled() const override;
    QValidator::State validate(QString &input, int &pos) const override;
    void fixup(QString &input) const override;

private:
    qint64 valueFromText(const QString &text) const;
    QString textFromValue(qint64 value) const;
    void updateEdit();

    qint64 m_value;
    qint64 m_minimum = std::numeric_limits<qint64>::min();
    qint64 m_maximum = std::numeric_limits<qint64>::max();
};

#endif // LONGSPINBOX_H
