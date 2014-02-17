//
//  Volume3DOverlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "Volume3DOverlay.h"

const float DEFAULT_SIZE = 1.0f;
const bool DEFAULT_IS_SOLID = false;

Volume3DOverlay::Volume3DOverlay() :
    _size(DEFAULT_SIZE),
    _isSolid(DEFAULT_IS_SOLID)
{
}

Volume3DOverlay::~Volume3DOverlay() {
}

void Volume3DOverlay::setProperties(const QScriptValue& properties) {
    Base3DOverlay::setProperties(properties);

    if (properties.property("size").isValid()) {
        setSize(properties.property("size").toVariant().toFloat());
    }

    if (properties.property("isSolid").isValid()) {
        setIsSolid(properties.property("isSolid").toVariant().toBool());
    }
    if (properties.property("isWire").isValid()) {
        setIsSolid(!properties.property("isWire").toVariant().toBool());
    }
    if (properties.property("solid").isValid()) {
        setIsSolid(properties.property("solid").toVariant().toBool());
    }
    if (properties.property("wire").isValid()) {
        setIsSolid(!properties.property("wire").toVariant().toBool());
    }
}
