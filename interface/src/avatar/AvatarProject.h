//
//  AvatarProject.h
//
//
//  Created by Thijs Wenker on 12/7/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_AvatarProject_h
#define hifi_AvatarProject_h

#include "MarketplaceItemUploader.h"
#include "FST.h"

#include <QDir>
#include <QObject>
#include <QDir>
#include <QFileInfo>
#include <QVariantHash>
#include <QUuid>
#include <QStandardPaths>

class AvatarProject : public QObject {
    Q_OBJECT
    Q_PROPERTY(FST* fst READ getFST)

    Q_PROPERTY(QStringList projectFiles READ getProjectFiles NOTIFY projectFilesChanged)

    Q_PROPERTY(QString projectFolderPath READ getProjectPath)
    Q_PROPERTY(QString projectFSTPath READ getFSTPath)
    Q_PROPERTY(QString projectFBXPath READ getFBXPath)
    Q_PROPERTY(QString name READ getProjectName)

public:
    Q_INVOKABLE bool write() {
        // Write FST here
        return false;
    }

    Q_INVOKABLE MarketplaceItemUploader* upload(bool updateExisting);
    Q_INVOKABLE void openInInventory();
    Q_INVOKABLE QStringList getProjectFiles() const { return _projectFiles; }

    /**
     * returns the AvatarProject or a nullptr on failure.
     */
    static AvatarProject* openAvatarProject(const QString& path);
    static AvatarProject* createAvatarProject(const QString& avatarProjectName, const QString& avatarModelPath);

    static bool isValidNewProjectName(const QString& projectName);

    static QString getDefaultProjectsPath() {
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/High Fidelity Projects";
    }

private:
    AvatarProject(const QString& fstPath, const QByteArray& data);
    AvatarProject(FST* fst);

    ~AvatarProject() {
        // TODO: cleanup FST / AvatarProjectUploader etc.
    }

    Q_INVOKABLE QString getProjectName() const { return _fst->getName(); }
    Q_INVOKABLE QString getProjectPath() const { return _projectPath; }
    Q_INVOKABLE QString getFSTPath() const { return _fst->getPath(); }
    Q_INVOKABLE QString getFBXPath() const { return _fst->getModelPath(); }

    FST* getFST() { return _fst; }

    void refreshProjectFiles();
    void appendDirectory(QString prefix, QDir dir);

    FST* _fst;

    QDir _directory;
    QStringList _projectFiles{};
    QString _projectPath;
};

#endif  // hifi_AvatarProject_h
