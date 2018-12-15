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

class AvatarProject : public QObject {
    Q_OBJECT
    Q_PROPERTY(FST* fst READ getFST)

    Q_PROPERTY(QStringList projectFiles MEMBER _projectFiles)

    Q_PROPERTY(QString projectFolderPath READ getProjectPath)
    Q_PROPERTY(QString projectFSTPath READ getFSTPath)
    Q_PROPERTY(QString projectFBXPath READ getFBXPath)

public:
    Q_INVOKABLE bool write() {
        // Write FST here
        return false;
    }

    Q_INVOKABLE MarketplaceItemUploader* upload();

    /**
     * returns the AvatarProject or a nullptr on failure.
     */
    static AvatarProject* openAvatarProject(const QString& path);

private:
    AvatarProject(const QString& fstPath, const QByteArray& data);

    ~AvatarProject() {
        // TODO: cleanup FST / AvatarProjectUploader etc.
    }

    Q_INVOKABLE QString getProjectPath() const { return _projectPath; }
    Q_INVOKABLE QString getFSTPath() const { return _fstPath; }
    Q_INVOKABLE QString getFBXPath() const { return _fst.getModelPath(); }

    FST* getFST() { return &_fst; }

    void refreshProjectFiles();
    void appendDirectory(QString prefix, QDir dir);

    FST _fst;

    QDir _directory;
    QStringList _projectFiles{};
    QString _projectPath;
    QString _fstPath;
    QString _fstFilename;
};

#endif  // hifi_AvatarProject_h
