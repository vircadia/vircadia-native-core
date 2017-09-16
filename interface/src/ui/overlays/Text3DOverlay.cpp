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
}

void Text3DOverlay::render(RenderArgs* args) {
    if (!_visible || !getParentVisible()) {
        return; // do nothing if we're not visible
    }

    Q_ASSERT(args->_batch);
    auto& batch = *args->_batch;

    // FIXME Start using the _renderTransform instead of calling for Transform and Dimensions from here, do the custom things needed in evalRenderTransform()
    Transform transform = getTransform();
    applyTransformTo(transform, true);
    setTransform(transform);
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

    // FIXME: Factor out textRenderer so that Text3DOverlay overlay parts can be grouped by pipeline
    //        for a gpu performance increase. Currently,
    //        Text renderer sets its own pipeline,
    _textRenderer->draw(batch, 0, 0, getText(), textColor, glm::vec2(-1.0f), getDrawInFront());
    //        so before we continue, we must reset the pipeline
    batch.setPipeline(args->_shapePipeline->pipeline);
    args->_shapePipeline->prepare(batch, args);
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
        setTextAlpha(textAlpha.toFloat());
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
