#pragma once

#include <ModelFactory.hpp>
#include <QObject>

#include "QLLModel.hpp"
#include "QSDModel.hpp"

using QLLModelPtr = std::unique_ptr<QLLModel>;
using QSDModelPtr = std::unique_ptr<QSDModel>;

class QModelFactory : public QObject, public ModelFactory {
    Q_OBJECT

    ModelFactory *super() {
        return this;
    }

public:
    QLLModelPtr loadLLM(const QString &path) {
        initLlama();
        return std::make_unique<QLLModel>(path);
    }

    QSDModelPtr loadSDM(const QString &path) {
        return std::make_unique<QSDModel>(path);
    }

};

using QModelFactoryPtr = std::unique_ptr<QModelFactory>;
