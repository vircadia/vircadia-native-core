//
//  Volume3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <AABox.h>
#include <SharedUtil.h>
#include <StreamUtils.h>

#include "Volume3DOverlay.h"

const float DEFAULT_SIZE = 1.0f;

Volume3DOverlay::Volume3DOverlay() :
    _dimensions(glm::vec3(DEFAULT_SIZE, DEFAULT_SIZE, DEFAULT_SIZE))
{
}

Volume3DOverlay::~Volume3DOverlay() {
}

void Volume3DOverlay::setProperties(const QScriptValue& properties) {
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
        glm::vec3 newDimensions;

        QScriptValue x = dimensions.property("x");
        QScriptValue y = dimensions.property("y");
        QScriptValue z = dimensions.property("z");


        if (x.isValid() && y.isValid() && z.isValid()) {
            newDimensions.x = x.toVariant().toFloat();
            newDimensions.y = y.toVariant().toFloat();
            newDimensions.z = z.toVariant().toFloat();
            validDimensions = true;
        } else {
            QScriptValue width = dimensions.property("width");
            QScriptValue height = dimensions.property("height");
            QScriptValue depth = dimensions.property("depth");
            if (width.isValid() && height.isValid() && depth.isValid()) {
                newDimensions.x = width.toVariant().toFloat();
                newDimensions.y = height.toVariant().toFloat();
                newDimensions.z = depth.toVariant().toFloat();
                validDimensions = true;
            }
        }

        // size, scale, dimensions is special, it might just be a single scalar, check that here
        if (!validDimensions && dimensions.isNumber()) {
            float size = dimensions.toVariant().toFloat();
            newDimensions.x = size;
            newDimensions.y = size;
            newDimensions.z = size;
            validDimensions = true;
        }

        if (validDimensions) {
            setDimensions(newDimensions);
        }
    }
}

bool Volume3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face) const {

    // extents is the entity relative, scaled, centered extents of the entity
    glm::vec3 position = getPosition();
    glm::mat4 rotation = glm::mat4_cast(getRotation());
    glm::mat4 translation = glm::translate(position);
    glm::mat4 entityToWorldMatrix = translation * rotation;
    glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

    glm::vec3 dimensions = _dimensions;
    glm::vec3 corner = dimensions * -0.5f; // since we're going to do the ray picking in the overlay frame of reference
    AABox overlayFrameBox(corner, dimensions);
    glm::vec3 overlayFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 overlayFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));

    // we can use the AABox's ray intersection by mapping our origin and direction into the overlays frame
    // and testing intersection there.
    if (overlayFrameBox.findRayIntersection(overlayFrameOrigin, overlayFrameDirection, distance, face)) {
        return true;
    }
    return false;
}

void Volume3DOverlay::writeToClone(Volume3DOverlay* clone) {
    Base3DOverlay::writeToClone(clone);
}
