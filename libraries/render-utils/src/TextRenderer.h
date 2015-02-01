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

// a special "character" that renders as a solid block
const char SOLID_BLOCK_CHAR = 127;

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


class TextRenderer {
public:
    enum EffectType { NO_EFFECT, SHADOW_EFFECT, OUTLINE_EFFECT };

    class Properties {
    public:
      QString font;
      float pointSize;
      EffectType effect;
      int effectThickness;
      QColor color;
    };

    static TextRenderer* getInstance(const char* family, float pointSize = -1, int weight = -1, bool italic = false,
        EffectType effect = NO_EFFECT, int effectThickness = 1, const QColor& color = QColor(255, 255, 255));

    ~TextRenderer();

    // returns the height of the tallest character
    int calculateHeight(const char * str) const;
    int calculateHeight(const QString & str) const;

    int computeWidth(char ch) const;
    int computeWidth(const QChar & ch) const;

    int computeWidth(const QString & str) const;
    int computeWidth(const char * str) const;

    // also returns the height of the tallest character
    inline int draw(int x, int y, const char* str,
      const glm::vec4& color = glm::vec4(-1),
      float maxWidth = -1
    ) {
      return drawString(x, y, QString(str), color, maxWidth);
    }

    float drawString(
      float x, float y,
      const QString & str,
      const glm::vec4& color = glm::vec4(-1),
      float maxWidth = -1);

private:
    TextRenderer(const Properties& properties);

    // the type of effect to apply
    const EffectType _effectType;

    // the thickness of the effect
    const int _effectThickness;

    const float _pointSize;

    // text color
    const QColor _color;

    friend class Font;
    Font * _font;
};


#endif // hifi_TextRenderer_h
