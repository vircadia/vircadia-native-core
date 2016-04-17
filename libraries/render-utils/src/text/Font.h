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

#include "Glyph.h"
#include "EffectType.h"
#include <gpu/Batch.h>
#include <gpu/Pipeline.h>

class Font {
public:
    using Pointer = std::shared_ptr<Font>;

    Font();

    void read(QIODevice& path);

    glm::vec2 computeExtent(const QString& str) const;
    float getFontSize() const { return _fontSize; }

    // Render string to batch
    void drawString(gpu::Batch& batch, float x, float y, const QString& str,
        const glm::vec4* color, EffectType effectType,
        const glm::vec2& bound, bool layered = false);

    static Pointer load(QIODevice& fontFile);
    static Pointer load(const QString& family);

private:
    QStringList tokenizeForWrapping(const QString& str) const;
    QStringList splitLines(const QString& str) const;
    glm::vec2 computeTokenExtent(const QString& str) const;

    const Glyph& getGlyph(const QChar& c) const;
    void rebuildVertices(float x, float y, const QString& str, const glm::vec2& bounds);

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
    float _leading = 0.0f;
    float _ascent = 0.0f;
    float _descent = 0.0f;
    float _spaceWidth = 0.0f;

    bool _initialized = false;

    // gpu structures
    gpu::PipelinePointer _pipeline;
    gpu::PipelinePointer _layeredPipeline;
    gpu::TexturePointer _texture;
    gpu::Stream::FormatPointer _format;
    gpu::BufferPointer _verticesBuffer;
    gpu::BufferPointer _indicesBuffer;
    gpu::BufferStreamPointer _stream;
    unsigned int _numVertices = 0;
    unsigned int _numIndices = 0;

    int _fontLoc = -1;
    int _outlineLoc = -1;
    int _colorLoc = -1;

    // last string render characteristics
    QString _lastStringRendered;
    glm::vec2 _lastBounds;
};

#endif

