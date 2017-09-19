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

#include <OctreeConstants.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <PathUtils.h>
#include <ViewFrustum.h>


QString const Grid3DOverlay::TYPE = "grid";
const float DEFAULT_SCALE = 100.0f;

Grid3DOverlay::Grid3DOverlay() {
    setDimensions(DEFAULT_SCALE);
    updateGrid();
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Grid3DOverlay::Grid3DOverlay(const Grid3DOverlay* grid3DOverlay) :
    Planar3DOverlay(grid3DOverlay),
    _majorGridEvery(grid3DOverlay->_majorGridEvery),
    _minorGridEvery(grid3DOverlay->_minorGridEvery)
{
    updateGrid();
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Grid3DOverlay::~Grid3DOverlay() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

AABox Grid3DOverlay::getBounds() const {
    if (_followCamera) {
        // This is a UI element that should always be in view, lie to the octree to avoid culling
        const AABox DOMAIN_BOX = AABox(glm::vec3(-TREE_SCALE / 2), TREE_SCALE);
        return DOMAIN_BOX;
    }
    return Planar3DOverlay::getBounds();
}

void Grid3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const float MAX_COLOR = 255.0f;

    float alpha = getAlpha();
    xColor color = getColor();
    glm::vec4 gridColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;

    if (batch) {
        auto minCorner = glm::vec2(-0.5f, -0.5f);
        auto maxCorner = glm::vec2(0.5f, 0.5f);

        auto position = getPosition();
        if (_followCamera) {
            // Get the camera position rounded to the nearest major grid line
            // This grid is for UI and should lie on worldlines
            auto cameraPosition =
                (float)_majorGridEvery * glm::round(args->getViewFrustum().getPosition() / (float)_majorGridEvery);

            position += glm::vec3(cameraPosition.x, 0.0f, cameraPosition.z);
        }

        // FIXME Start using the _renderTransform instead of calling for Transform from here, do the custom things needed in evalRenderTransform()
        Transform transform;
        transform.setRotation(getRotation());
        transform.setScale(glm::vec3(getDimensions(), 1.0f));
        transform.setTranslation(position);
        batch->setModelTransform(transform);
        const float MINOR_GRID_EDGE = 0.0025f;
        const float MAJOR_GRID_EDGE = 0.005f;
        DependencyManager::get<GeometryCache>()->renderGrid(*batch, minCorner, maxCorner,
            _minorGridRowDivisions, _minorGridColDivisions, MINOR_GRID_EDGE,
            _majorGridRowDivisions, _majorGridColDivisions, MAJOR_GRID_EDGE,
            gridColor, _drawInFront, _geometryId);
    }
}

const render::ShapeKey Grid3DOverlay::getShapeKey() {
    return render::ShapeKey::Builder().withOwnPipeline().withUnlit().withDepthBias();
}

void Grid3DOverlay::setProperties(const QVariantMap& properties) {
    Planar3DOverlay::setProperties(properties);
    if (properties["followCamera"].isValid()) {
        _followCamera = properties["followCamera"].toBool();
    }

    if (properties["majorGridEvery"].isValid()) {
        _majorGridEvery = properties["majorGridEvery"].toInt();
    }

    if (properties["minorGridEvery"].isValid()) {
        _minorGridEvery = properties["minorGridEvery"].toFloat();
    }

    updateGrid();
}

QVariant Grid3DOverlay::getProperty(const QString& property) {
    if (property == "followCamera") {
        return _followCamera;
    }
    if (property == "majorGridEvery") {
        return _majorGridEvery;
    }
    if (property == "minorGridEvery") {
        return _minorGridEvery;
    }

    return Planar3DOverlay::getProperty(property);
}

Grid3DOverlay* Grid3DOverlay::createClone() const {
    return new Grid3DOverlay(this);
}

void Grid3DOverlay::updateGrid() {
    const int MAJOR_GRID_EVERY_MIN = 1;
    const float MINOR_GRID_EVERY_MIN = 0.01f;

    _majorGridEvery = std::max(_majorGridEvery, MAJOR_GRID_EVERY_MIN);
    _minorGridEvery = std::max(_minorGridEvery, MINOR_GRID_EVERY_MIN);

    _majorGridRowDivisions = getDimensions().x / _majorGridEvery;
    _majorGridColDivisions = getDimensions().y / _majorGridEvery;

    _minorGridRowDivisions = getDimensions().x / _minorGridEvery;
    _minorGridColDivisions = getDimensions().y / _minorGridEvery;
}
