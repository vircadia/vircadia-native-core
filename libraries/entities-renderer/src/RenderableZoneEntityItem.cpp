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

bool RenderableZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    QString oldShapeURL = getCompoundShapeURL();
    bool somethingChanged = ZoneEntityItem::setProperties(properties);
    if (somethingChanged && oldShapeURL != getCompoundShapeURL()) {
        _compoundShapeModel = DependencyManager::get<GeometryCache>()->getGeometry(getCompoundShapeURL(), QUrl(), true);
    }
    return somethingChanged;
}

int RenderableZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                                ReadBitstreamToTreeParams& args,
                                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    QString oldShapeURL = getCompoundShapeURL();
    int bytesRead = ZoneEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead,
                                                                      args, propertyFlags, overwriteLocalData);
    if (oldShapeURL != getCompoundShapeURL()) {
        _compoundShapeModel = DependencyManager::get<GeometryCache>()->getGeometry(getCompoundShapeURL(), QUrl(), true);
    }
    return bytesRead;
}

bool RenderableZoneEntityItem::contains(const glm::vec3& point) const {
    if (getShapeType() != SHAPE_TYPE_COMPOUND) {
        return EntityItem::contains(point);
    }
    if (!_compoundShapeModel && hasCompoundShapeURL()) {
        const_cast<RenderableZoneEntityItem*>(this)->_compoundShapeModel = DependencyManager::get<GeometryCache>()->getGeometry(getCompoundShapeURL(), QUrl(), true);
    }
    
    if (EntityItem::contains(point) && _compoundShapeModel && _compoundShapeModel->isLoaded()) {
        const FBXGeometry& geometry = _compoundShapeModel->getFBXGeometry();
        glm::vec3 meshDimensions = geometry.getUnscaledMeshExtents().maximum - geometry.getUnscaledMeshExtents().minimum;
        return geometry.convexHullContains(worldToEntity(point) * meshDimensions);
    }
    
    return false;
}