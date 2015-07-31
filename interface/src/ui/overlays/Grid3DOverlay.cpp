//
//  Grid3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Created by Ryan Huffman on 11/06/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Grid3DOverlay.h"

#include <QScriptValue>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <PathUtils.h>
#include <ViewFrustum.h>

Grid3DOverlay::Grid3DOverlay() :
    _minorGridWidth(1.0),
    _majorGridEvery(5) {
}

Grid3DOverlay::Grid3DOverlay(const Grid3DOverlay* grid3DOverlay) :
    Planar3DOverlay(grid3DOverlay),
    _minorGridWidth(grid3DOverlay->_minorGridWidth),
    _majorGridEvery(grid3DOverlay->_majorGridEvery)
{
}

void Grid3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const int MINOR_GRID_DIVISIONS = 200;
    const int MAJOR_GRID_DIVISIONS = 100;
    const float MAX_COLOR = 255.0f;

    // center the grid around the camera position on the plane
    glm::vec3 rotated = glm::inverse(getRotation()) * args->_viewFrustum->getPosition();

    float spacing = _minorGridWidth;

    float alpha = getAlpha();
    xColor color = getColor();
    glm::vec4 gridColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;

    if (batch) {
        Transform transform;
        transform.setRotation(getRotation());


        // Minor grid
        {
            batch->_glLineWidth(1.0f);
            auto position = glm::vec3(_minorGridWidth * (floorf(rotated.x / spacing) - MINOR_GRID_DIVISIONS / 2),
                                      spacing * (floorf(rotated.y / spacing) - MINOR_GRID_DIVISIONS / 2),
                                      getPosition().z);
            float scale = MINOR_GRID_DIVISIONS * spacing;

            transform.setTranslation(position);
            transform.setScale(scale);

            batch->setModelTransform(transform);

            DependencyManager::get<GeometryCache>()->renderGrid(*batch, MINOR_GRID_DIVISIONS, MINOR_GRID_DIVISIONS, gridColor);
        }

        // Major grid
        {
            batch->_glLineWidth(4.0f);
            spacing *= _majorGridEvery;
            auto position = glm::vec3(spacing * (floorf(rotated.x / spacing) - MAJOR_GRID_DIVISIONS / 2),
                                      spacing * (floorf(rotated.y / spacing) - MAJOR_GRID_DIVISIONS / 2),
                                      getPosition().z);
            float scale = MAJOR_GRID_DIVISIONS * spacing;

            transform.setTranslation(position);
            transform.setScale(scale);

            batch->setModelTransform(transform);

            DependencyManager::get<GeometryCache>()->renderGrid(*batch, MAJOR_GRID_DIVISIONS, MAJOR_GRID_DIVISIONS, gridColor);
        }
    }
}

void Grid3DOverlay::setProperties(const QScriptValue& properties) {
    Planar3DOverlay::setProperties(properties);

    if (properties.property("minorGridWidth").isValid()) {
        _minorGridWidth = properties.property("minorGridWidth").toVariant().toFloat();
    }

    if (properties.property("majorGridEvery").isValid()) {
        _majorGridEvery = properties.property("majorGridEvery").toVariant().toInt();
    }
}

QScriptValue Grid3DOverlay::getProperty(const QString& property) {
    if (property == "minorGridWidth") {
        return _minorGridWidth;
    }
    if (property == "majorGridEvery") {
        return _majorGridEvery;
    }

    return Planar3DOverlay::getProperty(property);
}

Grid3DOverlay* Grid3DOverlay::createClone() const {
    return new Grid3DOverlay(this);
}

