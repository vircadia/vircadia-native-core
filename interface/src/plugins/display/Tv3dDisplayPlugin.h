//
//  Tv3dDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "SimpleDisplayPlugin.h"
#include "LegacyDisplayPlugin.h"

class Tv3dDisplayPlugin : public LegacyDisplayPlugin {
    Q_OBJECT
public:
    static const QString NAME;
    virtual const QString & getName();

    virtual bool isStereo() const final { return true; }
    void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize);
    //virtual bool isMouseOnScreen() const { return true; }
    //virtual bool isThrottled() const;
    //virtual void preDisplay();
};
