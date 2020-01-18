//
//  State.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "State.h"
#include <QDebug>

using namespace gpu;

State::State() {
}

State::~State() {
}

// WARNING: GLBackend::GLState::_resetStateCommands heavily relies on the fact that State::DEFAULT = State::Data()
// Please make sure to go check makeResetStateCommands() before modifying this value
const State::Data State::DEFAULT = State::Data();

State::Signature State::evalSignature(const Data& state) {
    Signature signature(0);

    if (state.fillMode != State::DEFAULT.fillMode) {
        signature.set(State::FILL_MODE);
    }
    if (state.cullMode != State::DEFAULT.cullMode) {
        signature.set(State::CULL_MODE);
    }
    if (state.flags.frontFaceClockwise != State::DEFAULT.flags.frontFaceClockwise) {
        signature.set(State::FRONT_FACE_CLOCKWISE);
    }
    if (state.flags.depthClampEnable != State::DEFAULT.flags.depthClampEnable) {
        signature.set(State::DEPTH_CLAMP_ENABLE);
    }
    if (state.flags.scissorEnable != State::DEFAULT.flags.scissorEnable) {
        signature.set(State::SCISSOR_ENABLE);
    }
    if (state.flags.multisampleEnable != State::DEFAULT.flags.multisampleEnable) {
        signature.set(State::MULTISAMPLE_ENABLE);
    }
    if (state.flags.antialisedLineEnable != State::DEFAULT.flags.antialisedLineEnable) {
        signature.set(State::ANTIALISED_LINE_ENABLE);
    }
    if (state.depthBias != State::DEFAULT.depthBias) {
        signature.set(State::DEPTH_BIAS);
    }
    if (state.depthBiasSlopeScale != State::DEFAULT.depthBiasSlopeScale) {
        signature.set(State::DEPTH_BIAS_SLOPE_SCALE);
    }
    if (state.depthTest != State::DEFAULT.depthTest) {
        signature.set(State::DEPTH_TEST); 
    }
    if (state.stencilActivation != State::DEFAULT.stencilActivation) {
        signature.set(State::STENCIL_ACTIVATION);
    }
    if (state.stencilTestFront != State::DEFAULT.stencilTestFront) {
        signature.set(State::STENCIL_TEST_FRONT);
    }
    if (state.stencilTestBack != State::DEFAULT.stencilTestBack) {
        signature.set(State::STENCIL_TEST_BACK);
    }
    if (state.sampleMask != State::DEFAULT.sampleMask) {
        signature.set(State::SAMPLE_MASK);
    }
    if (state.flags.alphaToCoverageEnable != State::DEFAULT.flags.alphaToCoverageEnable) {
        signature.set(State::ALPHA_TO_COVERAGE_ENABLE); 
    }
    if (state.blendFunction != State::DEFAULT.blendFunction) {
        signature.set(State::BLEND_FUNCTION);
    }
    if (state.colorWriteMask != State::DEFAULT.colorWriteMask) {
        signature.set(State::COLOR_WRITE_MASK);
    }

    return signature;
}

State::State(const Data& values) :
    _values(values) {
    _signature = evalSignature(_values);
}


template <typename T>
static std::string hex(T t) {
    std::stringstream stream;
    stream << std::hex << t;
    return stream.str();
}
