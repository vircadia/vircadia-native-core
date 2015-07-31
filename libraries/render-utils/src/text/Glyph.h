//
//  Created by Bradley Austin Davis on 2015/07/16
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Glyph_h
#define hifi_Glyph_h

#include <stdint.h>

#include <QChar>
#include <QRect>
#include <QIODevice>

#include <GLMHelpers.h>

// stores the font metrics for a single character
struct Glyph {
    QChar c;
    vec2 texOffset;
    vec2 texSize;
    vec2 size;
    vec2 offset;
    float d;  // xadvance - adjusts character positioning
    size_t indexOffset;

    // We adjust bounds because offset is the bottom left corner of the font but the top left corner of a QRect
    QRectF bounds() const;
    QRectF textureBounds() const;

    void read(QIODevice& in);
};

#endif
