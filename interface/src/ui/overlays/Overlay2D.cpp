//
//  Overlay2D.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Overlay2D.h"

#include <RegisteredMetaTypes.h>

Overlay2D::Overlay2D(const Overlay2D* overlay2D) :
    Overlay(overlay2D),
    _bounds(overlay2D->_bounds)
{
}

AABox Overlay2D::getBounds() const {
    return AABox(glm::vec3(_bounds.x(), _bounds.y(), 0.0f),
                 glm::vec3(_bounds.width(), _bounds.height(), 0.01f));
}

void Overlay2D::setProperties(const QVariantMap& properties) {
    Overlay::setProperties(properties);
    
    auto bounds = properties["bounds"];
    if (bounds.isValid()) {
        bool valid;
        auto rect = qRectFromVariant(bounds, valid);
        setBounds(rect);
    } else {
        QRect oldBounds = _bounds;
        QRect newBounds = oldBounds;
        if (properties["x"].isValid()) {
            newBounds.setX(properties["x"].toInt());
        } else {
            newBounds.setX(oldBounds.x());
        }
        if (properties["y"].isValid()) {
            newBounds.setY(properties["y"].toInt());
        } else {
            newBounds.setY(oldBounds.y());
        }
        if (properties["width"].isValid()) {
            newBounds.setWidth(properties["width"].toInt());
        } else {
            newBounds.setWidth(oldBounds.width());
        }
        if (properties["height"].isValid()) {
            newBounds.setHeight(properties["height"].toInt());
        } else {
            newBounds.setHeight(oldBounds.height());
        }
        setBounds(newBounds);
    }
}

QVariant Overlay2D::getProperty(const QString& property) {
    if (property == "bounds") {
        return qRectToVariant(_bounds);
    }
    if (property == "x") {
        return _bounds.x();
    }
    if (property == "y") {
        return _bounds.y();
    }
    if (property == "width") {
        return _bounds.width();
    }
    if (property == "height") {
        return _bounds.height();
    }

    return Overlay::getProperty(property);
}
