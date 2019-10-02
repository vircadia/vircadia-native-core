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
    Q_OBJECT
    using Parent = OpenGLDisplayPlugin;
public:
    ~HmdDisplayPlugin();
    bool isHmd() const override final { return true; }
    float getIPD() const override final { return _ipd; }
    glm::mat4 getEyeToHeadTransform(Eye eye) const override final;
    glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const override;
    glm::mat4 getCullingProjection(const glm::mat4& baseProjection) const override;
    glm::uvec2 getRecommendedUiSize() const override final;
    glm::uvec2 getRecommendedRenderSize() const override final { return _renderTargetSize; }
    bool isDisplayVisible() const override { return isHmdMounted(); }

    ivec4 eyeViewport(Eye eye) const;

    QRect getRecommendedHUDRect() const override final;

    virtual glm::mat4 getHeadPose() const override;

    bool wantVsync() const override {
        return false;
    }

    float stutterRate() const override;

    virtual bool onDisplayTextureReset() override { _clearPreviewFlag = true; return true; };

    void pluginUpdate() override {};

    std::function<void(gpu::Batch&, const gpu::TexturePointer&)> getHUDOperator() override;
    virtual StencilMaskMode getStencilMaskMode() const override { return StencilMaskMode::PAINT; }
    void updateVisionSqueezeParameters(float visionSqueezeX, float visionSqueezeY, float visionSqueezeTransition,
                                       int visionSqueezePerEye, float visionSqueezeGroundPlaneY,
                                       float visionSqueezeSpotlightSize);
    // Attempt to reserve two threads.
    int getRequiredThreadCount() const override { return 2; }

signals:
    void hmdMountedChanged();
    void hmdVisibleChanged(bool visible);

protected:
    virtual void hmdPresent() = 0;
    virtual bool isHmdMounted() const = 0;
    virtual void postPreview() {};
    virtual void updatePresentPose();

    bool internalActivate() override;
    void internalDeactivate() override;
    void compositePointer() override;
    void internalPresent() override;
    void customizeContext() override;
    void uncustomizeContext() override;
    void updateFrameData() override;
    glm::mat4 getViewCorrection() override;

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
    };

    QMap<uint32_t, FrameInfo> _frameInfos;
    FrameInfo _currentPresentFrameInfo;
    FrameInfo _currentRenderFrameInfo;
    RateCounter<> _stutterRate;

    bool _disablePreview { true };

    class VisionSqueezeParameters {
    public:
        float _visionSqueezeX { 0.0f };
        float _visionSqueezeY { 0.0f };
        float _spareA { 0.0f };
        float _spareB { 0.0f };
        glm::mat4 _leftProjection;
        glm::mat4 _rightProjection;
        glm::mat4 _hmdSensorMatrix;
        float _visionSqueezeTransition { 0.15f };
        int _visionSqueezePerEye { 0 };
        float _visionSqueezeGroundPlaneY { 0.0f };
        float _visionSqueezeSpotlightSize { 0.0f };

        VisionSqueezeParameters() {}
    };
    typedef gpu::BufferView UniformBufferView;
    gpu::BufferView _visionSqueezeParametersBuffer;

    virtual void setupCompositeScenePipeline(gpu::Batch& batch) override;

    float _visionSqueezeDeviceLowX { 0.0f };
    float _visionSqueezeDeviceHighX { 1.0f };
    float _visionSqueezeDeviceLowY { 0.0f };
    float _visionSqueezeDeviceHighY { 1.0f };

private:
    ivec4 getViewportForSourceSize(const uvec2& size) const;
    float getLeftCenterPixel() const;

    bool _monoPreview { true };
    bool _clearPreviewFlag { false };
    gpu::TexturePointer _previewTexture;
    glm::vec2 _lastWindowSize;

    struct HUDRenderer {
        gpu::Stream::FormatPointer format;
        gpu::BufferPointer vertices;
        gpu::BufferPointer indices;
        uint32_t indexCount { 0 };
        gpu::PipelinePointer pipeline { nullptr };

        gpu::BufferPointer uniformsBuffer;

        struct Uniforms {
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
        std::function<void(gpu::Batch&, const gpu::TexturePointer&)> render();
    } _hudRenderer;
};

const int drawTextureWithVisionSqueezeParamsSlot = 1; // must match binding in DrawTextureWithVisionSqueeze.slf
