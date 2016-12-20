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

#include <PathUtils.h>

#include <gpu/Context.h>

#include "EngineStats.h"
#include "Logging.h"

using namespace render;

Engine::Engine() :
    _sceneContext(std::make_shared<SceneContext>()),
    _renderContext(std::make_shared<RenderContext>()) {
    addJob<EngineStats>("Stats");
}

void Engine::load() {
    auto config = getConfiguration();
    const QString configFile= "config/render.json";

    QUrl path(PathUtils::resourcesPath() + configFile);
    QFile file(path.toString());
    if (!file.exists()) {
        qWarning() << "Engine configuration file" << path << "does not exist";
    } else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Engine configuration file" << path << "cannot be opened";
    } else {
        QString data = file.readAll();
        file.close();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &error);
        if (error.error == error.NoError) {
            config->setPresetList(doc.object());
            qCDebug(renderlogging) << "Engine configuration file" << path << "loaded";
        } else {
            qWarning() << "Engine configuration file" << path << "failed to load:" <<
                error.errorString() << "at offset" << error.offset;
        }
    }
}

void Engine::run() {
    for (auto job : _jobs) {
        job.run(_sceneContext, _renderContext);
    }

}

