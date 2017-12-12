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

/**jsdoc
 * These are the properties of a <code>shape</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.ShapeProperties
 *
 * @property {string} type=shape - Has the value <code>"shape"</code>. <em>Read-only.</em>
 * @property {Color} color=255,255,255 - The color of the overlay.
 * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
 * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
 * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
 * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from
 *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
 * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the
 *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0
 *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
 *     used.)
 * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
 * @property {string} anchor="" - If set to <code>"MyAvatar"</code> then the overlay is attached to your avatar, moving and
 *     rotating as you move your avatar.
 *
 * @property {string} name="" - A friendly name for the overlay.
 * @property {Vec3} position - The position of the overlay center. Synonyms: <code>p1</code>, <code>point</code>, and
 *     <code>start</code>.
 * @property {Vec3} localPosition - The local position of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>position</code>.
 * @property {Quat} rotation - The orientation of the overlay. Synonym: <code>orientation</code>.
 * @property {Quat} localRotation - The orientation of the overlay relative to its parent if the overlay has a
 *     <code>parentID</code> set, otherwise the same value as <code>rotation</code>.
 * @property {boolean} isSolid=false - Synonyms: <ode>solid</code>, <code>isFilled</code>,
 *     <code>filled</code>, and <code>filed</code>. Antonyms: <code>isWire</code> and <code>wire</code>.
 *     <strong>Deprecated:</strong> The erroneous property spelling "<code>filed</code>" is deprecated and support for it will
 *     be removed.
 * @property {boolean} isDashedLine=false - If <code>true</code>, a dashed line is drawn on the overlay's edges. Synonym:
 *     <code>dashed</code>.
 * @property {boolean} ignoreRayIntersection=false - If <code>true</code>,
 *     {@link Overlays.findRayIntersection|findRayIntersection} ignores the overlay.
 * @property {boolean} drawInFront=false - If <code>true</code>, the overlay is rendered in front of other overlays that don't
 *     have <code>drawInFront</code> set to <code>true</code>, and in front of entities.
 * @property {boolean} grabbable=false - Signal to grabbing scripts whether or not this overlay can be grabbed.
 * @property {Uuid} parentID=null - The avatar, entity, or overlay that the overlay is parented to.
 * @property {number} parentJointIndex=65535 - Integer value specifying the skeleton joint that the overlay is attached to if
 *     <code>parentID</code> is an avatar skeleton. A value of <code>65535</code> means "no joint".
 *
 * @property {Vec3} dimensions - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {Shape} shape=Hexagon - The geometrical shape of the overlay.
 * @property {number} borderSize - Not used.
 */
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
