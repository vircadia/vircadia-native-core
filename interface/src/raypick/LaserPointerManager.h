//
//  LaserPointerManager.h
//  interface/src/raypick
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
#include <glm/glm.hpp>

class LaserPointer;
class RayPickResult;

class LaserPointerManager {

public:
    static LaserPointerManager& getInstance();

    unsigned int createLaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance, bool enabled);
    void removeLaserPointer(const unsigned int uid) { _laserPointers.remove(uid); }
    void enableLaserPointer(const unsigned int uid);
    void disableLaserPointer(const unsigned int uid);
    const RayPickResult& getPrevRayPickResult(const unsigned int uid);

private:
    QHash<unsigned int, std::shared_ptr<LaserPointer>> _laserPointers;

};

#endif hifi_LaserPointerManager_h