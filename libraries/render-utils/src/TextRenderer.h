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

#include <gpu/GPUConfig.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QImage>
#include <QVector>

// FIXME, decouple from the GL headers
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

#include <gpu/Resource.h>
#include <gpu/Stream.h>

// a special "character" that renders as a solid block
const char SOLID_BLOCK_CHAR = 127;

// the standard sans serif font family
#define SANS_FONT_FAMILY "Helvetica"

// the standard mono font family
#define MONO_FONT_FAMILY "Courier"

// the Inconsolata font family
#ifdef Q_OS_WIN
#define INCONSOLATA_FONT_FAMILY "Fixedsys"
#define INCONSOLATA_FONT_WEIGHT QFont::Normal
#else
#define INCONSOLATA_FONT_FAMILY "Inconsolata"
#define INCONSOLATA_FONT_WEIGHT QFont::Bold
#endif


class TextRenderer {
public:
    enum EffectType { NO_EFFECT, SHADOW_EFFECT, OUTLINE_EFFECT };

    class Properties {
    public:
      QString font;
      EffectType effect;
      int effectThickness;
      QColor color;
    };

    static TextRenderer* getInstance(const char* family, int pointSize = -1, int weight = -1, bool italic = false,
        EffectType effect = NO_EFFECT, int effectThickness = 1, const QColor& color = QColor(255, 255, 255));

    ~TextRenderer();

    // returns the height of the tallest character
    int calculateHeight(const char * str) const;
    int calculateHeight(const QString & str) const;

    int computeWidth(char ch) const;
    int computeWidth(const QChar & ch) const;

    int computeWidth(const QString & str) const;
    int computeWidth(const char * str) const;

    // also returns the height of the tallest character
    inline int draw(int x, int y, const char* str,
      const glm::vec4& color = glm::vec4(-1),
      float fontSize = -1,
      float maxWidth = -1
    ) {
      return drawString(x, y, QString(str), color, fontSize, maxWidth);
    }

    float drawString(
      float x, float y,
      const QString & str,
      const glm::vec4& color = glm::vec4(-1),
      float fontSize = -1,
      float maxWidth = -1);

    void drawBatch();
    void clearBatch();
private:

    static QHash<Properties, TextRenderer*> _instances;

    // Allow fonts to scale appropriately when rendered in a 
    // scene where units are meters
    static const float DTP_TO_METERS; // = 0.003528f;
    static const float METERS_TO_DTP; // = 1.0 / DTP_TO_METERS;
    static const char SHADER_TEXT_FS[];
    static const char SHADER_TEXT_VS[];
  
    TextRenderer(const Properties& properties);

    // stores the font metrics for a single character
    struct Glyph {
      QChar c;
      glm::vec2 ul;
      glm::vec2 lr;
      glm::vec2 size;
      glm::vec2 offset;
      float d;  // xadvance - adjusts character positioning
      size_t indexOffset;

      int width() const {
        return size.x;
      }
      QRectF bounds() const;
      QRectF textureBounds(const glm::vec2 & textureSize) const;

      void read(QIODevice & in);
    };

    using TexturePtr = QSharedPointer < QOpenGLTexture >;
    using VertexArrayPtr = QSharedPointer< QOpenGLVertexArrayObject >;
    using ProgramPtr = QSharedPointer < QOpenGLShaderProgram >;
    using BufferPtr = QSharedPointer < QOpenGLBuffer >;

    const Glyph& getGlyph(const QChar & c) const;

    // the type of effect to apply
    EffectType _effectType;

    // the thickness of the effect
    int _effectThickness;
    
    // maps characters to cached glyph info
    QHash<QChar, Glyph> _glyphs;
    
    // the id of the glyph texture to which we're currently writing
    GLuint _currentTextureID;
    
    int _pointSize;
    
    // the height of the current row of characters
    int _rowHeight;
    
    // text color
    QColor _color;

    QString _family;
    float _fontSize{ 12 };
    float _leading{ 0 };
    float _ascent{ 0 };
    float _descent{ 0 };
    float _spaceWidth{ 0 };

    BufferPtr _vertices;
    BufferPtr _indices;
    TexturePtr _texture;
    VertexArrayPtr _vao;
    QImage _image;
    ProgramPtr _program;

    // Parse the signed distance field font file
    void read(const QString & path);
    void read(QIODevice & path);

    // Initialize the OpenGL structures
    void setupGL();
};


#endif // hifi_TextRenderer_h
