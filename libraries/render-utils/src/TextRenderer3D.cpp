//
//  TextRenderer3D.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 4/24/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextRenderer3D.h"

#include <gpu/GPUConfig.h>
#include <gpu/GLBackend.h>
#include <gpu/Shader.h>

#include <QBuffer>
#include <QImage>
#include <QStringList>
#include <QFile>

#include "GLMHelpers.h"
#include "MatrixStack.h"
#include "RenderUtilsLogging.h"

#include "sdf_text3D_vert.h"
#include "sdf_text3D_frag.h"

#include "GeometryCache.h"
#include "DeferredLightingEffect.h"

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

// stores the font metrics for a single character
struct Glyph3D {
    QChar c;
    glm::vec2 texOffset;
    glm::vec2 texSize;
    glm::vec2 size;
    glm::vec2 offset;
    float d;  // xadvance - adjusts character positioning
    size_t indexOffset;

    // We adjust bounds because offset is the bottom left corner of the font but the top left corner of a QRect
    QRectF bounds() const { return glmToRect(offset, size).translated(0.0f, -size.y); }
    QRectF textureBounds() const { return glmToRect(texOffset, texSize); }

    void read(QIODevice& in);
};

void Glyph3D::read(QIODevice& in) {
    uint16_t charcode;
    readStream(in, charcode);
    c = charcode;
    readStream(in, texOffset);
    readStream(in, size);
    readStream(in, offset);
    readStream(in, d);
    texSize = size;
}

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 tex;
    TextureVertex() {}
    TextureVertex(const glm::vec2& pos, const glm::vec2& tex) : pos(pos), tex(tex) {}
};

struct QuadBuilder {
    TextureVertex vertices[4];
    QuadBuilder(const glm::vec2& min, const glm::vec2& size,
                const glm::vec2& texMin, const glm::vec2& texSize) {
        // min = bottomLeft
        vertices[0] = TextureVertex(min,
                                    texMin + glm::vec2(0.0f, texSize.y));
        vertices[1] = TextureVertex(min + glm::vec2(size.x, 0.0f),
                                    texMin + texSize);
        vertices[2] = TextureVertex(min + size,
                                    texMin + glm::vec2(texSize.x, 0.0f));
        vertices[3] = TextureVertex(min + glm::vec2(0.0f, size.y),
                                    texMin);
    }
    QuadBuilder(const Glyph3D& glyph, const glm::vec2& offset) :
    QuadBuilder(offset + glm::vec2(glyph.offset.x, glyph.offset.y - glyph.size.y), glyph.size,
                    glyph.texOffset, glyph.texSize) {}
    
};

class Font3D {
public:
    Font3D();
    
    void read(QIODevice& path);

    glm::vec2 computeExtent(const QString& str) const;
    float getFontSize() const { return _fontSize; }
    
    // Render string to batch
    void drawString(gpu::Batch& batch, float x, float y, const QString& str,
            const glm::vec4* color, TextRenderer3D::EffectType effectType,
            const glm::vec2& bound);

private:
    QStringList tokenizeForWrapping(const QString& str) const;
    QStringList splitLines(const QString& str) const;
    glm::vec2 computeTokenExtent(const QString& str) const;
    
    const Glyph3D& getGlyph(const QChar& c) const;
    
    void setupGPU();
    
    // maps characters to cached glyph info
    // HACK... the operator[] const for QHash returns a
    // copy of the value, not a const value reference, so
    // we declare the hash as mutable in order to avoid such
    // copies
    mutable QHash<QChar, Glyph3D> _glyphs;
    
    // Font characteristics
    QString _family;
    float _fontSize = 0.0f;
    float _leading = 0.0f;
    float _ascent = 0.0f;
    float _descent = 0.0f;
    float _spaceWidth = 0.0f;
    
    bool _initialized = false;
    
    // gpu structures
    gpu::PipelinePointer _pipeline;
    gpu::TexturePointer _texture;
    gpu::Stream::FormatPointer _format;
    gpu::BufferPointer _verticesBuffer;
    gpu::BufferStreamPointer _stream;
    unsigned int _numVertices = 0;
    
    int _fontLoc = -1;
    int _outlineLoc = -1;
    int _colorLoc = -1;
    
    // last string render characteristics
    QString _lastStringRendered;
    glm::vec2 _lastBounds;
};

static QHash<QString, Font3D*> LOADED_FONTS;

Font3D* loadFont3D(QIODevice& fontFile) {
    Font3D* result = new Font3D();
    result->read(fontFile);
    return result;
}

Font3D* loadFont3D(const QString& family) {
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
            
            LOADED_FONTS[family] = loadFont3D(fontFile);
        }
    }
    return LOADED_FONTS[family];
}

Font3D::Font3D() {
    static bool fontResourceInitComplete = false;
    if (!fontResourceInitComplete) {
        Q_INIT_RESOURCE(fonts);
        fontResourceInitComplete = true;
    }
}

// NERD RAGE: why doesn't QHash have a 'const T & operator[] const' member
const Glyph3D& Font3D::getGlyph(const QChar& c) const {
    if (!_glyphs.contains(c)) {
        return _glyphs[QChar('?')];
    }
    return _glyphs[c];
}

QStringList Font3D::splitLines(const QString& str) const {
    return str.split('\n');
}

QStringList Font3D::tokenizeForWrapping(const QString& str) const {
    QStringList tokens;
    for(auto line : splitLines(str)) {
        if (!tokens.empty()) {
            tokens << QString('\n');
        }
        tokens << line.split(' ');
    }
    return tokens;
}

glm::vec2 Font3D::computeTokenExtent(const QString& token) const {
    glm::vec2 advance(0, _fontSize);
    foreach(QChar c, token) {
        Q_ASSERT(c != '\n');
        advance.x += (c == ' ') ? _spaceWidth : getGlyph(c).d;
    }
    return advance;
}

glm::vec2 Font3D::computeExtent(const QString& str) const {
    glm::vec2 extent = glm::vec2(0.0f, 0.0f);
    
    QStringList lines{ splitLines(str) };
    if (!lines.empty()) {
        for(const auto& line : lines) {
            glm::vec2 tokenExtent = computeTokenExtent(line);
            extent.x = std::max(tokenExtent.x, extent.x);
        }
        extent.y = lines.count() * _fontSize;
    }
    return extent;
}

void Font3D::read(QIODevice& in) {
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
    
    // Read character count
    uint16_t count;
    readStream(in, count);
    // read metrics data for each character
    QVector<Glyph3D> glyphs(count);
    // std::for_each instead of Qt foreach because we need non-const references
    std::for_each(glyphs.begin(), glyphs.end(), [&](Glyph3D& g) {
        g.read(in);
    });

    // read image data
    QImage image;
    if (!image.loadFromData(in.readAll(), "PNG")) {
        qFatal("Failed to read SDFF image");
    }

    _glyphs.clear();
    glm::vec2 imageSize = toGlm(image.size());
    foreach(Glyph3D g, glyphs) {
        // Adjust the pixel texture coordinates into UV coordinates,
        g.texSize /= imageSize;
        g.texOffset /= imageSize;
        // store in the character to glyph hash
        _glyphs[g.c] = g;
    };
    
    image = image.convertToFormat(QImage::Format_RGBA8888);
    
    gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    if (image.hasAlphaChannel()) {
        formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA);
        formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::BGRA);
    }
    _texture = gpu::TexturePointer(gpu::Texture::create2D(formatGPU, image.width(), image.height(),
                                   gpu::Sampler(gpu::Sampler::FILTER_MIN_POINT_MAG_LINEAR)));
    _texture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
    _texture->autoGenerateMips(-1);
}

void Font3D::setupGPU() {
    if (!_initialized) {
        _initialized = true;
        
        // Setup render pipeline
        auto vertexShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(sdf_text3D_vert)));
        auto pixelShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(sdf_text3D_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vertexShader, pixelShader));
        
        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);
        
        _fontLoc = program->getTextures().findLocation("Font");
        _outlineLoc = program->getUniforms().findLocation("Outline");
        _colorLoc = program->getUniforms().findLocation("Color");
        
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setBlendFunction(false,
                                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
        
        // Sanity checks
        static const int OFFSET = offsetof(TextureVertex, tex);
        assert(OFFSET == sizeof(glm::vec2));
        assert(sizeof(glm::vec2) == 2 * sizeof(float));
        assert(sizeof(TextureVertex) == 2 * sizeof(glm::vec2));
        assert(sizeof(QuadBuilder) == 4 * sizeof(TextureVertex));
        
        // Setup rendering structures
        _format.reset(new gpu::Stream::Format());
        _format->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
        _format->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), OFFSET);
    }
}

void Font3D::drawString(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4* color,
                      TextRenderer3D::EffectType effectType, const glm::vec2& bounds) {
    if (str == "") {
        return;
    }
    
    if (str != _lastStringRendered || bounds != _lastBounds) {
        _verticesBuffer.reset(new gpu::Buffer());
        _numVertices = 0;
        _lastStringRendered = str;
        _lastBounds = bounds;
        
        // Top left of text
        glm::vec2 advance = glm::vec2(x, y);
        foreach(const QString& token, tokenizeForWrapping(str)) {
            bool isNewLine = (token == QString('\n'));
            bool forceNewLine = false;
            
            // Handle wrapping
            if (!isNewLine && (bounds.x != -1) && (advance.x + computeExtent(token).x > x + bounds.x)) {
                // We are out of the x bound, force new line
                forceNewLine = true;
            }
            if (isNewLine || forceNewLine) {
                // Character return, move the advance to a new line
                advance = glm::vec2(x, advance.y - _leading);

                if (isNewLine) {
                    // No need to draw anything, go directly to next token
                    continue;
                } else if (computeExtent(token).x > bounds.x) {
                    // token will never fit, stop drawing
                    break;
                }
            }
            if ((bounds.y != -1) && (advance.y - _fontSize < -y - bounds.y)) {
                // We are out of the y bound, stop drawing
                break;
            }
            
            // Draw the token
            if (!isNewLine) {
                for (auto c : token) {
                    auto glyph = _glyphs[c];
                    
                    QuadBuilder qd(glyph, advance - glm::vec2(0.0f, _ascent));
                    _verticesBuffer->append(sizeof(QuadBuilder), (const gpu::Byte*)&qd);
                    _numVertices += 4;
                    
                    // Advance by glyph size
                    advance.x += glyph.d;
                }
                
                // Add space after all non return tokens
                advance.x += _spaceWidth;
            }
        }
    }
    
    setupGPU();
    batch.setPipeline(_pipeline);
    batch.setUniformTexture(_fontLoc, _texture);
    batch._glUniform1i(_outlineLoc, (effectType == TextRenderer3D::OUTLINE_EFFECT));
    batch._glUniform4fv(_colorLoc, 1, (const GLfloat*)color);
    
    batch.setInputFormat(_format);
    batch.setInputBuffer(0, _verticesBuffer, 0, _format->getChannels().at(0)._stride);
    batch.draw(gpu::QUADS, _numVertices, 0);
}

TextRenderer3D* TextRenderer3D::getInstance(const char* family,
        int weight, bool italic, EffectType effect, int effectThickness) {
    return new TextRenderer3D(family, weight, italic, effect, effectThickness);
}

TextRenderer3D::TextRenderer3D(const char* family, int weight, bool italic,
                           EffectType effect, int effectThickness) :
    _effectType(effect),
    _effectThickness(effectThickness),
    _font(loadFont3D(family)) {
    if (!_font) {
        qWarning() << "Unable to load font with family " << family;
        _font = loadFont3D("Courier");
    }
    if (1 != _effectThickness) {
        qWarning() << "Effect thickness not currently supported";
    }
    if (NO_EFFECT != _effectType && OUTLINE_EFFECT != _effectType) {
        qWarning() << "Effect type not currently supported";
    }
}

TextRenderer3D::~TextRenderer3D() {
}

glm::vec2 TextRenderer3D::computeExtent(const QString& str) const {
    if (_font) {
        return _font->computeExtent(str);
    }
    return glm::vec2(0.0f, 0.0f);
}

float TextRenderer3D::getFontSize() const {
    if (_font) {
        return _font->getFontSize();
    }
    return 0.0f;
}

void TextRenderer3D::draw(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color,
                         const glm::vec2& bounds) {
    // The font does all the OpenGL work
    if (_font) {
        // Cache color so that the pointer stays valid.
        _color = color;
        _font->drawString(batch, x, y, str, &_color, _effectType, bounds);
    }
}

