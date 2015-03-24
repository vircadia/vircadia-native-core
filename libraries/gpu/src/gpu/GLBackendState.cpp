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
typedef GLBackend::GLState::Command1<GLenum> Command1E;
typedef GLBackend::GLState::Command2<GLenum, GLenum> Command2E;
typedef GLBackend::GLState::Command2<GLfloat, GLfloat> Command2F;
typedef GLBackend::GLState::Command4<GLenum, GLenum, GLenum, GLenum> Command4E;
typedef GLBackend::GLState::Command4<GLfloat, GLfloat, GLfloat, GLfloat> Command4F;
typedef GLBackend::GLState::Command4<GLboolean, GLboolean, GLboolean, GLboolean> Command4B;

static GLenum GL_COMPARISON_FUNCTIONS[] = {
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS };

void glBackend_glPolygonMode(GLenum fillMode) {
    glPolygonMode(GL_FRONT_AND_BACK, fillMode);
}

void generateFillMode(GLBackend::GLState::Commands& commands, State::FillMode fillMode) {
    static GLenum GL_FILL_MODES[] = { GL_POINT, GL_LINE, GL_FILL };
    commands.push_back(CommandPointer(new Command1E(glBackend_glPolygonMode, GL_FILL_MODES[fillMode])));
}

void generateCullMode(GLBackend::GLState::Commands& commands, State::CullMode cullMode) {
    static GLenum CULL_MODES[] = { GL_FRONT_AND_BACK, GL_FRONT, GL_BACK };
    if (cullMode == State::CULL_NONE) {
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glDisable, GLenum(GL_CULL_FACE))));
    } else {
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glEnable, GLenum(GL_CULL_FACE))));
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glCullFace, CULL_MODES[cullMode])));
    }
}

void generateDepthBias(GLBackend::GLState::Commands& commands, const State& state) {
    if ((state.getDepthBias() == 0) && (state.getDepthBiasSlopeScale() == 0.0f)) {
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
    }
}

void generateFrontClockwise(GLBackend::GLState::Commands& commands, bool frontClockwise) {
    static GLenum GL_FRONT_FACES[] = { GL_CCW, GL_CW };
    commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glFrontFace, GL_FRONT_FACES[frontClockwise])));
}

#define ADD_ENABLE_DISABLE_COMMAND( NAME )     if (enable) {\
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glEnable, NAME)));\
    } else {\
        commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glDisable, NAME)));\
    }

void generateDepthClipEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_DEPTH_CLAMP);
}

void generateScissorEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_SCISSOR_TEST);
}

void generateMultisampleEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_MULTISAMPLE);
}

void generateAntialiasedLineEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_POINT_SMOOTH);
    ADD_ENABLE_DISABLE_COMMAND(GL_LINE_SMOOTH);
}

void generateDepthEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_DEPTH_TEST);
}

void generateDepthWriteMask(GLBackend::GLState::Commands& commands, bool write) {
    commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glDepthMask, write)));
}

void generateDepthFunc(GLBackend::GLState::Commands& commands, State::ComparisonFunction function) {
    commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glDepthFunc, GL_COMPARISON_FUNCTIONS[function])));
}

void generateStencilEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_STENCIL_TEST);
}

void generateStencilWriteMask(GLBackend::GLState::Commands& commands, uint32 mask) {
    commands.push_back(CommandPointer(new Command1E((Command1E::GLFunction)glStencilMask, mask)));
}

void generateStencilState(GLBackend::GLState::Commands& commands, const State& state) {
    auto frontTest(state.getStencilTestFront());
    auto backTest(state.getStencilTestBack());

    commands.push_back(CommandPointer(new Command4E((Command4E::GLFunction)glBlendEquationSeparate,
        GL_FRONT, GL_COMPARISON_FUNCTIONS[frontTest._function], state.getStencilReference(), state.getStencilReadMask())));
    commands.push_back(CommandPointer(new Command4E((Command4E::GLFunction)glBlendEquationSeparate,
        GL_BACK, GL_COMPARISON_FUNCTIONS[backTest._function], state.getStencilReference(), state.getStencilReadMask())));

    static GLenum STENCIL_OPS[] = {
        GL_KEEP,
        GL_ZERO,
        GL_REPLACE,
        GL_INCR_WRAP,
        GL_DECR_WRAP,
        GL_INVERT,
        GL_INCR,
        GL_DECR };

    commands.push_back(CommandPointer(new Command4E((Command4E::GLFunction)glStencilOpSeparate,
        GL_FRONT, STENCIL_OPS[frontTest._failOp], STENCIL_OPS[frontTest._passOp], STENCIL_OPS[frontTest._depthFailOp])));
    commands.push_back(CommandPointer(new Command4E((Command4E::GLFunction)glStencilOpSeparate,
        GL_BACK, STENCIL_OPS[backTest._failOp], STENCIL_OPS[backTest._passOp], STENCIL_OPS[backTest._depthFailOp])));
}

void generateAlphaToCoverageEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_SAMPLE_ALPHA_TO_COVERAGE);
}

void generateBlendEnable(GLBackend::GLState::Commands& commands, bool enable) {
    ADD_ENABLE_DISABLE_COMMAND(GL_BLEND);
}

void generateBlendFunction(GLBackend::GLState::Commands& commands, const State& state) {
    static GLenum GL_BLEND_OPS[] = {
        GL_FUNC_ADD,
        GL_FUNC_SUBTRACT,
        GL_FUNC_REVERSE_SUBTRACT,
        GL_MIN,
        GL_MAX };

    auto colorFunction = state.getBlendFunctionColor();
    auto alphaFunction = state.getBlendFunctionColor();

    commands.push_back(CommandPointer(new Command2E((Command2E::GLFunction)glBlendEquationSeparate,
        GL_BLEND_OPS[colorFunction._operation],
        GL_BLEND_OPS[alphaFunction._operation])));

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
        GL_ONE_MINUS_CONSTANT_COLOR
    };

    commands.push_back(CommandPointer(new Command4E((Command4E::GLFunction)glBlendFuncSeparate,
        BLEND_ARGS[colorFunction._source],
        BLEND_ARGS[colorFunction._destination],
        BLEND_ARGS[alphaFunction._source],
        BLEND_ARGS[alphaFunction._destination])));
}

void generateBlendFactor(GLBackend::GLState::Commands& commands, const Vec4& factor) {
    commands.push_back(CommandPointer(new Command4F((Command4F::GLFunction)glBlendColor,
        factor.x, factor.y, factor.z, factor.w)));
}

void generateColorWriteMask(GLBackend::GLState::Commands& commands, State::ColorMask mask) {
    commands.push_back(CommandPointer(new Command4B((Command4B::GLFunction)glColorMask,
         mask & State::ColorMask::WRITE_RED,
         mask & State::ColorMask::WRITE_GREEN,
         mask & State::ColorMask::WRITE_BLUE,
         mask & State::ColorMask::WRITE_ALPHA )));
}

GLBackend::GLState* GLBackend::syncGPUObject(const State& state) {
    GLState* object = Backend::getGPUObject<GLBackend::GLState>(state);

    // If GPU object already created then good
    if (object) {
        if (object->_stamp == state.getStamp()) {
            return object;
        }
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

    bool depthEnabled = false;
    bool depthState = false;

    bool stencilEnabled = false;
    bool stencilState = false;

    bool blendEnabled = false;
    bool blendFunction = false;
    bool blendFactor = false;

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
        case State::FRONT_CLOCKWISE: {
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

        case State::DEPTH_ENABLE: {
            depthEnabled = bool(field.second._integer);
            generateDepthEnable(object->_commands, depthEnabled);
            break;
        }
        case State::DEPTH_WRITE_MASK:
        case State::DEPTH_FUNC: {
            depthState = true;
            break;
        }
        case State::STENCIL_ENABLE: {
            stencilEnabled = bool(field.second._integer);
            generateStencilEnable(object->_commands, stencilEnabled);
            break;
        }
        case State::STENCIL_WRITE_MASK:
        case State::STENCIL_READ_MASK:
        case State::STENCIL_REFERENCE:
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

        case State::BLEND_ENABLE: {
            blendEnabled = field.second._integer;
            generateBlendEnable(object->_commands, blendEnabled);
            break;
        }
        case State::BLEND_FUNCTION_COLOR:
        case State::BLEND_FUNCTION_ALPHA: {
            blendFunction = true;
            break;
        }
        case State::BLEND_FACTOR_X:
        case State::BLEND_FACTOR_Y:
        case State::BLEND_FACTOR_Z:
        case State::BLEND_FACTOR_W: {
            blendFactor = true;
            break;
        }

        case State::COLOR_WRITE_MASK: {
            generateColorWriteMask(object->_commands, State::ColorMask(field.second._integer));
            break;
        }

        }
    }

    if (depthBias) {
        generateDepthBias(object->_commands, state);
    }

    if (depthEnabled) {
        generateDepthWriteMask(object->_commands, state.getDepthWriteMask());
        generateDepthFunc(object->_commands, state.getDepthFunc());
    }

    if (stencilEnabled) {
        generateStencilState(object->_commands, state);
        generateStencilWriteMask(object->_commands, state.getStencilWriteMask());
    }

    if (blendEnabled) {
        generateBlendFunction(object->_commands, state);
        generateBlendFactor(object->_commands, state.getBlendFactor());
    }

    return nullptr;
}
