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

// this include should disappear! as soon as the gpu::Context is in place
#include "GLBackend.h"

using namespace gpu;

Context::Context(Backend* backend) :
    _backend(backend) {
}

Context::Context(const Context& context) {
}

Context::~Context() {
}

bool Context::makeProgram(Shader& shader, const Shader::BindingSet& bindings) {
    if (shader.isProgram()) {
        return GLBackend::makeProgram(shader, bindings);
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