//
//  TextRenderer.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 4/24/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QApplication>
#include <QDesktopWidget>
#include <QFont>
#include <QPaintEngine>
#include <QtDebug>
#include <QString>
#include <QStringList>
#include <QWindow>

#include "InterfaceConfig.h"
#include "TextRenderer.h"

// the width/height of the cached glyph textures
const int IMAGE_SIZE = 256;

Glyph::Glyph(int textureID, const QPoint& location, const QRect& bounds, int width) :
    _textureID(textureID), _location(location), _bounds(bounds), _width(width) {
}

TextRenderer::TextRenderer(const char* family, int pointSize, int weight, bool italic,
        EffectType effectType, int effectThickness, QColor color) :
    _font(family, pointSize * QApplication::desktop()->windowHandle()->devicePixelRatio(), weight, italic),
    _metrics(_font),
    _effectType(effectType),
    _effectThickness(effectThickness),
    _x(IMAGE_SIZE),
    _y(IMAGE_SIZE),
    _rowHeight(0),
    _color(color) {
    
    _font.setKerning(false);
}

TextRenderer::~TextRenderer() {
    glDeleteTextures(_allTextureIDs.size(), _allTextureIDs.constData());
}

int TextRenderer::calculateHeight(const char* str) {
    int maxHeight = 0;
    for (const char* ch = str; *ch != 0; ch++) {
        const Glyph& glyph = getGlyph(*ch);
        if (glyph.textureID() == 0) {
            continue;
        }
        
        if (glyph.bounds().height() > maxHeight) {
            maxHeight = glyph.bounds().height();
        }
    }
    return maxHeight;
}

int TextRenderer::draw(int x, int y, const char* str) {
    glEnable(GL_TEXTURE_2D);    
    
    int maxHeight = 0;
    for (const char* ch = str; *ch != 0; ch++) {
        const Glyph& glyph = getGlyph(*ch);
        if (glyph.textureID() == 0) {
            x += glyph.width();
            continue;
        }
        
        if (glyph.bounds().height() > maxHeight) {
            maxHeight = glyph.bounds().height();
        }
    
        glBindTexture(GL_TEXTURE_2D, glyph.textureID());
    
        int left = x + glyph.bounds().x();
        int right = x + glyph.bounds().x() + glyph.bounds().width();
        int bottom = y + glyph.bounds().y();
        int top = y + glyph.bounds().y() + glyph.bounds().height();
    
        float scale = 1.0 / IMAGE_SIZE;
        float ls = glyph.location().x() * scale;
        float rs = (glyph.location().x() + glyph.bounds().width()) * scale;
        float bt = glyph.location().y() * scale;
        float tt = (glyph.location().y() + glyph.bounds().height()) * scale;
    
        glBegin(GL_QUADS);
        glTexCoord2f(ls, bt);
        glVertex2f(left, bottom);
        glTexCoord2f(rs, bt);
        glVertex2f(right, bottom);
        glTexCoord2f(rs, tt);
        glVertex2f(right, top);
        glTexCoord2f(ls, tt);
        glVertex2f(left, top);
        glEnd();
    
        x += glyph.width();
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    
    return maxHeight;
}

int TextRenderer::computeWidth(char ch)
{
    return getGlyph(ch).width();
}

int TextRenderer::computeWidth(const char* str)
{
    int width = 0;
    for (const char* ch = str; *ch != 0; ch++) {
        width += computeWidth(*ch);
    }
    return width;
}

const Glyph& TextRenderer::getGlyph(char c) {
    Glyph& glyph = _glyphs[c];
    if (glyph.isValid()) {
        return glyph;
    }
    // we use 'J' as a representative size for the solid block character
    QChar ch = (c == SOLID_BLOCK_CHAR) ? QChar('J') : QChar(c);
    QRect bounds = _metrics.boundingRect(ch);
    if (bounds.isEmpty()) {
        glyph = Glyph(0, QPoint(), QRect(), _metrics.width(ch));
        return glyph;
    }
    // grow the bounds to account for effect, if any
    if (_effectType == SHADOW_EFFECT) {
        bounds.adjust(-_effectThickness, 0, 0, _effectThickness);
    
    } else if (_effectType == OUTLINE_EFFECT) {
        bounds.adjust(-_effectThickness, -_effectThickness, _effectThickness, _effectThickness);
    }
    
    // grow the bounds to account for antialiasing
    bounds.adjust(-1, -1, 1, 1);
    
    if (_x + bounds.width() > IMAGE_SIZE) {
        // we can't fit it on the current row; move to next
        _y += _rowHeight;
        _x = _rowHeight = 0;
    }
    if (_y + bounds.height() > IMAGE_SIZE) {
        // can't fit it on current texture; make a new one
        glGenTextures(1, &_currentTextureID);
        _x = _y = _rowHeight = 0;
        
        glBindTexture(GL_TEXTURE_2D, _currentTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IMAGE_SIZE, IMAGE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        _allTextureIDs.append(_currentTextureID);
           
    } else {
        glBindTexture(GL_TEXTURE_2D, _currentTextureID);
    }
    // render the glyph into an image and copy it into the texture
    QImage image(bounds.width(), bounds.height(), QImage::Format_ARGB32);
    if (c == SOLID_BLOCK_CHAR) {
        image.fill(_color);
    
    } else {
        image.fill(0);
        QPainter painter(&image);
        painter.setFont(_font);
        if (_effectType == SHADOW_EFFECT) {
            for (int i = 0; i < _effectThickness; i++) {
                painter.drawText(-bounds.x() - 1 - i, -bounds.y() + 1 + i, ch);
            }
        } else if (_effectType == OUTLINE_EFFECT) {
            QPainterPath path;
            QFont font = _font;
            font.setStyleStrategy(QFont::ForceOutline);
            path.addText(-bounds.x() - 0.5, -bounds.y() + 0.5, font, ch);
            QPen pen;
            pen.setWidth(_effectThickness);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPath(path);
        }
        painter.setPen(_color);
        painter.drawText(-bounds.x(), -bounds.y(), ch);
    }    
    glTexSubImage2D(GL_TEXTURE_2D, 0, _x, _y, bounds.width(), bounds.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());
       
    glyph = Glyph(_currentTextureID, QPoint(_x, _y), bounds, _metrics.width(ch));
    _x += bounds.width();
    _rowHeight = qMax(_rowHeight, bounds.height());
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return glyph;
}
