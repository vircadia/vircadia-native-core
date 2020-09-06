//
//  Backend.cpp
//  interface/src/gpu
//
//  Created by Olivier Prat on 05/25/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Backend.h"

using namespace gpu;

// Counters for Buffer and Texture usage in GPU/Context

ContextMetricSize Backend::freeGPUMemSize;

ContextMetricCount Backend::bufferCount;
ContextMetricSize Backend::bufferGPUMemSize;

ContextMetricCount Backend::textureResidentCount;
ContextMetricCount Backend::textureFramebufferCount;
ContextMetricCount Backend::textureResourceCount;
ContextMetricCount Backend::textureExternalCount;

ContextMetricSize Backend::textureResidentGPUMemSize;
ContextMetricSize Backend::textureFramebufferGPUMemSize;
ContextMetricSize Backend::textureResourceGPUMemSize;
ContextMetricSize Backend::textureExternalGPUMemSize;

ContextMetricCount Backend::texturePendingGPUTransferCount;
ContextMetricSize Backend::texturePendingGPUTransferMemSize;

ContextMetricSize Backend::textureResourcePopulatedGPUMemSize;
ContextMetricSize Backend::textureResourceIdealGPUMemSize;

Backend::TransformCamera Backend::TransformCamera::getEyeCamera(int eye,
                                                                const StereoState& _stereo,
                                                                const Transform& view,
                                                                const Transform& previousView,
                                                                Vec2 normalizedJitter) const {
    TransformCamera result = *this;
    Transform eyeView = view;
    Transform eyePreviousView = previousView;
    if (!_stereo._skybox) {
        eyeView.postTranslate(-Vec3(_stereo._eyeViews[eye][3]));
        eyePreviousView.postTranslate(-Vec3(_stereo._eyeViews[eye][3]));
    } else {
        // FIXME: If "skybox" the ipd is set to 0 for now, let s try to propose a better solution for this in the future
        eyePreviousView.setTranslation(vec3());
    }
    result._projection = _stereo._eyeProjections[eye];

    // Apply jitter to projections
    normalizedJitter.x *= 2.0f;
    result._projection[2][0] += normalizedJitter.x;
    result._projection[2][1] += normalizedJitter.y;

    result.recomputeDerived(eyeView, eyePreviousView);

    result._stereoInfo = Vec4(1.0f, (float)eye, 1.0f / result._viewport.z, 1.0f / result._viewport.w);

    return result;
}

Backend::TransformCamera Backend::TransformCamera::getMonoCamera(bool isSkybox,
                                                                 const Transform& view,
                                                                 Transform previousView,
                                                                 Vec2 normalizedJitter) const {
    TransformCamera result = *this;

    if (isSkybox) {
        previousView.setTranslation(vec3());
    }
    result._projection[2][0] += normalizedJitter.x;
    result._projection[2][1] += normalizedJitter.y;
    result.recomputeDerived(view, previousView);

    result._stereoInfo = Vec4(0.0f, 0.0f, 1.0f / result._viewport.z, 1.0f / result._viewport.w);
    return result;
}

const Backend::TransformCamera& Backend::TransformCamera::recomputeDerived(const Transform& view,
                                                                           const Transform& previousView) const {
    _projectionInverse = glm::inverse(_projection);

    // Get the viewEyeToWorld matrix form the transformView as passed to the gpu::Batch
    // this is the "_viewInverse" fed to the shader
    // Genetrate the "_view" matrix as well from the xform
    view.getMatrix(_viewInverse);
    _view = glm::inverse(_viewInverse);
    previousView.getMatrix(_previousViewInverse);
    _previousView = glm::inverse(_previousViewInverse);

    Mat4 viewUntranslated = _view;
    viewUntranslated[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    _projectionViewUntranslated = _projection * viewUntranslated;

    viewUntranslated = _previousView;
    viewUntranslated[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    // We suppose that the projection, including jitter, hasn't changed from previous frame
    // This may sound counter-intuitive to use the same jitter value but in fact it is needed.
    // Think of it this way:
    // If we have no velocity (no camera or object movement) then we wish to reproject to exactly
    // the same pixel value. The current pixel UV when computing the motion vector is trivially computed
    // from the fragment coordinates but the previous UV is computed conceptually with an unjittered
    // projection * previous view matrix in the vertex shader and sent to the fragment to write in the
    // motion vector as an interpolated attribute. But since the current projection * view matrix is
    // jittered, the effectively rasterized interpolated previous UV will be slightly offset. That offset
    // is exactly the jitter amount. So we add that jitter amount to the projection in the projection * previous
    // view matrix computation. Hope this makes sense.
    _previousProjectionViewUntranslated = _projection * viewUntranslated;

    _stereoInfo = Vec4(0.0f);

    return *this;
}
