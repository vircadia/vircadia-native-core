//
//  Created by Bradley Austin Davis on 2016/02/15
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <ThreadSafeValueCache.h>

#include <QtGlobal>

#include "../OpenGLDisplayPlugin.h"

class HmdDisplayPlugin : public OpenGLDisplayPlugin {
    using Parent = OpenGLDisplayPlugin;
public:
    bool isHmd() const override final { return true; }
    float getIPD() const override final { return _ipd; }
    glm::mat4 getEyeToHeadTransform(Eye eye) const override final { return _eyeOffsets[eye]; }
    glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const override final { return _eyeProjections[eye]; }
    glm::mat4 getCullingProjection(const glm::mat4& baseProjection) const override final { return _cullingProjection; }
    glm::uvec2 getRecommendedUiSize() const override final;
    glm::uvec2 getRecommendedRenderSize() const override final { return _renderTargetSize; }
    void setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) override final;
    bool isDisplayVisible() const override { return isHmdMounted(); }

    virtual glm::mat4 getHeadPose() const override;

protected:
    virtual void hmdPresent() = 0;
    virtual bool isHmdMounted() const = 0;
    virtual void postPreview() {};

    void internalActivate() override;
    void compositeOverlay() override;
    void compositePointer() override;
    void internalPresent() override;
    void customizeContext() override;
    void uncustomizeContext() override;
    void updateFrameData() override;

    std::array<glm::mat4, 2> _eyeOffsets;
    std::array<glm::mat4, 2> _eyeProjections;
    glm::mat4 _cullingProjection;
    glm::uvec2 _renderTargetSize;
    float _ipd { 0.064f };
    using EyePoses = std::array<glm::mat4, 2>;
    QMap<uint32_t, EyePoses> _renderEyePoses;
    EyePoses _currentRenderEyePoses;
    ThreadSafeValueCache<glm::mat4> _headPoseCache { glm::mat4() };

private:
    bool _enablePreview { false };
    bool _monoPreview { true };
    ShapeWrapperPtr _sphereSection;
};

