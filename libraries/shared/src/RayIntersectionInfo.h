//
//  RayIntersectionInfo.h
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.09.09
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RayIntersectionInfo_h
#define hifi_RayIntersectionInfo_h

#include <glm/glm.hpp>

class Shape;

class RayIntersectionInfo {
public:
    RayIntersectionInfo() : _rayStart(0.0f), _rayDirection(1.0f, 0.0f, 0.0f), _rayLength(FLT_MAX), 
            _hitDistance(FLT_MAX), _hitNormal(1.0f, 0.0f, 0.0f), _hitShape(NULL) { }

    // input
    glm::vec3 _rayStart;
    glm::vec3 _rayDirection;
    float _rayLength;

    // output
    float _hitDistance;
    glm::vec3 _hitNormal;
    Shape* _hitShape;
};

#endif // hifi_RayIntersectionInfo_h
