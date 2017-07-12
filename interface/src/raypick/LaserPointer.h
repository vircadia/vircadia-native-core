//
//  LaserPointer.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointer_h
#define hifi_LaserPointer_h

#include <QString>
#include "glm/glm.hpp"

class LaserPointer {

public:
    LaserPointer(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance, bool enabled);
    ~LaserPointer();

    unsigned int getUID() { return _rayPickUID; }
    void enable();
    void disable();

    // void setRenderState(const QString& stateName);
    // void setRenderStateProperties(const QHash<QString, triplet of properties>& renderStateProperties);

private:
    unsigned int _rayPickUID;
};

#endif hifi_LaserPointer_h