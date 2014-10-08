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

static uint qHash(const TextRenderer::Properties& key, uint seed = 0) {
    // can be switched to qHash(key.font, seed) when we require Qt 5.3+
    return qHash(key.font.family(), qHash(key.font.pointSize(), seed));
}

static bool operator==(const TextRenderer::Properties& p1, const TextRenderer::Properties& p2) {
    return p1.font == p2.font && p1.effect == p2.effect && p1.effectThickness == p2.effectThickness && p1.color == p2.color;
}

TextRenderer* TextRenderer::getInstance(const char* family, int pointSize, int weight, bool italic,
        EffectType effect, int effectThickness, const QColor& color) {
    Properties properties = { QFont(family, pointSize, weight, italic), effect, effectThickness, color };
    TextRenderer*& instance = _instances[properties];
    if (!instance) {
        instance = new TextRenderer(properties);
    }
    return instance;
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
    
        float scale = QApplication::desktop()->windowHandle()->devicePixelRatio() / IMAGE_SIZE;
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
/*
        const int NUM_COORDS_PER_GLYPH = 16;
        float vertexBuffer[NUM_COORDS_PER_GLYPH] = { ls, bt, left, bottom, rs, bt, right, bottom, rs, tt, right, top, ls, tt, left, top };
        gpu::Buffer::Size offset = sizeof(vertexBuffer)*_numGlyphsBatched;
        if ((offset + sizeof(vertexBuffer)) > _glyphsBuffer.getSize()) {
            _glyphsBuffer.append(sizeof(vertexBuffer), (gpu::Buffer::Byte*) vertexBuffer);
        } else {
            _glyphsBuffer.setSubData(offset, sizeof(vertexBuffer), (gpu::Buffer::Byte*) vertexBuffer);
        }
        _numGlyphsBatched++;
*/
        x += glyph.width();
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    
 //   executeDrawBatch();
 //   clearDrawBatch();

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

TextRenderer::TextRenderer(const Properties& properties) :
    _font(properties.font),
    _metrics(_font),
    _effectType(properties.effect),
    _effectThickness(properties.effectThickness),
    _x(IMAGE_SIZE),
    _y(IMAGE_SIZE),
    _rowHeight(0),
    _color(properties.color),
    _glyphsBuffer(),
    _numGlyphsBatched(0)
{
    _font.setKerning(false);
}

const Glyph& TextRenderer::getGlyph(char c) {
    Glyph& glyph = _glyphs[c];
    if (glyph.isValid()) {
        return glyph;
    }
    // we use 'J' as a representative size for the solid block character
    QChar ch = (c == SOLID_BLOCK_CHAR) ? QChar('J') : QChar(c);
    QRect baseBounds = _metrics.boundingRect(ch);
    if (baseBounds.isEmpty()) {
        glyph = Glyph(0, QPoint(), QRect(), _metrics.width(ch));
        return glyph;
    }
    // grow the bounds to account for effect, if any
    if (_effectType == SHADOW_EFFECT) {
        baseBounds.adjust(-_effectThickness, 0, 0, _effectThickness);
    
    } else if (_effectType == OUTLINE_EFFECT) {
        baseBounds.adjust(-_effectThickness, -_effectThickness, _effectThickness, _effectThickness);
    }
    
    // grow the bounds to account for antialiasing
    baseBounds.adjust(-1, -1, 1, 1);
    
    // adjust bounds for device pixel scaling
    float ratio = QApplication::desktop()->windowHandle()->devicePixelRatio();
    QRect bounds(baseBounds.x() * ratio, baseBounds.y() * ratio, baseBounds.width() * ratio, baseBounds.height() * ratio);
    
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
        QFont font = _font;
        if (ratio == 1.0f) {
            painter.setFont(_font);
        } else {
            QFont enlargedFont = _font;
            enlargedFont.setPointSize(_font.pointSize() * ratio);
            painter.setFont(enlargedFont);
        }
        if (_effectType == SHADOW_EFFECT) {
            for (int i = 0; i < _effectThickness * ratio; i++) {
                painter.drawText(-bounds.x() - 1 - i, -bounds.y() + 1 + i, ch);
            }
        } else if (_effectType == OUTLINE_EFFECT) {
            QPainterPath path;
            QFont font = _font;
            font.setStyleStrategy(QFont::ForceOutline);
            path.addText(-bounds.x() - 0.5, -bounds.y() + 0.5, font, ch);
            QPen pen;
            pen.setWidth(_effectThickness * ratio);
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
       
    glyph = Glyph(_currentTextureID, QPoint(_x / ratio, _y / ratio), baseBounds, _metrics.width(ch));
    _x += bounds.width();
    _rowHeight = qMax(_rowHeight, bounds.height());
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return glyph;
}

void TextRenderer::executeDrawBatch() {
    if (_numGlyphsBatched<=0) {
        return;
    }

    glEnable(GL_TEXTURE_2D);

    GLuint textureID = 0;
    glBindTexture(GL_TEXTURE_2D, textureID);

    gpu::backend::syncGPUObject(_glyphsBuffer);
    GLuint vbo = _glyphsBuffer.getGLBufferObject();

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    const int NUM_POS_COORDS = 2;
    const int NUM_TEX_COORDS = 2;
    const int VERTEX_STRIDE = (NUM_POS_COORDS + NUM_TEX_COORDS) * sizeof(float);
    const int VERTEX_TEXCOORD_OFFSET = NUM_POS_COORDS * sizeof(float);
    glVertexPointer(2, GL_FLOAT, VERTEX_STRIDE, 0);
    glTexCoordPointer(2, GL_FLOAT, VERTEX_STRIDE, (GLvoid*) VERTEX_TEXCOORD_OFFSET );

    glDrawArrays(GL_QUADS, 0, _numGlyphsBatched * 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

}

void TextRenderer::clearDrawBatch() {
    _numGlyphsBatched = 0;
}

QHash<TextRenderer::Properties, TextRenderer*> TextRenderer::_instances;

Glyph::Glyph(int textureID, const QPoint& location, const QRect& bounds, int width) :
    _textureID(textureID), _location(location), _bounds(bounds), _width(width) {
}

using namespace gpu;

Buffer::Size Buffer::Sysmem::allocateMemory(Byte** dataAllocated, Size size) {
    if ( !dataAllocated ) { 
        qWarning() << "Buffer::Sysmem::allocateMemory() : Must have a valid dataAllocated pointer.";
        return NOT_ALLOCATED;
    }

    // Try to allocate if needed
    Size newSize = 0;
    if (size > 0) {
        // Try allocating as much as the required size + one block of memory
        newSize = size;
        (*dataAllocated) = new Byte[newSize];
        // Failed?
        if (!(*dataAllocated)) {
            qWarning() << "Buffer::Sysmem::allocate() : Can't allocate a system memory buffer of " << newSize << "bytes. Fails to create the buffer Sysmem.";
            return NOT_ALLOCATED;
        }
    }

    // Return what's actually allocated
    return newSize;
}

void Buffer::Sysmem::deallocateMemory(Byte* dataAllocated, Size size) {
    if (dataAllocated) {
        delete[] dataAllocated;
    }
}

Buffer::Sysmem::Sysmem() :
    _data(NULL),
    _size(0),
    _stamp(0)
{
}

Buffer::Sysmem::Sysmem(Size size, const Byte* bytes) :
    _data(NULL),
    _size(0),
    _stamp(0)
{
    if (size > 0) {
        _size = allocateMemory(&_data, size);
        if (_size >= size) {
            if (bytes) {
                memcpy(_data, bytes, size);
            }
        }
    }
}

Buffer::Sysmem::~Sysmem() {
    deallocateMemory( _data, _size );
    _data = NULL;
    _size = 0;
}

Buffer::Size Buffer::Sysmem::allocate(Size size) {
    if (size != _size) {
        Byte* newData = 0;
        Size newSize = 0;
        if (size > 0) {
            Size allocated = allocateMemory(&newData, size);
            if (allocated == NOT_ALLOCATED) {
                // early exit because allocation failed
                return 0;
            }
            newSize = allocated;
        }
        // Allocation was successful, can delete previous data
        deallocateMemory(_data, _size);
        _data = newData;
        _size = newSize;
        _stamp++;
    }
    return _size;
}

Buffer::Size Buffer::Sysmem::resize(Size size) {
    if (size != _size) {
        Byte* newData = 0;
        Size newSize = 0;
        if (size > 0) {
            Size allocated = allocateMemory(&newData, size);
            if (allocated == NOT_ALLOCATED) {
                // early exit because allocation failed
                return _size;
            }
            newSize = allocated;
            // Restore back data from old buffer in the new one
            if (_data) {
                Size copySize = ((newSize < _size)? newSize: _size);
                memcpy( newData, _data, copySize);
            }
        }
        // Reallocation was successful, can delete previous data
        deallocateMemory(_data, _size);
        _data = newData;
        _size = newSize;
        _stamp++;
    }
    return _size;
}

Buffer::Size Buffer::Sysmem::setData( Size size, const Byte* bytes ) {
    if (allocate(size) == size) {
        if (bytes) {
            memcpy( _data, bytes, _size );
            _stamp++;
        }
    }
    return _size;
}

Buffer::Size Buffer::Sysmem::setSubData( Size offset, Size size, const Byte* bytes) {
    if (((offset + size) <= getSize()) && bytes) {
        memcpy( _data + offset, bytes, size );
        _stamp++;
        return size;
    }
    return 0;
}

Buffer::Size Buffer::Sysmem::append(Size size, const Byte* bytes) {
    if (size > 0) {
        Size oldSize = getSize();
        Size totalSize = oldSize + size;
        if (resize(totalSize) == totalSize) {
            return setSubData(oldSize, size, bytes);
        }
    }
    return 0;
}

Buffer::Buffer() :
    _sysmem(NULL),
    _gpuObject(NULL) {
    _sysmem = new Sysmem();
}

Buffer::~Buffer() {
    if (_sysmem) {
        delete _sysmem;
        _sysmem = 0;
    }
    if (_gpuObject) {
        delete _gpuObject;
        _gpuObject = 0;
    }
}

Buffer::Size Buffer::resize(Size size) {
    return editSysmem().resize(size);
}

Buffer::Size Buffer::setData(Size size, const Byte* data) {
    return editSysmem().setData(size, data);
}

Buffer::Size Buffer::setSubData(Size offset, Size size, const Byte* data) {
    return editSysmem().setSubData( offset, size, data);
}

Buffer::Size Buffer::append(Size size, const Byte* data) {
    return editSysmem().append( size, data);
}

namespace gpu {
namespace backend {

void syncGPUObject(const Buffer& buffer) {
    BufferObject* object = buffer.getGPUObject();

    if (object && (object->_stamp == buffer.getSysmem().getStamp())) {
        return;
    }

    // need to have a gpu object?
    if (!object) {
        object = new BufferObject();
        glGenBuffers(1, &object->_buffer);
        buffer.setGPUObject(object);
    }

    // Now let's update the content of the bo with the sysmem version
    //if (object->_size < buffer.getSize()) {
        glBindBuffer(GL_COPY_WRITE_BUFFER, object->_buffer);
        glBufferData(GL_COPY_WRITE_BUFFER, buffer.getSysmem().getSize(), buffer.getSysmem().read(), GL_STATIC_DRAW);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        object->_stamp = buffer.getSysmem().getStamp();
        object->_size = buffer.getSysmem().getSize();
    //}
}

};
};