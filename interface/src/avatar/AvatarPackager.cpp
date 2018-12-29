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

#include "Application.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QUrl>

#include <OffscreenUi.h>
#include "ModelSelector.h"
#include <avatar/MarketplaceItemUploader.h>

#include <mutex>
#include "scripting/HMDScriptingInterface.h"
#include "ui/TabletScriptingInterface.h"

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
    static const QUrl url{ "hifi/AvatarPackagerWindow.qml" };

    const auto packageModelDialogCreated = [=](QQmlContext* context, QObject* newObject) {
        context->setContextProperty("AvatarPackagerCore", this);
    };

    static const QString SYSTEM_TABLET = "com.highfidelity.interface.tablet.system";
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet(SYSTEM_TABLET));
    auto hmd = DependencyManager::get<HMDScriptingInterface>();

    if (tablet->getToolbarMode()) {
        DependencyManager::get<OffscreenUi>()->show(url, "AvatarPackager", packageModelDialogCreated);
    } else {
        static const QUrl url("hifi/tablet/AvatarPackager.qml");
        if (!tablet->isPathLoaded(url)) {
            tablet->getTabletSurface()->getSurfaceContext()->setContextProperty("AvatarPackagerCore", this);
            tablet->pushOntoStack(url);
        }
    }
    return true;
}

void AvatarPackager::addCurrentProjectToRecentProjects() {
    const int MAX_RECENT_PROJECTS = 5;
    const QString& fstPath = _currentAvatarProject->getFSTPath();
    auto removeProjects = QVector<RecentAvatarProject>();
    for (auto project : _recentProjects) {
        if (project.getProjectFSTPath() == fstPath) {
            removeProjects.append(project);
        }
    }
    for (const auto removeProject : removeProjects) {
        _recentProjects.removeOne(removeProject);
    }

    const auto newRecentProject = RecentAvatarProject(_currentAvatarProject->getProjectName(), fstPath);
    _recentProjects.prepend(newRecentProject);

    while (_recentProjects.size() > MAX_RECENT_PROJECTS) {
        _recentProjects.pop_back();
    }

    _recentProjectsSetting.set(recentProjectsToVariantList(false));
    emit recentProjectsChanged();
}

QVariantList AvatarPackager::recentProjectsToVariantList(bool includeProjectPaths) {
    QVariantList result;
    for (const auto& project : _recentProjects) {
        QVariantMap projectVariant;
        projectVariant.insert("name", project.getProjectName());
        projectVariant.insert("path", project.getProjectFSTPath());
        if (includeProjectPaths) {
            projectVariant.insert("projectPath", project.getProjectPath());
        }
        result.append(projectVariant);
    }

    return result;
}
void AvatarPackager::recentProjectsFromVariantList(QVariantList projectsVariant) {
    _recentProjects.clear();
    for (const auto& projectVariant : projectsVariant) {
        auto map = projectVariant.toMap();
        _recentProjects.append(RecentAvatarProject(map.value("name").toString(), map.value("path").toString()));
    }
}

AvatarProject* AvatarPackager::openAvatarProject(const QString& avatarProjectFSTPath) {
    setAvatarProject(AvatarProject::openAvatarProject(avatarProjectFSTPath));
    return _currentAvatarProject;
}

AvatarProject* AvatarPackager::createAvatarProject(const QString& projectsFolder, const QString& avatarProjectName, const QString& avatarModelPath, const QString& textureFolder) {
    setAvatarProject(AvatarProject::createAvatarProject(projectsFolder, avatarProjectName, avatarModelPath, textureFolder));
    return _currentAvatarProject;
}

void AvatarPackager::setAvatarProject(AvatarProject* avatarProject) {
    if (avatarProject == _currentAvatarProject) {
        return;
    }
    if (_currentAvatarProject) {
        _currentAvatarProject->deleteLater();
    }
    _currentAvatarProject = avatarProject;
    if (_currentAvatarProject) {
        addCurrentProjectToRecentProjects();
        connect(_currentAvatarProject, &AvatarProject::nameChanged, this, &AvatarPackager::addCurrentProjectToRecentProjects);
        QQmlEngine::setObjectOwnership(_currentAvatarProject, QQmlEngine::CppOwnership);
    }
    emit avatarProjectChanged();
}
