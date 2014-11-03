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

#include "gpu/Resource.h"
#include "gpu/Stream.h"


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
    
    void drawBatch();
    void clearBatch();
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
    gpu::BufferPtr _glyphsBuffer;
    gpu::BufferPtr _glyphsColorBuffer;
    gpu::StreamFormatPtr _glyphsStreamFormat;
    gpu::StreamPtr _glyphsStream;
    int _numGlyphsBatched;

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
