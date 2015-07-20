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

#include <DeferredLightingEffect.h>
#include <RenderDeferredTask.h>
#include <TextRenderer3D.h>

#include "Application.h"

const xColor DEFAULT_BACKGROUND_COLOR = { 0, 0, 0 };
const float DEFAULT_BACKGROUND_ALPHA = 0.7f;
const float DEFAULT_MARGIN = 0.1f;
const int FIXED_FONT_POINT_SIZE = 40;
const int FIXED_FONT_SCALING_RATIO = FIXED_FONT_POINT_SIZE * 80.0f; // this is a ratio determined through experimentation
const float LINE_SCALE_RATIO = 1.2f;

Text3DOverlay::Text3DOverlay() :
    _backgroundColor(DEFAULT_BACKGROUND_COLOR),
    _backgroundAlpha(DEFAULT_BACKGROUND_ALPHA),
    _lineHeight(0.1f),
    _leftMargin(DEFAULT_MARGIN),
    _topMargin(DEFAULT_MARGIN),
    _rightMargin(DEFAULT_MARGIN),
    _bottomMargin(DEFAULT_MARGIN),
    _isFacingAvatar(false)
{
    _textRenderer = TextRenderer3D::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE);
}

Text3DOverlay::Text3DOverlay(const Text3DOverlay* text3DOverlay) :
    Planar3DOverlay(text3DOverlay),
    _text(text3DOverlay->_text),
    _backgroundColor(text3DOverlay->_backgroundColor),
    _backgroundAlpha(text3DOverlay->_backgroundAlpha),
    _lineHeight(text3DOverlay->_lineHeight),
    _leftMargin(text3DOverlay->_leftMargin),
    _topMargin(text3DOverlay->_topMargin),
    _rightMargin(text3DOverlay->_rightMargin),
    _bottomMargin(text3DOverlay->_bottomMargin),
    _isFacingAvatar(text3DOverlay->_isFacingAvatar)
{
     _textRenderer = TextRenderer3D::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE);
}

Text3DOverlay::~Text3DOverlay() {
    delete _textRenderer;
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


void Text3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    
    Q_ASSERT(args->_batch);
    auto& batch = *args->_batch;
    
    glm::quat rotation;
    
    if (_isFacingAvatar) {
        // rotate about vertical to face the camera
        rotation = args->_viewFrustum->getOrientation();
    } else {
        rotation = getRotation();
    }
    
    Transform transform;
    transform.setTranslation(getPosition());
    transform.setRotation(rotation);
    transform.setScale(getScale());
    
    batch.setModelTransform(transform);
    
    const float MAX_COLOR = 255.0f;
    xColor backgroundColor = getBackgroundColor();
    glm::vec4 quadColor(backgroundColor.red / MAX_COLOR, backgroundColor.green / MAX_COLOR, backgroundColor.blue / MAX_COLOR,
                        getBackgroundAlpha());
    
    glm::vec2 dimensions = getDimensions();
    glm::vec2 halfDimensions = dimensions * 0.5f;
    
    const float SLIGHTLY_BEHIND = -0.005f;
    
    glm::vec3 topLeft(-halfDimensions.x, -halfDimensions.y, SLIGHTLY_BEHIND);
    glm::vec3 bottomRight(halfDimensions.x, halfDimensions.y, SLIGHTLY_BEHIND);
    DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch, false, true, false, true);
    DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, quadColor);
    
    // Same font properties as textSize()
    float maxHeight = (float)_textRenderer->computeExtent("Xy").y * LINE_SCALE_RATIO;
    
    float scaleFactor =  (maxHeight / FIXED_FONT_SCALING_RATIO) * _lineHeight;
    
    glm::vec2 clipMinimum(0.0f, 0.0f);
    glm::vec2 clipDimensions((dimensions.x - (_leftMargin + _rightMargin)) / scaleFactor,
                             (dimensions.y - (_topMargin + _bottomMargin)) / scaleFactor);
    
    transform.setTranslation(getPosition());
    transform.postTranslate(glm::vec3(-(halfDimensions.x - _leftMargin) , halfDimensions.y - _topMargin, 0.01f));
    transform.setScale(scaleFactor);
    batch.setModelTransform(transform);
    
    glm::vec4 textColor = { _color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR, getAlpha() };
    _textRenderer->draw(batch, 0, 0, _text, textColor);
    
    batch.setPipeline(DrawOverlay3D::getOpaquePipeline());
}

void Text3DOverlay::setProperties(const QScriptValue& properties) {
    Planar3DOverlay::setProperties(properties);

    QScriptValue text = properties.property("text");
    if (text.isValid()) {
        setText(text.toVariant().toString());
    }

    QScriptValue backgroundColor = properties.property("backgroundColor");
    if (backgroundColor.isValid()) {
        QScriptValue red = backgroundColor.property("red");
        QScriptValue green = backgroundColor.property("green");
        QScriptValue blue = backgroundColor.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _backgroundColor.red = red.toVariant().toInt();
            _backgroundColor.green = green.toVariant().toInt();
            _backgroundColor.blue = blue.toVariant().toInt();
        }
    }

    if (properties.property("backgroundAlpha").isValid()) {
        _backgroundAlpha = properties.property("backgroundAlpha").toVariant().toFloat();
    }

    if (properties.property("lineHeight").isValid()) {
        setLineHeight(properties.property("lineHeight").toVariant().toFloat());
    }

    if (properties.property("leftMargin").isValid()) {
        setLeftMargin(properties.property("leftMargin").toVariant().toFloat());
    }

    if (properties.property("topMargin").isValid()) {
        setTopMargin(properties.property("topMargin").toVariant().toFloat());
    }

    if (properties.property("rightMargin").isValid()) {
        setRightMargin(properties.property("rightMargin").toVariant().toFloat());
    }

    if (properties.property("bottomMargin").isValid()) {
        setBottomMargin(properties.property("bottomMargin").toVariant().toFloat());
    }

    QScriptValue isFacingAvatarValue = properties.property("isFacingAvatar");
    if (isFacingAvatarValue.isValid()) {
        _isFacingAvatar = isFacingAvatarValue.toVariant().toBool();
    }

}

QScriptValue Text3DOverlay::getProperty(const QString& property) {
    if (property == "text") {
        return _text;
    }
    if (property == "backgroundColor") {
        return xColorToScriptValue(_scriptEngine, _backgroundColor);
    }
    if (property == "backgroundAlpha") {
        return _backgroundAlpha;
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
    if (property == "isFacingAvatar") {
        return _isFacingAvatar;
    }
    return Planar3DOverlay::getProperty(property);
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

