//
//  Engine.cpp
//  render/src/render
//
//  Created by Sam Gateau on 3/3/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Engine.h"

#include <QtCore/QFile>

#include <gpu/Context.h>


using namespace render;

const QString ENGINE_CONFIG_DEFAULT = "Default";
// TODO: Presets (e.g., "Highest Performance", "Highest Quality") will go here
const QMap<QString, QString> Engine::PRESETS = {};

Engine::Engine() :
    _namedConfig(QStringList() << "Render" << "Engine"),
    _sceneContext(std::make_shared<SceneContext>()),
    _renderContext(std::make_shared<RenderContext>()) {
}

QStringList Engine::getNamedConfigList() {
    QStringList list;
    list << ENGINE_CONFIG_DEFAULT << PRESETS.keys();
    auto current = _namedConfig.get();
    if (!list.contains(current)) {
        list << current;
    }

    return list;
}

QString Engine::getNamedConfig() {
    return _namedConfig.get();
}

void Engine::setNamedConfig(const QString& config) {
    _namedConfig.set(config);
    loadConfig();
}

void Engine::loadConfig() {
    auto config = getConfiguration();
    QString current = _namedConfig.get();

    if (_defaultConfig.isNull()) {
        // Set the default
        _defaultConfig =
            QJsonDocument::fromJson(config->toJSON().toUtf8()).object();
    }

    if (current == ENGINE_CONFIG_DEFAULT) {
        // Load the default
        config->load(_defaultConfig.toObject());
    } else if (PRESETS.contains(current)) {
        // Load a preset
        config->load(QJsonDocument::fromJson(PRESETS[current].toUtf8()).object());
    } else {
        QFile file(current);
        if (!file.exists()) {
            qWarning() << "Engine configuration file" << current << "does not exist";
        } else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Engine configuration file" << current << "cannot be opened";
        } else {
            QString data = file.readAll();
            file.close();
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &error);
            if (error.error == error.NoError) {
                config->load(doc.object());
                qDebug() << "Engine configuration file" << current << "loaded";
            } else {
                qWarning() << "Engine configuration file" << current << "failed to load:" <<
                    error.errorString() << "at offset" << error.offset;
            }
        }
    }
}

void Engine::run() {
    // Sync GPU state before beginning to render
    _renderContext->args->_context->syncCache();

    for (auto job : _jobs) {
        job.run(_sceneContext, _renderContext);
    }
}
