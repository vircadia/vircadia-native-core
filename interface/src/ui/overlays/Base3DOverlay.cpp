//
//  Base3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Base3DOverlay.h"

#include <QScriptValue>

#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>


const float DEFAULT_LINE_WIDTH = 1.0f;
const bool DEFAULT_IS_SOLID = false;
const bool DEFAULT_IS_DASHED_LINE = false;

Base3DOverlay::Base3DOverlay() :
    _lineWidth(DEFAULT_LINE_WIDTH),
    _isSolid(DEFAULT_IS_SOLID),
    _isDashedLine(DEFAULT_IS_DASHED_LINE),
    _ignoreRayIntersection(false),
    _drawInFront(false),
    _drawOnHUD(false)
{
}

Base3DOverlay::Base3DOverlay(const Base3DOverlay* base3DOverlay) :
    Overlay(base3DOverlay),
    _transform(base3DOverlay->_transform),
    _lineWidth(base3DOverlay->_lineWidth),
    _isSolid(base3DOverlay->_isSolid),
    _isDashedLine(base3DOverlay->_isDashedLine),
    _ignoreRayIntersection(base3DOverlay->_ignoreRayIntersection),
    _drawInFront(base3DOverlay->_drawInFront),
    _drawOnHUD(base3DOverlay->_drawOnHUD)
{
}

void Base3DOverlay::setProperties(const QScriptValue& properties) {
    Overlay::setProperties(properties);

    QScriptValue drawInFront = properties.property("drawInFront");

    if (drawInFront.isValid()) {
        bool value = drawInFront.toVariant().toBool();
        setDrawInFront(value);
    }

    QScriptValue drawOnHUD = properties.property("drawOnHUD");

    if (drawOnHUD.isValid()) {
        bool value = drawOnHUD.toVariant().toBool();
        setDrawOnHUD(value);
    }

    QScriptValue position = properties.property("position");

    // if "position" property was not there, check to see if they included aliases: point, p1
    if (!position.isValid()) {
        position = properties.property("p1");
        if (!position.isValid()) {
            position = properties.property("point");
        }
    }

    if (position.isValid()) {
        QScriptValue x = position.property("x");
        QScriptValue y = position.property("y");
        QScriptValue z = position.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            setPosition(newPosition);
        }
    }

    if (properties.property("lineWidth").isValid()) {
        setLineWidth(properties.property("lineWidth").toVariant().toFloat());
    }

    QScriptValue rotation = properties.property("rotation");

    if (rotation.isValid()) {
        glm::quat newRotation;

        // size, scale, dimensions is special, it might just be a single scalar, or it might be a vector, check that here
        QScriptValue x = rotation.property("x");
        QScriptValue y = rotation.property("y");
        QScriptValue z = rotation.property("z");
        QScriptValue w = rotation.property("w");


        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            setRotation(newRotation);
        }
    }

    if (properties.property("isSolid").isValid()) {
        setIsSolid(properties.property("isSolid").toVariant().toBool());
    }
    if (properties.property("isFilled").isValid()) {
        setIsSolid(properties.property("isSolid").toVariant().toBool());
    }
    if (properties.property("isWire").isValid()) {
        setIsSolid(!properties.property("isWire").toVariant().toBool());
    }
    if (properties.property("solid").isValid()) {
        setIsSolid(properties.property("solid").toVariant().toBool());
    }
    if (properties.property("filled").isValid()) {
        setIsSolid(properties.property("filled").toVariant().toBool());
    }
    if (properties.property("wire").isValid()) {
        setIsSolid(!properties.property("wire").toVariant().toBool());
    }

    if (properties.property("isDashedLine").isValid()) {
        setIsDashedLine(properties.property("isDashedLine").toVariant().toBool());
    }
    if (properties.property("dashed").isValid()) {
        setIsDashedLine(properties.property("dashed").toVariant().toBool());
    }
    if (properties.property("ignoreRayIntersection").isValid()) {
        setIgnoreRayIntersection(properties.property("ignoreRayIntersection").toVariant().toBool());
    }
}

QScriptValue Base3DOverlay::getProperty(const QString& property) {
    if (property == "position" || property == "start" || property == "p1" || property == "point") {
        return vec3toScriptValue(_scriptEngine, getPosition());
    }
    if (property == "lineWidth") {
        return _lineWidth;
    }
    if (property == "rotation") {
        return quatToScriptValue(_scriptEngine, getRotation());
    }
    if (property == "isSolid" || property == "isFilled" || property == "solid" || property == "filed") {
        return _isSolid;
    }
    if (property == "isWire" || property == "wire") {
        return !_isSolid;
    }
    if (property == "isDashedLine" || property == "dashed") {
        return _isDashedLine;
    }
    if (property == "ignoreRayIntersection") {
        return _ignoreRayIntersection;
    }
    if (property == "drawInFront") {
        return _drawInFront;
    }
    if (property == "drawOnHUD") {
        return _drawOnHUD;
    }

    return Overlay::getProperty(property);
}

bool Base3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face) {
    return false;
}
