//
//  TextRenderer.h
//  interface
//
//  Created by Andrzej Kapolka on 4/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__TextRenderer__
#define __interface__TextRenderer__

#include "svo-viewer-config.h"

#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QImage>
#include <QVector>


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

    TextRenderer(const char* family, int pointSize = -1, int weight = -1, bool italic = false,
                 EffectType effect = NO_EFFECT, int effectThickness = 1);
    ~TextRenderer();

    const QFontMetrics& metrics() const { return _metrics; }

    // returns the height of the tallest character
    int calculateHeight(const char* str);

    // also returns the height of the tallest character
    int draw(int x, int y, const char* str);
    
    int computeWidth(char ch);
    int computeWidth(const char* str);
    
private:

    const Glyph& getGlyph (char c);

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

#endif /* defined(__interface__TextRenderer__) */
