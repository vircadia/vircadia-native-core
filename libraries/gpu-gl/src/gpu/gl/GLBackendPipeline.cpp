//
//  GLBackendPipeline.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"
#include "GLShared.h"
#include "GLPipeline.h"
#include "GLShader.h"
#include "GLState.h"
#include "GLBuffer.h"
#include "GLTexture.h"

using namespace gpu;
using namespace gpu::gl;

void GLBackend::do_setPipeline(Batch& batch, size_t paramOffset) {
    PipelinePointer pipeline = batch._pipelines.get(batch._params[paramOffset + 0]._uint);

    if (_pipeline._pipeline == pipeline) {
        return;
    }

    // A true new Pipeline
    _stats._PSNumSetPipelines++;

    // null pipeline == reset
    if (!pipeline) {
        _pipeline._pipeline.reset();

        _pipeline._program = 0;
        _pipeline._programShader = nullptr;
        _pipeline._invalidProgram = true;

        _pipeline._state = nullptr;
        _pipeline._invalidState = true;
    } else {
        auto pipelineObject = GLPipeline::sync(*pipeline);
        if (!pipelineObject) {
            return;
        }

        // check the program cache
        // pick the program version 
        GLuint glprogram = pipelineObject->_program->getProgram();

        if (_pipeline._program != glprogram) {
            _pipeline._program = glprogram;
            _pipeline._programShader = pipelineObject->_program;
            _pipeline._invalidProgram = true;
        }

        // Now for the state
        if (_pipeline._state != pipelineObject->_state) {
            _pipeline._state = pipelineObject->_state;
            _pipeline._invalidState = true;
        }

        // Remember the new pipeline
        _pipeline._pipeline = pipeline;
    }

    // THis should be done on Pipeline::update...
    if (_pipeline._invalidProgram) {
        glUseProgram(_pipeline._program);
        (void) CHECK_GL_ERROR();
        _pipeline._invalidProgram = false;
    }
}

void GLBackend::updatePipeline() {
    if (_pipeline._invalidProgram) {
        // doing it here is aproblem for calls to glUniform.... so will do it on assing...
        glUseProgram(_pipeline._program);
        (void) CHECK_GL_ERROR();
        _pipeline._invalidProgram = false;
    }

    if (_pipeline._invalidState) {
        if (_pipeline._state) {
            // first reset to default what should be
            // the fields which were not to default and are default now
            resetPipelineState(_pipeline._state->_signature);
            
            // Update the signature cache with what's going to be touched
            _pipeline._stateSignatureCache |= _pipeline._state->_signature;

            // And perform
            for (auto command: _pipeline._state->_commands) {
                command->run(this);
            }
        } else {
            // No state ? anyway just reset everything
            resetPipelineState(0);
        }
        _pipeline._invalidState = false;
    }
}

void GLBackend::resetPipelineStage() {
    // First reset State to default
    State::Signature resetSignature(0);
    resetPipelineState(resetSignature);
    _pipeline._state = nullptr;
    _pipeline._invalidState = false;

    // Second the shader side
    _pipeline._invalidProgram = false;
    _pipeline._program = 0;
    _pipeline._programShader = nullptr;
    _pipeline._pipeline.reset();
    glUseProgram(0);
}

void GLBackend::releaseUniformBuffer(uint32_t slot) {
    auto& buf = _uniform._buffers[slot];
    if (buf) {
        auto* object = Backend::getGPUObject<GLBuffer>(*buf);
        if (object) {
            glBindBufferBase(GL_UNIFORM_BUFFER, slot, 0); // RELEASE
            (void) CHECK_GL_ERROR();
        }
        buf.reset();
    }
}

void GLBackend::resetUniformStage() {
    for (uint32_t i = 0; i < _uniform._buffers.size(); i++) {
        releaseUniformBuffer(i);
    }
}

void GLBackend::do_setUniformBuffer(Batch& batch, size_t paramOffset) {
    GLuint slot = batch._params[paramOffset + 3]._uint;
    BufferPointer uniformBuffer = batch._buffers.get(batch._params[paramOffset + 2]._uint);
    GLintptr rangeStart = batch._params[paramOffset + 1]._uint;
    GLsizeiptr rangeSize = batch._params[paramOffset + 0]._uint;

    if (!uniformBuffer) {
        releaseUniformBuffer(slot);
        return;
    }
    
    // check cache before thinking
    if (_uniform._buffers[slot] == uniformBuffer) {
        return;
    }

    // Sync BufferObject
    auto* object = syncGPUObject(*uniformBuffer);
    if (object) {
        glBindBufferRange(GL_UNIFORM_BUFFER, slot, object->_buffer, rangeStart, rangeSize);

        _uniform._buffers[slot] = uniformBuffer;
        (void) CHECK_GL_ERROR();
    } else {
        releaseResourceTexture(slot);
        return;
    }
}

void GLBackend::releaseResourceTexture(uint32_t slot) {
    auto& tex = _resource._textures[slot];
    if (tex) {
        auto* object = Backend::getGPUObject<GLTexture>(*tex);
        if (object) {
            GLuint target = object->_target;
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(target, 0); // RELEASE
            (void) CHECK_GL_ERROR();
        }
        tex.reset();
    }
}

void GLBackend::resetResourceStage() {
    for (uint32_t i = 0; i < _resource._textures.size(); i++) {
        releaseResourceTexture(i);
    }
}

void GLBackend::do_setResourceTexture(Batch& batch, size_t paramOffset) {
    GLuint slot = batch._params[paramOffset + 1]._uint;
    TexturePointer resourceTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);

    if (!resourceTexture) {
        releaseResourceTexture(slot);
        return;
    }
    // check cache before thinking
    if (_resource._textures[slot] == resourceTexture) {
        return;
    }

    // One more True texture bound
    _stats._RSNumTextureBounded++;

    // Always make sure the GLObject is in sync
    GLTexture* object = syncGPUObject(resourceTexture);
    if (object) {
        GLuint to = object->_texture;
        GLuint target = object->_target;
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(target, to);

        (void) CHECK_GL_ERROR();

        _resource._textures[slot] = resourceTexture;

        _stats._RSAmountTextureMemoryBounded += object->size();

    } else {
        releaseResourceTexture(slot);
        return;
    }
}

int GLBackend::ResourceStageState::findEmptyTextureSlot() const {
    // start from the end of the slots, try to find an empty one that can be used
    for (auto i = MAX_NUM_RESOURCE_TEXTURES - 1; i > 0; i--) {
        if (!_textures[i]) {
            return i;
        }
    }
    return -1;
}

