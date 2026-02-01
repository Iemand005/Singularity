#pragma once

#include <QObject>
#include <ModelFactory.hpp>

#include "QLLModel.hpp"
#include "QSDModel.hpp"

// Large Language model pointer
using QLLModelPtr = std::unique_ptr<QLLModel>;
// Stable Diffusion model pointer
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
