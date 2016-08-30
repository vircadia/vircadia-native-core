//
//  Planar3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Planar3DOverlay.h"

#include <Extents.h>
#include <GeometryUtil.h>
#include <RegisteredMetaTypes.h>

Planar3DOverlay::Planar3DOverlay() :
    Base3DOverlay(),
    _dimensions{1.0f, 1.0f}
{
}

Planar3DOverlay::Planar3DOverlay(const Planar3DOverlay* planar3DOverlay) :
    Base3DOverlay(planar3DOverlay),
    _dimensions(planar3DOverlay->_dimensions)
{
}

AABox Planar3DOverlay::getBounds() const {
    auto halfDimensions = glm::vec3{_dimensions / 2.0f, 0.01f};

    auto extents = Extents{-halfDimensions, halfDimensions};
    extents.transform(getTransform());

    return AABox(extents);
}

void Planar3DOverlay::setProperties(const QVariantMap& properties) {
    Base3DOverlay::setProperties(properties);

    auto dimensions = properties["dimensions"];

    // if "dimensions" property was not there, check to see if they included aliases: scale
    if (!dimensions.isValid()) {
        dimensions = properties["scale"];
        if (!dimensions.isValid()) {
            dimensions = properties["size"];
        }
    }

    if (dimensions.isValid()) {
        setDimensions(vec2FromVariant(dimensions));
    }
}

QVariant Planar3DOverlay::getProperty(const QString& property) {
    if (property == "dimensions" || property == "scale" || property == "size") {
        return vec2toVariant(getDimensions());
    }

    return Base3DOverlay::getProperty(property);
}

bool Planar3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    // FIXME - face and surfaceNormal not being returned
    return findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), getDimensions(), distance);
}
