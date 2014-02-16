//
//  TextOverlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "TextOverlay.h"
#include "TextRenderer.h"

TextOverlay::TextOverlay() :
    _leftMargin(DEFAULT_MARGIN),
    _topMargin(DEFAULT_MARGIN)
{
}

TextOverlay::~TextOverlay() {
}

void TextOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const float MAX_COLOR = 255;
    glColor4f(_color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR, _alpha);

    glBegin(GL_QUADS);
        glVertex2f(_bounds.left(), _bounds.top());
        glVertex2f(_bounds.right(), _bounds.top());
        glVertex2f(_bounds.right(), _bounds.bottom());
        glVertex2f(_bounds.left(), _bounds.bottom());
    glEnd();

    //TextRenderer(const char* family, int pointSize = -1, int weight = -1, bool italic = false,
    //             EffectType effect = NO_EFFECT, int effectThickness = 1);

    TextRenderer textRenderer(SANS_FONT_FAMILY, 11, 50);
    const int leftAdjust = -1; // required to make text render relative to left edge of bounds
    const int topAdjust = -2; // required to make text render relative to top edge of bounds
    int x = _bounds.left() + _leftMargin + leftAdjust;
    int y = _bounds.top() + _topMargin + topAdjust;
    
    glColor3f(1.0f, 1.0f, 1.0f);
    QStringList lines = _text.split("\n");
    int lineOffset = 0;
    foreach(QString thisLine, lines) {
        if (lineOffset == 0) {
            lineOffset = textRenderer.calculateHeight(qPrintable(thisLine));
        }
        lineOffset += textRenderer.draw(x, y + lineOffset, qPrintable(thisLine));

        const int lineGap = 2;
        lineOffset += lineGap;
    }

}

void TextOverlay::setProperties(const QScriptValue& properties) {
    Overlay2D::setProperties(properties);

    QScriptValue text = properties.property("text");
    if (text.isValid()) {
        setText(text.toVariant().toString());
    }

    if (properties.property("leftMargin").isValid()) {
        setLeftMargin(properties.property("leftMargin").toVariant().toInt());
    }

    if (properties.property("topMargin").isValid()) {
        setTopMargin(properties.property("topMargin").toVariant().toInt());
    }
}


