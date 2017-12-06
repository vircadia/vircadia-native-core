//
//  Text3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Text3DOverlay.h"

#include <TextureCache.h>
#include <GeometryCache.h>
#include <RegisteredMetaTypes.h>
#include <RenderDeferredTask.h>
#include <TextRenderer3D.h>

#include <AbstractViewStateInterface.h>

const int FIXED_FONT_POINT_SIZE = 40;
const int FIXED_FONT_SCALING_RATIO = FIXED_FONT_POINT_SIZE * 80.0f; // this is a ratio determined through experimentation
const float LINE_SCALE_RATIO = 1.2f;

QString const Text3DOverlay::TYPE = "text3d";

Text3DOverlay::Text3DOverlay() {
    _textRenderer = TextRenderer3D::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE);
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Text3DOverlay::Text3DOverlay(const Text3DOverlay* text3DOverlay) :
    Billboard3DOverlay(text3DOverlay),
    _text(text3DOverlay->_text),
    _backgroundColor(text3DOverlay->_backgroundColor),
    _textAlpha(text3DOverlay->_textAlpha),
    _lineHeight(text3DOverlay->_lineHeight),
    _leftMargin(text3DOverlay->_leftMargin),
    _topMargin(text3DOverlay->_topMargin),
    _rightMargin(text3DOverlay->_rightMargin),
    _bottomMargin(text3DOverlay->_bottomMargin)
{
    _textRenderer = TextRenderer3D::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE);
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Text3DOverlay::~Text3DOverlay() {
    delete _textRenderer;
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

const QString Text3DOverlay::getText() const {
    QMutexLocker lock(&_mutex);
    return _text;
}

void Text3DOverlay::setText(const QString& text) {
    QMutexLocker lock(&_mutex);
    _text = text;
}

xColor Text3DOverlay::getBackgroundColor() {
    if (_colorPulse == 0.0f) {
        return _backgroundColor;
    }

    float pulseLevel = updatePulse();
    xColor result = _backgroundColor;
    if (_colorPulse < 0.0f) {
        result.red *= (1.0f - pulseLevel);
        result.green *= (1.0f - pulseLevel);
        result.blue *= (1.0f - pulseLevel);
    } else {
        result.red *= pulseLevel;
        result.green *= pulseLevel;
        result.blue *= pulseLevel;
    }
    return result;
}

void Text3DOverlay::update(float deltatime) {
    if (usecTimestampNow() > _transformExpiry) {
        Transform transform = getTransform();
        applyTransformTo(transform);
        setTransform(transform);
    }
    Parent::update(deltatime);
}

void Text3DOverlay::render(RenderArgs* args) {
    if (!_renderVisible || !getParentVisible()) {
        return; // do nothing if we're not visible
    }

    Q_ASSERT(args->_batch);
    auto& batch = *args->_batch;

    auto transform = getRenderTransform();
    batch.setModelTransform(transform);

    const float MAX_COLOR = 255.0f;
    xColor backgroundColor = getBackgroundColor();
    glm::vec4 quadColor(backgroundColor.red / MAX_COLOR, backgroundColor.green / MAX_COLOR,
                        backgroundColor.blue / MAX_COLOR, getBackgroundAlpha());

    glm::vec2 dimensions = getDimensions();
    glm::vec2 halfDimensions = dimensions * 0.5f;

    const float SLIGHTLY_BEHIND = -0.001f;

    glm::vec3 topLeft(-halfDimensions.x, -halfDimensions.y, SLIGHTLY_BEHIND);
    glm::vec3 bottomRight(halfDimensions.x, halfDimensions.y, SLIGHTLY_BEHIND);
    DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch, false, quadColor.a < 1.0f, false, false, false, false);
    DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, quadColor, _geometryId);

    // Same font properties as textSize()
    float maxHeight = (float)_textRenderer->computeExtent("Xy").y * LINE_SCALE_RATIO;

    float scaleFactor =  (maxHeight / FIXED_FONT_SCALING_RATIO) * _lineHeight;

    glm::vec2 clipMinimum(0.0f, 0.0f);
    glm::vec2 clipDimensions((dimensions.x - (_leftMargin + _rightMargin)) / scaleFactor,
                             (dimensions.y - (_topMargin + _bottomMargin)) / scaleFactor);

    transform.postTranslate(glm::vec3(-(halfDimensions.x - _leftMargin),
                                      halfDimensions.y - _topMargin, 0.001f));
    transform.setScale(scaleFactor);
    batch.setModelTransform(transform);

    glm::vec4 textColor = { _color.red / MAX_COLOR, _color.green / MAX_COLOR,
                            _color.blue / MAX_COLOR, getTextAlpha() };

    // FIXME: Factor out textRenderer so that Text3DOverlay overlay parts can be grouped by pipeline for a gpu performance increase.
    _textRenderer->draw(batch, 0, 0, getText(), textColor, glm::vec2(-1.0f), true);
}

const render::ShapeKey Text3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder();
    if (isTransparent()) {
        builder.withTranslucent();
    }
    return builder.build();
}

void Text3DOverlay::setProperties(const QVariantMap& properties) {
    Billboard3DOverlay::setProperties(properties);

    auto text = properties["text"];
    if (text.isValid()) {
        setText(text.toString());
    }

    auto textAlpha = properties["textAlpha"];
    if (textAlpha.isValid()) {
        float prevTextAlpha = getTextAlpha();
        setTextAlpha(textAlpha.toFloat());
        // Update our payload key if necessary to handle transparency
        if ((prevTextAlpha < 1.0f && _textAlpha >= 1.0f) || (prevTextAlpha >= 1.0f && _textAlpha < 1.0f)) {
            auto itemID = getRenderItemID();
            if (render::Item::isValidID(itemID)) {
                render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
                render::Transaction transaction;
                transaction.updateItem(itemID);
                scene->enqueueTransaction(transaction);
            }
        }
    }

    bool valid;
    auto backgroundColor = properties["backgroundColor"];
    if (backgroundColor.isValid()) {
        auto color = xColorFromVariant(backgroundColor, valid);
        if (valid) {
            _backgroundColor = color;
        }
    }

    if (properties["backgroundAlpha"].isValid()) {
        setAlpha(properties["backgroundAlpha"].toFloat());
    }

    if (properties["lineHeight"].isValid()) {
        setLineHeight(properties["lineHeight"].toFloat());
    }

    if (properties["leftMargin"].isValid()) {
        setLeftMargin(properties["leftMargin"].toFloat());
    }

    if (properties["topMargin"].isValid()) {
        setTopMargin(properties["topMargin"].toFloat());
    }

    if (properties["rightMargin"].isValid()) {
        setRightMargin(properties["rightMargin"].toFloat());
    }

    if (properties["bottomMargin"].isValid()) {
        setBottomMargin(properties["bottomMargin"].toFloat());
    }
}

/**jsdoc
 * These are the properties of a <code>text3d</code> {@link Overlays.OverlayType|OverlayType}.
 * @typedef {object} Overlays.Text3DProperties
 *
 * @property {string} type=text3d - Has the value <code>"text3d"</code>. <em>Read-only.</em>
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
 * @property {boolean} isFacingAvatar - If <code>true</code>, the overlay is rotated to face the user's camera about an axis
 *     parallel to the user's avatar's "up" direction.
 *
 * @property {string} text="" - The text to display. Text does not automatically wrap; use <code>\n</code> for a line break.
 * @property {number} textAlpha=1 - The text alpha value.
 * @property {Color} backgroundColor=0,0,0 - The background color.
 * @property {number} backgroundAlpha=0.7 - The background alpha value.
 * @property {number} lineHeight=1 - The height of a line of text in meters.
 * @property {number} leftMargin=0.1 - The left margin, in meters.
 * @property {number} topMargin=0.1 - The top margin, in meters.
 * @property {number} rightMargin=0.1 - The right margin, in meters.
 * @property {number} bottomMargin=0.1 - The bottom margin, in meters.
 */

QVariant Text3DOverlay::getProperty(const QString& property) {
    if (property == "text") {
        return getText();
    }
    if (property == "textAlpha") {
        return _textAlpha;
    }
    if (property == "backgroundColor") {
        return xColorToVariant(_backgroundColor);
    }
    if (property == "backgroundAlpha") {
        return Billboard3DOverlay::getProperty("alpha");
    }
    if (property == "lineHeight") {
        return _lineHeight;
    }
    if (property == "leftMargin") {
        return _leftMargin;
    }
    if (property == "topMargin") {
        return _topMargin;
    }
    if (property == "rightMargin") {
        return _rightMargin;
    }
    if (property == "bottomMargin") {
        return _bottomMargin;
    }

    return Billboard3DOverlay::getProperty(property);
}

Text3DOverlay* Text3DOverlay::createClone() const {
    return new Text3DOverlay(this);;
}

QSizeF Text3DOverlay::textSize(const QString& text) const {
    auto extents = _textRenderer->computeExtent(text);

    float maxHeight = (float)_textRenderer->computeExtent("Xy").y * LINE_SCALE_RATIO;
    float pointToWorldScale = (maxHeight / FIXED_FONT_SCALING_RATIO) * _lineHeight;

    return QSizeF(extents.x, extents.y) * pointToWorldScale;
}

bool Text3DOverlay::findRayIntersection(const glm::vec3 &origin, const glm::vec3 &direction, float &distance,
                                            BoxFace &face, glm::vec3& surfaceNormal) {
    Transform transform = getTransform();
    applyTransformTo(transform, true);
    setTransform(transform);
    return Billboard3DOverlay::findRayIntersection(origin, direction, distance, face, surfaceNormal);
}

