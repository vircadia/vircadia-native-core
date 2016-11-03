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
#include <StreamHelpers.h>

#include <gpu/Shader.h>

#include <QBuffer>
#include <QImage>
#include <QStringList>
#include <QFile>

#include "text/Font.h"

#include "GLMHelpers.h"
#include "MatrixStack.h"
#include "RenderUtilsLogging.h"

#include "sdf_text3D_vert.h"
#include "sdf_text3D_frag.h"

#include "GeometryCache.h"

const float TextRenderer3D::DEFAULT_POINT_SIZE = 12;

TextRenderer3D* TextRenderer3D::getInstance(const char* family, float pointSize,
    bool bold, bool italic, EffectType effect, int effectThickness) {
    return new TextRenderer3D(family, pointSize, false, italic, effect, effectThickness);
}

TextRenderer3D::TextRenderer3D(const char* family, float pointSize, int weight, bool italic,
        EffectType effect, int effectThickness) :
    _effectType(effect),
    _effectThickness(effectThickness),
    _font(Font::load(family)) {
    if (!_font) {
        qWarning() << "Unable to load font with family " << family;
        _font = Font::load("Courier");
    }
    if (1 != _effectThickness) {
        qWarning() << "Effect thickness not currently supported";
    }
    if (NO_EFFECT != _effectType && OUTLINE_EFFECT != _effectType) {
        qWarning() << "Effect type not currently supported";
    }
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
                         const glm::vec2& bounds, bool layered) {
    // The font does all the OpenGL work
    if (_font) {
        // Cache color so that the pointer stays valid.
        _color = color;
        _font->drawString(batch, x, y, str, &_color, _effectType, bounds, layered);
    }
}

