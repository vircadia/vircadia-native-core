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
