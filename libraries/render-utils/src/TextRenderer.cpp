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

#include "TextRenderer.h"
#include "MatrixStack.h"

#include <gpu/GPUConfig.h>
#include <QApplication>
#include <QtDebug>
#include <QString>
#include <QStringList>
#include <QBuffer>
#include <QFile>

// FIXME, decouple from the GL headers
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gpu/GLBackend.h"
#include "gpu/Stream.h"
#include "FontInconsolataMedium.h"
#include "FontRoboto.h"
#include "FontTimeless.h"
#include "FontCourierPrime.h"

namespace Shaders {
  // Normally we could use 'enum class' to avoid namespace pollution,
  // but we want easy conversion to GLuint
  namespace Attributes {
    enum {
      Position = 0,
      TexCoord = 1,
    };
  };
}

// Helper functions for reading binary data from an IO device
template <class T>
void readStream(QIODevice & in, T & t) {
  in.read((char*)&t, sizeof(t));
}

template <typename T, size_t N>
void readStream(T(&t)[N]) {
  in.read((char*)t, N);
}

glm::uvec2 toGlm(const QSize & size) {
  return glm::uvec2(size.width(), size.height());
}

const char SHADER_TEXT_VS[] = R"XXXX(#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  vTexCoord = TexCoord;
  gl_Position = Projection * ModelView * vec4(Position, 1);
})XXXX";

const char SHADER_TEXT_FS[] = R"XXXX(#version 330

uniform sampler2D Font;
uniform vec4 Color = vec4(1);
uniform bool Outline = false;

in vec2 vTexCoord;
out vec4 FragColor;

const float gamma = 2.6;
const float smoothing = 100.0;

void main() {
  FragColor = vec4(1);
 
  // retrieve signed distance
  float sdf = texture(Font, vTexCoord).r;
  if (Outline) {
    if (sdf > 0.8) {
      sdf = 1.0 - sdf;
    } else {
      sdf += 0.2;
    }
  }
  // perform adaptive anti-aliasing of the edges
  // The larger we're rendering, the less anti-aliasing we need
  float s = smoothing * length(fwidth(vTexCoord));
  float w = clamp( s, 0.0, 0.5);
  float a = smoothstep(0.5 - w, 0.5 + w, sdf);

  // gamma correction for linear attenuation
  a = pow(a, 1.0 / gamma);

  if (a < 0.01) {
    discard;
  }

  // final color
  FragColor = vec4(Color.rgb, a);
})XXXX";

// stores the font metrics for a single character
struct Glyph {
  QChar c;
  glm::vec2 texOffset;
  glm::vec2 texSize;
  glm::vec2 size;
  glm::vec2 offset;
  float d;  // xadvance - adjusts character positioning
  size_t indexOffset;

  QRectF bounds() const;
  QRectF textureBounds(const glm::vec2 & textureSize) const;

  void read(QIODevice & in);
};

void Glyph::read(QIODevice & in) {
  uint16_t charcode;
  readStream(in, charcode);
  c = charcode;
  readStream(in, texOffset);
  readStream(in, size);
  readStream(in, offset);
  readStream(in, d);
  texSize = size;
}

const float DEFAULT_POINT_SIZE = 12;

class Font {
public:
  using TexturePtr = QSharedPointer < QOpenGLTexture >;
  using VertexArrayPtr = QSharedPointer< QOpenGLVertexArrayObject >;
  using ProgramPtr = QSharedPointer < QOpenGLShaderProgram >;
  using BufferPtr = QSharedPointer < QOpenGLBuffer >;

  // maps characters to cached glyph info
  QHash<QChar, Glyph> _glyphs;
  

  // the id of the glyph texture to which we're currently writing
  GLuint _currentTextureID;

  int _pointSize;

  // the height of the current row of characters
  int _rowHeight;

  QString _family;
  float _fontSize{ 0 };
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

  const Glyph & Font::getGlyph(const QChar & c) const;
  void read(QIODevice & path);
  // Initialize the OpenGL structures
  void setupGL();


  glm::vec2 computeExtent(const QString & str) const;

  glm::vec2 drawString(
    float x, float y,
    const QString & str,
    const glm::vec4& color,
    TextRenderer::EffectType effectType,
    float maxWidth) const;
};

const Glyph & Font::getGlyph(const QChar & c) const {
  if (!_glyphs.contains(c)) {
    return _glyphs[QChar('?')];
  }
  return _glyphs[c];
};

void Font::read(QIODevice & in) {
  uint8_t header[4];
  readStream(in, header);
  if (memcmp(header, "SDFF", 4)) {
    qFatal("Bad SDFF file");
  }

  uint16_t version;
  readStream(in, version);

  // read font name
  _family = "";
  if (version > 0x0001) {
    char c;
    readStream(in, c);
    while (c) {
      _family += c;
      readStream(in, c);
    }
  }

  // read font data
  readStream(in, _leading);
  readStream(in, _ascent);
  readStream(in, _descent);
  readStream(in, _spaceWidth);
  _fontSize = _ascent + _descent;
  _rowHeight = _fontSize + _descent;

  // Read character count
  uint16_t count;
  readStream(in, count);
  // read metrics data for each character
  QVector<Glyph> glyphs(count);
  // std::for_each instead of Qt foreach because we need non-const references
  std::for_each(glyphs.begin(), glyphs.end(), [&](Glyph & g){
    g.read(in);
  });

  // read image data
  if (!_image.loadFromData(in.readAll(), "PNG")) {
    qFatal("Failed to read SDFF image");
  }

  _glyphs.clear();
  foreach(Glyph g, glyphs) {
    // Adjust the pixel texture coordinates into UV coordinates, 
    glm::vec2 imageSize = toGlm(_image.size());
    g.texSize /= imageSize;
    g.texOffset /= imageSize;
    // store in the character to glyph hash
    _glyphs[g.c] = g;
  };

  setupGL();
}

struct TextureVertex {
  glm::vec2 pos;
  glm::vec2 tex;
  TextureVertex() {
  }
  TextureVertex(const glm::vec2 & pos, const glm::vec2 & tex)
    : pos(pos), tex(tex) {
  }
  TextureVertex(const QPointF & pos, const QPointF & tex)
    : pos(pos.x(), pos.y()), tex(tex.x(), tex.y()) {
  }
};

struct QuadBuilder {
  TextureVertex vertices[4];
  QuadBuilder(const QRectF & r, const QRectF & tr) {
    vertices[0] = TextureVertex(r.bottomLeft(), tr.topLeft());
    vertices[1] = TextureVertex(r.bottomRight(), tr.topRight());
    vertices[2] = TextureVertex(r.topRight(), tr.bottomRight());
    vertices[3] = TextureVertex(r.topLeft(), tr.bottomLeft());
  }
};

QRectF glmToRect(const glm::vec2 & pos, const glm::vec2 & size) {
  QRectF result(pos.x, pos.y, size.x, size.y);
  return result;
}


QRectF Glyph::bounds() const {
  return glmToRect(offset, size);
}

QRectF Glyph::textureBounds(const glm::vec2 & textureSize) const {
  return glmToRect(texOffset, texSize);
}

void Font::setupGL() {
  _texture = TexturePtr(new QOpenGLTexture(_image, QOpenGLTexture::DontGenerateMipMaps));
  _program = ProgramPtr(new QOpenGLShaderProgram()); 
  if (!_program->create()) {
    qFatal("Could not create text shader");
  }
  if (
    !_program->addShaderFromSourceCode(QOpenGLShader::Vertex, SHADER_TEXT_VS) ||
    !_program->addShaderFromSourceCode(QOpenGLShader::Fragment, SHADER_TEXT_FS) ||
    !_program->link()) {
    qFatal(_program->log().toLocal8Bit().constData());
  }

  std::vector<TextureVertex> vertexData;
  std::vector<GLuint> indexData;
  vertexData.reserve(_glyphs.size() * 4);
  std::for_each(_glyphs.begin(), _glyphs.end(), [&](Glyph & m){
    GLuint index = (GLuint)vertexData.size();

    QRectF bounds = m.bounds();
    QRectF texBounds = m.textureBounds(toGlm(_image.size()));
    QuadBuilder qb(bounds, texBounds);
    for (int i = 0; i < 4; ++i) {
      vertexData.push_back(qb.vertices[i]);
    }

    m.indexOffset = indexData.size() * sizeof(GLuint);
    // FIXME use triangle strips + primitive restart index
    indexData.push_back(index + 0);
    indexData.push_back(index + 1);
    indexData.push_back(index + 2);
    indexData.push_back(index + 0);
    indexData.push_back(index + 2);
    indexData.push_back(index + 3);
  });

  //_vao = VertexArrayPtr(new VertexArray());
  _vao = VertexArrayPtr(new QOpenGLVertexArrayObject());
  _vao->create();
  _vao->bind();


  _vertices = BufferPtr(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
  _vertices->create();
  _vertices->bind();
  _vertices->allocate(&vertexData[0], sizeof(TextureVertex) * vertexData.size());

  _indices = BufferPtr(new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer));
  _indices->create();
  _indices->bind();
  _indices->allocate(&indexData[0], sizeof(GLuint) * indexData.size());

  GLsizei stride = (GLsizei)sizeof(TextureVertex);
  void* offset = (void*)offsetof(TextureVertex, tex);

  glEnableVertexAttribArray(Shaders::Attributes::Position);
  glVertexAttribPointer(Shaders::Attributes::Position, 3, GL_FLOAT, false, stride, nullptr);
  glEnableVertexAttribArray(Shaders::Attributes::TexCoord);
  glVertexAttribPointer(Shaders::Attributes::TexCoord, 2, GL_FLOAT, false, stride, offset);
  _vao->release();
}

glm::vec2 Font::computeExtent(const QString & str) const {
  glm::vec2 advance(0, _rowHeight - _descent);
  float maxX = 0;
  foreach(QString token, str.split(" ")) {
    foreach(QChar c, token) {
      if (QChar('\n') == c) {
        maxX = std::max(advance.x, maxX);
        advance.x = 0;
        advance.y += _rowHeight;
        continue;
      }
      if (!_glyphs.contains(c)) {
        c = QChar('?');
      }
      const Glyph & m = _glyphs[c];
      advance.x += m.d;
    }
    advance.x += _spaceWidth;
  }
  return glm::vec2(maxX, advance.y);
}

glm::vec2 Font::drawString(
  float x, float y,
  const QString & str,
  const glm::vec4& color,
  TextRenderer::EffectType effectType,
  float maxWidth) const {

  // Stores how far we've moved from the start of the string, in DTP units
  glm::vec2 advance(0, -_rowHeight - _descent);

  _program->bind();
  _program->setUniformValue("Color", color.r, color.g, color.b, color.a);
  _program->setUniformValue("Projection", fromGlm(MatrixStack::projection().top()));
  if (effectType == TextRenderer::OUTLINE_EFFECT) {
    _program->setUniformValue("Outline", true);
  }
  _texture->bind();
  _vao->bind();

  MatrixStack & mv = MatrixStack::modelview();
  // scale the modelview into font units
  mv.translate(glm::vec3(0, _ascent, 0));
  foreach(QString token, str.split(" ")) {
    // float tokenWidth = measureWidth(token, fontSize);
    // if (wrap && 0 != advance.x && (advance.x + tokenWidth) > maxWidth) {
    //  advance.x = 0;
    //  advance.y -= _rowHeight;
    // }

    foreach(QChar c, token) {
      if (QChar('\n') == c) {
        advance.x = 0;
        advance.y -= _rowHeight;
        continue;
      }

      if (!_glyphs.contains(c)) {
        c = QChar('?');
      }

      // get metrics for this character to speed up measurements
      const Glyph & m = _glyphs[c];
      //if (wrap && ((advance.x + m.d) > maxWidth)) {
      //  advance.x = 0;
      //  advance.y -= _rowHeight;
      //}

      // We create an offset vec2 to hold the local offset of this character
      // This includes compensating for the inverted Y axis of the font
      // coordinates
      glm::vec2 offset(advance);
      offset.y -= m.size.y;
      // Bind the new position
      mv.withPush([&] {
        mv.translate(offset);
        _program->setUniformValue("ModelView", fromGlm(mv.top()));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(m.indexOffset));
      });
      advance.x += m.d;//+ m.offset.x;// font->getAdvance(m, mFontSize);
    }
    advance.x += _spaceWidth;
  }

  _vao->release();
  _program->release();

  return advance;
}

TextRenderer* TextRenderer::getInstance(const char* family, float pointSize, int weight, bool italic,
  EffectType effect, int effectThickness, const QColor& color) {
  if (pointSize < 0) {
    pointSize = DEFAULT_POINT_SIZE;
  }
  return new TextRenderer(family, pointSize, weight, italic, effect, effectThickness, color);
}

static QHash<QString, Font*> LOADED_FONTS;

template <class T, size_t N >
void fillBuffer(QBuffer & buffer, T(&t)[N]) {
  buffer.setData((const char*)t, N);
}

Font * loadFont(QIODevice & buffer) {
  Font * result = new Font();
  result->read(buffer);
  return result;
}

template <class T, size_t N >
Font * loadFont(T(&t)[N]) {
  QBuffer buffer;
  buffer.setData((const char*)t, N);
  buffer.open(QBuffer::ReadOnly);
  return loadFont(buffer);
}

Font * loadFont(const QString & family) {
  
  if (!LOADED_FONTS.contains(family)) {
    if (family == MONO_FONT_FAMILY) {
      
      LOADED_FONTS[family] = loadFont(SDFF_COURIER_PRIME);
    } else if (family == INCONSOLATA_FONT_FAMILY) {
      LOADED_FONTS[family] = loadFont(SDFF_INCONSOLATA_MEDIUM);
    } else if (family == SANS_FONT_FAMILY) {
      LOADED_FONTS[family] = loadFont(SDFF_ROBOTO);
    } else {
      if (!LOADED_FONTS.contains(SERIF_FONT_FAMILY)) {
        LOADED_FONTS[SERIF_FONT_FAMILY] = loadFont(SDFF_TIMELESS);
      }
      LOADED_FONTS[family] = LOADED_FONTS[SERIF_FONT_FAMILY];
    }
  }
  return LOADED_FONTS[family];
}

TextRenderer::TextRenderer(const char* family, float pointSize, int weight, bool italic,
  EffectType effect, int effectThickness, const QColor& color) :
  _effectType(effect),
  _pointSize(pointSize),
  _effectThickness(effectThickness),
  _color(color),
  _font(loadFont(family))
{
  
}

TextRenderer::~TextRenderer() {
}

glm::vec2 TextRenderer::computeExtent(const QString & str) const {
  float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
  return _font->computeExtent(str) * scale;
}

float TextRenderer::draw(
  float x, float y,
  const QString & str,
  const glm::vec4& color,
  float maxWidth) {
  glm::vec4 actualColor(color);
  if (actualColor.r < 0) {
    actualColor = glm::vec4(_color.redF(), _color.greenF(), _color.blueF(), 1.0);
  }

  float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
  glm::vec2 result;
  MatrixStack::withGlMatrices([&] {
    MatrixStack & mv = MatrixStack::modelview();
    // scale the modelview into font units
    mv.translate(glm::vec2(x, y)).scale(glm::vec3(scale, -scale, scale));
    result = _font->drawString(x, y, str, actualColor, _effectType, maxWidth);
  });
  return result.x;
}

