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
    extents.transform(_transform);
    
    return AABox(extents);
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
        return vec2toScriptValue(_scriptEngine, getDimensions());
    }

    return Base3DOverlay::getProperty(property);
}

bool Planar3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    // FIXME - face and surfaceNormal not being returned
    return findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), getDimensions(), distance);
}
