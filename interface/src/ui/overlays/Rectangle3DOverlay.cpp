//
//  Rectangle3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Rectangle3DOverlay.h"

#include <GeometryCache.h>
#include <SharedUtil.h>


QString const Rectangle3DOverlay::TYPE = "rectangle3d";

Rectangle3DOverlay::Rectangle3DOverlay() :
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _rectGeometryIds.size(); ++i) {
        _rectGeometryIds[i] = geometryCache->allocateID();
    }
    qDebug() << "Building rect3d overlay";
}

Rectangle3DOverlay::Rectangle3DOverlay(const Rectangle3DOverlay* rectangle3DOverlay) :
    Planar3DOverlay(rectangle3DOverlay),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _rectGeometryIds.size(); ++i) {
        _rectGeometryIds[i] = geometryCache->allocateID();
    }
    qDebug() << "Building rect3d overlay";
}

Rectangle3DOverlay::~Rectangle3DOverlay() {
    qDebug() << "Destryoing rect3d overlay";
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryCacheID);
        for (size_t i = 0; i < _rectGeometryIds.size(); ++i) {
            geometryCache->releaseID(_rectGeometryIds[i]);
        }
    }
}

void Rectangle3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 rectangleColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    glm::vec3 position = getPosition();
    glm::vec2 dimensions = getDimensions();
    glm::vec2 halfDimensions = dimensions * 0.5f;
    glm::quat rotation = getRotation();

    auto batch = args->_batch;

    if (batch) {
        Transform transform;
        transform.setTranslation(position);
        transform.setRotation(rotation);

        batch->setModelTransform(transform);
        auto geometryCache = DependencyManager::get<GeometryCache>();

        if (getIsSolid()) {
            glm::vec3 topLeft(-halfDimensions.x, -halfDimensions.y, 0.0f);
            glm::vec3 bottomRight(halfDimensions.x, halfDimensions.y, 0.0f);
            geometryCache->bindSimpleProgram(*batch);
            geometryCache->renderQuad(*batch, topLeft, bottomRight, rectangleColor, _geometryCacheID);
        } else {
            geometryCache->bindSimpleProgram(*batch, false, false, false, true, true);
            if (getIsDashedLine()) {
                glm::vec3 point1(-halfDimensions.x, -halfDimensions.y, 0.0f);
                glm::vec3 point2(halfDimensions.x, -halfDimensions.y, 0.0f);
                glm::vec3 point3(halfDimensions.x, halfDimensions.y, 0.0f);
                glm::vec3 point4(-halfDimensions.x, halfDimensions.y, 0.0f);

                geometryCache->renderDashedLine(*batch, point1, point2, rectangleColor, _rectGeometryIds[0]);
                geometryCache->renderDashedLine(*batch, point2, point3, rectangleColor, _rectGeometryIds[1]);
                geometryCache->renderDashedLine(*batch, point3, point4, rectangleColor, _rectGeometryIds[2]);
                geometryCache->renderDashedLine(*batch, point4, point1, rectangleColor, _rectGeometryIds[3]);
            } else {
                if (halfDimensions != _previousHalfDimensions) {
                    QVector<glm::vec3> border;
                    border << glm::vec3(-halfDimensions.x, -halfDimensions.y, 0.0f);
                    border << glm::vec3(halfDimensions.x, -halfDimensions.y, 0.0f);
                    border << glm::vec3(halfDimensions.x, halfDimensions.y, 0.0f);
                    border << glm::vec3(-halfDimensions.x, halfDimensions.y, 0.0f);
                    border << glm::vec3(-halfDimensions.x, -halfDimensions.y, 0.0f);
                    geometryCache->updateVertices(_geometryCacheID, border, rectangleColor);

                    _previousHalfDimensions = halfDimensions;
                }
                geometryCache->renderVertices(*batch, gpu::LINE_STRIP, _geometryCacheID);
            }
        }
    }
}

const render::ShapeKey Rectangle3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withOwnPipeline();
    if (getAlpha() != 1.0f) {
        builder.withTranslucent();
    }
    return builder.build();
}

void Rectangle3DOverlay::setProperties(const QVariantMap& properties) {
    Planar3DOverlay::setProperties(properties);
}

Rectangle3DOverlay* Rectangle3DOverlay::createClone() const {
    return new Rectangle3DOverlay(this);
}
