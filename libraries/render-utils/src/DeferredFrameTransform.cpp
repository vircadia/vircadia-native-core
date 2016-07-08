//
//  DeferredFrameTransform.cpp
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DeferredFrameTransform.h"

#include "gpu/Context.h"
#include "render/Engine.h"

DeferredFrameTransform::DeferredFrameTransform() {
    FrameTransform frameTransform;
    _frameTransformBuffer = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(FrameTransform), (const gpu::Byte*) &frameTransform));
}

void DeferredFrameTransform::update(RenderArgs* args) {

    // Update the depth info with near and far (same for stereo)
    auto nearZ = args->getViewFrustum().getNearClip();
    auto farZ = args->getViewFrustum().getFarClip();

    auto& frameTransformBuffer = _frameTransformBuffer.edit<FrameTransform>();
    frameTransformBuffer.depthInfo = glm::vec4(nearZ*farZ, farZ - nearZ, -farZ, 0.0f);

    frameTransformBuffer.pixelInfo = args->_viewport;

    //_parametersBuffer.edit<Parameters>()._ditheringInfo.y += 0.25f;

    Transform cameraTransform;
    args->getViewFrustum().evalViewTransform(cameraTransform);
    cameraTransform.getMatrix(frameTransformBuffer.invView);
    cameraTransform.getInverseMatrix(frameTransformBuffer.view);

    args->getViewFrustum().evalProjectionMatrix(frameTransformBuffer.projectionMono);

    // Running in stero ?
    bool isStereo = args->_context->isStereo();
    if (!isStereo) {
        frameTransformBuffer.projection[0] = frameTransformBuffer.projectionMono;
        frameTransformBuffer.stereoInfo = glm::vec4(0.0f, (float)args->_viewport.z, 0.0f, 0.0f);
        frameTransformBuffer.invpixelInfo = glm::vec4(1.0f / args->_viewport.z, 1.0f / args->_viewport.w, 0.0f, 0.0f);
    } else {

        mat4 projMats[2];
        mat4 eyeViews[2];
        args->_context->getStereoProjections(projMats);
        args->_context->getStereoViews(eyeViews);

        for (int i = 0; i < 2; i++) {
            // Compose the mono Eye space to Stereo clip space Projection Matrix
            auto sideViewMat = projMats[i] * eyeViews[i];
            frameTransformBuffer.projection[i] = sideViewMat;
        }

        frameTransformBuffer.stereoInfo = glm::vec4(1.0f, (float)(args->_viewport.z >> 1), 0.0f, 1.0f);
        frameTransformBuffer.invpixelInfo = glm::vec4(1.0f / (float)(args->_viewport.z >> 1), 1.0f / args->_viewport.w, 0.0f, 0.0f);

    }
}

void GenerateDeferredFrameTransform::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, DeferredFrameTransformPointer& frameTransform) {
    if (!frameTransform) {
        frameTransform = std::make_shared<DeferredFrameTransform>();
    }
    frameTransform->update(renderContext->args);
}
