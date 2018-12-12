//
//  AvatarPackager.cpp
//
//
//  Created by Thijs Wenker on 12/6/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarPackager.h"

#include <QQmlContext>
#include <QUrl>

#include <OffscreenUi.h>

bool AvatarPackager::open() {
    static const QUrl url{ "hifi/AvatarPackager.qml" };

    const auto packageModelDialogCreated = [=](QQmlContext* context, QObject* newObject) {
        context->setContextProperty("AvatarPackagerCore", this);
    };
    DependencyManager::get<OffscreenUi>()->show(url, "AvatarPackager", packageModelDialogCreated);
    return true;
}

QObject* AvatarPackager::openAvatarProject(QString avatarProjectFSTPath) {
    if (_currentAvatarProject) {
        _currentAvatarProject->deleteLater();
    }
    _currentAvatarProject = AvatarProject::openAvatarProject(avatarProjectFSTPath);
    return _currentAvatarProject;
}
