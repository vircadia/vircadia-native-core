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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gpu/GLBackend.h"
#include "gpu/Stream.h"
#include "FontInconsolataMedium.h"
#include "FontRoboto.h"
#include "FontJackInput.h"
#include "FontTimeless.h"

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

const char TextRenderer::SHADER_TEXT_VS[] = R"XXXX(#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  vTexCoord = TexCoord;
  gl_Position = Projection * ModelView * vec4(Position, 1);
})XXXX";

const char TextRenderer::SHADER_TEXT_FS[] = R"XXXX(#version 330

uniform sampler2D Font;
uniform vec4 Color;

in vec2 vTexCoord;
out vec4 FragColor;

const float gamma = 2.6;
const float smoothing = 100.0;

void main() {
  FragColor = vec4(1);
 
  // retrieve signed distance
  float sdf = texture(Font, vTexCoord).r;

  // perform adaptive anti-aliasing of the edges
  // The larger we're rendering, the less anti-aliasing we need
  float s = smoothing * length(fwidth(vTexCoord));
  float w = clamp( s, 0.0, 0.5);
  float a = smoothstep(0.5 - w, 0.5 + w, sdf);

  // gamma correction for linear attenuation
  a = pow(a, 1.0 / gamma);

  if (a < 0.001) {
    discard;
  }

  // final color
  FragColor = vec4(Color.rgb, a);
})XXXX";

const float TextRenderer::DTP_TO_METERS = 0.003528f;
const float TextRenderer::METERS_TO_DTP = 1.0f / TextRenderer::DTP_TO_METERS;

static uint qHash(const TextRenderer::Properties& key, uint seed = 0) {
    // can be switched to qHash(key.font, seed) when we require Qt 5.3+
    return qHash(key.font);
}

static bool operator==(const TextRenderer::Properties& p1, const TextRenderer::Properties& p2) {
    return p1.font == p2.font && p1.effect == p2.effect && p1.effectThickness == p2.effectThickness && p1.color == p2.color;
}

TextRenderer* TextRenderer::getInstance(const char* family, int pointSize, int weight, bool italic,
        EffectType effect, int effectThickness, const QColor& color) {
    Properties properties = { QString(family), effect, effectThickness, color };
    TextRenderer*& instance = _instances[properties];
    if (!instance) {
        instance = new TextRenderer(properties);
    }
    return instance;
}



TextRenderer::TextRenderer(const Properties& properties) :
  _effectType(properties.effect),
  _effectThickness(properties.effectThickness),
  _rowHeight(0),
  _color(properties.color) {

  QBuffer buffer;
  if (properties.font == MONO_FONT_FAMILY) {
    buffer.setData((const char*)SDFF_JACKINPUT, sizeof(SDFF_JACKINPUT));
  } else if (properties.font == INCONSOLATA_FONT_FAMILY) {
    buffer.setData((const char*)SDFF_INCONSOLATA_MEDIUM, sizeof(SDFF_INCONSOLATA_MEDIUM));
  } else if (properties.font == SANS_FONT_FAMILY) {
    buffer.setData((const char*)SDFF_ROBOTO, sizeof(SDFF_ROBOTO));
  } else {
    buffer.setData((const char*)SDFF_TIMELESS, sizeof(SDFF_TIMELESS));
  }
  buffer.open(QIODevice::ReadOnly);
  read(buffer);
}


TextRenderer::~TextRenderer() {
}

const TextRenderer::Glyph & TextRenderer::getGlyph(const QChar & c) const {
  if (!_glyphs.contains(c)) {
    return _glyphs[QChar('?')];
  }
  return _glyphs[c];
};

int TextRenderer::calculateHeight(const char* str) const {
    int maxHeight = 0;
    for (const char* ch = str; *ch != 0; ch++) {
        const Glyph& glyph = getGlyph(*ch);
        if (glyph.bounds().height() > maxHeight) {
            maxHeight = glyph.bounds().height();
        }
    }
    return maxHeight;
}


template <class T>
void readStream(QIODevice & in, T & t) {
  in.read((char*)&t, sizeof(t));
}

template <typename T, size_t N>
void readStream(T(&t)[N]) {
  in.read((char*)t, N);
}

void TextRenderer::read(const QString & path) {
  QFile file(path);
  file.open(QFile::ReadOnly);
  read(file);
}

void TextRenderer::Glyph::read(QIODevice & in) {
  uint16_t charcode;
  readStream(in, charcode);
  c = charcode;
  readStream(in, ul);
  readStream(in, size);
  readStream(in, offset);
  readStream(in, d);
  lr = ul + size;
}

void TextRenderer::read(QIODevice & in) {
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

  // read metrics data
  _glyphs.clear();

  uint16_t count;
  readStream(in, count);

  for (int i = 0; i < count; ++i) {
    Glyph g;
    g.read(in);
    _glyphs[g.c] = g;
  }

  // read image data
  if (!_image.loadFromData(in.readAll(), "PNG")) {
    qFatal("Failed to read SDFF image");
  }
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

glm::uvec2 toGlm(const QSize & size) {
  return glm::uvec2(size.width(), size.height());
}

QRectF TextRenderer::Glyph::bounds() const {
  return glmToRect(offset, size);
}

QRectF TextRenderer::Glyph::textureBounds(const glm::vec2 & textureSize) const {
  glm::vec2 pos = ul;
  glm::vec2 size = lr - ul;
  pos /= textureSize;
  size /= textureSize;
  return glmToRect(pos, size);
}

void TextRenderer::setupGL() {
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

int TextRenderer::computeWidth(const QChar & ch) const
{
    return getGlyph(ch).width();
}

int TextRenderer::computeWidth(const QString & str) const
{
    int width = 0;
    foreach(QChar c, str) {
        width += computeWidth(c);
    }
    return width;
}

int TextRenderer::computeWidth(const char * str) const {
  int width = 0;
  while (*str) {
    width += computeWidth(*str++);
  }
  return width;
}

int TextRenderer::computeWidth(char ch) const {
  return computeWidth(QChar(ch));
}

QHash<TextRenderer::Properties, TextRenderer*> TextRenderer::_instances;

float TextRenderer::drawString(
  float x, float y,
  const QString & str,
  const glm::vec4& color,
  float fontSize,
  float maxWidth) {

  // This is a hand made scale intended to match the previous scale of text in the application
  float scale = 0.25f; //  DTP_TO_METERS;
  if (fontSize > 0.0) {
    scale *= fontSize / _fontSize;
  }
  
  //bool wrap = false; // (maxWidth == maxWidth);
  //if (wrap) {
  //  maxWidth /= scale;
  //}

  // Stores how far we've moved from the start of the string, in DTP units
  static const float SPACE_ADVANCE = getGlyph(' ').d;
  glm::vec2 advance;
  MatrixStack::withGlMatrices([&] {
    // Fetch the matrices out of GL
    _program->bind();
    _program->setUniformValue("Color", color.r, color.g, color.b, color.a);
    _program->setUniformValue("Projection", fromGlm(MatrixStack::projection().top()));
    _texture->bind();
    _vao->bind();

    MatrixStack & mv = MatrixStack::modelview();
    MatrixStack & pr = MatrixStack::projection();
    // scale the modelview into font units
    mv.translate(glm::vec2(x, y)).translate(glm::vec2(0, scale * -_ascent)).scale(glm::vec3(scale, -scale, scale));

    foreach(QString token, str.split(" ")) {
      // float tokenWidth = measureWidth(token, fontSize);
      // if (wrap && 0 != advance.x && (advance.x + tokenWidth) > maxWidth) {
      //  advance.x = 0;
      //  advance.y -= (_ascent + _descent);
      // }

      foreach(QChar c, token) {
        if (QChar('\n') == c) {
          advance.x = 0;
          advance.y -= (_ascent + _descent);
          return;
        }

        if (!_glyphs.contains(c)) {
          c = QChar('?');
        }

        // get metrics for this character to speed up measurements
        const Glyph & m = _glyphs[c];
        //if (wrap && ((advance.x + m.d) > maxWidth)) {
        //  advance.x = 0;
        //  advance.y -= (_ascent + _descent);
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
      advance.x += SPACE_ADVANCE;
    }

    _vao->release();
    _program->release();
  });

  return advance.x * scale;
}


#if 0

int TextRenderer::draw(int x, int y, const char* str, const glm::vec4& color) {
  int compactColor = ((int(color.x * 255.0f) & 0xFF)) |
    ((int(color.y * 255.0f) & 0xFF) << 8) |
    ((int(color.z * 255.0f) & 0xFF) << 16) |
    ((int(color.w * 255.0f) & 0xFF) << 24);

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
    //glBindTexture(GL_TEXTURE_2D, glyph.textureID());

    int left = x + glyph.bounds().x();
    int right = x + glyph.bounds().x() + glyph.bounds().width();
    int bottom = y + glyph.bounds().y();
    int top = y + glyph.bounds().y() + glyph.bounds().height();

    glm::vec2  leftBottom = glm::vec2(float(left), float(bottom));
    glm::vec2  rightTop = glm::vec2(float(right), float(top));

    float scale = QApplication::desktop()->windowHandle()->devicePixelRatio() / IMAGE_SIZE;
    float ls = glyph.location().x() * scale;
    float rs = (glyph.location().x() + glyph.bounds().width()) * scale;
    float bt = glyph.location().y() * scale;
    float tt = (glyph.location().y() + glyph.bounds().height()) * scale;

    const int NUM_COORDS_SCALARS_PER_GLYPH = 16;
    float vertexBuffer[NUM_COORDS_SCALARS_PER_GLYPH] = { leftBottom.x, leftBottom.y, ls, bt,
      rightTop.x, leftBottom.y, rs, bt,
      rightTop.x, rightTop.y, rs, tt,
      leftBottom.x, rightTop.y, ls, tt, };

    const int NUM_COLOR_SCALARS_PER_GLYPH = 4;
    int colorBuffer[NUM_COLOR_SCALARS_PER_GLYPH] = { compactColor, compactColor, compactColor, compactColor };

    gpu::Buffer::Size offset = sizeof(vertexBuffer) * _numGlyphsBatched;
    gpu::Buffer::Size colorOffset = sizeof(colorBuffer) * _numGlyphsBatched;
    if ((offset + sizeof(vertexBuffer)) > _glyphsBuffer->getSize()) {
      _glyphsBuffer->append(sizeof(vertexBuffer), (gpu::Buffer::Byte*) vertexBuffer);
      _glyphsColorBuffer->append(sizeof(colorBuffer), (gpu::Buffer::Byte*) colorBuffer);
    } else {
      _glyphsBuffer->setSubData(offset, sizeof(vertexBuffer), (gpu::Buffer::Byte*) vertexBuffer);
      _glyphsColorBuffer->setSubData(colorOffset, sizeof(colorBuffer), (gpu::Buffer::Byte*) colorBuffer);
    }
    _numGlyphsBatched++;

    x += glyph.width();
  }

  // TODO: remove these calls once we move to a full batched rendering of the text, for now, one draw call per draw() function call
  drawBatch();
  clearBatch();

  return maxHeight;
}
//_glyphsStreamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
//const int NUM_POS_COORDS = 2;
//const int VERTEX_TEXCOORD_OFFSET = NUM_POS_COORDS * sizeof(float);
//_glyphsStreamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), VERTEX_TEXCOORD_OFFSET);

//_glyphsStreamFormat->setAttribute(gpu::Stream::COLOR, 1, gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA));

//_glyphsStream->addBuffer(_glyphsBuffer, 0, _glyphsStreamFormat->getChannels().at(0)._stride);
//_glyphsStream->addBuffer(_glyphsColorBuffer, 0, _glyphsStreamFormat->getChannels().at(1)._stride);

//_font.setKerning(false);
#endif
