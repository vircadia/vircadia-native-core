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


#include <gpu/GPUConfig.h>
#include <gpu/GLBackend.h>
#include <gpu/Stream.h>

#include <QApplication>
#include <QtDebug>
#include <QString>
#include <QStringList>
#include <QBuffer>
#include <QFile>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GLMHelpers.h"
#include "MatrixStack.h"
#include "RenderUtilsLogging.h"
#include "TextRenderer.h"

#include "sdf_text_vert.h"
#include "sdf_text_frag.h"

const float DEFAULT_POINT_SIZE = 12;

// Helper functions for reading binary data from an IO device
template<class T>
void readStream(QIODevice& in, T& t) {
    in.read((char*) &t, sizeof(t));
}

template<typename T, size_t N>
void readStream(QIODevice& in, T (&t)[N]) {
    in.read((char*) t, N);
}

template<class T, size_t N>
void fillBuffer(QBuffer& buffer, T (&t)[N]) {
    buffer.setData((const char*) t, N);
}

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 tex;
    TextureVertex() {
    }
    TextureVertex(const glm::vec2& pos, const glm::vec2& tex) :
    pos(pos), tex(tex) {
    }
    TextureVertex(const QPointF& pos, const QPointF& tex) :
    pos(pos.x(), pos.y()), tex(tex.x(), tex.y()) {
    }
};

struct QuadBuilder {
    TextureVertex vertices[4];
    QuadBuilder(const QRectF& r, const QRectF& tr) {
        vertices[0] = TextureVertex(r.bottomLeft(), tr.topLeft());
        vertices[1] = TextureVertex(r.bottomRight(), tr.topRight());
        vertices[2] = TextureVertex(r.topLeft(), tr.bottomLeft());
        vertices[3] = TextureVertex(r.topRight(), tr.bottomRight());
    }
};

// FIXME support the shadow effect, or remove it from the API
// FIXME figure out how to improve the anti-aliasing on the 
// interior of the outline fonts

// stores the font metrics for a single character
struct Glyph {
    QChar c;
    glm::vec2 texOffset;
    glm::vec2 texSize;
    glm::vec2 size;
    glm::vec2 offset;
    float d;  // xadvance - adjusts character positioning
    size_t indexOffset;

    QRectF bounds() const { return glmToRect(offset, size); }
    QRectF textureBounds() const { return glmToRect(texOffset, texSize); }

    void read(QIODevice& in);
};

void Glyph::read(QIODevice& in) {
    uint16_t charcode;
    readStream(in, charcode);
    c = charcode;
    readStream(in, texOffset);
    readStream(in, size);
    readStream(in, offset);
    readStream(in, d);
    texSize = size;
}

class Font {
public:
    Font();
    
    void read(QIODevice& path);

    glm::vec2 computeExtent(const QString& str) const;
    
    // Render string to batch
    void drawString(gpu::Batch& batch, float x, float y, const QString& str,
            const glm::vec4& color, TextRenderer::EffectType effectType,
            const glm::vec2& bound);

private:
    QStringList tokenizeForWrapping(const QString& str) const;
    QStringList splitLines(const QString& str) const;
    glm::vec2 computeTokenExtent(const QString& str) const;
    
    const Glyph& getGlyph(const QChar& c) const;
    
    // maps characters to cached glyph info
    // HACK... the operator[] const for QHash returns a
    // copy of the value, not a const value reference, so
    // we declare the hash as mutable in order to avoid such
    // copies
    mutable QHash<QChar, Glyph> _glyphs;
    
    // Font characteristics
    QString _family;
    float _fontSize = 0.0f;
    float _rowHeight = 0.0f;
    float _leading = 0.0f;
    float _ascent = 0.0f;
    float _descent = 0.0f;
    float _spaceWidth = 0.0f;
    
    // gpu structures
    gpu::PipelinePointer _pipeline;
    gpu::TexturePointer _texture;
    gpu::Stream::FormatPointer _format;
    gpu::BufferView _vertices;
    gpu::BufferView _texCoords;
    
    // last string render characteristics
    QString _lastStringRendered;
    glm::vec2 _lastBounds;
};

static QHash<QString, Font*> LOADED_FONTS;

Font* loadFont(QIODevice& fontFile) {
    Font* result = new Font();
    result->read(fontFile);
    return result;
}

Font* loadFont(const QString& family) {
    if (!LOADED_FONTS.contains(family)) {
        
        const QString SDFF_COURIER_PRIME_FILENAME = ":/CourierPrime.sdff";
        const QString SDFF_INCONSOLATA_MEDIUM_FILENAME = ":/InconsolataMedium.sdff";
        const QString SDFF_ROBOTO_FILENAME = ":/Roboto.sdff";
        const QString SDFF_TIMELESS_FILENAME = ":/Timeless.sdff";
        
        QString loadFilename;
        
        if (family == MONO_FONT_FAMILY) {
            loadFilename = SDFF_COURIER_PRIME_FILENAME;
        } else if (family == INCONSOLATA_FONT_FAMILY) {
            loadFilename = SDFF_INCONSOLATA_MEDIUM_FILENAME;
        } else if (family == SANS_FONT_FAMILY) {
            loadFilename = SDFF_ROBOTO_FILENAME;
        } else {
            if (!LOADED_FONTS.contains(SERIF_FONT_FAMILY)) {
                loadFilename = SDFF_TIMELESS_FILENAME;
            } else {
                LOADED_FONTS[family] = LOADED_FONTS[SERIF_FONT_FAMILY];
            }
        }
        
        if (!loadFilename.isEmpty()) {
            QFile fontFile(loadFilename);
            fontFile.open(QIODevice::ReadOnly);
            
            qCDebug(renderutils) << "Loaded font" << loadFilename << "from Qt Resource System.";
            
            LOADED_FONTS[family] = loadFont(fontFile);
        }
    }
    return LOADED_FONTS[family];
}

Font::Font() {
    static bool fontResourceInitComplete = false;
    if (!fontResourceInitComplete) {
        Q_INIT_RESOURCE(fonts);
        fontResourceInitComplete = true;
    }
}

// NERD RAGE: why doesn't QHash have a 'const T & operator[] const' member
const Glyph& Font::getGlyph(const QChar& c) const {
    if (!_glyphs.contains(c)) {
        return _glyphs[QChar('?')];
    }
    return _glyphs[c];
}

QStringList Font::splitLines(const QString& str) const {
    return str.split('\n');
}

QStringList Font::tokenizeForWrapping(const QString& str) const {
    QStringList tokens;
    for(auto line : splitLines(str)) {
        if (!tokens.empty()) {
            tokens << QString('\n');
        }
        tokens << line.split(' ');
    }
    return tokens;
}

glm::vec2 Font::computeTokenExtent(const QString& token) const {
    glm::vec2 advance(0, _fontSize);
    foreach(QChar c, token) {
        Q_ASSERT(c != '\n');
        advance.x += (c == ' ') ? _spaceWidth : getGlyph(c).d;
    }
    return advance;
}

glm::vec2 Font::computeExtent(const QString& str) const {
    glm::vec2 extent = glm::vec2(0.0f, 0.0f);
    
    QStringList tokens = splitLines(str);
    foreach(const QString& token, tokens) {
        glm::vec2 tokenExtent = computeTokenExtent(token);
        extent.x = std::max(tokenExtent.x, extent.x);
    }
    extent.y = tokens.count() * _rowHeight;
    
    return extent;
}

void Font::read(QIODevice& in) {
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
    _rowHeight = _fontSize + _leading;

    // Read character count
    uint16_t count;
    readStream(in, count);
    // read metrics data for each character
    QVector<Glyph> glyphs(count);
    // std::for_each instead of Qt foreach because we need non-const references
    std::for_each(glyphs.begin(), glyphs.end(), [&](Glyph& g) {
        g.read(in);
    });

    // read image data
    QImage image;
    if (!image.loadFromData(in.readAll(), "PNG")) {
        qFatal("Failed to read SDFF image");
    }
    gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    if (image.hasAlphaChannel()) {
        formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA);
        formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::BGRA);
    }
    _texture = gpu::TexturePointer(
        gpu::Texture::create2D(formatGPU, image.width(), image.height(),
            gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
    _texture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
    _texture->autoGenerateMips(-1);

    _glyphs.clear();
    glm::vec2 imageSize = toGlm(image.size());
    foreach(Glyph g, glyphs) {
        // Adjust the pixel texture coordinates into UV coordinates,
        g.texSize /= imageSize;
        g.texOffset /= imageSize;
        // store in the character to glyph hash
        _glyphs[g.c] = g;
    };
    
    // Setup render pipeline
    auto vertexShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(sdf_text_vert)));
    auto pixelShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(sdf_text_frag)));
    gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vertexShader, pixelShader));
    
    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding("Outline", 0));
    slotBindings.insert(gpu::Shader::Binding("Offset", 1));
    slotBindings.insert(gpu::Shader::Binding("Color", 2));
    gpu::Shader::makeProgram(*program, slotBindings);
    
    gpu::StatePointer state = gpu::StatePointer(new gpu::State());
    
    _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
}

void Font::drawString(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color,
                      TextRenderer::EffectType effectType, const glm::vec2& bounds) {
    // Top left of text
    glm::vec2 advance = glm::vec2(0.0f, 0.0f);
    
    if (str != _lastStringRendered || bounds != _lastBounds) {
        gpu::BufferPointer vertices = gpu::BufferPointer(new gpu::Buffer());
        foreach(const QString& token, tokenizeForWrapping(str)) {
            bool isNewLine = token == QString('\n');
            bool forceNewLine = false;
            
            // Handle wrapping
            if (!isNewLine && (bounds.x != -1) && (advance.x + computeExtent(token).x)) {
                // We are out of the x bound, force new line
                forceNewLine = true;
            }
            if (isNewLine || forceNewLine) {
                // Character return, move the advance to a new line
                advance = glm::vec2(0.0f, advance.y + _rowHeight);
                if (isNewLine) {
                    // No need to draw anything, go directly to next token
                    continue;
                }
            }
            if ((bounds.y != -1) && (advance.y + _fontSize > bounds.y)) {
                // We are out of the y bound, stop drawing
                break;
            }
            
            // Draw the token
            if (!isNewLine) {
                for (auto c : token) {
                    auto glyph = _glyphs[c];
                    
                    // Build translated quad and add it to the buffer
                    QuadBuilder qd(glyph.bounds().translated(advance.x, advance.y),
                                   glyph.textureBounds());
                    vertices->append(sizeof(QuadBuilder), (const gpu::Byte*)qd.vertices);
                    
                    // Advance by glyph size
                    advance.x += glyph.d;
                }
                
                // Add space after all non return tokens
                advance.x += _spaceWidth;
            }
        }
        
        // Setup rendering structures
        static const int STRIDES = sizeof(TextureVertex);
        static const int OFFSET = offsetof(TextureVertex, tex);
        _format = gpu::Stream::FormatPointer(new gpu::Stream::Format());
        _format->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION,
                              gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
        _format->setAttribute(gpu::Stream::TEXCOORD, gpu::Stream::TEXCOORD,
                              gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
        
        _vertices = gpu::BufferView(vertices, 0, vertices->getSize(), STRIDES,
                                    _format->getAttributes().at(gpu::Stream::POSITION)._element);
        _texCoords = gpu::BufferView(vertices, OFFSET, vertices->getSize(), STRIDES,
                                     _format->getAttributes().at(gpu::Stream::TEXCOORD)._element);
        _lastStringRendered = str;
        _lastBounds = bounds;
    }
    
    batch.setInputFormat(_format);
    batch.setInputBuffer(0, _vertices);
    batch.setInputBuffer(1, _texCoords);
    batch.setUniformTexture(2, _texture);
    batch._glUniform1f(0, (effectType == TextRenderer::OUTLINE_EFFECT) ? 1.0f : 0.0f);
    batch._glUniform2f(1, x, y);
    batch._glUniform4fv(2, 4, (const GLfloat*)&color);
    batch.draw(gpu::QUADS, _vertices.getNumElements());
}

TextRenderer* TextRenderer::getInstance(const char* family, float pointSize,
        int weight, bool italic, EffectType effect, int effectThickness,
        const QColor& color) {
    if (pointSize < 0) {
        pointSize = DEFAULT_POINT_SIZE;
    }
    return new TextRenderer(family, pointSize, weight, italic, effect,
            effectThickness, color);
}

TextRenderer::TextRenderer(const char* family, float pointSize, int weight,
        bool italic, EffectType effect, int effectThickness,
        const QColor& color) :
        _effectType(effect), _effectThickness(effectThickness), _pointSize(pointSize), _color(color), _font(loadFont(family)) {
    if (!_font) {
        qWarning() << "Unable to load font with family " << family;
        _font = loadFont("Courier");
    }
    if (1 != _effectThickness) {
        qWarning() << "Effect thickness not current supported";
    }
    if (NO_EFFECT != _effectType && OUTLINE_EFFECT != _effectType) {
        qWarning() << "Effect thickness not current supported";
    }
}

TextRenderer::~TextRenderer() {
}

glm::vec2 TextRenderer::computeExtent(const QString& str) const {
    float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
    if (_font) {
        return _font->computeExtent(str) * scale;
    }
    return glm::vec2(0.1f,0.1f);
}

void TextRenderer::draw(float x, float y, const QString& str, const glm::vec4& color, const glm::vec2& bounds) {
    gpu::Batch batch;
    draw(batch, x, y, str, color, bounds);
    gpu::GLBackend::renderBatch(batch);
}

void TextRenderer::draw(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color,
                         const glm::vec2& bounds) {
 
    glm::vec4 actualColor(color);
    if (actualColor.r < 0) {
        actualColor = toGlm(_color);
    }
    
    float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
    
    // The font does all the OpenGL work
    if (_font) {
        _font->drawString(batch, x, y, str, actualColor, _effectType, bounds / scale);
    }
}

