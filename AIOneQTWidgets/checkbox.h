#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QCheckBox>

class CheckBox : public QCheckBox
{
    Q_OBJECT
public:
    CheckBox();

    bool isChecked() {
        return checkState() == Qt::Checked;
    }
};

#endif // CHECKBOX_H
