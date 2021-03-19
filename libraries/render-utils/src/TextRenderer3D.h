//
//  TextRenderer3D.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 4/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextRenderer3D_h
#define hifi_TextRenderer3D_h

#include <memory>
#include <glm/glm.hpp>

#include "text/Font.h"
#include "TextEffect.h"
#include "TextAlignment.h"
#include "FontFamilies.h"

class TextRenderer3D {
public:
    static TextRenderer3D* getInstance(const char* family);

    glm::vec2 computeExtent(const QString& str) const;
    float getFontSize() const; // Pixel size
    
    void draw(gpu::Batch& batch, float x, float y, const glm::vec2& bounds,
              const QString& str, const glm::vec4& color, bool unlit, bool forward);
    void draw(gpu::Batch& batch, float x, float y, const glm::vec2& bounds, float scale,
              const QString& str, const QString& font, const glm::vec4& color, const glm::vec3& effectColor,
              float effectThickness, TextEffect effect, TextAlignment alignment, bool unlit, bool forward);

private:
    TextRenderer3D(const char* family);

    QString _family;

    std::shared_ptr<Font> _font;
    Font::DrawInfo _drawInfo;

};

#endif // hifi_TextRenderer3D_h
