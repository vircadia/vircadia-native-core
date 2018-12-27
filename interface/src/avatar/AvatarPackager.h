//
//  AvatarPackager.h
//
//
//  Created by Thijs Wenker on 12/6/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_AvatarPackager_h
#define hifi_AvatarPackager_h

#include <QObject>
#include <DependencyManager.h>

#include "FileDialogHelper.h"

#include "avatar/AvatarProject.h"
#include "SettingHandle.h"

class RecentAvatarProject {
public:
    RecentAvatarProject() = default;
    

    RecentAvatarProject(QString projectName, QString projectFSTPath) {
        _projectName = projectName;
        _projectFSTPath = projectFSTPath;
    }
    RecentAvatarProject(const RecentAvatarProject& other) {
        _projectName = other._projectName;
        _projectFSTPath = other._projectFSTPath;
    }

    ~RecentAvatarProject() = default;

    QString getProjectName() const { return _projectName; }

    QString getProjectFSTPath() const { return _projectFSTPath; }

    bool operator==(const RecentAvatarProject& other) const {
        return _projectName == other._projectName && _projectFSTPath == other._projectFSTPath;
    }

private:
    QString _projectName;
    QString _projectFSTPath;

};

class AvatarPackager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(AvatarProject* currentAvatarProject READ getAvatarProject NOTIFY avatarProjectChanged)
    Q_PROPERTY(QString AVATAR_PROJECTS_PATH READ getAvatarProjectsPath CONSTANT)
    Q_PROPERTY(QVariantList recentProjects READ getRecentProjects NOTIFY recentProjectsChanged)
public: 
    AvatarPackager();
    bool open();

    Q_INVOKABLE AvatarProject* createAvatarProject(const QString& projectsFolder, const QString& avatarProjectName, const QString& avatarModelPath, const QString& textureFolder);
    Q_INVOKABLE AvatarProject* openAvatarProject(const QString& avatarProjectFSTPath);
    Q_INVOKABLE bool isValidNewProjectName(const QString& projectPath, const QString& projectName) { return AvatarProject::isValidNewProjectName(projectPath, projectName); }

signals:
    void avatarProjectChanged();
    void recentProjectsChanged();

private:
    Q_INVOKABLE AvatarProject* getAvatarProject() const { return _currentAvatarProject; };
    Q_INVOKABLE QString getAvatarProjectsPath() const { return AvatarProject::getDefaultProjectsPath(); }
    Q_INVOKABLE QVariantList getRecentProjects() { return recentProjectsToVariantList(); }

    void addRecentProject(QString fstPath, QString projectName);

    AvatarProject* _currentAvatarProject{ nullptr };
    QVector<RecentAvatarProject> _recentProjects;
    QVariantList recentProjectsToVariantList() {
        QVariantList result;
        for (const auto& project : _recentProjects) {
            QVariantMap projectVariant;
            projectVariant.insert("name", project.getProjectName());
            projectVariant.insert("path", project.getProjectFSTPath());
            result.append(projectVariant);
        }
        
        return result;
    }

    void recentProjectsFromVariantList(QVariantList projectsVariant) {
        _recentProjects.clear();
        for (const auto& projectVariant : projectsVariant) {
            auto map = projectVariant.toMap();
            _recentProjects.append(RecentAvatarProject(map.value("name").toString(), map.value("path").toString()));
        }
    }


    Setting::Handle<QVariantList> _recentProjectsSetting{ "io.highfidelity.avatarPackager.recentProjects", QVariantList() };
};

#endif  // hifi_AvatarPackager_h
