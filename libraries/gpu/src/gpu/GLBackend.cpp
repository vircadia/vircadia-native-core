//
//  GLBackend.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackendShared.h"

#include <mutex>
#include <queue>
#include <list>
#include <glm/gtc/type_ptr.hpp>

using namespace gpu;

GLBackend::CommandCall GLBackend::_commandCalls[Batch::NUM_COMMANDS] = 
{
    (&::gpu::GLBackend::do_draw),
    (&::gpu::GLBackend::do_drawIndexed),
    (&::gpu::GLBackend::do_drawInstanced),
    (&::gpu::GLBackend::do_drawIndexedInstanced),
    (&::gpu::GLBackend::do_multiDrawIndirect),
    (&::gpu::GLBackend::do_multiDrawIndexedIndirect),

    (&::gpu::GLBackend::do_setInputFormat),
    (&::gpu::GLBackend::do_setInputBuffer),
    (&::gpu::GLBackend::do_setIndexBuffer),
    (&::gpu::GLBackend::do_setIndirectBuffer),

    (&::gpu::GLBackend::do_setModelTransform),
    (&::gpu::GLBackend::do_setViewTransform),
    (&::gpu::GLBackend::do_setProjectionTransform),
    (&::gpu::GLBackend::do_setViewportTransform),

    (&::gpu::GLBackend::do_setPipeline),
    (&::gpu::GLBackend::do_setStateBlendFactor),
    (&::gpu::GLBackend::do_setStateScissorRect),

    (&::gpu::GLBackend::do_setUniformBuffer),
    (&::gpu::GLBackend::do_setResourceTexture),

    (&::gpu::GLBackend::do_setFramebuffer),
    (&::gpu::GLBackend::do_clearFramebuffer),
    (&::gpu::GLBackend::do_blit),

    (&::gpu::GLBackend::do_beginQuery),
    (&::gpu::GLBackend::do_endQuery),
    (&::gpu::GLBackend::do_getQuery),

    (&::gpu::GLBackend::do_resetStages),

    (&::gpu::GLBackend::do_runLambda),

    (&::gpu::GLBackend::do_glActiveBindTexture),

    (&::gpu::GLBackend::do_glUniform1i),
    (&::gpu::GLBackend::do_glUniform1f),
    (&::gpu::GLBackend::do_glUniform2f),
    (&::gpu::GLBackend::do_glUniform3f),
    (&::gpu::GLBackend::do_glUniform4f),
    (&::gpu::GLBackend::do_glUniform3fv),
    (&::gpu::GLBackend::do_glUniform4fv),
    (&::gpu::GLBackend::do_glUniform4iv),
    (&::gpu::GLBackend::do_glUniformMatrix4fv),

    (&::gpu::GLBackend::do_glColor4f),
};

void GLBackend::init() {
    static std::once_flag once;
    std::call_once(once, [] {
        qCDebug(gpulogging) << "GL Version: " << QString((const char*) glGetString(GL_VERSION));

        qCDebug(gpulogging) << "GL Shader Language Version: " << QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));

        qCDebug(gpulogging) << "GL Vendor: " << QString((const char*) glGetString(GL_VENDOR));

        qCDebug(gpulogging) << "GL Renderer: " << QString((const char*) glGetString(GL_RENDERER));

        glewExperimental = true;
        GLenum err = glewInit();
        glGetError();
        if (GLEW_OK != err) {
            /* Problem: glewInit failed, something is seriously wrong. */
            qCDebug(gpulogging, "Error: %s\n", glewGetErrorString(err));
        }
        qCDebug(gpulogging, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

#if defined(Q_OS_WIN)
        if (wglewGetExtension("WGL_EXT_swap_control")) {
            int swapInterval = wglGetSwapIntervalEXT();
            qCDebug(gpulogging, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
        }
#endif

#if defined(Q_OS_LINUX)
        // TODO: Write the correct  code for Linux...
        /* if (wglewGetExtension("WGL_EXT_swap_control")) {
            int swapInterval = wglGetSwapIntervalEXT();
            qCDebug(gpulogging, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
        }*/
#endif
    });
}

Backend* GLBackend::createBackend() {
    return new GLBackend();
}

GLBackend::GLBackend() :
    _input(),
    _pipeline(),
    _output()
{
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &_uboAlignment);
    initInput();
    initTransform();
}

GLBackend::~GLBackend() {
    resetStages();

    killInput();
    killTransform();
}

void GLBackend::renderPassTransfer(Batch& batch) {
    const size_t numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();

    { // Sync all the buffers
        PROFILE_RANGE("syncGPUBuffer");

        for (auto& cached : batch._buffers._items) {
            if (cached._data) {
                syncGPUObject(*cached._data);
            }
        }
    }

    { // Sync all the buffers
        PROFILE_RANGE("syncCPUTransform");
        _transform._cameras.resize(0);
        _transform._cameraOffsets.clear();
        _transform._objects.resize(0);
        _transform._objectOffsets.clear();

        for (_commandIndex = 0; _commandIndex < numCommands; ++_commandIndex) {
            switch (*command) {
            case Batch::COMMAND_draw:
            case Batch::COMMAND_drawIndexed:
            case Batch::COMMAND_drawInstanced:
            case Batch::COMMAND_drawIndexedInstanced:
                _transform.preUpdate(_commandIndex, _stereo);
                break;

            case Batch::COMMAND_setModelTransform:
            case Batch::COMMAND_setViewportTransform:
            case Batch::COMMAND_setViewTransform:
            case Batch::COMMAND_setProjectionTransform: {
                CommandCall call = _commandCalls[(*command)];
                (this->*(call))(batch, *offset);
                break;
            }

            default:
                break;
            }
            command++;
            offset++;
        }
    }

    { // Sync the transform buffers
        PROFILE_RANGE("syncGPUTransform");
        _transform.transfer();
    }
}

void GLBackend::renderPassDraw(Batch& batch) {
    _transform._objectsItr = _transform._objectOffsets.begin();
    _transform._camerasItr = _transform._cameraOffsets.begin();
    const size_t numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();
    for (_commandIndex = 0; _commandIndex < numCommands; ++_commandIndex) {
        switch (*command) {
            // Ignore these commands on this pass, taken care of in the transfer pass
            // Note we allow COMMAND_setViewportTransform to occur in both passes
            // as it both updates the transform object (and thus the uniforms in the 
            // UBO) as well as executes the actual viewport call
            case Batch::COMMAND_setModelTransform:
            case Batch::COMMAND_setViewTransform:
            case Batch::COMMAND_setProjectionTransform:
                break;

            default: {
                CommandCall call = _commandCalls[(*command)];
                (this->*(call))(batch, *offset);
                break;
            }
        }

        command++;
        offset++;
    }
}

void GLBackend::render(Batch& batch) {
    // Finalize the batch by moving all the instanced rendering into the command buffer
    batch.preExecute();

    _stereo._skybox = batch.isSkyboxEnabled();
    // Allow the batch to override the rendering stereo settings
    // for things like full framebuffer copy operations (deferred lighting passes)
    bool savedStereo = _stereo._enable;
    if (!batch.isStereoEnabled()) {
        _stereo._enable = false;
    }

    {
        PROFILE_RANGE("Transfer");
        renderPassTransfer(batch);
    }

    {
        PROFILE_RANGE(_stereo._enable ? "LeftRender" : "Render");
        renderPassDraw(batch);
    }

    if (_stereo._enable) {
        PROFILE_RANGE("RightRender");
        _stereo._pass = 1;
        renderPassDraw(batch);
        _stereo._pass = 0;
    }

    // Restore the saved stereo state for the next batch
    _stereo._enable = savedStereo;
}

bool GLBackend::checkGLError(const char* name) {
    GLenum error = glGetError();
    if (!error) {
        return false;
    }
    else {
        switch (error) {
        case GL_INVALID_ENUM:
            qCDebug(gpulogging) << "GLBackend::" << name << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_INVALID_VALUE:
            qCDebug(gpulogging) << "GLBackend" << name << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
            break;
        case GL_INVALID_OPERATION:
            qCDebug(gpulogging) << "GLBackend" << name << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            qCDebug(gpulogging) << "GLBackend" << name << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_OUT_OF_MEMORY:
            qCDebug(gpulogging) << "GLBackend" << name << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
            break;
        case GL_STACK_UNDERFLOW:
            qCDebug(gpulogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
            break;
        case GL_STACK_OVERFLOW:
            qCDebug(gpulogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
            break;
        }
        return true;
    }
}

bool GLBackend::checkGLErrorDebug(const char* name) {
#ifdef DEBUG
    return checkGLError(name);
#else
    Q_UNUSED(name);
    return false;
#endif
}

void GLBackend::syncCache() {
    syncTransformStateCache();
    syncPipelineStateCache();
    syncInputStateCache();
    syncOutputStateCache();

    glEnable(GL_LINE_SMOOTH);
}

void GLBackend::do_draw(Batch& batch, uint32 paramOffset) {
    updateInput();
    updateTransform();
    updatePipeline();

    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 1]._uint;
    uint32 startVertex = batch._params[paramOffset + 0]._uint;
    glDrawArrays(mode, startVertex, numVertices);
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_drawIndexed(Batch& batch, uint32 paramOffset) {
    updateInput();
    updateTransform();
    updatePipeline();

    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numIndices = batch._params[paramOffset + 1]._uint;
    uint32 startIndex = batch._params[paramOffset + 0]._uint;

    GLenum glType = _elementTypeToGLType[_input._indexBufferType];
    
    auto typeByteSize = TYPE_SIZE[_input._indexBufferType];
    GLvoid* indexBufferByteOffset = reinterpret_cast<GLvoid*>(startIndex * typeByteSize + _input._indexBufferOffset);

    glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_drawInstanced(Batch& batch, uint32 paramOffset) {
    updateInput();
    updateTransform();
    updatePipeline();

    GLint numInstances = batch._params[paramOffset + 4]._uint;
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 3]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 2]._uint;
    uint32 startVertex = batch._params[paramOffset + 1]._uint;

    glDrawArraysInstancedARB(mode, startVertex, numVertices, numInstances);
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_drawIndexedInstanced(Batch& batch, uint32 paramOffset) {
    updateInput();
    updateTransform();
    updatePipeline();

    GLint numInstances = batch._params[paramOffset + 4]._uint;
    GLenum mode = _primitiveToGLmode[(Primitive)batch._params[paramOffset + 3]._uint];
    uint32 numIndices = batch._params[paramOffset + 2]._uint;
    uint32 startIndex = batch._params[paramOffset + 1]._uint;
    // FIXME glDrawElementsInstancedBaseVertexBaseInstance is only available in GL 4.3 
    // and higher, so currently we ignore this field
    uint32 startInstance = batch._params[paramOffset + 0]._uint;
    GLenum glType = _elementTypeToGLType[_input._indexBufferType];

    auto typeByteSize = TYPE_SIZE[_input._indexBufferType];
    GLvoid* indexBufferByteOffset = reinterpret_cast<GLvoid*>(startIndex * typeByteSize + _input._indexBufferOffset);
    
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
#else
    glDrawElementsInstanced(mode, numIndices, glType, indexBufferByteOffset, numInstances);
    Q_UNUSED(startInstance); 
#endif
    (void)CHECK_GL_ERROR();
}


void GLBackend::do_multiDrawIndirect(Batch& batch, uint32 paramOffset) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    updateInput();
    updateTransform();
    updatePipeline();

    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = _primitiveToGLmode[(Primitive)batch._params[paramOffset + 1]._uint];

    glMultiDrawArraysIndirect(mode, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, _input._indirectBufferStride);
#else
    // FIXME implement the slow path
#endif
    (void)CHECK_GL_ERROR();

}

void GLBackend::do_multiDrawIndexedIndirect(Batch& batch, uint32 paramOffset) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    updateInput();
    updateTransform();
    updatePipeline();

    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = _primitiveToGLmode[(Primitive)batch._params[paramOffset + 1]._uint];
    GLenum indexType = _elementTypeToGLType[_input._indexBufferType];
  
    glMultiDrawElementsIndirect(mode, indexType, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, _input._indirectBufferStride);
#else
    // FIXME implement the slow path
#endif
    (void)CHECK_GL_ERROR();
}


void GLBackend::do_resetStages(Batch& batch, uint32 paramOffset) {
    resetStages();
}

void GLBackend::do_runLambda(Batch& batch, uint32 paramOffset) {
    std::function<void()> f = batch._lambdas.get(batch._params[paramOffset]._uint);
    f();
}

void GLBackend::resetStages() {
    resetInputStage();
    resetPipelineStage();
    resetTransformStage();
    resetUniformStage();
    resetResourceStage();
    resetOutputStage();
    resetQueryStage();

    (void) CHECK_GL_ERROR();
}

// TODO: As long as we have gl calls explicitely issued from interface
// code, we need to be able to record and batch these calls. THe long 
// term strategy is to get rid of any GL calls in favor of the HIFI GPU API

#define ADD_COMMAND_GL(call) _commands.push_back(COMMAND_##call); _commandOffsets.push_back(_params.size());

//#define DO_IT_NOW(call, offset) runLastCommand();
#define DO_IT_NOW(call, offset) 

void Batch::_glActiveBindTexture(GLenum unit, GLenum target, GLuint texture) {
    // clean the cache on the texture unit we are going to use so the next call to setResourceTexture() at the same slot works fine
    setResourceTexture(unit - GL_TEXTURE0, nullptr);

    ADD_COMMAND_GL(glActiveBindTexture);
    _params.push_back(texture);
    _params.push_back(target);
    _params.push_back(unit);


    DO_IT_NOW(_glActiveBindTexture, 3);
}
void GLBackend::do_glActiveBindTexture(Batch& batch, uint32 paramOffset) {
    glActiveTexture(batch._params[paramOffset + 2]._uint);
    glBindTexture(
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._uint);

    (void) CHECK_GL_ERROR();
}

void Batch::_glUniform1i(GLint location, GLint v0) {
    if (location < 0) {
        return;
    }
    ADD_COMMAND_GL(glUniform1i);
    _params.push_back(v0);
    _params.push_back(location);
    
    DO_IT_NOW(_glUniform1i, 1);
}
void GLBackend::do_glUniform1i(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform1f(
        batch._params[paramOffset + 1]._int,
        batch._params[paramOffset + 0]._int);
    (void) CHECK_GL_ERROR();
}

void Batch::_glUniform1f(GLint location, GLfloat v0) {
    if (location < 0) {
        return;
    }
    ADD_COMMAND_GL(glUniform1f);
    _params.push_back(v0);
    _params.push_back(location);

    DO_IT_NOW(_glUniform1f, 1);
}
void GLBackend::do_glUniform1f(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniform1f(
        batch._params[paramOffset + 1]._int,
        batch._params[paramOffset + 0]._float);
    (void) CHECK_GL_ERROR();
}

void Batch::_glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    ADD_COMMAND_GL(glUniform2f);

    _params.push_back(v1);
    _params.push_back(v0);
    _params.push_back(location);

    DO_IT_NOW(_glUniform2f, 1);
}

void GLBackend::do_glUniform2f(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform2f(
        batch._params[paramOffset + 2]._int,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void) CHECK_GL_ERROR();
}

void Batch::_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    ADD_COMMAND_GL(glUniform3f);

    _params.push_back(v2);
    _params.push_back(v1);
    _params.push_back(v0);
    _params.push_back(location);

    DO_IT_NOW(_glUniform3f, 1);
}

void GLBackend::do_glUniform3f(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform3f(
        batch._params[paramOffset + 3]._int,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void) CHECK_GL_ERROR();
}


void Batch::_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    ADD_COMMAND_GL(glUniform4f);

    _params.push_back(v3);
    _params.push_back(v2);
    _params.push_back(v1);
    _params.push_back(v0);
    _params.push_back(location);

    DO_IT_NOW(_glUniform4f, 1);
}


void GLBackend::do_glUniform4f(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform4f(
        batch._params[paramOffset + 4]._int,
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void)CHECK_GL_ERROR();
}

void Batch::_glUniform3fv(GLint location, GLsizei count, const GLfloat* value) {
    ADD_COMMAND_GL(glUniform3fv);

    const int VEC3_SIZE = 3 * sizeof(float);
    _params.push_back(cacheData(count * VEC3_SIZE, value));
    _params.push_back(count);
    _params.push_back(location);

    DO_IT_NOW(_glUniform3fv, 3);
}
void GLBackend::do_glUniform3fv(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform3fv(
        batch._params[paramOffset + 2]._int,
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.editData(batch._params[paramOffset + 0]._uint));

    (void) CHECK_GL_ERROR();
}


void Batch::_glUniform4fv(GLint location, GLsizei count, const GLfloat* value) {
    ADD_COMMAND_GL(glUniform4fv);

    const int VEC4_SIZE = 4 * sizeof(float);
    _params.push_back(cacheData(count * VEC4_SIZE, value));
    _params.push_back(count);
    _params.push_back(location);

    DO_IT_NOW(_glUniform4fv, 3);
}
void GLBackend::do_glUniform4fv(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    
    GLint location = batch._params[paramOffset + 2]._int;
    GLsizei count = batch._params[paramOffset + 1]._uint;
    const GLfloat* value = (const GLfloat*)batch.editData(batch._params[paramOffset + 0]._uint);
    glUniform4fv(location, count, value);

    (void) CHECK_GL_ERROR();
}

void Batch::_glUniform4iv(GLint location, GLsizei count, const GLint* value) {
    ADD_COMMAND_GL(glUniform4iv);

    const int VEC4_SIZE = 4 * sizeof(int);
    _params.push_back(cacheData(count * VEC4_SIZE, value));
    _params.push_back(count);
    _params.push_back(location);

    DO_IT_NOW(_glUniform4iv, 3);
}
void GLBackend::do_glUniform4iv(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform4iv(
        batch._params[paramOffset + 2]._int,
        batch._params[paramOffset + 1]._uint,
        (const GLint*)batch.editData(batch._params[paramOffset + 0]._uint));

    (void) CHECK_GL_ERROR();
}

void Batch::_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    ADD_COMMAND_GL(glUniformMatrix4fv);

    const int MATRIX4_SIZE = 16 * sizeof(float);
    _params.push_back(cacheData(count * MATRIX4_SIZE, value));
    _params.push_back(transpose);
    _params.push_back(count);
    _params.push_back(location);

    DO_IT_NOW(_glUniformMatrix4fv, 4);
}
void GLBackend::do_glUniformMatrix4fv(Batch& batch, uint32 paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniformMatrix4fv(
        batch._params[paramOffset + 3]._int,
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.editData(batch._params[paramOffset + 0]._uint));
    (void) CHECK_GL_ERROR();
}

void Batch::_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    ADD_COMMAND_GL(glColor4f);

    _params.push_back(alpha);
    _params.push_back(blue);
    _params.push_back(green);
    _params.push_back(red);

    DO_IT_NOW(_glColor4f, 4);
}
void GLBackend::do_glColor4f(Batch& batch, uint32 paramOffset) {

    glm::vec4 newColor(
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float); 

    if (_input._colorAttribute != newColor) {
        _input._colorAttribute = newColor;
        glVertexAttrib4fv(gpu::Stream::COLOR, &_input._colorAttribute.r);
    }
    (void) CHECK_GL_ERROR();
}
