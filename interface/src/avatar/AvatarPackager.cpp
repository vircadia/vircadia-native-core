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
#include <QQmlEngine>
#include <QUrl>

#include <OffscreenUi.h>
#include "ModelSelector.h"
#include <avatar/MarketplaceItemUploader.h>

#include <thread>
#include <mutex>

std::once_flag setupQMLTypesFlag;
AvatarPackager::AvatarPackager() {
    std::call_once(setupQMLTypesFlag, []() {
        qmlRegisterType<FST>();
        qmlRegisterType<MarketplaceItemUploader>();
        qRegisterMetaType<AvatarPackager*>();
        qRegisterMetaType<AvatarProject*>();
    });

    recentProjectsFromVariantList(_recentProjectsSetting.get());

    QDir defaultProjectsDir(AvatarProject::getDefaultProjectsPath());
    defaultProjectsDir.mkpath(".");
}

bool AvatarPackager::open() {
    static const QUrl url{ "hifi/AvatarPackager.qml" };

    const auto packageModelDialogCreated = [=](QQmlContext* context, QObject* newObject) {
        context->setContextProperty("AvatarPackagerCore", this);
    };
    DependencyManager::get<OffscreenUi>()->show(url, "AvatarPackager", packageModelDialogCreated);
    return true;
}

AvatarProject* AvatarPackager::openAvatarProject(const QString& avatarProjectFSTPath) {
    if (_currentAvatarProject) {
        _currentAvatarProject->deleteLater();
    }
    _currentAvatarProject = AvatarProject::openAvatarProject(avatarProjectFSTPath);
    if (_currentAvatarProject) {
        addRecentProject(avatarProjectFSTPath, _currentAvatarProject->getProjectName());
    }
    qDebug() << "_currentAvatarProject has" << (QQmlEngine::objectOwnership(_currentAvatarProject) == QQmlEngine::CppOwnership ? "CPP" : "JS") << "OWNERSHIP";
    QQmlEngine::setObjectOwnership(_currentAvatarProject, QQmlEngine::CppOwnership);
    emit avatarProjectChanged();
    return _currentAvatarProject;
}

void AvatarPackager::addRecentProject(QString fstPath, QString projectName) {
    const int MAX_RECENT_PROJECTS = 5;
    auto removeProjects = QVector<RecentAvatarProject>();
    for (auto project : _recentProjects) {
        if (project.getProjectFSTPath() == fstPath) {
            removeProjects.append(project);
        }
    }
    for (const auto removeProject : removeProjects) {
        _recentProjects.removeOne(removeProject);
    }

    RecentAvatarProject newRecentProject = RecentAvatarProject(projectName, fstPath);
    _recentProjects.prepend(newRecentProject);

    while (_recentProjects.size() > MAX_RECENT_PROJECTS) {
        _recentProjects.pop_back();
    }

    _recentProjectsSetting.set(recentProjectsToVariantList());
    emit recentProjectsChanged();
}

AvatarProject* AvatarPackager::createAvatarProject(const QString& projectsFolder, const QString& avatarProjectName, const QString& avatarModelPath, const QString& textureFolder) {
    if (_currentAvatarProject) {
        _currentAvatarProject->deleteLater();
    }
    _currentAvatarProject = AvatarProject::createAvatarProject(projectsFolder, avatarProjectName, avatarModelPath, textureFolder);
    if (_currentAvatarProject) {
        addRecentProject(_currentAvatarProject->getFSTPath(), _currentAvatarProject->getProjectName());
    }
    qDebug() << "_currentAvatarProject has" << (QQmlEngine::objectOwnership(_currentAvatarProject) == QQmlEngine::CppOwnership ? "CPP" : "JS") << "OWNERSHIP";
    QQmlEngine::setObjectOwnership(_currentAvatarProject, QQmlEngine::CppOwnership);
    emit avatarProjectChanged();
    return _currentAvatarProject;
}
