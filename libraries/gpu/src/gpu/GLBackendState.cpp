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

const GLBackend::GLState::Commands makeResetStateCommands();
const GLBackend::GLState::Commands GLBackend::GLState::_resetStateCommands = makeResetStateCommands();


const GLBackend::GLState::Commands makeResetStateCommands() {
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
        
        std::make_shared<Command1B>(&GLBackend::do_setStateAlphaToCoverageEnable, DEFAULT.alphaToCoverageEnable),
        
        std::make_shared<Command1U>(&GLBackend::do_setStateSampleMask, DEFAULT.sampleMask),
        
        std::make_shared<CommandBlend>(&GLBackend::do_setStateBlend, DEFAULT.blendFunction),
        
        std::make_shared<Command1U>(&GLBackend::do_setStateColorWriteMask, DEFAULT.colorWriteMask)
    };
}

void generateFillMode(GLBackend::GLState::Commands& commands, State::FillMode fillMode) {
    commands.push_back(std::make_shared<Command1I>(&GLBackend::do_setStateFillMode, int32(fillMode)));
}

void generateCullMode(GLBackend::GLState::Commands& commands, State::CullMode cullMode) {
    commands.push_back(std::make_shared<Command1I>(&GLBackend::do_setStateCullMode, int32(cullMode)));
}

void generateFrontFaceClockwise(GLBackend::GLState::Commands& commands, bool isClockwise) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateFrontFaceClockwise, isClockwise));
}

void generateDepthClampEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateDepthClampEnable, enable));
}

void generateScissorEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateScissorEnable, enable));
}

void generateMultisampleEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateMultisampleEnable, enable));
}

void generateAntialiasedLineEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateAntialiasedLineEnable, enable));
}

void generateDepthBias(GLBackend::GLState::Commands& commands, const State& state) {
     commands.push_back(std::make_shared<CommandDepthBias>(&GLBackend::do_setStateDepthBias, Vec2(state.getDepthBias(), state.getDepthBiasSlopeScale())));
}

void generateDepthTest(GLBackend::GLState::Commands& commands, const State::DepthTest& test) {
    commands.push_back(std::make_shared<CommandDepthTest>(&GLBackend::do_setStateDepthTest, int32(test.getRaw())));
}

void generateStencil(GLBackend::GLState::Commands& commands, const State& state) {
    commands.push_back(std::make_shared<CommandStencil>(&GLBackend::do_setStateStencil, state.getStencilActivation(), state.getStencilTestFront(), state.getStencilTestBack()));
}

void generateAlphaToCoverageEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(std::make_shared<Command1B>(&GLBackend::do_setStateAlphaToCoverageEnable, enable));
}

void generateSampleMask(GLBackend::GLState::Commands& commands, uint32 mask) {
    commands.push_back(std::make_shared<Command1U>(&GLBackend::do_setStateSampleMask, mask));
}

void generateBlend(GLBackend::GLState::Commands& commands, const State& state) {
    commands.push_back(std::make_shared<CommandBlend>(&GLBackend::do_setStateBlend, state.getBlendFunction()));
}

void generateColorWriteMask(GLBackend::GLState::Commands& commands, uint32 mask) {
    commands.push_back(std::make_shared<Command1U>(&GLBackend::do_setStateColorWriteMask, mask));
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

    // go thorugh the list of state fields in the State and record the corresponding gl command
    for (int i = 0; i < State::NUM_FIELDS; i++) {
        if (state.getSignature()[i]) {
            switch(i) {
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

ComparisonFunction comparisonFuncFromGL(GLenum func) {
    if (func == GL_NEVER) {
        return NEVER;
    } else if (func == GL_LESS) {
        return LESS;
    } else if (func == GL_EQUAL) {
        return EQUAL;
    } else if (func == GL_LEQUAL) {
        return LESS_EQUAL;
    } else if (func == GL_GREATER) {
        return GREATER;
    } else if (func == GL_NOTEQUAL) {
        return NOT_EQUAL;
    } else if (func == GL_GEQUAL) {
        return GREATER_EQUAL;
    } else if (func == GL_ALWAYS) {
        return ALWAYS;
    }

    return ALWAYS;
}

State::StencilOp stencilOpFromGL(GLenum stencilOp) {
    if (stencilOp == GL_KEEP) {
        return State::STENCIL_OP_KEEP;
    } else if (stencilOp == GL_ZERO) {
        return State::STENCIL_OP_ZERO;
    } else if (stencilOp == GL_REPLACE) {
        return State::STENCIL_OP_REPLACE;
    } else if (stencilOp == GL_INCR_WRAP) {
        return State::STENCIL_OP_INCR_SAT;
    } else if (stencilOp == GL_DECR_WRAP) {
        return State::STENCIL_OP_DECR_SAT;
    } else if (stencilOp == GL_INVERT) {
        return State::STENCIL_OP_INVERT;
    } else if (stencilOp == GL_INCR) {
        return State::STENCIL_OP_INCR;
    } else if (stencilOp == GL_DECR) {
        return State::STENCIL_OP_DECR;
    }

    return State::STENCIL_OP_KEEP;
}

State::BlendOp blendOpFromGL(GLenum blendOp) {
    if (blendOp == GL_FUNC_ADD) {
        return State::BLEND_OP_ADD;
    } else if (blendOp == GL_FUNC_SUBTRACT) {
        return State::BLEND_OP_SUBTRACT;
    } else if (blendOp == GL_FUNC_REVERSE_SUBTRACT) {
        return State::BLEND_OP_REV_SUBTRACT;
    } else if (blendOp == GL_MIN) {
        return State::BLEND_OP_MIN;
    } else if (blendOp == GL_MAX) {
        return State::BLEND_OP_MAX;
    }

    return State::BLEND_OP_ADD;
}

State::BlendArg blendArgFromGL(GLenum blendArg) {
    if (blendArg == GL_ZERO) {
        return State::ZERO;
    } else if (blendArg == GL_ONE) {
        return State::ONE;
    } else if (blendArg == GL_SRC_COLOR) {
        return State::SRC_COLOR;
    } else if (blendArg == GL_ONE_MINUS_SRC_COLOR) {
        return State::INV_SRC_COLOR;
    } else if (blendArg == GL_DST_COLOR) {
        return State::DEST_COLOR;
    } else if (blendArg == GL_ONE_MINUS_DST_COLOR) {
        return State::INV_DEST_COLOR;
    } else if (blendArg == GL_SRC_ALPHA) {
        return State::SRC_ALPHA;
    } else if (blendArg == GL_ONE_MINUS_SRC_ALPHA) {
        return State::INV_SRC_ALPHA;
    } else if (blendArg == GL_DST_ALPHA) {
        return State::DEST_ALPHA;
    } else if (blendArg == GL_ONE_MINUS_DST_ALPHA) {
        return State::INV_DEST_ALPHA;
    } else if (blendArg == GL_CONSTANT_COLOR) {
        return State::FACTOR_COLOR;
    } else if (blendArg == GL_ONE_MINUS_CONSTANT_COLOR) {
        return State::INV_FACTOR_COLOR;
    } else if (blendArg == GL_CONSTANT_ALPHA) {
        return State::FACTOR_ALPHA;
    } else if (blendArg == GL_ONE_MINUS_CONSTANT_ALPHA) {
        return State::INV_FACTOR_ALPHA;
    }

    return State::ONE;
}

void GLBackend::getCurrentGLState(State::Data& state) {
    {
        GLint modes[2];
        glGetIntegerv(GL_POLYGON_MODE, modes);
        if (modes[0] == GL_FILL) {
            state.fillMode = State::FILL_FACE;
        } else {
            if (modes[0] == GL_LINE) {
                state.fillMode = State::FILL_LINE;
            } else {
                state.fillMode = State::FILL_POINT;
            }
        }
    }
    {
        if (glIsEnabled(GL_CULL_FACE)) {
            GLint mode;
            glGetIntegerv(GL_CULL_FACE_MODE, &mode);
            state.cullMode = (mode == GL_FRONT ? State::CULL_FRONT : State::CULL_BACK);
        } else {
            state.cullMode = State::CULL_NONE;
        }
    }
    {
        GLint winding;
        glGetIntegerv(GL_FRONT_FACE, &winding);
        state.frontFaceClockwise = (winding == GL_CW);
        state.depthClampEnable = glIsEnabled(GL_DEPTH_CLAMP);
        state.scissorEnable = glIsEnabled(GL_SCISSOR_TEST);
        state.multisampleEnable = glIsEnabled(GL_MULTISAMPLE);
        state.antialisedLineEnable = glIsEnabled(GL_LINE_SMOOTH);
    }
    {
        if (glIsEnabled(GL_POLYGON_OFFSET_FILL)) {
            glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &state.depthBiasSlopeScale);
            glGetFloatv(GL_POLYGON_OFFSET_UNITS, &state.depthBias);
        }
    }
    {
        GLboolean isEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean writeMask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &writeMask);
        GLint func;
        glGetIntegerv(GL_DEPTH_FUNC, &func);

        state.depthTest = State::DepthTest(isEnabled, writeMask, comparisonFuncFromGL(func));
    }
    {
        GLboolean isEnabled = glIsEnabled(GL_STENCIL_TEST);

        GLint frontWriteMask;
        GLint frontReadMask;
        GLint frontRef;
        GLint frontFail;
        GLint frontDepthFail;
        GLint frontPass;
        GLint frontFunc;
        glGetIntegerv(GL_STENCIL_WRITEMASK, &frontWriteMask);
        glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontReadMask);
        glGetIntegerv(GL_STENCIL_REF, &frontRef);
        glGetIntegerv(GL_STENCIL_FAIL, &frontFail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &frontDepthFail);
        glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &frontPass);
        glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);

        GLint backWriteMask;
        GLint backReadMask;
        GLint backRef;
        GLint backFail;
        GLint backDepthFail;
        GLint backPass;
        GLint backFunc;
        glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &backWriteMask);
        glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backReadMask);
        glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
        glGetIntegerv(GL_STENCIL_BACK_FAIL, &backFail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &backDepthFail);
        glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &backPass);
        glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);

        state.stencilActivation = State::StencilActivation(isEnabled, frontWriteMask, backWriteMask);
        state.stencilTestFront = State::StencilTest(frontRef, frontReadMask, comparisonFuncFromGL(frontFunc), stencilOpFromGL(frontFail), stencilOpFromGL(frontDepthFail), stencilOpFromGL(frontPass));
        state.stencilTestBack = State::StencilTest(backRef, backReadMask, comparisonFuncFromGL(backFunc), stencilOpFromGL(backFail), stencilOpFromGL(backDepthFail), stencilOpFromGL(backPass));
    }
    {
        GLint mask = 0xFFFFFFFF;
        
#ifdef GPU_PROFILE_CORE
        if (glIsEnabled(GL_SAMPLE_MASK)) {
            glGetIntegerv(GL_SAMPLE_MASK, &mask);
            state.sampleMask = mask;
        }
#endif
        state.sampleMask = mask;
    }
    {
        state.alphaToCoverageEnable = glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    {
        GLboolean isEnabled = glIsEnabled(GL_BLEND);
        GLint srcRGB;
        GLint srcA;
        GLint dstRGB;
        GLint dstA;
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcA);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstA);

        GLint opRGB;
        GLint opA;
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &opRGB);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &opA);

        state.blendFunction = State::BlendFunction(isEnabled,
            blendArgFromGL(srcRGB), blendOpFromGL(opRGB), blendArgFromGL(dstRGB),
            blendArgFromGL(srcA), blendOpFromGL(opA), blendArgFromGL(dstA));
    }
    {
        GLboolean mask[4];
        glGetBooleanv(GL_COLOR_WRITEMASK, mask);
        state.colorWriteMask = (mask[0] ? State::WRITE_RED : 0)
                             | (mask[1] ? State::WRITE_GREEN : 0)
                             | (mask[2] ? State::WRITE_BLUE : 0)
                             | (mask[3] ? State::WRITE_ALPHA : 0);
    }

    (void) CHECK_GL_ERROR();
}

void GLBackend::syncPipelineStateCache() {
    State::Data state;

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    getCurrentGLState(state);
    State::Signature signature = State::evalSignature(state);
    
    _pipeline._stateCache = state;
    _pipeline._stateSignatureCache = signature;
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
        (void) CHECK_GL_ERROR();

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
        (void) CHECK_GL_ERROR();

        _pipeline._stateCache.cullMode = State::CullMode(mode);
    }
}

void GLBackend::do_setStateFrontFaceClockwise(bool isClockwise) {
    if (_pipeline._stateCache.frontFaceClockwise != isClockwise) {
        static GLenum  GL_FRONT_FACES[] = { GL_CCW, GL_CW };
        glFrontFace(GL_FRONT_FACES[isClockwise]);
        (void) CHECK_GL_ERROR();
    
        _pipeline._stateCache.frontFaceClockwise = isClockwise;
    }
}

void GLBackend::do_setStateDepthClampEnable(bool enable) {
    if (_pipeline._stateCache.depthClampEnable != enable) {
        if (enable) {
            glEnable(GL_DEPTH_CLAMP);
        } else {
            glDisable(GL_DEPTH_CLAMP);
        }
        (void) CHECK_GL_ERROR();

        _pipeline._stateCache.depthClampEnable = enable;
    }
}

void GLBackend::do_setStateScissorEnable(bool enable) {
    if (_pipeline._stateCache.scissorEnable != enable) {
        if (enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        (void) CHECK_GL_ERROR();

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
        (void) CHECK_GL_ERROR();

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
        (void) CHECK_GL_ERROR();

        _pipeline._stateCache.antialisedLineEnable = enable;
    }
}

void GLBackend::do_setStateDepthBias(Vec2 bias) {
    if ( (bias.x != _pipeline._stateCache.depthBias) || (bias.y != _pipeline._stateCache.depthBiasSlopeScale)) {
        if ((bias.x != 0.0f) || (bias.y != 0.0f)) {
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
        if (CHECK_GL_ERROR()) {
            qDebug() << "DepthTest" << (test.isEnabled() ? "Enabled" : "Disabled")
                    << "Mask=" << (test.getWriteMask() ? "Write" : "no Write")
                    << "Func=" << test.getFunction()
                    << "Raw=" << test.getRaw();
        }

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
        (void) CHECK_GL_ERROR();

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
        (void) CHECK_GL_ERROR();
        _pipeline._stateCache.alphaToCoverageEnable = enable;
    }
}

void GLBackend::do_setStateSampleMask(uint32 mask) {
    if (_pipeline._stateCache.sampleMask != mask) {
#ifdef GPU_CORE_PROFILE
        if (mask == 0xFFFFFFFF) {
            glDisable(GL_SAMPLE_MASK);
        } else {
            glEnable(GL_SAMPLE_MASK);
            glSampleMaski(0, mask);
        }
#endif
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
            (void) CHECK_GL_ERROR();

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
            (void) CHECK_GL_ERROR();
        } else {
            glDisable(GL_BLEND);
        }

        _pipeline._stateCache.blendFunction = function;
    }
}

void GLBackend::do_setStateColorWriteMask(uint32 mask) {
    if (_pipeline._stateCache.colorWriteMask != mask) {
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
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_setStateScissorRect(Batch& batch, uint32 paramOffset) {
    
    Vec4 rect(batch._params[paramOffset + 0]._float,
                batch._params[paramOffset + 1]._float,
                batch._params[paramOffset + 2]._float,
                batch._params[paramOffset + 3]._float);

    glScissor(rect.x, rect.y, rect.z, rect.w);
    (void) CHECK_GL_ERROR();
}
