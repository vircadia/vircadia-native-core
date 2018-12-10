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

public:
    bool open();

    Q_INVOKABLE void openAvatarProjectWithoutReturnType() {
        openAvatarProject();
    }

    Q_INVOKABLE QObject* openAvatarProject();
};

#endif // hifi_AvatarPackager_h
