#include "Font.h"

#include <QFile>
#include <QImage>
#include <QNetworkReply>

#include <ColorUtils.h>

#include <StreamHelpers.h>
#include <shaders/Shaders.h>

#include "../render-utils/ShaderConstants.h"
#include "../RenderUtilsLogging.h"
#include "FontFamilies.h"
#include "../StencilMaskPass.h"

#include "NetworkAccessManager.h"

static std::mutex fontMutex;

std::map<std::tuple<bool, bool, bool>, gpu::PipelinePointer> Font::_pipelines;
gpu::Stream::FormatPointer Font::_format;

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 tex;
    glm::vec4 bounds;
    TextureVertex() {}
    TextureVertex(const glm::vec2& pos, const glm::vec2& tex, const glm::vec4& bounds) : pos(pos), tex(tex), bounds(bounds) {}
};

static const int NUMBER_OF_INDICES_PER_QUAD = 6;  // 1 quad = 2 triangles
static const int VERTICES_PER_QUAD = 4;           // 1 quad = 4 vertices (must match value in sdf_text3D.slv)
const float DOUBLE_MAX_OFFSET_PIXELS = 20.0f;     // must match value in sdf_text3D.slh

struct QuadBuilder {
    TextureVertex vertices[VERTICES_PER_QUAD];

    QuadBuilder(const Glyph& glyph, const glm::vec2& offset, float scale, bool enlargeForShadows) {
        glm::vec2 min = offset + glm::vec2(glyph.offset.x, glyph.offset.y - glyph.size.y);
        glm::vec2 size = glyph.size;
        glm::vec2 texMin = glyph.texOffset;
        glm::vec2 texSize = glyph.texSize;

        // We need the pre-adjustment bounds for clamping
        glm::vec4 bounds = glm::vec4(texMin, texSize);
        if (enlargeForShadows) {
            glm::vec2 imageSize = glyph.size / glyph.texSize;
            glm::vec2 sizeDelta = 0.5f * DOUBLE_MAX_OFFSET_PIXELS * scale * imageSize;
            glm::vec2 oldSize = size;
            size += sizeDelta;
            min.y -= sizeDelta.y;

            texSize = texSize * (size / oldSize);
        }

        // min = bottomLeft
        vertices[0] = TextureVertex(min,
                                    texMin + glm::vec2(0.0f, texSize.y), bounds);
        vertices[1] = TextureVertex(min + glm::vec2(size.x, 0.0f),
                                    texMin + texSize, bounds);
        vertices[2] = TextureVertex(min + glm::vec2(0.0f, size.y),
                                    texMin, bounds);
        vertices[3] = TextureVertex(min + size,
                                    texMin + glm::vec2(texSize.x, 0.0f), bounds);
    }

};

Font::Pointer Font::load(QIODevice& fontFile) {
    Pointer font = std::make_shared<Font>();
    font->read(fontFile);
    return font;
}

static QHash<QString, Font::Pointer> LOADED_FONTS;

Font::Pointer Font::load(const QString& family) {
    std::lock_guard<std::mutex> lock(fontMutex);
    if (!LOADED_FONTS.contains(family)) {
        QString loadFilename;

        if (family == ROBOTO_FONT_FAMILY) {
            loadFilename = ":/Roboto.sdff";
        } else if (family == INCONSOLATA_FONT_FAMILY) {
            loadFilename = ":/InconsolataMedium.sdff";
        } else if (family == COURIER_FONT_FAMILY) {
            loadFilename = ":/CourierPrime.sdff";
        } else if (family == TIMELESS_FONT_FAMILY) {
            loadFilename = ":/Timeless.sdff";
        } else if (family.startsWith("http")) {
            auto loadingFont = std::make_shared<Font>();
            loadingFont->setLoaded(false);
            LOADED_FONTS[family] = loadingFont;

            auto& networkAccessManager = NetworkAccessManager::getInstance();

            QNetworkRequest networkRequest;
            networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
            networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
            networkRequest.setUrl(family);

            auto networkReply = networkAccessManager.get(networkRequest);
            connect(networkReply, &QNetworkReply::finished, loadingFont.get(), &Font::handleFontNetworkReply);
        } else if (!LOADED_FONTS.contains(ROBOTO_FONT_FAMILY)) {
            // Unrecognized font and we haven't loaded Roboto yet
            loadFilename = ":/Roboto.sdff";
        } else {
            // Unrecognized font but we've already loaded Roboto
            LOADED_FONTS[family] = LOADED_FONTS[ROBOTO_FONT_FAMILY];
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

void Font::handleFontNetworkReply() {
    auto requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        setLoaded(true);
        read(*requestReply);
    } else {
        qDebug() << "Error downloading " << requestReply->url() << " - " << requestReply->errorString();
    }
}

Font::Font() {
    static std::once_flag once;
    std::call_once(once, []{
        Q_INIT_RESOURCE(fonts);
    });
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

    QStringList lines = splitLines(str);
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
        qDebug() << "Bad SDFF file";
        _loaded = false;
        return;
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
        qDebug() << "Failed to read SDFF image";
        _loaded = false;
        return;
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
    // FIXME: We're forcing this to use only one mip, and then manually doing anisotropic filtering in the shader,
    // and also calling textureLod.  Shouldn't this just use anisotropic filtering and auto-generate mips?
    // We should also use smoothstep for anti-aliasing, as explained here: https://github.com/libgdx/libgdx/wiki/Distance-field-fonts
    _texture = gpu::Texture::create2D(formatGPU, image.width(), image.height(), gpu::Texture::SINGLE_MIP,
                                      gpu::Sampler(gpu::Sampler::FILTER_MIN_POINT_MAG_LINEAR));
    _texture->setStoredMipFormat(formatMip);
    _texture->assignStoredMip(0, image.byteCount(), image.constBits());
}

void Font::setupGPU() {
    if (_pipelines.empty()) {
        using namespace shader::render_utils::program;

        static const std::vector<std::tuple<bool, bool, bool, uint32_t>> keys = {
            std::make_tuple(false, false, false, sdf_text3D), std::make_tuple(true, false, false, sdf_text3D_translucent),
            std::make_tuple(false, true, false, sdf_text3D_unlit), std::make_tuple(true, true, false, sdf_text3D_translucent_unlit),
            std::make_tuple(false, false, true, sdf_text3D_forward), std::make_tuple(true, false, true, sdf_text3D_forward/*sdf_text3D_translucent_forward*/),
            std::make_tuple(false, true, true, sdf_text3D_translucent_unlit/*sdf_text3D_unlit_forward*/), std::make_tuple(true, true, true, sdf_text3D_translucent_unlit/*sdf_text3D_translucent_unlit_forward*/)
        };
        for (auto& key : keys) {
            auto state = std::make_shared<gpu::State>();
            state->setCullMode(gpu::State::CULL_BACK);
            state->setDepthTest(true, true, gpu::LESS_EQUAL);
            state->setBlendFunction(std::get<0>(key),
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            if (std::get<0>(key)) {
                PrepareStencil::testMask(*state);
            } else {
                PrepareStencil::testMaskDrawShape(*state);
            }
            _pipelines[std::make_tuple(std::get<0>(key), std::get<1>(key), std::get<2>(key))] = gpu::Pipeline::create(gpu::Shader::createProgram(std::get<3>(key)), state);
        }

        // Sanity checks
        static const int TEX_COORD_OFFSET = offsetof(TextureVertex, tex);
        static const int TEX_BOUNDS_OFFSET = offsetof(TextureVertex, bounds);
        assert(TEX_COORD_OFFSET == sizeof(glm::vec2));
        assert(sizeof(TextureVertex) == 2 * sizeof(glm::vec2) + sizeof(glm::vec4));
        assert(sizeof(QuadBuilder) == 4 * sizeof(TextureVertex));

        // Setup rendering structures
        _format = std::make_shared<gpu::Stream::Format>();
        _format->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ), 0);
        _format->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), TEX_COORD_OFFSET);
        _format->setAttribute(gpu::Stream::TEXCOORD1, 0, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW), TEX_BOUNDS_OFFSET);
    }
}

void Font::buildVertices(Font::DrawInfo& drawInfo, const QString& str, const glm::vec2& origin, const glm::vec2& bounds, float scale, bool enlargeForShadows) {
    drawInfo.verticesBuffer = std::make_shared<gpu::Buffer>();
    drawInfo.indicesBuffer = std::make_shared<gpu::Buffer>();
    drawInfo.indexCount = 0;
    int numVertices = 0;

    drawInfo.string = str;
    drawInfo.bounds = bounds;
    drawInfo.origin = origin;

    float enlargedBoundsX = bounds.x - 0.5f * DOUBLE_MAX_OFFSET_PIXELS * float(enlargeForShadows);

    // Top left of text
    glm::vec2 advance = origin;
    foreach(const QString& token, tokenizeForWrapping(str)) {
        bool isNewLine = (token == QString('\n'));
        bool forceNewLine = false;

        // Handle wrapping
        if (!isNewLine && (bounds.x != -1) && (advance.x + computeExtent(token).x > origin.x + enlargedBoundsX)) {
            // We are out of the x bound, force new line
            forceNewLine = true;
        }
        if (isNewLine || forceNewLine) {
            // Character return, move the advance to a new line
            advance = glm::vec2(origin.x, advance.y - _leading);

            if (isNewLine) {
                // No need to draw anything, go directly to next token
                continue;
            } else if (computeExtent(token).x > enlargedBoundsX) {
                // token will never fit, stop drawing
                break;
            }
        }
        if ((bounds.y != -1) && (advance.y - _fontSize < origin.y - bounds.y)) {
            // We are out of the y bound, stop drawing
            break;
        }

        // Draw the token
        if (!isNewLine) {
            for (auto c : token) {
                auto glyph = _glyphs[c];
                quint16 verticesOffset = numVertices;

                QuadBuilder qd(glyph, advance - glm::vec2(0.0f, _ascent), scale, enlargeForShadows);
                drawInfo.verticesBuffer->append(qd);
                numVertices += VERTICES_PER_QUAD;

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
                drawInfo.indicesBuffer->append(sizeof(indices), (const gpu::Byte*)indices);
                drawInfo.indexCount += NUMBER_OF_INDICES_PER_QUAD;

                // Advance by glyph size
                advance.x += glyph.d;
            }

            // Add space after all non return tokens
            advance.x += _spaceWidth;
        }
    }
}

void Font::drawString(gpu::Batch& batch, Font::DrawInfo& drawInfo, const QString& str, const glm::vec4& color,
                      const glm::vec3& effectColor, float effectThickness, TextEffect effect,
                      const glm::vec2& origin, const glm::vec2& bounds, float scale, bool unlit, bool forward) {
    if (!_loaded || str == "") {
        return;
    }

    int textEffect = (int)effect;
    const int SHADOW_EFFECT = (int)TextEffect::SHADOW_EFFECT;

    // If we're switching to or from shadow effect mode, we need to rebuild the vertices
    if (str != drawInfo.string || bounds != drawInfo.bounds || origin != drawInfo.origin ||
            (drawInfo.params.effect != textEffect && (textEffect == SHADOW_EFFECT || drawInfo.params.effect == SHADOW_EFFECT)) ||
            (textEffect == SHADOW_EFFECT && scale != _scale)) {
        _scale = scale;
        buildVertices(drawInfo, str, origin, bounds, scale, textEffect == SHADOW_EFFECT);
    }

    setupGPU();

    if (!drawInfo.paramsBuffer || drawInfo.params.color != color || drawInfo.params.effectColor != effectColor ||
            drawInfo.params.effectThickness != effectThickness || drawInfo.params.effect != textEffect) {
        drawInfo.params.color = color;
        drawInfo.params.effectColor = effectColor;
        drawInfo.params.effectThickness = effectThickness;
        drawInfo.params.effect = textEffect;

        // need the gamma corrected color here
        DrawParams gpuDrawParams;
        gpuDrawParams.color = ColorUtils::sRGBToLinearVec4(drawInfo.params.color);
        gpuDrawParams.effectColor = ColorUtils::sRGBToLinearVec3(drawInfo.params.effectColor);
        gpuDrawParams.effectThickness = drawInfo.params.effectThickness;
        gpuDrawParams.effect = drawInfo.params.effect;
        if (!drawInfo.paramsBuffer) {
            drawInfo.paramsBuffer = std::make_shared<gpu::Buffer>(sizeof(DrawParams), nullptr);
        }
        drawInfo.paramsBuffer->setSubData(0, sizeof(DrawParams), (const gpu::Byte*)&gpuDrawParams);
    }

    batch.setPipeline(_pipelines[std::make_tuple(color.a < 1.0f, unlit, forward)]);
    batch.setInputFormat(_format);
    batch.setInputBuffer(0, drawInfo.verticesBuffer, 0, _format->getChannels().at(0)._stride);
    batch.setResourceTexture(render_utils::slot::texture::TextFont, _texture);
    batch.setUniformBuffer(0, drawInfo.paramsBuffer, 0, sizeof(DrawParams));
    batch.setIndexBuffer(gpu::UINT16, drawInfo.indicesBuffer, 0);
    batch.drawIndexed(gpu::TRIANGLES, drawInfo.indexCount, 0);
}
