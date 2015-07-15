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
#include "GLBackendShared.h"

#include "Format.h"

using namespace gpu;

GLBackend::GLPipeline::GLPipeline() :
    _program(nullptr),
    _state(nullptr)
{}

GLBackend::GLPipeline::~GLPipeline() {
    _program = nullptr;
    _state = nullptr;
}

GLBackend::GLPipeline* GLBackend::syncGPUObject(const Pipeline& pipeline) {
    GLPipeline* object = Backend::getGPUObject<GLBackend::GLPipeline>(pipeline);

    // If GPU object already created then good
    if (object) {
        return object;
    }

    // No object allocated yet, let's see if it's worth it...
    ShaderPointer shader = pipeline.getProgram();
    GLShader* programObject = GLBackend::syncGPUObject((*shader));
    if (programObject == nullptr) {
        return nullptr;
    }

    StatePointer state = pipeline.getState();
    GLState* stateObject = GLBackend::syncGPUObject((*state));
    if (stateObject == nullptr) {
        return nullptr;
    }

    // Program and state are valid, we can create the pipeline object
    if (!object) {
        object = new GLPipeline();
        Backend::setGPUObject(pipeline, object);
    }

    object->_program = programObject;
    object->_state = stateObject;

    return object;
}

void GLBackend::do_setPipeline(Batch& batch, uint32 paramOffset) {
    PipelinePointer pipeline = batch._pipelines.get(batch._params[paramOffset + 0]._uint);

    if (_pipeline._pipeline == pipeline) {
        return;
    }

    // null pipeline == reset
    if (!pipeline) {
        _pipeline._pipeline.reset();

        _pipeline._program = 0;
        _pipeline._invalidProgram = true;

#if (GPU_TRANSFORM_PROFILE == GPU_CORE)
#else
        _pipeline._program_transformObject_model = -1;
        _pipeline._program_transformCamera_viewInverse = -1;
        _pipeline._program_transformCamera_viewport = -1;
#endif

        _pipeline._state = nullptr;
        _pipeline._invalidState = true;
    } else {
        auto pipelineObject = syncGPUObject((*pipeline));
        if (!pipelineObject) {
            return;
        }

        // check the program cache
        if (_pipeline._program != pipelineObject->_program->_program) {
            _pipeline._program = pipelineObject->_program->_program;
            _pipeline._invalidProgram = true;

#if (GPU_TRANSFORM_PROFILE == GPU_CORE)
#else
            _pipeline._program_transformObject_model = pipelineObject->_program->_transformObject_model;
            _pipeline._program_transformCamera_viewInverse = pipelineObject->_program->_transformCamera_viewInverse;
            _pipeline._program_transformCamera_viewport = pipelineObject->_program->_transformCamera_viewport;
#endif
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

#if (GPU_TRANSFORM_PROFILE == GPU_CORE)
#else
    // If shader program needs the model we need to provide it
    if (_pipeline._program_transformObject_model >= 0) {
        glUniformMatrix4fv(_pipeline._program_transformObject_model, 1, false, (const GLfloat*) &_transform._transformObject._model);
    }

    // If shader program needs the inverseView we need to provide it
    if (_pipeline._program_transformCamera_viewInverse >= 0) {
        glUniformMatrix4fv(_pipeline._program_transformCamera_viewInverse, 1, false, (const GLfloat*) &_transform._transformCamera._viewInverse);
    }

    // If shader program needs the viewport we need to provide it
    if (_pipeline._program_transformCamera_viewport >= 0) {
        glUniform4fv(_pipeline._program_transformCamera_viewport, 1, (const GLfloat*) &_transform._transformCamera._viewport);
    }
#endif
}

void GLBackend::do_setUniformBuffer(Batch& batch, uint32 paramOffset) {
    GLuint slot = batch._params[paramOffset + 3]._uint;
    BufferPointer uniformBuffer = batch._buffers.get(batch._params[paramOffset + 2]._uint);
    GLintptr rangeStart = batch._params[paramOffset + 1]._uint;
    GLsizeiptr rangeSize = batch._params[paramOffset + 0]._uint;

#if (GPU_FEATURE_PROFILE == GPU_CORE)
    GLuint bo = getBufferID(*uniformBuffer);
    glBindBufferRange(GL_UNIFORM_BUFFER, slot, bo, rangeStart, rangeSize);
#else
    // because we rely on the program uniform mechanism we need to have
    // the program bound, thank you MacOSX Legacy profile.
    updatePipeline();
    
    GLfloat* data = (GLfloat*) (uniformBuffer->getData() + rangeStart);
    glUniform4fv(slot, rangeSize / sizeof(GLfloat[4]), data);
 
    // NOT working so we ll stick to the uniform float array until we move to core profile
    // GLuint bo = getBufferID(*uniformBuffer);
    //glUniformBufferEXT(_shader._program, slot, bo);
#endif
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_setResourceTexture(Batch& batch, uint32 paramOffset) {
    GLuint slot = batch._params[paramOffset + 1]._uint;
    TexturePointer uniformTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);

    if (!uniformTexture) {
        return;
    }

    GLTexture* object = GLBackend::syncGPUObject(*uniformTexture);
    if (object) {
        GLuint to = object->_texture;
        GLuint target = object->_target;
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(target, to);

        (void) CHECK_GL_ERROR();

    } else {
        return;
    }
}

