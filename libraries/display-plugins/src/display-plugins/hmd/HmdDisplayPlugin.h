//
//  Created by Bradley Austin Davis on 2016/02/15
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <ThreadSafeValueCache.h>

#include <array>

#include <QtGlobal>
#include <Transform.h>

#include <gpu/Format.h>
#include <gpu/Stream.h>

#include "../CompositorHelper.h"
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
    bool isDisplayVisible() const override { return isHmdMounted(); }

    QRect getRecommendedOverlayRect() const override final;

    virtual glm::mat4 getHeadPose() const override;

    bool setHandLaser(uint32_t hands, HandLaserMode mode, const vec4& color, const vec3& direction) override;

    bool wantVsync() const override {
        return false;
    }

protected:
    virtual void hmdPresent() = 0;
    virtual bool isHmdMounted() const = 0;
    virtual void postPreview() {};
    virtual void updatePresentPose();

    bool beginFrameRender(uint32_t frameIndex) override;
    bool internalActivate() override;
    void internalDeactivate() override;
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
    std::array<mat4, 2> _handPoses;

    Transform _presentUiModelTransform;
    std::array<HandLaserInfo, 2> _presentHandLasers;
    std::array<mat4, 2> _presentHandPoses;
    std::array<std::pair<vec3, vec3>, 2> _presentHandLaserPoints;

    std::array<mat4, 2> _eyeOffsets;
    std::array<mat4, 2> _eyeProjections;
    std::array<mat4, 2> _eyeInverseProjections;

    mat4 _cullingProjection;
    uvec2 _renderTargetSize;
    float _ipd { 0.064f };

    struct FrameInfo {
        mat4 renderPose;
        mat4 presentPose;
        double sensorSampleTime { 0 };
        double predictedDisplayTime { 0 };
        mat3 presentReprojection;
    };

    QMap<uint32_t, FrameInfo> _frameInfos;
    FrameInfo _currentPresentFrameInfo;
    FrameInfo _currentRenderFrameInfo;

    bool _disablePreview{ true };
private:
    ivec4 getViewportForSourceSize(const uvec2& size) const;
    float getLeftCenterPixel() const;

    bool _disablePreviewItemAdded { false };
    bool _monoPreview { true };
    bool _clearPreviewFlag { false };
    gpu::TexturePointer _previewTexture;
    glm::vec2 _lastWindowSize;

    struct OverlayRenderer {
        gpu::Stream::FormatPointer format;
        gpu::BufferPointer vertices;
        gpu::BufferPointer indices;
        uint32_t indexCount { 0 };
        gpu::PipelinePointer pipeline;
        int32_t uniformsLocation { -1 };

        // FIXME this is stupid, use the built in transformation pipeline
        std::array<gpu::BufferPointer, 2> uniformBuffers;
        std::array<mat4, 2> mvps;

        struct Uniforms {
            mat4 mvp;
            vec4 glowPoints { -1 };
            vec4 glowColors[2];
            vec2 resolution { CompositorHelper::VIRTUAL_SCREEN_SIZE };
            float radius { 0.005f };
            float alpha { 1.0f };
        } uniforms;
        
        struct Vertex {
            vec3 pos;
            vec2 uv;
        } vertex;

        static const size_t VERTEX_OFFSET { offsetof(Vertex, pos) };
        static const size_t TEXTURE_OFFSET { offsetof(Vertex, uv) };
        static const int VERTEX_STRIDE { sizeof(Vertex) };

        void build();
        void updatePipeline();
        void render(HmdDisplayPlugin& plugin);
    } _overlayRenderer;
};
