//
//  Created by Bradley Austin Davis on 2016/02/15
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtGlobal>

#include "../WindowOpenGLDisplayPlugin.h"

class HmdDisplayPlugin : public WindowOpenGLDisplayPlugin {
public:
    bool isHmd() const override final { return true; }
    float getIPD() const override final { return _ipd; }
    glm::mat4 getEyeToHeadTransform(Eye eye) const override final { return _eyeOffsets[eye]; }
    glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const override final { return _eyeProjections[eye]; }
    glm::mat4 getCullingProjection(const glm::mat4& baseProjection) const override final { return _cullingProjection; }
    glm::uvec2 getRecommendedUiSize() const override final { return uvec2(1920, 1080); }
    glm::uvec2 getRecommendedRenderSize() const override final { return _renderTargetSize; }
    void activate() override;
    void deactivate() override;

protected:
    void internalPresent() override;
    void customizeContext() override;

    std::array<glm::mat4, 2> _eyeOffsets;
    std::array<glm::mat4, 2> _eyeProjections;
    glm::mat4 _cullingProjection;
    glm::uvec2 _renderTargetSize;
    float _ipd { 0.064f };
private:
    bool _enablePreview { false };
    bool _monoPreview { true };
};

