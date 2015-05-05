//
//  RenderableZoneEntityItem.cpp
//
//
//  Created by Clement on 4/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableZoneEntityItem.h"

#include <DependencyManager.h>
#include <GeometryCache.h>

EntityItem* RenderableZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableZoneEntityItem(entityID, properties);
}

template<typename Lambda>
void RenderableZoneEntityItem::changeProperties(Lambda setNewProperties) {
    QString oldShapeURL = getCompoundShapeURL();
    glm::vec3 oldPosition = getPosition(), oldDimensions = getDimensions();
    glm::quat oldRotation = getRotation();
    
    setNewProperties();
    
    if (oldShapeURL != getCompoundShapeURL()) {
        if (!_model) {
            _model = getModel();
            _needsInitialSimulation = true;
        }
        _model->setURL(getCompoundShapeURL(), QUrl(), true, true);
    }
    if (oldPosition != getPosition() ||
        oldRotation != getRotation() ||
        oldDimensions != getDimensions()) {
        _needsInitialSimulation = true;
    }
}

bool RenderableZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    changeProperties([&]() {
        somethingChanged = this->ZoneEntityItem::setProperties(properties);
    });
    return somethingChanged;
}

int RenderableZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                                ReadBitstreamToTreeParams& args,
                                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    int bytesRead = 0;
    changeProperties([&]() {
        bytesRead = ZoneEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead,
                                                                     args, propertyFlags, overwriteLocalData);
    });
    return bytesRead;
}

Model* RenderableZoneEntityItem::getModel() {
    Model* model = new Model();
    model->init();
    return model;
}

void RenderableZoneEntityItem::initialSimulation() {
    _model->setScaleToFit(true, getDimensions());
    _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
    _model->setRotation(getRotation());
    _model->setTranslation(getPosition());
    _model->simulate(0.0f);
    _needsInitialSimulation = false;
}

void RenderableZoneEntityItem::render(RenderArgs* args) {
    if (_drawZoneBoundaries) {
        // TODO: Draw the zone boundaries...
    }
}

bool RenderableZoneEntityItem::contains(const glm::vec3& point) const {
    if (getShapeType() != SHAPE_TYPE_COMPOUND) {
        return EntityItem::contains(point);
    }
    
    if (_model && !_model->isActive() && hasCompoundShapeURL()) {
        // Since we have a delayload, we need to update the geometry if it has been downloaded
        _model->setURL(getCompoundShapeURL(), QUrl(), true);
    }
    
    if (_model && _model->isActive() && EntityItem::contains(point)) {
        if (_needsInitialSimulation) {
            const_cast<RenderableZoneEntityItem*>(this)->initialSimulation();
        }
        return _model->convexHullContains(point);
    }
    
    return false;
}
