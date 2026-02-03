#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QCheckBox>

class CheckBox : public QCheckBox
{
    Q_OBJECT
public:
    CheckBox();
    CheckBox(QWidget *parent = nullptr) : QCheckBox(parent) {}

    bool checked() {
        return checkState() == Qt::Checked;
    }
};

#endif // CHECKBOX_H
