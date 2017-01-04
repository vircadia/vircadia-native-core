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
#include "GLBackend.h"
#include "GLState.h"

#include <gpu/GPULogging.h>

using namespace gpu;
using namespace gpu::gl;

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

void GLBackend::syncPipelineStateCache() {
    State::Data state;

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Point size is always on
    // FIXME CORE
    //glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_PROGRAM_POINT_SIZE_EXT);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    // Default line width accross the board
    glLineWidth(1.0f);

    getCurrentGLState(state);
    State::Signature signature = State::evalSignature(state);

    _pipeline._stateCache = state;
    _pipeline._stateSignatureCache = signature;
}


void GLBackend::do_setStateFillMode(int32 mode) {
    if (_pipeline._stateCache.fillMode != mode) {
        static GLenum GL_FILL_MODES[] = { GL_POINT, GL_LINE, GL_FILL };
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL_MODES[mode]);
        (void)CHECK_GL_ERROR();

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
        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.cullMode = State::CullMode(mode);
    }
}

void GLBackend::do_setStateFrontFaceClockwise(bool isClockwise) {
    if (_pipeline._stateCache.frontFaceClockwise != isClockwise) {
        static GLenum  GL_FRONT_FACES[] = { GL_CCW, GL_CW };
        glFrontFace(GL_FRONT_FACES[isClockwise]);
        (void)CHECK_GL_ERROR();

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
        (void)CHECK_GL_ERROR();

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
        (void)CHECK_GL_ERROR();

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
        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.multisampleEnable = enable;
    }
}

void GLBackend::do_setStateAntialiasedLineEnable(bool enable) {
    if (_pipeline._stateCache.antialisedLineEnable != enable) {
        if (enable) {
            glEnable(GL_LINE_SMOOTH);
        } else {
            glDisable(GL_LINE_SMOOTH);
        }
        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.antialisedLineEnable = enable;
    }
}

void GLBackend::do_setStateDepthBias(Vec2 bias) {
    if ((bias.x != _pipeline._stateCache.depthBias) || (bias.y != _pipeline._stateCache.depthBiasSlopeScale)) {
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
        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.depthBias = bias.x;
        _pipeline._stateCache.depthBiasSlopeScale = bias.y;
    }
}

void GLBackend::do_setStateDepthTest(State::DepthTest test) {
    const auto& current = _pipeline._stateCache.depthTest;
    if (current != test) {
        if (test.isEnabled()) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        if (test.getWriteMask() != current.getWriteMask()) {
            glDepthMask(test.getWriteMask());
        }
        if (test.getFunction() != current.getFunction()) {
            glDepthFunc(COMPARISON_TO_GL[test.getFunction()]);
        }
        if (CHECK_GL_ERROR()) {
            qCDebug(gpulogging) << "DepthTest" << (test.isEnabled() ? "Enabled" : "Disabled")
                << "Mask=" << (test.getWriteMask() ? "Write" : "no Write")
                << "Func=" << test.getFunction()
                << "Raw=" << test.getRaw();
        }
        _pipeline._stateCache.depthTest = test;
    }
}

void GLBackend::do_setStateStencil(State::StencilActivation activation, State::StencilTest testFront, State::StencilTest testBack) {
    const auto& currentActivation = _pipeline._stateCache.stencilActivation;
    const auto& currentTestFront = _pipeline._stateCache.stencilTestFront;
    const auto& currentTestBack = _pipeline._stateCache.stencilTestBack;
    if ((currentActivation != activation)
        || (currentTestFront != testFront)
        || (currentTestBack != testBack)) {

        if (activation.isEnabled()) {
            glEnable(GL_STENCIL_TEST);
        } else {
            glDisable(GL_STENCIL_TEST);
        }

        if (activation.getWriteMaskFront() != activation.getWriteMaskBack()) {
            glStencilMaskSeparate(GL_FRONT, activation.getWriteMaskFront());
            glStencilMaskSeparate(GL_BACK, activation.getWriteMaskBack());
        } else {
            glStencilMask(activation.getWriteMaskFront());
        }

        static GLenum STENCIL_OPS[State::NUM_STENCIL_OPS] = {
            GL_KEEP,
            GL_ZERO,
            GL_REPLACE,
            GL_INCR_WRAP,
            GL_DECR_WRAP,
            GL_INVERT,
            GL_INCR,
            GL_DECR };

        if (testFront != testBack) {
            glStencilOpSeparate(GL_FRONT, STENCIL_OPS[testFront.getFailOp()], STENCIL_OPS[testFront.getDepthFailOp()], STENCIL_OPS[testFront.getPassOp()]);
            glStencilFuncSeparate(GL_FRONT, COMPARISON_TO_GL[testFront.getFunction()], testFront.getReference(), testFront.getReadMask());

            glStencilOpSeparate(GL_BACK, STENCIL_OPS[testBack.getFailOp()], STENCIL_OPS[testBack.getDepthFailOp()], STENCIL_OPS[testBack.getPassOp()]);
            glStencilFuncSeparate(GL_BACK, COMPARISON_TO_GL[testBack.getFunction()], testBack.getReference(), testBack.getReadMask());
        } else {
            glStencilOp(STENCIL_OPS[testFront.getFailOp()], STENCIL_OPS[testFront.getDepthFailOp()], STENCIL_OPS[testFront.getPassOp()]);
            glStencilFunc(COMPARISON_TO_GL[testFront.getFunction()], testFront.getReference(), testFront.getReadMask());
        }

        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.stencilActivation = activation;
        _pipeline._stateCache.stencilTestFront = testFront;
        _pipeline._stateCache.stencilTestBack = testBack;
    }
}

void GLBackend::do_setStateAlphaToCoverageEnable(bool enable) {
    if (_pipeline._stateCache.alphaToCoverageEnable != enable) {
        if (enable) {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        } else {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.alphaToCoverageEnable = enable;
    }
}

void GLBackend::do_setStateSampleMask(uint32 mask) {
    if (_pipeline._stateCache.sampleMask != mask) {
        if (mask == 0xFFFFFFFF) {
            glDisable(GL_SAMPLE_MASK);
        } else {
            glEnable(GL_SAMPLE_MASK);
            glSampleMaski(0, mask);
        }
        (void)CHECK_GL_ERROR();
        _pipeline._stateCache.sampleMask = mask;
    }
}

void GLBackend::do_setStateBlend(State::BlendFunction function) {
    if (_pipeline._stateCache.blendFunction != function) {
        if (function.isEnabled()) {
            glEnable(GL_BLEND);

            glBlendEquationSeparate(BLEND_OPS_TO_GL[function.getOperationColor()], BLEND_OPS_TO_GL[function.getOperationAlpha()]);
            (void)CHECK_GL_ERROR();


            glBlendFuncSeparate(BLEND_ARGS_TO_GL[function.getSourceColor()], BLEND_ARGS_TO_GL[function.getDestinationColor()],
                BLEND_ARGS_TO_GL[function.getSourceAlpha()], BLEND_ARGS_TO_GL[function.getDestinationAlpha()]);
        } else {
            glDisable(GL_BLEND);
        }
        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.blendFunction = function;
    }
}

void GLBackend::do_setStateColorWriteMask(uint32 mask) {
    if (_pipeline._stateCache.colorWriteMask != mask) {
        glColorMask(mask & State::ColorMask::WRITE_RED,
            mask & State::ColorMask::WRITE_GREEN,
            mask & State::ColorMask::WRITE_BLUE,
            mask & State::ColorMask::WRITE_ALPHA);
        (void)CHECK_GL_ERROR();

        _pipeline._stateCache.colorWriteMask = mask;
    }
}


void GLBackend::do_setStateBlendFactor(const Batch& batch, size_t paramOffset) {
    Vec4 factor(batch._params[paramOffset + 0]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 3]._float);

    glBlendColor(factor.x, factor.y, factor.z, factor.w);
    (void)CHECK_GL_ERROR();
}

void GLBackend::do_setStateScissorRect(const Batch& batch, size_t paramOffset) {
    Vec4i rect;
    memcpy(&rect, batch.readData(batch._params[paramOffset]._uint), sizeof(Vec4i));

    if (_stereo._enable) {
        rect.z /= 2;
        if (_stereo._pass) {
            rect.x += rect.z;
        }
    }
    glScissor(rect.x, rect.y, rect.z, rect.w);
    (void)CHECK_GL_ERROR();
}

