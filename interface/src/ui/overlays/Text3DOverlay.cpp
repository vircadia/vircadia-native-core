//
//  Text3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include "Application.h"
#include "Text3DOverlay.h"
#include "ui/TextRenderer.h"

const xColor DEFAULT_BACKGROUND_COLOR = { 0, 0, 0 };
const float DEFAULT_BACKGROUND_ALPHA = 0.7f;
const float DEFAULT_MARGIN = 0.1f;
const int FIXED_FONT_POINT_SIZE = 40;
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
}

Text3DOverlay::~Text3DOverlay() {
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

    glPushMatrix(); {
        glTranslatef(_position.x, _position.y, _position.z);
        glm::quat rotation;
        
        if (_isFacingAvatar) {
            // rotate about vertical to face the camera
            rotation = Application::getInstance()->getCamera()->getRotation();
        } else {
            rotation = getRotation();
        }
        
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        const float MAX_COLOR = 255.0f;
        xColor backgroundColor = getBackgroundColor();
        glColor4f(backgroundColor.red / MAX_COLOR, backgroundColor.green / MAX_COLOR, backgroundColor.blue / MAX_COLOR, 
            getBackgroundAlpha());

        glm::vec2 dimensions = getDimensions();
        glm::vec2 halfDimensions = dimensions * 0.5f;
        
        const float SLIGHTLY_BEHIND = -0.005f;

        glBegin(GL_QUADS);
            glVertex3f(-halfDimensions.x, -halfDimensions.y, SLIGHTLY_BEHIND);
            glVertex3f(halfDimensions.x, -halfDimensions.y, SLIGHTLY_BEHIND);
            glVertex3f(halfDimensions.x, halfDimensions.y, SLIGHTLY_BEHIND);
            glVertex3f(-halfDimensions.x, halfDimensions.y, SLIGHTLY_BEHIND);
        glEnd();
        
        const int FIXED_FONT_SCALING_RATIO = FIXED_FONT_POINT_SIZE * 40.0f; // this is a ratio determined through experimentation
        
        // Same font properties as textSize()
        TextRenderer* textRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE);
        float maxHeight = (float)textRenderer->calculateHeight("Xy") * LINE_SCALE_RATIO;
        
        float scaleFactor =  (maxHeight / FIXED_FONT_SCALING_RATIO) * _lineHeight; 

        glTranslatef(-(halfDimensions.x - _leftMargin), halfDimensions.y - _topMargin, 0.0f);

        glm::vec2 clipMinimum(0.0f, 0.0f);
        glm::vec2 clipDimensions((dimensions.x - (_leftMargin + _rightMargin)) / scaleFactor, 
                                 (dimensions.y - (_topMargin + _bottomMargin)) / scaleFactor);

        glScalef(scaleFactor, -scaleFactor, 1.0);
        enableClipPlane(GL_CLIP_PLANE0, -1.0f, 0.0f, 0.0f, clipMinimum.x + clipDimensions.x);
        enableClipPlane(GL_CLIP_PLANE1, 1.0f, 0.0f, 0.0f, -clipMinimum.x);
        enableClipPlane(GL_CLIP_PLANE2, 0.0f, -1.0f, 0.0f, clipMinimum.y + clipDimensions.y);
        enableClipPlane(GL_CLIP_PLANE3, 0.0f, 1.0f, 0.0f, -clipMinimum.y);
    
        glColor3f(_color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR);
        float alpha = getAlpha();
        QStringList lines = _text.split("\n");
        int lineOffset = maxHeight;
        foreach(QString thisLine, lines) {
            textRenderer->draw(0, lineOffset, qPrintable(thisLine), alpha);
            lineOffset += maxHeight;
        }

        glDisable(GL_CLIP_PLANE0);
        glDisable(GL_CLIP_PLANE1);
        glDisable(GL_CLIP_PLANE2);
        glDisable(GL_CLIP_PLANE3);
        
    } glPopMatrix();
    
}

void Text3DOverlay::enableClipPlane(GLenum plane, float x, float y, float z, float w) {
    GLdouble coefficients[] = { x, y, z, w };
    glClipPlane(plane, coefficients);
    glEnable(plane);
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

    QFont font(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE);  // Same font properties as render()
    QFontMetrics fontMetrics(font);
    const float TEXT_SCALE_ADJUST = 1.02f;  // Experimentally detemined for the specified font
    const int TEXT_HEIGHT_ADJUST = -6;
    float scaleFactor = _lineHeight * TEXT_SCALE_ADJUST * LINE_SCALE_RATIO / (float)FIXED_FONT_POINT_SIZE;

    QStringList lines = text.split(QRegExp("\r\n|\r|\n"));

    float width = 0.0f;
    for (int i = 0; i < lines.count(); i += 1) {
        width = std::max(width, scaleFactor * (float)fontMetrics.width(qPrintable(lines[i])));
    }

    float height = lines.count() * scaleFactor * (float)(fontMetrics.height() + TEXT_HEIGHT_ADJUST);

    return QSizeF(width, height);
}

