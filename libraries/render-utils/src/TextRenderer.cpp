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
#include <QApplication>
#include <QtDebug>
#include <QString>
#include <QStringList>
#include <QBuffer>
#include <QFile>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

// FIXME, decouple from the GL headers
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gpu/GLBackend.h"
#include "gpu/Stream.h"

#include "GLMHelpers.h"
#include "MatrixStack.h"
#include "RenderUtilsLogging.h"
#include "TextRenderer.h"

#include "sdf_text_vert.h"
#include "sdf_text_frag.h"

// Helper functions for reading binary data from an IO device
template<class T>
void readStream(QIODevice & in, T & t) {
    in.read((char*) &t, sizeof(t));
}

template<typename T, size_t N>
void readStream(QIODevice & in, T (&t)[N]) {
    in.read((char*) t, N);
}

template<class T, size_t N>
void fillBuffer(QBuffer & buffer, T (&t)[N]) {
    buffer.setData((const char*) t, N);
}

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
    
    Font();
    
    using TexturePtr = QSharedPointer < QOpenGLTexture >;
    using VertexArrayPtr = QSharedPointer< QOpenGLVertexArrayObject >;
    using ProgramPtr = QSharedPointer < QOpenGLShaderProgram >;
    using BufferPtr = QSharedPointer < QOpenGLBuffer >;

    // maps characters to cached glyph info
    // HACK... the operator[] const for QHash returns a 
    // copy of the value, not a const value reference, so
    // we declare the hash as mutable in order to avoid such 
    // copies
    mutable QHash<QChar, Glyph> _glyphs;

    // the id of the glyph texture to which we're currently writing
    GLuint _currentTextureID;

    int _pointSize;

    // the height of the current row of characters
    int _rowHeight;

    QString _family;
    float _fontSize { 0 };
    float _leading { 0 };
    float _ascent { 0 };
    float _descent { 0 };
    float _spaceWidth { 0 };

    BufferPtr _vertices;
    BufferPtr _indices;
    TexturePtr _texture;
    VertexArrayPtr _vao;
    QImage _image;
    ProgramPtr _program;

    const Glyph & getGlyph(const QChar & c) const;
    void read(QIODevice& path);
    // Initialize the OpenGL structures
    void setupGL();

    glm::vec2 computeExtent(const QString & str) const;

    glm::vec2 computeTokenExtent(const QString & str) const;

    glm::vec2 drawString(float x, float y, const QString & str,
            const glm::vec4& color, TextRenderer::EffectType effectType,
            const glm::vec2& bound);

private:
    QStringList tokenizeForWrapping(const QString & str) const;

    bool _initialized;
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

Font::Font() : _initialized(false) {
    static bool fontResourceInitComplete = false;
    if (!fontResourceInitComplete) {
        Q_INIT_RESOURCE(fonts);
        fontResourceInitComplete = true;
    }
}

// NERD RAGE: why doesn't QHash have a 'const T & operator[] const' member
const Glyph & Font::getGlyph(const QChar & c) const {
    if (!_glyphs.contains(c)) {
        return _glyphs[QChar('?')];
    }
    return _glyphs[c];
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
    _rowHeight = _fontSize + _descent;

    // Read character count
    uint16_t count;
    readStream(in, count);
    // read metrics data for each character
    QVector<Glyph> glyphs(count);
    // std::for_each instead of Qt foreach because we need non-const references
    std::for_each(glyphs.begin(), glyphs.end(), [&](Glyph & g) {
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
}

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 tex;
    TextureVertex() {
    }
    TextureVertex(const glm::vec2 & pos, const glm::vec2 & tex) :
            pos(pos), tex(tex) {
    }
    TextureVertex(const QPointF & pos, const QPointF & tex) :
            pos(pos.x(), pos.y()), tex(tex.x(), tex.y()) {
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

QRectF Glyph::bounds() const {
    return glmToRect(offset, size);
}

QRectF Glyph::textureBounds(const glm::vec2 & textureSize) const {
    return glmToRect(texOffset, texSize);
}

void Font::setupGL() {
    if (_initialized) {
        return;
    }
    _initialized = true;

    _texture = TexturePtr(
            new QOpenGLTexture(_image, QOpenGLTexture::GenerateMipMaps));
    _program = ProgramPtr(new QOpenGLShaderProgram());
    if (!_program->create()) {
        qFatal("Could not create text shader");
    }
    if (!_program->addShaderFromSourceCode(QOpenGLShader::Vertex, sdf_text_vert) || //
        !_program->addShaderFromSourceCode(QOpenGLShader::Fragment, sdf_text_frag) || //
        !_program->link()) {
        qFatal("%s", _program->log().toLocal8Bit().constData());
    }

    std::vector<TextureVertex> vertexData;
    std::vector<GLuint> indexData;
    vertexData.reserve(_glyphs.size() * 4);
    std::for_each(_glyphs.begin(), _glyphs.end(), [&](Glyph & m) {
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

    _vao = VertexArrayPtr(new QOpenGLVertexArrayObject());
    _vao->create();
    _vao->bind();

    _vertices = BufferPtr(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
    _vertices->create();
    _vertices->bind();
    _vertices->allocate(&vertexData[0],
            sizeof(TextureVertex) * vertexData.size());
    _indices = BufferPtr(new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer));
    _indices->create();
    _indices->bind();
    _indices->allocate(&indexData[0], sizeof(GLuint) * indexData.size());

    GLsizei stride = (GLsizei) sizeof(TextureVertex);
    void* offset = (void*) offsetof(TextureVertex, tex);
    int posLoc = _program->attributeLocation("Position");
    int texLoc = _program->attributeLocation("TexCoord");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, false, stride, nullptr);
    glEnableVertexAttribArray(texLoc);
    glVertexAttribPointer(texLoc, 2, GL_FLOAT, false, stride, offset);
    _vao->release();
}

// FIXME there has to be a cleaner way of doing this
QStringList Font::tokenizeForWrapping(const QString & str) const {
    QStringList result;
    foreach(const QString & token1, str.split(" ")) {
        bool lineFeed = false;
        if (token1.isEmpty()) {
            result << token1;
            continue;
        }
        foreach(const QString & token2, token1.split("\n")) {
            if (lineFeed) {
                result << "\n";
            }
            if (token2.size()) {
                result << token2;
            }
            lineFeed = true;
        }
    }
    return result;
}


glm::vec2 Font::computeTokenExtent(const QString & token) const {
    glm::vec2 advance(0, _rowHeight - _descent);
    foreach(QChar c, token) {
        assert(c != ' ' && c != '\n');
        const Glyph & m = getGlyph(c);
        advance.x += m.d;
    }
    return advance;
}


glm::vec2 Font::computeExtent(const QString & str) const {
    glm::vec2 extent(0, _rowHeight - _descent);
    // FIXME, come up with a better method of splitting text
    // that will allow wrapping but will preserve things like
    // tabs or consecutive spaces
    bool firstTokenOnLine = true;
    float lineWidth = 0.0f;
    QStringList tokens = tokenizeForWrapping(str);
    foreach(const QString & token, tokens) {
        if (token == "\n") {
            extent.x = std::max(lineWidth, extent.x);
            lineWidth = 0.0f;
            extent.y += _rowHeight;
            firstTokenOnLine = true;
            continue;
        }
        if (!firstTokenOnLine) {
            lineWidth += _spaceWidth;
        }
        lineWidth += computeTokenExtent(token).x;
        firstTokenOnLine = false;
    }
    extent.x = std::max(lineWidth, extent.x);
    return extent;
}

// FIXME support the maxWidth parameter and allow the text to automatically wrap
// even without explicit line feeds.
glm::vec2 Font::drawString(float x, float y, const QString & str,
        const glm::vec4& color, TextRenderer::EffectType effectType,
        const glm::vec2& bounds) {

    setupGL();

    // Stores how far we've moved from the start of the string, in DTP units
    glm::vec2 advance(0, -_rowHeight - _descent);

    _program->bind();
    _program->setUniformValue("Color", color.r, color.g, color.b, color.a);
    _program->setUniformValue("Projection",
            fromGlm(MatrixStack::projection().top()));
    if (effectType == TextRenderer::OUTLINE_EFFECT) {
        _program->setUniformValue("Outline", true);
    }
    // Needed?
    glEnable(GL_TEXTURE_2D);
    _texture->bind();
    _vao->bind();

    MatrixStack & mv = MatrixStack::modelview();
    // scale the modelview into font units
    mv.translate(glm::vec3(0, _ascent, 0));
    foreach(const QString & token, tokenizeForWrapping(str)) {
        if (token == "\n") {
            advance.x = 0.0f;
            advance.y -= _rowHeight;
            // If we've wrapped right out of the bounds, then we're 
            // done with rendering the tokens
            if (bounds.y > 0 && std::abs(advance.y) > bounds.y) {
                break;
            }
            continue;
        }

        glm::vec2 tokenExtent = computeTokenExtent(token);
        if (bounds.x > 0 && advance.x > 0) {
            // We check if we'll be out of bounds
            if (advance.x + tokenExtent.x >= bounds.x) {
                // We're out of bounds, so wrap to the next line
                advance.x = 0.0f;
                advance.y -= _rowHeight;
                // If we've wrapped right out of the bounds, then we're 
                // done with rendering the tokens
                if (bounds.y > 0 && std::abs(advance.y) > bounds.y) {
                    break;
                }
            }
        }

        foreach(const QChar & c, token) {
            // get metrics for this character to speed up measurements
            const Glyph & m = getGlyph(c);
            // We create an offset vec2 to hold the local offset of this character
            // This includes compensating for the inverted Y axis of the font
            // coordinates
            glm::vec2 offset(advance);
            offset.y -= m.size.y;
            // Bind the new position
            mv.withPush([&] {
                        mv.translate(offset);
                        // FIXME find a better (and GL ES 3.1 compatible) way of rendering the text
                        // that doesn't involve a single GL call per character.
                        // Most likely an 'indirect' call or an 'instanced' call.
                        _program->setUniformValue("ModelView", fromGlm(mv.top()));
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(m.indexOffset));
                    });
            advance.x += m.d;
        }
        advance.x += _spaceWidth;
    }

    _vao->release();
    _texture->release(); // TODO: Brad & Sam, let's discuss this. Without this non-textured quads get their colors borked.
    _program->release();
    // FIXME, needed?
    // glDisable(GL_TEXTURE_2D);
    
    
    return advance;
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

glm::vec2 TextRenderer::computeExtent(const QString & str) const {
    float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
    if (_font) {
        return _font->computeExtent(str) * scale;
    }
    return glm::vec2(0.1f,0.1f);
}

float TextRenderer::draw(float x, float y, const QString & str,
    const glm::vec4& color, const glm::vec2 & bounds) {
    glm::vec4 actualColor(color);
    if (actualColor.r < 0) {
        actualColor = toGlm(_color);
    }

    float scale = (_pointSize / DEFAULT_POINT_SIZE) * 0.25f;
    glm::vec2 result;

    MatrixStack::withPushAll([&] {
        MatrixStack & mv = MatrixStack::modelview();
        MatrixStack & pr = MatrixStack::projection();
        gpu::GLBackend::fetchMatrix(GL_MODELVIEW_MATRIX, mv.top());
        gpu::GLBackend::fetchMatrix(GL_PROJECTION_MATRIX, pr.top());

        // scale the modelview into font units
        // FIXME migrate the constant scale factor into the geometry of the
        // fonts so we don't have to flip the Y axis here and don't have to
        // scale at all.
        mv.translate(glm::vec2(x, y)).scale(glm::vec3(scale, -scale, scale));
        // The font does all the OpenGL work
        if (_font) {
            result = _font->drawString(x, y, str, actualColor, _effectType, bounds / scale);
        }
    });
    return result.x;
}

