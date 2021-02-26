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

#include "text/Font.h"

TextRenderer3D* TextRenderer3D::getInstance(const char* family) {
    return new TextRenderer3D(family);
}

TextRenderer3D::TextRenderer3D(const char* family) :
    _family(family),
    _font(Font::load(family)) {
    if (!_font) {
        qWarning() << "Unable to load font with family " << family;
        _font = Font::load(ROBOTO_FONT_FAMILY);
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

void TextRenderer3D::draw(gpu::Batch& batch, float x, float y, const glm::vec2& bounds,
                          const QString& str, const glm::vec4& color, bool unlit, bool forward) {
    if (_font) {
        _font->drawString(batch, _drawInfo, str, color, glm::vec3(0.0f), 0, TextEffect::NO_EFFECT, TextAlignment::LEFT, { x, y }, bounds, 1.0f, unlit, forward);
    }
}

void TextRenderer3D::draw(gpu::Batch& batch, float x, float y, const glm::vec2& bounds, float scale,
                          const QString& str, const QString& font, const glm::vec4& color, const glm::vec3& effectColor,
                          float effectThickness, TextEffect effect, TextAlignment alignment, bool unlit, bool forward) {
    if (font != _family) {
        _family = font;
        _font = Font::load(_family);
    }
    if (_font) {
        _font->drawString(batch, _drawInfo, str, color, effectColor, effectThickness, effect, alignment, { x, y }, bounds, scale, unlit, forward);
    }
}