//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLState.h"
#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;

typedef GLState::Command Command;
typedef GLState::CommandPointer CommandPointer;
typedef GLState::Command1<uint32> Command1U;
typedef GLState::Command1<int32> Command1I;
typedef GLState::Command1<bool> Command1B;
typedef GLState::Command1<Vec2> CommandDepthBias;
typedef GLState::Command1<State::DepthTest> CommandDepthTest;
typedef GLState::Command3<State::StencilActivation, State::StencilTest, State::StencilTest> CommandStencil;
typedef GLState::Command1<State::BlendFunction> CommandBlend;

const GLState::Commands makeResetStateCommands();

// NOTE: This must stay in sync with the ordering of the State::Field enum
const GLState::Commands makeResetStateCommands() {
    // Since State::DEFAULT is a static defined in another .cpp the initialisation order is random
    // and we have a 50/50 chance that State::DEFAULT is not yet initialized.
    // Since State::DEFAULT = State::Data() it is much easier to not use the actual State::DEFAULT
    // but another State::Data object with a default initialization.
    const State::Data DEFAULT = State::Data();

    auto depthBiasCommand = std::make_shared<CommandDepthBias>(&GLBackend::do_setStateDepthBias,
        Vec2(DEFAULT.depthBias, DEFAULT.depthBiasSlopeScale));
    auto stencilCommand = std::make_shared<CommandStencil>(&GLBackend::do_setStateStencil, DEFAULT.stencilActivation,
        DEFAULT.stencilTestFront, DEFAULT.stencilTestBack);

    // The state commands to reset to default,
    // WARNING depending on the order of the State::Field enum
    return {
        std::make_shared<Command1I>(&GLBackend::do_setStateFillMode, DEFAULT.fillMode),
        std::make_shared<Command1I>(&GLBackend::do_setStateCullMode, DEFAULT.cullMode),
        std::make_shared<Command1B>(&GLBackend::do_setStateFrontFaceClockwise, DEFAULT.frontFaceClockwise),
        std::make_shared<Command1B>(&GLBackend::do_setStateDepthClampEnable, DEFAULT.depthClampEnable),
        std::make_shared<Command1B>(&GLBackend::do_setStateScissorEnable, DEFAULT.scissorEnable),
        std::make_shared<Command1B>(&GLBackend::do_setStateMultisampleEnable, DEFAULT.multisampleEnable),
        std::make_shared<Command1B>(&GLBackend::do_setStateAntialiasedLineEnable, DEFAULT.antialisedLineEnable),

        // Depth bias has 2 fields in State but really one call in GLBackend
        CommandPointer(depthBiasCommand),
        CommandPointer(depthBiasCommand),

        std::make_shared<CommandDepthTest>(&GLBackend::do_setStateDepthTest, DEFAULT.depthTest),

        // Depth bias has 3 fields in State but really one call in GLBackend
        CommandPointer(stencilCommand),
        CommandPointer(stencilCommand),
        CommandPointer(stencilCommand),

        std::make_shared<Command1U>(&GLBackend::do_setStateSampleMask, DEFAULT.sampleMask),

        std::make_shared<Command1B>(&GLBackend::do_setStateAlphaToCoverageEnable, DEFAULT.alphaToCoverageEnable),

        std::make_shared<CommandBlend>(&GLBackend::do_setStateBlend, DEFAULT.blendFunction),

        std::make_shared<Command1U>(&GLBackend::do_setStateColorWriteMask, DEFAULT.colorWriteMask)
    };
}

const GLState::Commands GLState::_resetStateCommands = makeResetStateCommands();


void generateFillMode(GLState::Commands& commands, State::FillMode fillMode) {
    commands.push_back(std::make_shared<Command1I>(&GLBackend::do_setStateFillMode, int32(fillMode)));
}

void generateCullMode(GLState::Commands& commands, State::CullMode cullMode) {
    commands.push_back(std::make_shared<Command1I>(&GLBackend::do_setStateCullMode, int32(cullMode)));
}

void generateFrontFaceClockwise(GLState::Commands& commands, bool isClockwise) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateFrontFaceClockwise, isClockwise));
}

void generateDepthClampEnable(GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateDepthClampEnable, enable));
}

void generateScissorEnable(GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateScissorEnable, enable));
}

void generateMultisampleEnable(GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateMultisampleEnable, enable));
}

void generateAntialiasedLineEnable(GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateAntialiasedLineEnable, enable));
}

void generateDepthBias(GLState::Commands& commands, const State& state) {
    commands.push_back(std::make_shared<CommandDepthBias>(&GLBackend::do_setStateDepthBias, Vec2(state.getDepthBias(), state.getDepthBiasSlopeScale())));
}

void generateDepthTest(GLState::Commands& commands, const State::DepthTest& test) {
    commands.push_back(std::make_shared<CommandDepthTest>(&GLBackend::do_setStateDepthTest, int32(test.getRaw())));
}

void generateStencil(GLState::Commands& commands, const State& state) {
    commands.push_back(std::make_shared<CommandStencil>(&GLBackend::do_setStateStencil, state.getStencilActivation(), state.getStencilTestFront(), state.getStencilTestBack()));
}

void generateAlphaToCoverageEnable(GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateAlphaToCoverageEnable, enable));
}

void generateSampleMask(GLState::Commands& commands, uint32 mask) {
    commands.push_back(std::make_shared<Command1U>(&GLBackend::do_setStateSampleMask, mask));
}

void generateBlend(GLState::Commands& commands, const State& state) {
    commands.push_back(std::make_shared<CommandBlend>(&GLBackend::do_setStateBlend, state.getBlendFunction()));
}

void generateColorWriteMask(GLState::Commands& commands, uint32 mask) {
    commands.push_back(std::make_shared<Command1U>(&GLBackend::do_setStateColorWriteMask, mask));
}

GLState* GLState::sync(const State& state) {
    GLState* object = Backend::getGPUObject<GLState>(state);

    // If GPU object already created then good
    if (object) {
        return object;
    }

    // Else allocate and create the GLState
    if (!object) {
        object = new GLState();
        Backend::setGPUObject(state, object);
    }

    // here, we need to regenerate something so let's do it all
    object->_commands.clear();
    object->_stamp = state.getStamp();
    object->_signature = state.getSignature();

    bool depthBias = false;
    bool stencilState = false;

    // go thorugh the list of state fields in the State and record the corresponding gl command
    for (int i = 0; i < State::NUM_FIELDS; i++) {
        if (state.getSignature()[i]) {
            switch (i) {
            case State::FILL_MODE: {
                generateFillMode(object->_commands, state.getFillMode());
                break;
            }
            case State::CULL_MODE: {
                generateCullMode(object->_commands, state.getCullMode());
                break;
            }
            case State::DEPTH_BIAS:
            case State::DEPTH_BIAS_SLOPE_SCALE: {
                depthBias = true;
                break;
            }
            case State::FRONT_FACE_CLOCKWISE: {
                generateFrontFaceClockwise(object->_commands, state.isFrontFaceClockwise());
                break;
            }
            case State::DEPTH_CLAMP_ENABLE: {
                generateDepthClampEnable(object->_commands, state.isDepthClampEnable());
                break;
            }
            case State::SCISSOR_ENABLE: {
                generateScissorEnable(object->_commands, state.isScissorEnable());
                break;
            }
            case State::MULTISAMPLE_ENABLE: {
                generateMultisampleEnable(object->_commands, state.isMultisampleEnable());
                break;
            }
            case State::ANTIALISED_LINE_ENABLE: {
                generateAntialiasedLineEnable(object->_commands, state.isAntialiasedLineEnable());
                break;
            }
            case State::DEPTH_TEST: {
                generateDepthTest(object->_commands, state.getDepthTest());
                break;
            }

            case State::STENCIL_ACTIVATION:
            case State::STENCIL_TEST_FRONT:
            case State::STENCIL_TEST_BACK: {
                stencilState = true;
                break;
            }

            case State::SAMPLE_MASK: {
                generateSampleMask(object->_commands, state.getSampleMask());
                break;
            }
            case State::ALPHA_TO_COVERAGE_ENABLE: {
                generateAlphaToCoverageEnable(object->_commands, state.isAlphaToCoverageEnabled());
                break;
            }

            case State::BLEND_FUNCTION: {
                generateBlend(object->_commands, state);
                break;
            }

            case State::COLOR_WRITE_MASK: {
                generateColorWriteMask(object->_commands, state.getColorWriteMask());
                break;
            }
            }
        }
    }

    if (depthBias) {
        generateDepthBias(object->_commands, state);
    }

    if (stencilState) {
        generateStencil(object->_commands, state);
    }

    return object;
}

