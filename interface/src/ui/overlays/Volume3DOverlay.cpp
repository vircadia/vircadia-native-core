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

void Volume3DOverlay::setProperties(const QVariantMap& properties) {
    Base3DOverlay::setProperties(properties);

    auto dimensions = properties["dimensions"];

    // if "dimensions" property was not there, check to see if they included aliases: scale, size
    if (!dimensions.isValid()) {
        dimensions = properties["scale"];
        if (!dimensions.isValid()) {
            dimensions = properties["size"];
        }
    }

    if (dimensions.isValid()) {
        setDimensions(vec3FromVariant(dimensions));
    }
}

QVariant Volume3DOverlay::getProperty(const QString& property) {
    if (property == "dimensions" || property == "scale" || property == "size") {
        return vec3toVariant(getDimensions());
    }

    return Base3DOverlay::getProperty(property);
}

bool Volume3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                            float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    // extents is the entity relative, scaled, centered extents of the entity
    glm::mat4 worldToEntityMatrix;
    Transform transform = getTransform();
#ifndef USE_SN_SCALE
    transform.setScale(1.0f);  // ignore any inherited scale from SpatiallyNestable
#endif
    transform.getInverseMatrix(worldToEntityMatrix);

    glm::vec3 overlayFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 overlayFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));

    // we can use the AABox's ray intersection by mapping our origin and direction into the overlays frame
    // and testing intersection there.
    return _localBoundingBox.findRayIntersection(overlayFrameOrigin, overlayFrameDirection, distance, face, surfaceNormal);
}
