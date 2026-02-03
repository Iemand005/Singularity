#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QObject>
#include <QProgressBar>
#include <QWidget>

class ProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    ProgressBar();
    ProgressBar(QWidget *parent = nullptr) : QProgressBar(parent) {}

    void setValueWithMax(int value, int max) {
        if (indeterminate) setIndeterminate(false);
        this->setValue(value);
        this->setMaximum(max);
    }

    void showIntermediate() {
        setIndeterminate();
        show();
    }

public slots:

    void setPercentage(float percentage) {
        int scale = 10000;
        if (indeterminate) setIndeterminate(false);
        this->setValue(percentage * scale);
        this->setMaximum(scale);
    }

    void setIndeterminate(bool indeterminate = true) {
        this->indeterminate = indeterminate;
        this->setMaximum(indeterminate ? 0 : 100);
        this->setValue(0);
        this->setTextVisible(!indeterminate);
    }

    void showAndReset() {
        this->setValue(0);
        this->show();
    }

    bool isIndeterminate() {
        return this->indeterminate;
    }

private:
    bool indeterminate = false;
};

#endif // PROGRESSBAR_H
