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

Shape3DOverlay::Shape3DOverlay(const Shape3DOverlay* shape3DOverlay) :
    Volume3DOverlay(shape3DOverlay),
    _shape(shape3DOverlay->_shape)
{
}

void Shape3DOverlay::render(RenderArgs* args) {
    if (!_renderVisible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 cubeColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;
    if (batch) {
        auto geometryCache = DependencyManager::get<GeometryCache>();
        auto shapePipeline = args->_shapePipeline;
        if (!shapePipeline) {
            shapePipeline = _isSolid ? geometryCache->getOpaqueShapePipeline() : geometryCache->getWireShapePipeline();
        }

        batch->setModelTransform(getRenderTransform());
        if (_isSolid) {
            geometryCache->renderSolidShapeInstance(args, *batch, _shape, cubeColor, shapePipeline);
        } else {
            geometryCache->renderWireShapeInstance(args, *batch, _shape, cubeColor, shapePipeline);
        }
    }
}

const render::ShapeKey Shape3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder();
    if (isTransparent()) {
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

Transform Shape3DOverlay::evalRenderTransform() {
    // TODO: handle registration point??
    glm::vec3 position = getWorldPosition();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getWorldOrientation();

    Transform transform;
    transform.setScale(dimensions);
    transform.setTranslation(position);
    transform.setRotation(rotation);
    return transform;
}
