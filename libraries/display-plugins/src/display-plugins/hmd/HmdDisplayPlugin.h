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
#include <Transform.h>

#include "../OpenGLDisplayPlugin.h"

#ifdef Q_OS_WIN
#define HMD_HAND_LASER_SUPPORT
#endif

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

    QRect getRecommendedOverlayRect() const override final;

    virtual glm::mat4 getHeadPose() const override;

    bool setHandLaser(uint32_t hands, HandLaserMode mode, const vec4& color, const vec3& direction) override;

protected:
    virtual void hmdPresent() = 0;
    virtual bool isHmdMounted() const = 0;
    virtual void postPreview() {};
    virtual void updatePresentPose();

    bool internalActivate() override;
    void internalDeactivate() override;
    void compositeScene() override;
    void compositeOverlay() override;
    void compositePointer() override;
    void internalPresent() override;
    void customizeContext() override;
    void uncustomizeContext() override;
    void updateFrameData() override;
    void compositeExtra() override;

    struct HandLaserInfo {
        HandLaserMode mode { HandLaserMode::None };
        vec4 color { 1.0f };
        vec3 direction { 0, 0, -1 };

        // Is this hand laser info suitable for drawing?
        bool valid() const {
            return (mode != HandLaserMode::None && color.a > 0.0f && direction != vec3());
        }
    };

    Transform _uiModelTransform;
    std::array<HandLaserInfo, 2> _handLasers;
    std::array<glm::mat4, 2> _handPoses;

    Transform _presentUiModelTransform;
    std::array<HandLaserInfo, 2> _presentHandLasers;
    std::array<mat4, 2> _presentHandPoses;

    std::array<glm::mat4, 2> _eyeOffsets;
    std::array<glm::mat4, 2> _eyeProjections;
    std::array<glm::mat4, 2> _eyeInverseProjections;

    glm::mat4 _cullingProjection;
    glm::uvec2 _renderTargetSize;
    float _ipd { 0.064f };

    struct FrameInfo {
        glm::mat4 rawRenderPose;
        glm::mat4 renderPose;
        glm::mat4 rawPresentPose;
        glm::mat4 presentPose;
        double sensorSampleTime { 0 };
        double predictedDisplayTime { 0 };
        glm::mat3 presentReprojection;
    };

    QMap<uint32_t, FrameInfo> _frameInfos;
    FrameInfo _currentPresentFrameInfo;
    FrameInfo _currentRenderFrameInfo;

private:
    void updateOverlayProgram();
#ifdef HMD_HAND_LASER_SUPPORT
    void updateLaserProgram();
#endif
    void updateReprojectionProgram();

    bool _enablePreview { false };
    bool _monoPreview { true };
    bool _enableReprojection { true };
    bool _firstPreview { true };

    ProgramPtr _overlayProgram;
    struct OverlayUniforms {
        int32_t mvp { -1 };
        int32_t alpha { -1 };
        int32_t glowColors { -1 };
        int32_t glowPoints { -1 };
        int32_t resolution { -1 };
        int32_t radius { -1 };
    } _overlayUniforms;

    ProgramPtr _previewProgram;
    struct PreviewUniforms {
        int32_t previewTexture { -1 };
    } _previewUniforms;

    float _previewAspect { 0 };
    GLuint _previewTextureID { 0 };
    glm::uvec2 _prevWindowSize { 0, 0 };
    qreal _prevDevicePixelRatio { 0 };

    ProgramPtr _reprojectionProgram;
    struct ReprojectionUniforms {
        int32_t reprojectionMatrix { -1 };
        int32_t inverseProjectionMatrix { -1 };
        int32_t projectionMatrix { -1 };
    } _reprojectionUniforms;

    ShapeWrapperPtr _sphereSection;

#ifdef HMD_HAND_LASER_SUPPORT
    ProgramPtr _laserProgram;
    struct LaserUniforms {
        int32_t mvp { -1 };
        int32_t color { -1 };
    } _laserUniforms;
    ShapeWrapperPtr _laserGeometry;
#endif
};

