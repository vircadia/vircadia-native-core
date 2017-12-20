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
    if (!_renderVisible) {
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

        auto position = getWorldPosition();
        if (_followCamera) {
            // Get the camera position rounded to the nearest major grid line
            // This grid is for UI and should lie on worldlines
            auto cameraPosition =
                (float)_majorGridEvery * glm::round(args->getViewFrustum().getPosition() / (float)_majorGridEvery);

            position += glm::vec3(cameraPosition.x, 0.0f, cameraPosition.z);
        }

        Transform transform = getRenderTransform();
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

/**jsdoc
 * These are the properties of a <code>grid</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.GridProperties
 *
 * @property {string} type=grid - Has the value <code>"grid"</code>. <em>Read-only.</em>
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
 * @property {Vec2} dimensions=1,1 - The dimensions of the overlay. Synonyms: <code>scale</code>, <code>size</code>.
 *
 * @property {boolean} followCamera=true - If <code>true</code>, the grid is always visible even as the camera moves to another
 *     position.
 * @property {number} majorGridEvery=5 - Integer number of <code>minorGridEvery</code> intervals at which to draw a thick grid 
 *     line. Minimum value = <code>1</code>.
 * @property {number} minorGridEvery=1 - Real number of meters at which to draw thin grid lines. Minimum value = 
 *     <code>0.001</code>.
 */
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

Transform Grid3DOverlay::evalRenderTransform() {
    Transform transform;
    transform.setRotation(getWorldOrientation());
    transform.setScale(glm::vec3(getDimensions(), 1.0f));
    return transform;
}
