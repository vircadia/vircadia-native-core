//
//  Volume3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Volume3DOverlay.h"

#include <Extents.h>
#include <RegisteredMetaTypes.h>

Volume3DOverlay::Volume3DOverlay(const Volume3DOverlay* volume3DOverlay) :
    Base3DOverlay(volume3DOverlay)
{
}

AABox Volume3DOverlay::getBounds() const {
    auto extents = Extents{_localBoundingBox};
    extents.rotate(getRotation());
    extents.shiftBy(getPosition());
    
    return AABox(extents);
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


        if (x.isValid() && x.isNumber() &&
            y.isValid() && y.isNumber() &&
            z.isValid() && z.isNumber()) {
            newDimensions.x = x.toNumber();
            newDimensions.y = y.toNumber();
            newDimensions.z = z.toNumber();
            validDimensions = true;
        } else {
            QScriptValue width = dimensions.property("width");
            QScriptValue height = dimensions.property("height");
            QScriptValue depth = dimensions.property("depth");
            if (width.isValid() && width.isNumber() &&
                height.isValid() && height.isNumber() &&
                depth.isValid() && depth.isNumber()) {
                newDimensions.x = width.toNumber();
                newDimensions.y = height.toNumber();
                newDimensions.z = depth.toNumber();
                validDimensions = true;
            }
        }

        // size, scale, dimensions is special, it might just be a single scalar, check that here
        if (!validDimensions && dimensions.isNumber()) {
            float size = dimensions.toNumber();
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

QScriptValue Volume3DOverlay::getProperty(const QString& property) {
    if (property == "dimensions" || property == "scale" || property == "size") {
        return vec3toScriptValue(_scriptEngine, getDimensions());
    }

    return Base3DOverlay::getProperty(property);
}

bool Volume3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                            float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    // extents is the entity relative, scaled, centered extents of the entity
    glm::mat4 worldToEntityMatrix;
    _transform.getInverseMatrix(worldToEntityMatrix);
    
    glm::vec3 overlayFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 overlayFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));

    // we can use the AABox's ray intersection by mapping our origin and direction into the overlays frame
    // and testing intersection there.
    return _localBoundingBox.findRayIntersection(overlayFrameOrigin, overlayFrameDirection, distance, face, surfaceNormal);
}
