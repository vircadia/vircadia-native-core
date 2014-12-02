//
//  ShapeInfo.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeInfo_h
#define hifi_ShapeInfo_h

#ifdef USE_BULLET_PHYSICS

//#include <btBulletDynamicsCommon.h>
#include <QVector>
#include <glm/glm.hpp>

class ShapeInfo {
public:
    ShapeInfo() : _type(INVALID_SHAPE_PROXYTYPE) {}


    void setBox(const btVector3& halfExtents);
    void setBox(const glm::vec3& halfExtents);
    void setSphere(float radius);
    void setCylinder(float radius, float height);
    void setCapsule(float radius, float height);

    int computeHash() const;
    int computeHash2() const;
   
    int _type;
    QVector<glm::vec3> _data;
};

#endif // USE_BULLET_PHYSICS
#endif // hifi_ShapeInfo_h

