//
//  GLBackendState.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/22/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackendShared.h"

#include "Format.h"

using namespace gpu;

GLBackend::GLState::GLState()
{}

GLBackend::GLState::~GLState() {
}


typedef GLBackend::GLState::Command Command;
typedef GLBackend::GLState::CommandPointer CommandPointer;
typedef GLBackend::GLState::Command1<uint32> Command1U;
typedef GLBackend::GLState::Command1<int32> Command1I;
typedef GLBackend::GLState::Command1<bool> Command1B;
typedef GLBackend::GLState::Command1<Vec2> CommandDepthBias;
typedef GLBackend::GLState::Command1<State::DepthTest> CommandDepthTest;
typedef GLBackend::GLState::Command3<State::StencilActivation, State::StencilTest, State::StencilTest> CommandStencil;
typedef GLBackend::GLState::Command1<State::BlendFunction> CommandBlend;

// The state commands to reset to default,
// WARNING depending on the order of the State::Field enum
const GLBackend::GLState::Commands GLBackend::GLState::_resetStateCommands = {
    CommandPointer(new Command1I(&GLBackend::do_setStateFillMode, State::DEFAULT.fillMode)),
    CommandPointer(new Command1I(&GLBackend::do_setStateCullMode, State::DEFAULT.cullMode)),
    CommandPointer(new Command1B(&GLBackend::do_setStateFrontFaceClockwise, State::DEFAULT.frontFaceClockwise)),
    CommandPointer(new Command1B(&GLBackend::do_setStateDepthClipEnable, State::DEFAULT.depthClipEnable)),
    CommandPointer(new Command1B(&GLBackend::do_setStateScissorEnable, State::DEFAULT.scissorEnable)),
    CommandPointer(new Command1B(&GLBackend::do_setStateMultisampleEnable, State::DEFAULT.multisampleEnable)),
    CommandPointer(new Command1B(&GLBackend::do_setStateAntialiasedLineEnable, State::DEFAULT.antialisedLineEnable)),

    // Depth bias has 2 fields in State but really one call in GLBackend
    CommandPointer(new CommandDepthBias(&GLBackend::do_setStateDepthBias, Vec2(State::DEFAULT.depthBias, State::DEFAULT.depthBiasSlopeScale))),
    CommandPointer(new CommandDepthBias(&GLBackend::do_setStateDepthBias, Vec2(State::DEFAULT.depthBias, State::DEFAULT.depthBiasSlopeScale))),
 
    CommandPointer(new CommandDepthTest(&GLBackend::do_setStateDepthTest, State::DEFAULT.depthTest)),

    // Depth bias has 3 fields in State but really one call in GLBackend
    CommandPointer(new CommandStencil(&GLBackend::do_setStateStencil, State::DEFAULT.stencilActivation, State::DEFAULT.stencilTestFront, State::DEFAULT.stencilTestBack)),
    CommandPointer(new CommandStencil(&GLBackend::do_setStateStencil, State::DEFAULT.stencilActivation, State::DEFAULT.stencilTestFront, State::DEFAULT.stencilTestBack)),
    CommandPointer(new CommandStencil(&GLBackend::do_setStateStencil, State::DEFAULT.stencilActivation, State::DEFAULT.stencilTestFront, State::DEFAULT.stencilTestBack)),

    CommandPointer(new Command1B(&GLBackend::do_setStateAlphaToCoverageEnable, State::DEFAULT.alphaToCoverageEnable)),

    CommandPointer(new Command1U(&GLBackend::do_setStateSampleMask, State::DEFAULT.sampleMask)),

    CommandPointer(new CommandBlend(&GLBackend::do_setStateBlend, State::DEFAULT.blendFunction)),

    CommandPointer(new Command1U(&GLBackend::do_setStateColorWriteMask, State::DEFAULT.colorWriteMask))
};

void generateFillMode(GLBackend::GLState::Commands& commands, State::FillMode fillMode) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateFillMode, int32(fillMode))));
}

void generateCullMode(GLBackend::GLState::Commands& commands, State::CullMode cullMode) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateCullMode, int32(cullMode))));
}

void generateFrontFaceClockwise(GLBackend::GLState::Commands& commands, bool isClockwise) {
    commands.push_back(CommandPointer(new Command1B(&GLBackend::do_setStateFrontFaceClockwise, isClockwise)));
}

void generateDepthClipEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1B(&GLBackend::do_setStateDepthClipEnable, enable)));
}

void generateScissorEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1B(&GLBackend::do_setStateScissorEnable, enable)));
}

void generateMultisampleEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1B(&GLBackend::do_setStateMultisampleEnable, enable)));
}

void generateAntialiasedLineEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1B(&GLBackend::do_setStateAntialiasedLineEnable, enable)));
}

void generateDepthBias(GLBackend::GLState::Commands& commands, const State& state) {
     commands.push_back(CommandPointer(new CommandDepthBias(&GLBackend::do_setStateDepthBias, Vec2(state.getDepthBias(), state.getDepthBiasSlopeScale()))));
}

void generateDepthTest(GLBackend::GLState::Commands& commands, State::DepthTest& test) {
    commands.push_back(CommandPointer(new CommandDepthTest(&GLBackend::do_setStateDepthTest, int32(test.getRaw()))));
}

void generateStencil(GLBackend::GLState::Commands& commands, const State& state) {
    commands.push_back(CommandPointer(new CommandStencil(&GLBackend::do_setStateStencil, state.getStencilActivation(), state.getStencilTestFront(), state.getStencilTestBack())));
}

void generateAlphaToCoverageEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1B(&GLBackend::do_setStateAlphaToCoverageEnable, enable)));
}

void generateSampleMask(GLBackend::GLState::Commands& commands, uint32 mask) {
    commands.push_back(CommandPointer(new Command1U(&GLBackend::do_setStateSampleMask, mask)));
}

void generateBlend(GLBackend::GLState::Commands& commands, const State& state) {
    commands.push_back(CommandPointer(new CommandBlend(&GLBackend::do_setStateBlend, state.getBlendFunction())));
}

void generateColorWriteMask(GLBackend::GLState::Commands& commands, uint32 mask) {
    commands.push_back(CommandPointer(new Command1U(&GLBackend::do_setStateColorWriteMask, mask)));
}

GLBackend::GLState* GLBackend::syncGPUObject(const State& state) {
    GLState* object = Backend::getGPUObject<GLBackend::GLState>(state);

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
    bool blendState = false;

    // go thorugh the list of state fields in the State and record the corresponding gl command
    for (auto field: state.getFields()) {
        switch(field.first) {
        case State::FILL_MODE: {
            generateFillMode(object->_commands, State::FillMode(field.second._integer));
            break;
        }
        case State::CULL_MODE: {
            generateCullMode(object->_commands, State::CullMode(field.second._integer));
            break;
        }
        case State::DEPTH_BIAS:
        case State::DEPTH_BIAS_SLOPE_SCALE: {
            depthBias = true;
            break;
        }
        case State::FRONT_FACE_CLOCKWISE: {
            generateFrontFaceClockwise(object->_commands, bool(field.second._integer));
            break;
        }
        case State::DEPTH_CLIP_ENABLE: {
            generateDepthClipEnable(object->_commands, bool(field.second._integer));
            break;
        }
        case State::SCISSOR_ENABLE: {
            generateScissorEnable(object->_commands, bool(field.second._integer));
            break;
        }
        case State::MULTISAMPLE_ENABLE: {
            generateMultisampleEnable(object->_commands, bool(field.second._integer));
            break;
        }
        case State::ANTIALISED_LINE_ENABLE: {
            generateAntialiasedLineEnable(object->_commands, bool(field.second._integer));
            break;
        }
        case State::DEPTH_TEST: {
            generateDepthTest(object->_commands, State::DepthTest(field.second._integer));
            break;
        }

        case State::STENCIL_ACTIVATION:
        case State::STENCIL_TEST_FRONT:
        case State::STENCIL_TEST_BACK: {
            stencilState = true;
            break;
        }

        case State::SAMPLE_MASK: {
            generateSampleMask(object->_commands, (field.second._unsigned_integer));
            break;
        }
        case State::ALPHA_TO_COVERAGE_ENABLE: {
            generateAlphaToCoverageEnable(object->_commands, bool(field.second._integer));
            break;
        }

        case State::BLEND_FUNCTION: {
            generateBlend(object->_commands, state);
            break;
        }

        case State::COLOR_WRITE_MASK: {
            generateColorWriteMask(object->_commands, field.second._integer);
            break;
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




void GLBackend::resetPipelineState(State::Signature nextSignature) {
    auto currentNotSignature = ~_pipeline._stateSignatureCache; 
    auto nextNotSignature = ~nextSignature;
    auto fieldsToBeReset = currentNotSignature ^ (currentNotSignature | nextNotSignature);
    if (fieldsToBeReset.any()) {
        for (auto i = 0; i < State::NUM_FIELDS; i++) {
            if (fieldsToBeReset[i]) {
                GLState::_resetStateCommands[i]->run(this);
                _pipeline._stateSignatureCache.reset(i);
            }
        }
    }
}

static GLenum GL_COMPARISON_FUNCTIONS[] = {
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS };

void GLBackend::do_setStateFillMode(int32 mode) {
    if (_pipeline._stateCache.fillMode != mode) {
        static GLenum GL_FILL_MODES[] = { GL_POINT, GL_LINE, GL_FILL };
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL_MODES[mode]);
        CHECK_GL_ERROR();

        _pipeline._stateCache.fillMode = State::FillMode(mode);
    }
}

void GLBackend::do_setStateCullMode(int32 mode) {
    if (_pipeline._stateCache.cullMode != mode) {
        static GLenum GL_CULL_MODES[] = { GL_FRONT_AND_BACK, GL_FRONT, GL_BACK };
        if (mode == State::CULL_NONE) {
            glDisable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_CULL_MODES[mode]);
        }
        CHECK_GL_ERROR();

        _pipeline._stateCache.cullMode = State::CullMode(mode);
    }
}

void GLBackend::do_setStateFrontFaceClockwise(bool isClockwise) {
    if (_pipeline._stateCache.frontFaceClockwise != isClockwise) {
        static GLenum  GL_FRONT_FACES[] = { GL_CCW, GL_CW };
        glFrontFace(GL_FRONT_FACES[isClockwise]);
        CHECK_GL_ERROR();
    
        _pipeline._stateCache.frontFaceClockwise = isClockwise;
    }
}

void GLBackend::do_setStateDepthClipEnable(bool enable) {
    if (_pipeline._stateCache.depthClipEnable != enable) {
        if (enable) {
            glEnable(GL_DEPTH_CLAMP);
        } else {
            glDisable(GL_DEPTH_CLAMP);
        }
        CHECK_GL_ERROR();

        _pipeline._stateCache.depthClipEnable = enable;
    }
}

void GLBackend::do_setStateScissorEnable(bool enable) {
    if (_pipeline._stateCache.scissorEnable != enable) {
        if (enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        CHECK_GL_ERROR();

        _pipeline._stateCache.scissorEnable = enable;
    }
}

void GLBackend::do_setStateMultisampleEnable(bool enable) {
    if (_pipeline._stateCache.multisampleEnable != enable) {
        if (enable) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
        CHECK_GL_ERROR();

        _pipeline._stateCache.multisampleEnable = enable;
    }
}

void GLBackend::do_setStateAntialiasedLineEnable(bool enable) {
    if (_pipeline._stateCache.antialisedLineEnable != enable) {
        if (enable) {
            glEnable(GL_POINT_SMOOTH);
            glEnable(GL_LINE_SMOOTH);
        } else {
            glDisable(GL_POINT_SMOOTH);
            glDisable(GL_LINE_SMOOTH);
        }
        CHECK_GL_ERROR();

        _pipeline._stateCache.antialisedLineEnable = enable;
    }
}

void GLBackend::do_setStateDepthBias(Vec2 bias) {
    if ( (bias.x != _pipeline._stateCache.depthBias) || (bias.y != _pipeline._stateCache.depthBiasSlopeScale)) {
        if ((bias.x != 0.f) || (bias.y != 0.f)) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glEnable(GL_POLYGON_OFFSET_POINT);
            glPolygonOffset(bias.x, bias.y);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
            glDisable(GL_POLYGON_OFFSET_POINT);
        }
         _pipeline._stateCache.depthBias = bias.x;
         _pipeline._stateCache.depthBiasSlopeScale = bias.y;
    }
}

void GLBackend::do_setStateDepthTest(State::DepthTest test) {
    if (_pipeline._stateCache.depthTest != test) {
        if (test.isEnabled()) {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(test.getWriteMask());
            glDepthFunc(GL_COMPARISON_FUNCTIONS[test.getFunction()]);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        CHECK_GL_ERROR();

         _pipeline._stateCache.depthTest = test;
    }
}

void GLBackend::do_setStateStencil(State::StencilActivation activation, State::StencilTest frontTest, State::StencilTest backTest) {
    
    if ((_pipeline._stateCache.stencilActivation != activation)
        || (_pipeline._stateCache.stencilTestFront != frontTest)
        || (_pipeline._stateCache.stencilTestBack != backTest)) {

        if (activation.isEnabled()) {
            glEnable(GL_STENCIL_TEST);
            glStencilMaskSeparate(GL_FRONT, activation.getWriteMaskFront());
            glStencilMaskSeparate(GL_BACK, activation.getWriteMaskBack());

            static GLenum STENCIL_OPS[] = {
                GL_KEEP,
                GL_ZERO,
                GL_REPLACE,
                GL_INCR_WRAP,
                GL_DECR_WRAP,
                GL_INVERT,
                GL_INCR,
                GL_DECR };

            glStencilFuncSeparate(GL_FRONT, STENCIL_OPS[frontTest.getFailOp()], STENCIL_OPS[frontTest.getPassOp()], STENCIL_OPS[frontTest.getDepthFailOp()]);
            glStencilFuncSeparate(GL_FRONT, GL_COMPARISON_FUNCTIONS[frontTest.getFunction()], frontTest.getReference(), frontTest.getReadMask());

            glStencilFuncSeparate(GL_BACK, STENCIL_OPS[backTest.getFailOp()], STENCIL_OPS[backTest.getPassOp()], STENCIL_OPS[backTest.getDepthFailOp()]);
            glStencilFuncSeparate(GL_BACK, GL_COMPARISON_FUNCTIONS[backTest.getFunction()], backTest.getReference(), backTest.getReadMask());
        } else {
            glDisable(GL_STENCIL_TEST);
        }
        CHECK_GL_ERROR();

        _pipeline._stateCache.stencilActivation = activation;
        _pipeline._stateCache.stencilTestFront = frontTest;
        _pipeline._stateCache.stencilTestBack = backTest;
    }
}

void GLBackend::do_setStateAlphaToCoverageEnable(bool enable) {
    if (_pipeline._stateCache.alphaToCoverageEnable != enable) {
        if (enable) {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        } else {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
        CHECK_GL_ERROR();
        _pipeline._stateCache.alphaToCoverageEnable = enable;
    }
}

void GLBackend::do_setStateSampleMask(uint32 mask) {
    if (_pipeline._stateCache.sampleMask != mask) {
    // TODO
        _pipeline._stateCache.sampleMask = mask;
    }
}

void GLBackend::do_setStateBlend(State::BlendFunction function) {
    if (_pipeline._stateCache.blendFunction != function) {
        if (function.isEnabled()) {
            glEnable(GL_BLEND);

            static GLenum GL_BLEND_OPS[] = {
                GL_FUNC_ADD,
                GL_FUNC_SUBTRACT,
                GL_FUNC_REVERSE_SUBTRACT,
                GL_MIN,
                GL_MAX };

            glBlendEquationSeparate(GL_BLEND_OPS[function.getOperationColor()], GL_BLEND_OPS[function.getOperationAlpha()]);
            CHECK_GL_ERROR();

            static GLenum BLEND_ARGS[] = {
                GL_ZERO,
                GL_ONE,
                GL_SRC_COLOR,
                GL_ONE_MINUS_SRC_COLOR,
                GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA,
                GL_DST_ALPHA,
                GL_ONE_MINUS_DST_ALPHA,
                GL_DST_COLOR,
                GL_ONE_MINUS_DST_COLOR,
                GL_SRC_ALPHA_SATURATE,
                GL_CONSTANT_COLOR,
                GL_ONE_MINUS_CONSTANT_COLOR,
                GL_CONSTANT_ALPHA,
                GL_ONE_MINUS_CONSTANT_ALPHA,
            };

            glBlendFuncSeparate(BLEND_ARGS[function.getSourceColor()], BLEND_ARGS[function.getDestinationColor()],
                                BLEND_ARGS[function.getSourceAlpha()], BLEND_ARGS[function.getDestinationAlpha()]);
            CHECK_GL_ERROR();
        } else {
            glDisable(GL_BLEND);
        }

        _pipeline._stateCache.blendFunction = function;
    }
}

void GLBackend::do_setStateColorWriteMask(uint32 mask) {
    if (_pipeline._stateCache.colorWriteMask = mask) {
        glColorMask(mask & State::ColorMask::WRITE_RED,
                mask & State::ColorMask::WRITE_GREEN,
                mask & State::ColorMask::WRITE_BLUE,
                mask & State::ColorMask::WRITE_ALPHA );

        _pipeline._stateCache.colorWriteMask = mask;
    }
}


void GLBackend::do_setStateBlendFactor(Batch& batch, uint32 paramOffset) {
    
    Vec4 factor(batch._params[paramOffset + 0]._float,
                batch._params[paramOffset + 1]._float,
                batch._params[paramOffset + 2]._float,
                batch._params[paramOffset + 3]._float);

    glBlendColor(factor.x, factor.y, factor.z, factor.w);
    CHECK_GL_ERROR();
}