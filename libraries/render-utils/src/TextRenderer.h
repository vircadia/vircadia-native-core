//
//  TextRenderer.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 4/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextRenderer_h
#define hifi_TextRenderer_h

#include <gpu/GPUConfig.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QImage>
#include <QVector>

#include <gpu/Resource.h>
#include <gpu/Stream.h>

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
class Font;

// TextRenderer is actually a fairly thin wrapper around a Font class
// defined in the cpp file.  
class TextRenderer {
public:
    enum EffectType { NO_EFFECT, SHADOW_EFFECT, OUTLINE_EFFECT };

    static TextRenderer* getInstance(const char* family, float pointSize = -1, int weight = -1, bool italic = false,
        EffectType effect = NO_EFFECT, int effectThickness = 1, const QColor& color = QColor(255, 255, 255));

    ~TextRenderer();

    glm::vec2 computeExtent(const QString& str) const;
    
    void draw(float x, float y, const QString& str, const glm::vec4& color = glm::vec4(-1.0f),
               const glm::vec2& bounds = glm::vec2(-1.0f));
    void draw(gpu::Batch& batch, float x, float y, const QString& str, const glm::vec4& color = glm::vec4(-1.0f),
               const glm::vec2& bounds = glm::vec2(-1.0f));

private:
    TextRenderer(const char* family, float pointSize = -1, int weight = -1, bool italic = false,
        EffectType effect = NO_EFFECT, int effectThickness = 1, const QColor& color = QColor(255, 255, 255));

    // the type of effect to apply
    const EffectType _effectType;

    // the thickness of the effect
    const int _effectThickness;

    const float _pointSize;

    // text color
    const QColor _color;

    Font* _font;
};


#endif // hifi_TextRenderer_h
