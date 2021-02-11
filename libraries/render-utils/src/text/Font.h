//
//  Created by Bradley Austin Davis on 2015/07/16
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Font_h
#define hifi_Font_h

#include <QObject>

#include "Glyph.h"
#include "TextEffect.h"
#include "TextAlignment.h"
#include <gpu/Batch.h>
#include <gpu/Pipeline.h>

class Font : public QObject {
    Q_OBJECT

public:
    using Pointer = std::shared_ptr<Font>;

    Font();

    void read(QIODevice& path);

    struct DrawParams {
        vec4 color { 0 };

        vec3 effectColor { 0 };
        float effectThickness { 0 };

        int effect { 0 };

#if defined(__clang__)
        __attribute__((unused))
#endif
        vec3 _spare;
    };

    struct DrawInfo {
        gpu::BufferPointer verticesBuffer { nullptr };
        gpu::BufferPointer indicesBuffer { nullptr };
        gpu::BufferPointer paramsBuffer { nullptr };
        uint32_t indexCount;

        QString string;
        glm::vec2 origin;
        glm::vec2 bounds;
        DrawParams params;
    };

    glm::vec2 computeExtent(const QString& str) const;
    float getFontSize() const { return _fontSize; }

    // Render string to batch
    void drawString(gpu::Batch& batch, DrawInfo& drawInfo, const QString& str, const glm::vec4& color,
                    const glm::vec3& effectColor, float effectThickness, TextEffect effect, TextAlignment alignment,
                    const glm::vec2& origin, const glm::vec2& bound, float scale, bool unlit, bool forward);

    static Pointer load(const QString& family);

    bool isLoaded() const { return _loaded; }
    void setLoaded(bool loaded) { _loaded = loaded; }

public slots:
    void handleFontNetworkReply();

private:
    static Pointer load(QIODevice& fontFile);
    QStringList tokenizeForWrapping(const QString& str) const;
    QStringList splitLines(const QString& str) const;
    glm::vec2 computeTokenExtent(const QString& str) const;

    const Glyph& getGlyph(const QChar& c) const;
    void buildVertices(DrawInfo& drawInfo, const QString& str, const glm::vec2& origin, const glm::vec2& bounds, float scale, bool enlargeForShadows,
                       TextAlignment alignment);

    void setupGPU();

    // maps characters to cached glyph info
    // HACK... the operator[] const for QHash returns a
    // copy of the value, not a const value reference, so
    // we declare the hash as mutable in order to avoid such
    // copies
    mutable QHash<QChar, Glyph> _glyphs;

    // Font characteristics
    QString _family;
    float _fontSize { 0.0f };
    float _leading { 0.0f };
    float _ascent { 0.0f };
    float _descent { 0.0f };
    float _spaceWidth { 0.0f };

    float _scale { 0.0f };
    TextAlignment _alignment;

    bool _loaded { true };

    gpu::TexturePointer _texture;
    gpu::BufferStreamPointer _stream;

    static std::map<std::tuple<bool, bool, bool>, gpu::PipelinePointer> _pipelines;
    static gpu::Stream::FormatPointer _format;
};

#endif

