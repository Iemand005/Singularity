#pragma once

#include <ModelFactory.hpp>
#include <QObject>

#include "QLLModel.hpp"
#include "QSDModel.hpp"

using QLLModelPtr = std::unique_ptr<QLLModel>;

class QModelFactory : public ModelFactory, public QObject {

    ModelFactory *super() {
        return this;
    }

public:
    QLLModelPtr loadModel(QString &path) {
        return ((ModelFactory *)this)->loadModel(path.toStdString());
    }

};
