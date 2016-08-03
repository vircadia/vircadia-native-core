//
//  RenderableTextEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableTextEntityItem_h
#define hifi_RenderableTextEntityItem_h

#include <TextEntityItem.h>
#include <TextRenderer3D.h>
#include <Interpolate.h>

#include "RenderableEntityItem.h"

const int FIXED_FONT_POINT_SIZE = 40;

class RenderableTextEntityItem : public TextEntityItem  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableTextEntityItem(const EntityItemID& entityItemID) : TextEntityItem(entityItemID) { }
    ~RenderableTextEntityItem() { delete _textRenderer; }

    virtual void render(RenderArgs* args) override;

    bool isTransparent() override { return Interpolate::calculateFadeRatio(_fadeStartTime) < 1.0f; }

    SIMPLE_RENDERABLE();
    
private:
    TextRenderer3D* _textRenderer = TextRenderer3D::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE / 2.0f);
    quint64 _fadeStartTime { usecTimestampNow() };
};


#endif // hifi_RenderableTextEntityItem_h
