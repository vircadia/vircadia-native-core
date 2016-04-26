#include "Font.h"

#include <QFile>
#include <QImage>

#include <ColorUtils.h>

#include <StreamHelpers.h>

#include "sdf_text3D_vert.h"
#include "sdf_text3D_frag.h"
#include "sdf_text3D_overlay_frag.h"

#include "../RenderUtilsLogging.h"
#include "FontFamilies.h"

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 tex;
    TextureVertex() {}
    TextureVertex(const glm::vec2& pos, const glm::vec2& tex) : pos(pos), tex(tex) {}
};

static const int NUMBER_OF_INDICES_PER_QUAD = 6; // 1 quad = 2 triangles
static const int VERTICES_PER_QUAD = 4; // 1 quad = 4 vertices

struct QuadBuilder {
    TextureVertex vertices[VERTICES_PER_QUAD];

    QuadBuilder(const glm::vec2& min, const glm::vec2& size,
                const glm::vec2& texMin, const glm::vec2& texSize) {
        // min = bottomLeft
        vertices[0] = TextureVertex(min,
                                    texMin + glm::vec2(0.0f, texSize.y));
        vertices[1] = TextureVertex(min + glm::vec2(size.x, 0.0f),
                                    texMin + texSize);
        vertices[2] = TextureVertex(min + glm::vec2(0.0f, size.y),
                                    texMin);
        vertices[3] = TextureVertex(min + size,
                                    texMin + glm::vec2(texSize.x, 0.0f));
    }
    QuadBuilder(const Glyph& glyph, const glm::vec2& offset) :
    QuadBuilder(offset + glm::vec2(glyph.offset.x, glyph.offset.y - glyph.size.y), glyph.size,
                    glyph.texOffset, glyph.texSize) {}

};



static QHash<QString, Font::Pointer> LOADED_FONTS;

Font::Pointer Font::load(QIODevice& fontFile) {
    Pointer font = std::make_shared<Font>();
    font->read(fontFile);
    return font;
}

Font::Pointer Font::load(const QString& family) {
    if (!LOADED_FONTS.contains(family)) {

        static const QString SDFF_COURIER_PRIME_FILENAME{ ":/CourierPrime.sdff" };
        static const QString SDFF_INCONSOLATA_MEDIUM_FILENAME{ ":/InconsolataMedium.sdff" };
        static const QString SDFF_ROBOTO_FILENAME{ ":/Roboto.sdff" };
        static const QString SDFF_TIMELESS_FILENAME{ ":/Timeless.sdff" };

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

            LOADED_FONTS[family] = load(fontFile);
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

    image = image.convertToFormat(QImage::Format_RGBA8888);

    gpu::Element formatGPU = gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB);
    gpu::Element formatMip = gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::RGB);
    if (image.hasAlphaChannel()) {
        formatGPU = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        formatMip = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::BGRA);
    }
    _texture = gpu::TexturePointer(gpu::Texture::create2D(formatGPU, image.width(), image.height(),
                                   gpu::Sampler(gpu::Sampler::FILTER_MIN_POINT_MAG_LINEAR)));
    _texture->assignStoredMip(0, formatMip, image.byteCount(), image.constBits());
    _texture->autoGenerateMips(-1);
}

void Font::setupGPU() {
    if (!_initialized) {
        _initialized = true;

        // Setup render pipeline
        {
            auto vertexShader = gpu::Shader::createVertex(std::string(sdf_text3D_vert));
            auto pixelShader = gpu::Shader::createPixel(std::string(sdf_text3D_frag));
            auto pixelShaderOverlay = gpu::Shader::createPixel(std::string(sdf_text3D_overlay_frag));
            gpu::ShaderPointer program = gpu::Shader::createProgram(vertexShader, pixelShader);
            gpu::ShaderPointer programOverlay = gpu::Shader::createProgram(vertexShader, pixelShaderOverlay);

            gpu::Shader::BindingSet slotBindings;
            gpu::Shader::makeProgram(*program, slotBindings);
            gpu::Shader::makeProgram(*programOverlay, slotBindings);

            _fontLoc = program->getTextures().findLocation("Font");
            _outlineLoc = program->getUniforms().findLocation("Outline");
            _colorLoc = program->getUniforms().findLocation("Color");

            auto state = std::make_shared<gpu::State>();
            state->setCullMode(gpu::State::CULL_BACK);
            state->setDepthTest(true, true, gpu::LESS_EQUAL);
            state->setBlendFunction(true,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            _pipeline = gpu::Pipeline::create(program, state);

            auto layeredState = std::make_shared<gpu::State>();
            layeredState->setCullMode(gpu::State::CULL_BACK);
            layeredState->setDepthTest(true, true, gpu::LESS_EQUAL);
            _layeredPipeline = gpu::Pipeline::create(programOverlay, layeredState);
        }

        // Sanity checks
        static const int OFFSET = offsetof(TextureVertex, tex);
        assert(OFFSET == sizeof(glm::vec2));
        assert(sizeof(glm::vec2) == 2 * sizeof(float));
        assert(sizeof(TextureVertex) == 2 * sizeof(glm::vec2));
        assert(sizeof(QuadBuilder) == 4 * sizeof(TextureVertex));

        // Setup rendering structures
        _format = std::make_shared<gpu::Stream::Format>();
        _format->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
        _format->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), OFFSET);
    }
}

void Font::rebuildVertices(float x, float y, const QString& str, const glm::vec2& bounds) {
    _verticesBuffer = std::make_shared<gpu::Buffer>();
    _numVertices = 0;
    _indicesBuffer = std::make_shared<gpu::Buffer>();
    _numIndices = 0;

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
                quint16 verticesOffset = _numVertices;

                QuadBuilder qd(glyph, advance - glm::vec2(0.0f, _ascent));
                _verticesBuffer->append(sizeof(QuadBuilder), (const gpu::Byte*)&qd);
                _numVertices += 4;
                
                // Sam's recommended triangle slices
                // Triangle tri1 = { v0, v1, v3 };
                // Triangle tri2 = { v1, v2, v3 };
                // NOTE: Random guy on the internet's recommended triangle slices
                // Triangle tri1 = { v0, v1, v2 };
                // Triangle tri2 = { v2, v3, v0 };

                // The problem here being that the 4 vertices are { ll, lr, ul, ur }, a Z pattern
                // Additionally, you want to ensure that the shared side vertices are used sequentially
                // to improve cache locality
                //
                //  2 -- 3
                //  |    |
                //  |    |
                //  0 -- 1
                //
                //  { 0, 1, 2 } -> { 2, 1, 3 }
                quint16 indices[NUMBER_OF_INDICES_PER_QUAD];
                indices[0] = verticesOffset + 0;
                indices[1] = verticesOffset + 1;
                indices[2] = verticesOffset + 2;
                indices[3] = verticesOffset + 2;
                indices[4] = verticesOffset + 1;
                indices[5] = verticesOffset + 3;
                _indicesBuffer->append(sizeof(indices), (const gpu::Byte*)indices);
                _numIndices += NUMBER_OF_INDICES_PER_QUAD;
                

                // Advance by glyph size
                advance.x += glyph.d;
            }

            // Add space after all non return tokens
            advance.x += _spaceWidth;
        }
    }
}

void Font::drawString(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4* color,
                      EffectType effectType, const glm::vec2& bounds, bool layered) {
    if (str == "") {
        return;
    }

    if (str != _lastStringRendered || bounds != _lastBounds) {
        rebuildVertices(x, y, str, bounds);
    }

    setupGPU();

    batch.setPipeline(layered ? _layeredPipeline : _pipeline);
    batch.setResourceTexture(_fontLoc, _texture);
    batch._glUniform1i(_outlineLoc, (effectType == OUTLINE_EFFECT));
    
    // need the gamma corrected color here
    glm::vec4 lrgba = ColorUtils::sRGBToLinearVec4(*color);
    batch._glUniform4fv(_colorLoc, 1, (const float*)&lrgba);

    batch.setInputFormat(_format);
    batch.setInputBuffer(0, _verticesBuffer, 0, _format->getChannels().at(0)._stride);
    batch.setIndexBuffer(gpu::UINT16, _indicesBuffer, 0);
    batch.drawIndexed(gpu::TRIANGLES, _numIndices, 0);
}
