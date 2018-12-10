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

class AvatarProject : public QObject {
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
    AvatarProject(QString fstPath) {
        
    }

    ~AvatarProject() {
        // TODO: cleanup FST / AvatarProjectUploader etc.
    }
};

#endif // hifi_AvatarProject_h
