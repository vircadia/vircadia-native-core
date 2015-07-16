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

#include <glm/glm.hpp>
#include <QColor>

// the standard sans serif font family
#define SANS_FONT_FAMILY "Helvetica"

// the standard sans serif font family
#define SERIF_FONT_FAMILY "Timeless"

// the standard mono font family
#define MONO_FONT_FAMILY "Courier"

// the Inconsolata font family
#ifdef Q_OS_WIN
#define INCONSOLATA_FONT_FAMILY "Fixedsys"
#define INCONSOLATA_FONT_WEIGHT QFont::Normal
#else
#define INCONSOLATA_FONT_FAMILY "Inconsolata"
#define INCONSOLATA_FONT_WEIGHT QFont::Bold
#endif

namespace gpu {
class Batch;
}
class Font3D;

// TextRenderer3D is actually a fairly thin wrapper around a Font class
// defined in the cpp file.
class TextRenderer3D {
public:
    enum EffectType { NO_EFFECT, SHADOW_EFFECT, OUTLINE_EFFECT };

    static TextRenderer3D* getInstance(const char* family, int weight = -1, bool italic = false,
                                       EffectType effect = NO_EFFECT, int effectThickness = 1);
    ~TextRenderer3D();

    glm::vec2 computeExtent(const QString& str) const;
    float getFontSize() const; // Pixel size
    
    void draw(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color = glm::vec4(1.0f),
              const glm::vec2& bounds = glm::vec2(-1.0f));

private:
    TextRenderer3D(const char* family, int weight = -1, bool italic = false,
                   EffectType effect = NO_EFFECT, int effectThickness = 1);

    // the type of effect to apply
    const EffectType _effectType;

    // the thickness of the effect
    const int _effectThickness;

    // text color
    glm::vec4 _color;

    Font3D* _font;
};


#endif // hifi_TextRenderer3D_h
