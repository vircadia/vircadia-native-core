//
//  Shape3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "Shape3DOverlay.h"

#include <SharedUtil.h>
#include <StreamUtils.h>
#include <GeometryCache.h>
#include <DependencyManager.h>

QString const Shape3DOverlay::TYPE = "shape";

Shape3DOverlay::Shape3DOverlay(const Shape3DOverlay* Shape3DOverlay) :
    Volume3DOverlay(Shape3DOverlay)
{
}

void Shape3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 cubeColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    // TODO: handle registration point??
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();

    auto batch = args->_batch;

    if (batch) {
        Transform transform;
        transform.setTranslation(position);
        transform.setRotation(rotation);
        auto geometryCache = DependencyManager::get<GeometryCache>();
        auto pipeline = args->_pipeline;
        if (!pipeline) {
            pipeline = _isSolid ? geometryCache->getOpaqueShapePipeline() : geometryCache->getWireShapePipeline();
        }

        transform.setScale(dimensions);
        batch->setModelTransform(transform);
        if (_isSolid) {
            geometryCache->renderSolidShapeInstance(*batch, _shape, cubeColor, pipeline);
        } else {
            geometryCache->renderWireShapeInstance(*batch, _shape, cubeColor, pipeline);
        }
    }
}

const render::ShapeKey Shape3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder();
    if (getAlpha() != 1.0f) {
        builder.withTranslucent();
    }
    if (!getIsSolid()) {
        builder.withUnlit().withDepthBias();
    }
    return builder.build();
}

Shape3DOverlay* Shape3DOverlay::createClone() const {
    return new Shape3DOverlay(this);
}


static const std::array<QString, GeometryCache::Shape::NUM_SHAPES> shapeStrings { {
    "Line",
    "Triangle",
    "Quad",
    "Hexagon",
    "Octagon",
    "Circle",
    "Cube",
    "Sphere",
    "Tetrahedron",
    "Octahedron",
    "Dodecahedron",
    "Icosahedron",
    "Torus",
    "Cone",
    "Cylinder"
} };


void Shape3DOverlay::setProperties(const QVariantMap& properties) {
    Volume3DOverlay::setProperties(properties);

    auto shape = properties["shape"];
    if (shape.isValid()) {
        const QString shapeStr = shape.toString();
        for (size_t i = 0; i < shapeStrings.size(); ++i) {
            if (shapeStr == shapeStrings[i]) {
                this->_shape = static_cast<GeometryCache::Shape>(i);
                break;
            }
        }
    }

    auto borderSize = properties["borderSize"];

    if (borderSize.isValid()) {
        float value = borderSize.toFloat();
        setBorderSize(value);
    }
}

QVariant Shape3DOverlay::getProperty(const QString& property) {
    if (property == "borderSize") {
        return _borderSize;
    }

    if (property == "shape") {
        return shapeStrings[_shape];
    }

    return Volume3DOverlay::getProperty(property);
}
