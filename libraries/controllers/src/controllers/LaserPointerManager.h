//
//  LaserPointerManager.h
//  libraries/controllers/src
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointerManager_h
#define hifi_LaserPointerManager_h

#include <QHash>
#include <QString>
#include <memory>
#include "Transform.h"

class LaserPointer;

class LaserPointerManager {

public:
    static LaserPointerManager& getInstance();

    unsigned int createLaserPointer(const QString& jointName, const Transform& offsetTransform, const uint16_t filter, const float maxDistance, bool enabled);
    void removeLaserPointer(const unsigned int uid) { _laserPointers.remove(uid); }
    void enableLaserPointer(const unsigned int uid);
    void disableLaserPointer(const unsigned int uid);

private:
    QHash<unsigned int, std::shared_ptr<LaserPointer>> _laserPointers;

};

#endif hifi_LaserPointerManager_h