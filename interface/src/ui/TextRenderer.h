//
//  TextRenderer.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 4/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextRenderer_h
#define hifi_TextRenderer_h

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QImage>
#include <QVector>

#include <assert.h>

#include "InterfaceConfig.h"

// a special "character" that renders as a solid block
const char SOLID_BLOCK_CHAR = 127;

// the standard sans serif font family
#define SANS_FONT_FAMILY "Helvetica"

// the standard mono font family
#define MONO_FONT_FAMILY "Courier"

// the Inconsolata font family
#define INCONSOLATA_FONT_FAMILY "Inconsolata"

class Glyph;

namespace gpu {
class Buffer;
typedef int  Stamp;

namespace backend {

    class BufferObject {
    public:
        Stamp  _stamp;
        GLuint _buffer;
        GLuint _size;

        BufferObject() :
            _stamp(0),
            _buffer(0),
            _size(0)
        {}
    };
    void syncGPUObject(const Buffer& buffer);
};

class Buffer {
public:

    typedef unsigned char Byte;
    typedef unsigned int  Size;

    static const Size MIN_ALLOCATION_BLOCK_SIZE = 256;
    static const Size NOT_ALLOCATED = -1;

    Buffer();
    Buffer(const Buffer& buf );
    ~Buffer();

    // The size in bytes of data stored in the buffer
    inline Size getSize() const { return getSysmem().getSize(); }
    inline const Byte* getData() const { return getSysmem().read(); }

    // Resize the buffer
    // Keep previous data [0 to min(pSize, mSize)]
    Size resize(Size pSize);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    Size setData(Size size, const Byte* data);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    Size setSubData(Size offset, Size size, const Byte* data);

    // Append new data at the end of the current buffer
    // do a resize( size + getSIze) and copy the new data
    // \return the number of bytes copied
    Size append(Size size, const Byte* data);

    // this is a temporary hack so the current rendering code can access the underneath gl Buffer Object
    // TODO: remove asap, when the backend is doing more of the gl features
    inline GLuint  getGLBufferObject() const { backend::syncGPUObject(*this); return getGPUObject()->_buffer; }

protected:

    // Sysmem is the underneath cache for the data in ram.
    class Sysmem {
    public:

        Sysmem();
        Sysmem(Size size , const Byte* bytes);
        ~Sysmem();

        Size getSize() const { return _size; }

        // Allocate the byte array
        // \param pSize The nb of bytes to allocate, if already exist, content is lost.
        // \return The nb of bytes allocated, nothing if allready the appropriate size.
        Size allocate(Size pSize);

        // Resize the byte array
        // Keep previous data [0 to min(pSize, mSize)]
        Size resize(Size pSize);

        // Assign data bytes and size (allocate for size, then copy bytes if exists)
        Size setData( Size size, const Byte* bytes );

        // Update Sub data, 
        // doesn't allocate and only copy size * bytes at the offset location
        // only if all fits in the existing allocated buffer
        Size setSubData( Size offset, Size size, const Byte* bytes);

        // Append new data at the end of the current buffer
        // do a resize( size + getSIze) and copy the new data
        // \return the number of bytes copied
        Size append(Size size, const Byte* data);

        // Access the byte array.
        // The edit version allow to map data.
        inline const Byte* read() const { return _data; } 
        inline Byte* edit() { _stamp++; return _data; }

        template< typename T >
        const T* read() const { return reinterpret_cast< T* > ( _data ); } 
        template< typename T >
        T* edit() const { _stamp++; return reinterpret_cast< T* > ( _data ); } 

        // Access the current version of the sysmem, used to compare if copies are in sync
        inline Stamp getStamp() const { return _stamp; }

        static Size allocateMemory(Byte** memAllocated, Size size);
        static void deallocateMemory(Byte* memDeallocated, Size size);

    private:
        Sysmem(const Sysmem& sysmem) {}
        Sysmem &operator=(const Sysmem &other) {return *this;}

        Byte* _data;
        Size  _size;
        Stamp _stamp;
    };

    Sysmem*                         _sysmem;

    typedef backend::BufferObject GPUObject;
    mutable backend::BufferObject*  _gpuObject;

    inline const Sysmem& getSysmem() const { assert(_sysmem); return (*_sysmem); }
    inline Sysmem& editSysmem() { assert(_sysmem); return (*_sysmem); }
  
    inline GPUObject* getGPUObject() const { return _gpuObject; }
    inline void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }

    friend void backend::syncGPUObject(const Buffer& buffer);
};

};

class TextRenderer {
public:

    enum EffectType { NO_EFFECT, SHADOW_EFFECT, OUTLINE_EFFECT };

    class Properties {
    public:
        QFont font;
        EffectType effect;
        int effectThickness;
        QColor color;
    };

    static TextRenderer* getInstance(const char* family, int pointSize = -1, int weight = -1, bool italic = false,
        EffectType effect = NO_EFFECT, int effectThickness = 1, const QColor& color = QColor(255, 255, 255));

    ~TextRenderer();

    const QFontMetrics& metrics() const { return _metrics; }

    // returns the height of the tallest character
    int calculateHeight(const char* str);

    // also returns the height of the tallest character
    int draw(int x, int y, const char* str);
    
    int computeWidth(char ch);
    int computeWidth(const char* str);
    
    void executeDrawBatch();
    void clearDrawBatch();
private:

    TextRenderer(const Properties& properties);

    const Glyph& getGlyph(char c);

    // the font to render
    QFont _font;
    
    // the font metrics
    QFontMetrics _metrics;

    // the type of effect to apply
    EffectType _effectType;

    // the thickness of the effect
    int _effectThickness;
    
    // maps characters to cached glyph info
    QHash<char, Glyph> _glyphs;
    
    // the id of the glyph texture to which we're currently writing
    GLuint _currentTextureID;
    
    // the position within the current glyph texture
    int _x, _y;
    
    // the height of the current row of characters
    int _rowHeight;
    
    // the list of all texture ids for which we're responsible
    QVector<GLuint> _allTextureIDs;
    
    // text color
    QColor _color;
    
    // Graphics Buffer containing the current accumulated glyphs to render
    gpu::Buffer _glyphsBuffer;
    int         _numGlyphsBatched;

    static QHash<Properties, TextRenderer*> _instances;
};

class Glyph {
public:
    
    Glyph(int textureID = 0, const QPoint& location = QPoint(), const QRect& bounds = QRect(), int width = 0);
    
    GLuint textureID() const { return _textureID; }
    const QPoint& location () const { return _location; }
    const QRect& bounds() const { return _bounds; }
    int width () const { return _width; }
    
    bool isValid() { return _width != 0; }
    
private:
    
    // the id of the OpenGL texture containing the glyph
    GLuint _textureID;
    
    // the location of the character within the texture
    QPoint _location;
    
    // the bounds of the character
    QRect _bounds;
    
    // the width of the character (distance to next, as opposed to bounds width)
    int _width;
};

#endif // hifi_TextRenderer_h
