//
//  TextOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "TextOverlay.h"
#include "ui/TextRenderer.h"

TextOverlay::TextOverlay() :
    _backgroundColor(DEFAULT_BACKGROUND_COLOR),
    _leftMargin(DEFAULT_MARGIN),
    _topMargin(DEFAULT_MARGIN),
    _fontSize(DEFAULT_FONTSIZE)
{
}

TextOverlay::~TextOverlay() {
}

xColor TextOverlay::getBackgroundColor() {
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


void TextOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }


    const float MAX_COLOR = 255.0f;
    xColor backgroundColor = getBackgroundColor();
    float alpha = getAlpha();
    glColor4f(backgroundColor.red / MAX_COLOR, backgroundColor.green / MAX_COLOR, backgroundColor.blue / MAX_COLOR, alpha);

    glBegin(GL_QUADS);
        glVertex2f(_bounds.left(), _bounds.top());
        glVertex2f(_bounds.right(), _bounds.top());
        glVertex2f(_bounds.right(), _bounds.bottom());
        glVertex2f(_bounds.left(), _bounds.bottom());
    glEnd();

    //TextRenderer(const char* family, int pointSize = -1, int weight = -1, bool italic = false,
    //             EffectType effect = NO_EFFECT, int effectThickness = 1);
    TextRenderer* textRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY, _fontSize, 50);
    
    const int leftAdjust = -1; // required to make text render relative to left edge of bounds
    const int topAdjust = -2; // required to make text render relative to top edge of bounds
    int x = _bounds.left() + _leftMargin + leftAdjust;
    int y = _bounds.top() + _topMargin + topAdjust;
    
    glColor3f(_color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR);
    QStringList lines = _text.split("\n");
    int lineOffset = 0;
    foreach(QString thisLine, lines) {
        if (lineOffset == 0) {
            lineOffset = textRenderer->calculateHeight(qPrintable(thisLine));
        }
        lineOffset += textRenderer->draw(x, y + lineOffset, qPrintable(thisLine));

        const int lineGap = 2;
        lineOffset += lineGap;
    }
}

void TextOverlay::setProperties(const QScriptValue& properties) {
    Overlay2D::setProperties(properties);
    
    QScriptValue font = properties.property("font");
    if (font.isObject()) {
        if (font.property("size").isValid()) {
            setFontSize(font.property("size").toInt32());
        }
    }

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

    if (properties.property("leftMargin").isValid()) {
        setLeftMargin(properties.property("leftMargin").toVariant().toInt());
    }

    if (properties.property("topMargin").isValid()) {
        setTopMargin(properties.property("topMargin").toVariant().toInt());
    }
}


QScriptValue TextOverlay::getProperty(const QString& property) {
    if (property == "text") {
        return _text;
    }

    return Overlay::getProperty(property);
}


