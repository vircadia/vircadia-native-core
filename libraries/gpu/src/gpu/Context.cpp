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

void Context::syncCache() {
    PROFILE_RANGE(__FUNCTION__);
    _backend->syncCache();
}

void Context::downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) {
    _backend->downloadFramebuffer(srcFramebuffer, region, destImage);
}

