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

#include "avatar/AvatarProject.h"

class AvatarPackager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(QObject* currentAvatarProject READ getAvatarProject NOTIFY avatarProjectChanged)

public:
    AvatarPackager();
    bool open();

    Q_INVOKABLE QObject* openAvatarProject(QString avatarProjectFSTPath);

signals:
    void avatarProjectChanged();

private:
    Q_INVOKABLE AvatarProject* getAvatarProject() const { return _currentAvatarProject; };
    //Q_INVOKABLE QObject* openAvatarProject();
    Q_INVOKABLE QObject* uploadItem();

    AvatarProject* _currentAvatarProject{ nullptr };
};

#endif  // hifi_AvatarPackager_h
