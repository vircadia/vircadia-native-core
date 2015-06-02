//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "StereoDisplayPlugin.h"

class SideBySideStereoDisplayPlugin : public StereoDisplayPlugin {
    Q_OBJECT
public:
    SideBySideStereoDisplayPlugin();
    virtual const QString & getName() override;

    void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize) override;
private:
    static const QString NAME;
};
