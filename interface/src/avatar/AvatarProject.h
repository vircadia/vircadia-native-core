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

#include <QObject>
#include <QDir>
#include <QFileInfo>

class AvatarProject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString projectFolderPath READ getProjectPath)
    Q_PROPERTY(QString projectFSTPath READ getFSTPath)
    Q_PROPERTY(QString projectFBXPath READ getFBXPath)

public:
    Q_INVOKABLE bool write() {
        // Write FST here
        return false;
    }

    Q_INVOKABLE QObject* upload() {
        // TODO: create new AvatarProjectUploader here, launch it and return it for status tracking in QML
        return nullptr;
    }

    /**
     * returns the AvatarProject or a nullptr on failure.
     */
    static AvatarProject* openAvatarProject(QString path);

private:
    AvatarProject(QString fstPath) :
        _fstPath(fstPath) {
        auto fileInfo = QFileInfo(_fstPath);
        _projectPath = fileInfo.absoluteDir().absolutePath();

        _fstPath = _projectPath + "TemporaryFBX.fbx";

    }

    ~AvatarProject() {
        // TODO: cleanup FST / AvatarProjectUploader etc.
    }

    Q_INVOKABLE QString getProjectPath() const { return _projectPath; }
    Q_INVOKABLE QString getFSTPath() const { return _fstPath; }
    Q_INVOKABLE QString getFBXPath() const { return _fbxPath; }

    QString _projectPath;
    QString _fstPath;
    // TODO: handle this in the FST Class
    QString _fbxPath;
    
};

#endif // hifi_AvatarProject_h
