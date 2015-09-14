//
//  Context.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Context.h"

using namespace gpu;

Context::CreateBackend Context::_createBackendCallback = nullptr;
Context::MakeProgram Context::_makeProgramCallback = nullptr;
std::once_flag Context::_initialized;

Context::Context() {
    if (_createBackendCallback) {
        _backend.reset(_createBackendCallback());
    }
}

Context::Context(const Context& context) {
}

Context::~Context() {
}

bool Context::makeProgram(Shader& shader, const Shader::BindingSet& bindings) {
    if (shader.isProgram() && _makeProgramCallback) {
        return _makeProgramCallback(shader, bindings);
    }
    return false;
}

void Context::render(Batch& batch) {
    PROFILE_RANGE(__FUNCTION__);
    _backend->render(batch);
}

void Context::enableStereo(bool enable) {
    _backend->enableStereo(enable);
}

bool Context::isStereo() {
    return _backend->isStereo();
}

void Context::setStereoProjections(const mat4 eyeProjections[2]) {
    _backend->setStereoProjections(eyeProjections);
}

void Context::setStereoViews(const mat4 eyeViews[2]) {
    _backend->setStereoViews(eyeViews);
}

void Context::syncCache() {
    PROFILE_RANGE(__FUNCTION__);
    _backend->syncCache();
}

void Context::downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) {
    _backend->downloadFramebuffer(srcFramebuffer, region, destImage);
}

const Backend::TransformCamera& Backend::TransformCamera::recomputeDerived() const {
    _projectionInverse = glm::inverse(_projection);
    _viewInverse = glm::inverse(_view);

    Mat4 viewUntranslated = _view;
    viewUntranslated[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    _projectionViewUntranslated = _projection * viewUntranslated;
    return *this;
}

Backend::TransformCamera Backend::TransformCamera::getEyeCamera(int eye, const StereoState& _stereo) const {
    TransformCamera result = *this;
    if (!_stereo._skybox) {
        result._view = _stereo._eyeViews[eye] * result._view;
    } else {
        glm::mat4 skyboxView = _stereo._eyeViews[eye];
        skyboxView[3] = vec4(0, 0, 0, 1);
        result._view = skyboxView * result._view;
    }
    result._projection = _stereo._eyeProjections[eye];
    result.recomputeDerived();
    return result;
}
