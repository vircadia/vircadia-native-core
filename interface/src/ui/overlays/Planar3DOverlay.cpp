//
//  Planar3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <PlaneShape.h>
#include <RayIntersectionInfo.h>
#include <SharedUtil.h>
#include <StreamUtils.h>

#include "Planar3DOverlay.h"

const float DEFAULT_SIZE = 1.0f;

Planar3DOverlay::Planar3DOverlay() :
    _dimensions(glm::vec2(DEFAULT_SIZE, DEFAULT_SIZE))
{
}

Planar3DOverlay::~Planar3DOverlay() {
}

void Planar3DOverlay::setProperties(const QScriptValue& properties) {
    Base3DOverlay::setProperties(properties);

    QScriptValue dimensions = properties.property("dimensions");

    // if "dimensions" property was not there, check to see if they included aliases: scale
    if (!dimensions.isValid()) {
        dimensions = properties.property("scale");
        if (!dimensions.isValid()) {
            dimensions = properties.property("size");
        }
    }

    if (dimensions.isValid()) {
        bool validDimensions = false;
        glm::vec2 newDimensions;

        QScriptValue x = dimensions.property("x");
        QScriptValue y = dimensions.property("y");

        if (x.isValid() && y.isValid()) {
            newDimensions.x = x.toVariant().toFloat();
            newDimensions.y = y.toVariant().toFloat();
            validDimensions = true;
        } else {
            QScriptValue width = dimensions.property("width");
            QScriptValue height = dimensions.property("height");
            if (width.isValid() && height.isValid()) {
                newDimensions.x = width.toVariant().toFloat();
                newDimensions.y = height.toVariant().toFloat();
                validDimensions = true;
            }
        }

        // size, scale, dimensions is special, it might just be a single scalar, check that here
        if (!validDimensions && dimensions.isNumber()) {
            float size = dimensions.toVariant().toFloat();
            newDimensions.x = size;
            newDimensions.y = size;
            validDimensions = true;
        }

        if (validDimensions) {
            setDimensions(newDimensions);
        }
    }
}

QScriptValue Planar3DOverlay::getProperty(const QString& property) {
    if (property == "dimensions" || property == "scale" || property == "size") {
        return vec2toScriptValue(_scriptEngine, _dimensions);
    }

    return Base3DOverlay::getProperty(property);
}

bool Planar3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face) const {

    RayIntersectionInfo rayInfo;
    rayInfo._rayStart = origin;
    rayInfo._rayDirection = direction;
    rayInfo._rayLength = std::numeric_limits<float>::max();

    PlaneShape plane;

    const glm::vec3 UNROTATED_NORMAL(0.0f, 0.0f, -1.0f);
    glm::vec3 normal = _rotation * UNROTATED_NORMAL;
    plane.setNormal(normal);
    plane.setPoint(_position); // the position is definitely a point on our plane

    bool intersects = plane.findRayIntersection(rayInfo);

    if (intersects) {
        distance = rayInfo._hitDistance;
        // TODO: if it intersects, we want to check to see if the intersection point is within our dimensions
        // glm::vec3 hitAt = origin + direction * distance;
        // _dimensions
    }
    return intersects;
}

void Planar3DOverlay::writeToClone(Planar3DOverlay* clone) {
    Base3DOverlay::writeToClone(clone);
    clone->setDimensions(getDimensions());
}
