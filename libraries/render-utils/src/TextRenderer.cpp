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

#include <gpu/GPUConfig.h>
#include <gpu/GLBackend.h>

#include <QBuffer>
#include <QImage>
#include <QStringList>
#include <QFile>

#include "GLMHelpers.h"
#include "MatrixStack.h"
#include "RenderUtilsLogging.h"

#include "sdf_text_vert.h"
#include "sdf_text_frag.h"

#include "GeometryCache.h"
#include "DeferredLightingEffect.h"

// FIXME support the shadow effect, or remove it from the API
// FIXME figure out how to improve the anti-aliasing on the
// interior of the outline fonts
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

// stores the font metrics for a single character
struct Glyph {
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
    QuadBuilder(const Glyph& glyph, const glm::vec2& offset) :
    QuadBuilder(offset + glyph.offset - glm::vec2(0.0f, glyph.size.y), glyph.size,
                    glyph.texOffset, glyph.texSize) {}
    
};

QDebug operator<<(QDebug debug, glm::vec2& value) {
    debug << value.x << value.y;
    return debug;
}

QDebug operator<<(QDebug debug, glm::vec3& value) {
    debug << value.x << value.y << value.z;
    return debug;
}

QDebug operator<<(QDebug debug, TextureVertex& value) {
    debug << "Pos:" << value.pos << ", Tex:" << value.tex;
    return debug;
}

QDebug operator<<(QDebug debug, QuadBuilder& value) {
    debug << '\n' << value.vertices[0]
    << '\n' << value.vertices[1]
    << '\n' << value.vertices[2]
    << '\n' << value.vertices[3];
    return debug;
}

class Font {
public:
    Font();
    
    void read(QIODevice& path);

    glm::vec2 computeExtent(const QString& str) const;
    float getRowHeight() const { return _rowHeight; }
    
    // Render string to batch
    void drawString(gpu::Batch& batch, float x, float y, const QString& str,
            const glm::vec4& color, TextRenderer::EffectType effectType,
            const glm::vec2& bound);

private:
    QStringList tokenizeForWrapping(const QString& str) const;
    QStringList splitLines(const QString& str) const;
    glm::vec2 computeTokenExtent(const QString& str) const;
    
    const Glyph& getGlyph(const QChar& c) const;
    
    void setupGPU();
    
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

    _glyphs.clear();
    glm::vec2 imageSize = toGlm(image.size());
    foreach(Glyph g, glyphs) {
        // Adjust the pixel texture coordinates into UV coordinates,
        g.texSize /= imageSize;
        g.texOffset /= imageSize;
        // store in the character to glyph hash
        _glyphs[g.c] = g;
    };
    
    qDebug() << _family << "size" << image.size();
    qDebug() << _family << "format" << image.format();
    image = image.convertToFormat(QImage::Format_RGBA8888);
    qDebug() << _family << "size" << image.size();
    qDebug() << _family << "format" << image.format();
    
    gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::UINT8, gpu::RGB);
    if (image.hasAlphaChannel()) {
        formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA);
        formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::BGRA);
    }
    _texture = gpu::TexturePointer(gpu::Texture::create2D(formatGPU, image.width(), image.height(),
                                   gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
    _texture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
    _texture->autoGenerateMips(-1);
}

#include <gpu/Shader.h>
void Font::setupGPU() {
    if (!_initialized) {
        _initialized = true;
        
        // Setup render pipeline
        auto vertexShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(sdf_text_vert)));
        auto pixelShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(sdf_text_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vertexShader, pixelShader));
        
        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);
        
        _fontLoc = program->getTextures().findLocation("Font");
        _outlineLoc = program->getUniforms().findLocation("Outline");
        _colorLoc = program->getUniforms().findLocation("Color");
        
        
        auto f = [&] (QString str, const gpu::Shader::SlotSet& set) {
            if (set.size() == 0) {
                return;
            }
            qDebug() << str << set.size();
            for (auto slot : set) {
                qDebug() << "    " << QString::fromStdString(slot._name) << slot._location;
            }
        };
        f("getUniforms:", program->getUniforms());
        f("getBuffers:", program->getBuffers());
        f("getTextures:", program->getTextures());
        f("getSamplers:", program->getSamplers());
        f("getInputs:", program->getInputs());
        f("getOutputs:", program->getOutputs());
        
        qDebug() << "Texture:" << _texture.get();
        
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
        assert(sizeof(glm::vec3) == 3 * sizeof(float));
        assert(sizeof(TextureVertex) == sizeof(glm::vec2) + sizeof(glm::vec2));
        assert(sizeof(QuadBuilder) == 4 * sizeof(TextureVertex));
        
        // Setup rendering structures
        _format.reset(new gpu::Stream::Format());
        _format->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
        _format->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), OFFSET);
    }
}

void Font::drawString(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color,
                      TextRenderer::EffectType effectType, const glm::vec2& bounds) {
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
            if (!isNewLine && (bounds.x != -1) && (advance.x + computeExtent(token).x > bounds.x)) {
                // We are out of the x bound, force new line
                forceNewLine = true;
            }
            if (isNewLine || forceNewLine) {
                // Character return, move the advance to a new line
                advance = glm::vec2(0.0f, advance.y - _rowHeight);

                if (isNewLine) {
                    // No need to draw anything, go directly to next token
                    continue;
                } else if (computeExtent(token).x > bounds.x) {
                    // token will never fit, stop drawing
                    break;
                }
            }
            if ((bounds.y != -1) && (advance.y - _fontSize < -bounds.y)) {
                // We are out of the y bound, stop drawing
                break;
            }
            
            // Draw the token
            if (!isNewLine) {
                for (auto c : token) {
                    auto glyph = _glyphs[c];
                    
                    QuadBuilder qd(glyph, advance - glm::vec2(0.0f, _fontSize));
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
    batch._glUniform1f(_outlineLoc, (effectType == TextRenderer::OUTLINE_EFFECT) ? 1.0f : 0.0f);
    batch._glUniform4fv(_colorLoc, 1, (const GLfloat*)&color);
    
    batch.setInputFormat(_format);
    batch.setInputBuffer(0, _verticesBuffer, 0, _format->getChannels().at(0)._stride);
    batch.draw(gpu::QUADS, _numVertices, 0);
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

TextRenderer::TextRenderer(const char* family, float pointSize, int weight, bool italic,
                           EffectType effect, int effectThickness, const QColor& color) :
    _effectType(effect),
    _effectThickness(effectThickness),
    _pointSize(pointSize),
    _color(toGlm(color)),
    _font(loadFont(family)) {
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
    if (_font) {
        float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
        return _font->computeExtent(str) * scale;
    }
    return glm::vec2(0.1f,0.1f);
}

float TextRenderer::getRowHeight() const {
    if (_font) {
        return _font->getRowHeight();
    }
    return 1.0f;
}

void TextRenderer::draw(float x, float y, const QString& str, const glm::vec4& color, const glm::vec2& bounds) {
    // The font does all the OpenGL work
    if (_font) {
        float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
        glPushMatrix();
        gpu::Batch batch;
        Transform transform;
        transform.setTranslation(glm::vec3(x, y, 0.0f));
        transform.setScale(glm::vec3(scale, -scale, scale));
        batch.setModelTransform(transform);
        draw3D(batch, 0.0f, 0.0f, str, color, bounds);
        gpu::GLBackend::renderBatch(batch, true);
        glPopMatrix();
    }
}

void TextRenderer::draw3D(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color,
                         const glm::vec2& bounds) {
    // The font does all the OpenGL work
    if (_font) {
        glm::vec4 actualColor(color);
        if (actualColor.r < 0) {
            actualColor = _color;
        }
        _font->drawString(batch, x, y, str, actualColor, _effectType, bounds);
    }
}

