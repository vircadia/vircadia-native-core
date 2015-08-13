//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "../MainWindowOpenGLDisplayPlugin.h"

class StereoDisplayPlugin : public MainWindowOpenGLDisplayPlugin {
    Q_OBJECT
public:
    StereoDisplayPlugin();
    virtual bool isStereo() const override final { return true; }
    virtual bool isSupported() const override final;

    virtual void activate() override;

    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const override;
    virtual glm::vec3 getEyeOffset(Eye eye) const override;

protected:
    float _ipd{ 0.064f };
};
