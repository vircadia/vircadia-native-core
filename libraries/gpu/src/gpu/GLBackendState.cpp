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
typedef GLBackend::GLState::Command1<int32> Command1I;
typedef GLBackend::GLState::Command1<State::DepthTest> CommandDepthTest;
typedef GLBackend::GLState::Command3<State::StencilActivation, State::StencilTest, State::StencilTest> CommandStencil;
typedef GLBackend::GLState::Command2<State::BlendFunction, Vec4> CommandBlend;


void generateFillMode(GLBackend::GLState::Commands& commands, State::FillMode fillMode) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateFillMode, int32(fillMode))));
}

void generateCullMode(GLBackend::GLState::Commands& commands, State::CullMode cullMode) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateCullMode, int32(cullMode))));
}

void generateDepthBias(GLBackend::GLState::Commands& commands, const State& state) {
  /*  if ((state.getDepthBias() == 0) && (state.getDepthBiasSlopeScale() == 0.0f)) {
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glDisable, GLenum(GL_POLYGON_OFFSET_FILL))));
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glDisable, GLenum(GL_POLYGON_OFFSET_LINE))));
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glDisable, GLenum(GL_POLYGON_OFFSET_POINT))));
    } else {
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glEnable, GLenum(GL_POLYGON_OFFSET_FILL))));
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glEnable, GLenum(GL_POLYGON_OFFSET_LINE))));
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glEnable, GLenum(GL_POLYGON_OFFSET_POINT))));
        commands.push_back(CommandPointer(new Command2F((Command2F::GLFunction)glPolygonOffset,
            state.getDepthBiasSlopeScale(),
            state.getDepthBias())));
    }*/
}

void generateFrontClockwise(GLBackend::GLState::Commands& commands, bool frontClockwise) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateFrontClockwise, int32(frontClockwise))));
}

void generateDepthClipEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateDepthClipEnable, int32(enable))));
}

void generateScissorEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateScissorEnable, int32(enable))));
}

void generateMultisampleEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateMultisampleEnable, int32(enable))));
}

void generateAntialiasedLineEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateAntialiasedLineEnable, int32(enable))));
}

void generateDepthTest(GLBackend::GLState::Commands& commands, State::DepthTest& test) {
    commands.push_back(CommandPointer(new CommandDepthTest(&GLBackend::do_setStateDepthTest, int32(test.getRaw()))));
}

void generateStencil(GLBackend::GLState::Commands& commands, const State& state) {
    commands.push_back(CommandPointer(new CommandStencil(&GLBackend::do_setStateStencil, state.getStencilActivation(), state.getStencilTestFront(), state.getStencilTestBack())));
}

void generateAlphaToCoverageEnable(GLBackend::GLState::Commands& commands, bool enable) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateAlphaToCoverageEnable, int32(enable))));
}

void generateBlend(GLBackend::GLState::Commands& commands, const State& state) {
    commands.push_back(CommandPointer(new CommandBlend(&GLBackend::do_setStateBlend, state.getBlendFunction(), state.getBlendFactor())));
}

void generateColorWriteMask(GLBackend::GLState::Commands& commands, int32 mask) {
    commands.push_back(CommandPointer(new Command1I(&GLBackend::do_setStateColorWriteMask, int32(mask))));
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
        case State::FRONT_FACE: {
            generateFrontClockwise(object->_commands, bool(field.second._integer));
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
            break;
        }
        case State::ALPHA_TO_COVERAGE_ENABLE: {
            generateAlphaToCoverageEnable(object->_commands, bool(field.second._integer));
            break;
        }

        case State::BLEND_FUNCTION:
        case State::BLEND_FACTOR_X:
        case State::BLEND_FACTOR_Y:
        case State::BLEND_FACTOR_Z:
        case State::BLEND_FACTOR_W: {
            blendState = true;
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

    if (blendState) {
        generateBlend(object->_commands, state);
    }

    return object;
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
    static GLenum GL_FILL_MODES[] = { GL_POINT, GL_LINE, GL_FILL };
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL_MODES[mode]);
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateCullMode(int32 mode) {
    static GLenum GL_CULL_MODES[] = { GL_FRONT_AND_BACK, GL_FRONT, GL_BACK };
    if (mode == State::CULL_NONE) {
        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT_AND_BACK);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_CULL_MODES[mode]);
    }
}

void GLBackend::do_setStateFrontClockwise(int32 isFrontClockwise) {
    static GLenum  GL_FRONT_FACES[] = { GL_CCW, GL_CW };
    glFrontFace(GL_FRONT_FACES[isFrontClockwise]);
}

void GLBackend::do_setStateDepthClipEnable(int32 enable) {
    if (enable) {
        glEnable(GL_DEPTH_CLAMP);
    } else {
        glDisable(GL_DEPTH_CLAMP);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateScissorEnable(int32 enable) {
    if (enable) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateMultisampleEnable(int32 enable) {
    if (enable) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateAntialiasedLineEnable(int32 enable) {
    if (enable) {
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
    } else {
        glDisable(GL_POINT_SMOOTH);
        glDisable(GL_LINE_SMOOTH);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateDepthTest(State::DepthTest test) {
    if (test._enabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(test._writeMask);
        glDepthFunc(GL_COMPARISON_FUNCTIONS[test._function]);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateStencil(State::StencilActivation activation, State::StencilTest frontTest, State::StencilTest backTest) {
    if (activation._enabled) {
        glEnable(GL_STENCIL_TEST);
        glStencilMaskSeparate(GL_FRONT, activation._frontWriteMask);
        glStencilMaskSeparate(GL_BACK, activation._backWriteMask);

         static GLenum STENCIL_OPS[] = {
            GL_KEEP,
            GL_ZERO,
            GL_REPLACE,
            GL_INCR_WRAP,
            GL_DECR_WRAP,
            GL_INVERT,
            GL_INCR,
            GL_DECR };

        glStencilFuncSeparate(GL_FRONT, STENCIL_OPS[frontTest._failOp], STENCIL_OPS[frontTest._passOp], STENCIL_OPS[frontTest._depthFailOp]);
        glStencilFuncSeparate(GL_FRONT, GL_COMPARISON_FUNCTIONS[frontTest._function], frontTest._reference, frontTest._readMask);

        glStencilFuncSeparate(GL_BACK, STENCIL_OPS[backTest._failOp], STENCIL_OPS[backTest._passOp], STENCIL_OPS[backTest._depthFailOp]);
        glStencilFuncSeparate(GL_BACK, GL_COMPARISON_FUNCTIONS[backTest._function], backTest._reference, backTest._readMask);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateAlphaToCoverageEnable(int32 enable) {
    if (enable) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateBlend(State::BlendFunction function, Vec4 factor ) {
    if (function._enabled) {
        glEnable(GL_BLEND);

        static GLenum GL_BLEND_OPS[] = {
            GL_FUNC_ADD,
            GL_FUNC_SUBTRACT,
            GL_FUNC_REVERSE_SUBTRACT,
            GL_MIN,
            GL_MAX };

        glBlendEquationSeparate(GL_BLEND_OPS[function._operationColor], GL_BLEND_OPS[function._operationAlpha]);

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

        glBlendFuncSeparate(BLEND_ARGS[function._sourceColor], BLEND_ARGS[function._destinationColor],
                            BLEND_ARGS[function._sourceAlpha], BLEND_ARGS[function._destinationAlpha]);

        glBlendColor(factor.x, factor.y, factor.z, factor.w);
    } else {
        glDisable(GL_BLEND);
    }
    CHECK_GL_ERROR();
}

void GLBackend::do_setStateColorWriteMask(int32 mask) {
    glColorMask(mask & State::ColorMask::WRITE_RED,
                mask & State::ColorMask::WRITE_GREEN,
                mask & State::ColorMask::WRITE_BLUE,
                mask & State::ColorMask::WRITE_ALPHA );
}