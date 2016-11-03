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
#include <QColor>

namespace gpu {
class Batch;
}
class Font;

#include "text/EffectType.h"
#include "text/FontFamilies.h"

// TextRenderer3D is actually a fairly thin wrapper around a Font class
// defined in the cpp file.
class TextRenderer3D {
public:
    static const float DEFAULT_POINT_SIZE;

    static TextRenderer3D* getInstance(const char* family, float pointSize = DEFAULT_POINT_SIZE, 
            bool bold = false, bool italic = false, EffectType effect = NO_EFFECT, int effectThickness = 1);

    glm::vec2 computeExtent(const QString& str) const;
    float getFontSize() const; // Pixel size
    
    void draw(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color = glm::vec4(1.0f),
              const glm::vec2& bounds = glm::vec2(-1.0f), bool layered = false);

private:
    TextRenderer3D(const char* family, float pointSize, int weight = -1, bool italic = false,
                   EffectType effect = NO_EFFECT, int effectThickness = 1);

    // the type of effect to apply
    const EffectType _effectType;

    // the thickness of the effect
    const int _effectThickness;

    // text color
    glm::vec4 _color;

    std::shared_ptr<Font> _font;
};


#endif // hifi_TextRenderer3D_h
