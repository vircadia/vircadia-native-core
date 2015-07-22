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
#include "GPULogging.h"
#include "GLBackendShared.h"
#include <glm/gtc/type_ptr.hpp>

using namespace gpu;

GLBackend::CommandCall GLBackend::_commandCalls[Batch::NUM_COMMANDS] = 
{
    (&::gpu::GLBackend::do_draw),
    (&::gpu::GLBackend::do_drawIndexed),
    (&::gpu::GLBackend::do_drawInstanced),
    (&::gpu::GLBackend::do_drawIndexedInstanced),
    (&::gpu::GLBackend::do_clearFramebuffer),
    
    (&::gpu::GLBackend::do_setInputFormat),
    (&::gpu::GLBackend::do_setInputBuffer),
    (&::gpu::GLBackend::do_setIndexBuffer),

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
    (&::gpu::GLBackend::do_blit),

    (&::gpu::GLBackend::do_beginQuery),
    (&::gpu::GLBackend::do_endQuery),
    (&::gpu::GLBackend::do_getQuery),

    (&::gpu::GLBackend::do_glEnable),
    (&::gpu::GLBackend::do_glDisable),

    (&::gpu::GLBackend::do_glEnableClientState),
    (&::gpu::GLBackend::do_glDisableClientState),

    (&::gpu::GLBackend::do_glCullFace),
    (&::gpu::GLBackend::do_glAlphaFunc),

    (&::gpu::GLBackend::do_glDepthFunc),
    (&::gpu::GLBackend::do_glDepthMask),
    (&::gpu::GLBackend::do_glDepthRange),

    (&::gpu::GLBackend::do_glBindBuffer),

    (&::gpu::GLBackend::do_glBindTexture),
    (&::gpu::GLBackend::do_glActiveTexture),
    (&::gpu::GLBackend::do_glTexParameteri),

    (&::gpu::GLBackend::do_glDrawBuffers),

    (&::gpu::GLBackend::do_glUseProgram),
    (&::gpu::GLBackend::do_glUniform1i),
    (&::gpu::GLBackend::do_glUniform1f),
    (&::gpu::GLBackend::do_glUniform2f),
    (&::gpu::GLBackend::do_glUniform3f),
    (&::gpu::GLBackend::do_glUniform3fv),
    (&::gpu::GLBackend::do_glUniform4fv),
    (&::gpu::GLBackend::do_glUniform4iv),
    (&::gpu::GLBackend::do_glUniformMatrix4fv),

    (&::gpu::GLBackend::do_glEnableVertexAttribArray),
    (&::gpu::GLBackend::do_glDisableVertexAttribArray),
    
    (&::gpu::GLBackend::do_glColor4f),
    (&::gpu::GLBackend::do_glLineWidth),
};

GLBackend::GLBackend() :
    _input(),
    _transform(),
    _pipeline(),
    _output()
{
    initInput();
    initTransform();
}

GLBackend::~GLBackend() {
    killInput();
    killTransform();
}

void GLBackend::render(Batch& batch) {

    uint32 numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();

    for (unsigned int i = 0; i < numCommands; i++) {
        CommandCall call = _commandCalls[(*command)];
        (this->*(call))(batch, *offset);
        command++;
        offset++;
    }
}

void GLBackend::renderBatch(Batch& batch, bool syncCache) {
    GLBackend backend;
    if (syncCache) {
        backend.syncCache();
    }
    backend.render(batch);
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

    glDrawElements(mode, numIndices, glType, reinterpret_cast<GLvoid*>(startIndex + _input._indexBufferOffset));
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
    (void) CHECK_GL_ERROR();
}

void GLBackend::do_clearFramebuffer(Batch& batch, uint32 paramOffset) {

    uint32 masks = batch._params[paramOffset + 7]._uint;
    Vec4 color;
    color.x = batch._params[paramOffset + 6]._float;
    color.y = batch._params[paramOffset + 5]._float;
    color.z = batch._params[paramOffset + 4]._float;
    color.w = batch._params[paramOffset + 3]._float;
    float depth = batch._params[paramOffset + 2]._float;
    int stencil = batch._params[paramOffset + 1]._int;
    int useScissor = batch._params[paramOffset + 0]._int;

    GLuint glmask = 0;
    if (masks & Framebuffer::BUFFER_STENCIL) {
        glClearStencil(stencil);
        glmask |= GL_STENCIL_BUFFER_BIT;
    }

    if (masks & Framebuffer::BUFFER_DEPTH) {
        glClearDepth(depth);
        glmask |= GL_DEPTH_BUFFER_BIT;
    } 

    std::vector<GLenum> drawBuffers;
    if (masks & Framebuffer::BUFFER_COLORS) {
        for (unsigned int i = 0; i < Framebuffer::MAX_NUM_RENDER_BUFFERS; i++) {
            if (masks & (1 << i)) {
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
            }
        }

        if (!drawBuffers.empty()) {
            glDrawBuffers(drawBuffers.size(), drawBuffers.data());
            glClearColor(color.x, color.y, color.z, color.w);
            glmask |= GL_COLOR_BUFFER_BIT;
        }
    }

    // Apply scissor if needed and if not already on
    bool doEnableScissor = (useScissor && (!_pipeline._stateCache.scissorEnable));
    if (doEnableScissor) {
        glEnable(GL_SCISSOR_TEST);
    }

    glClear(glmask);

    // Restore scissor if needed
    if (doEnableScissor) {
        glDisable(GL_SCISSOR_TEST);
    }

    // Restore the color draw buffers only if a frmaebuffer is bound
    if (_output._framebuffer && !drawBuffers.empty()) {
        auto glFramebuffer = syncGPUObject(*_output._framebuffer);
        if (glFramebuffer) {
            glDrawBuffers(glFramebuffer->_colorBuffers.size(), glFramebuffer->_colorBuffers.data());
        }
    }

    (void) CHECK_GL_ERROR();
}

// TODO: As long as we have gl calls explicitely issued from interface
// code, we need to be able to record and batch these calls. THe long 
// term strategy is to get rid of any GL calls in favor of the HIFI GPU API

#define ADD_COMMAND_GL(call) _commands.push_back(COMMAND_##call); _commandOffsets.push_back(_params.size());

//#define DO_IT_NOW(call, offset) runLastCommand();
#define DO_IT_NOW(call, offset) 


void Batch::_glEnable(GLenum cap) {
    ADD_COMMAND_GL(glEnable);

    _params.push_back(cap);

    DO_IT_NOW(_glEnable, 1);
}
void GLBackend::do_glEnable(Batch& batch, uint32 paramOffset) {
    glEnable(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glDisable(GLenum cap) {
    ADD_COMMAND_GL(glDisable);

    _params.push_back(cap);

    DO_IT_NOW(_glDisable, 1);
}
void GLBackend::do_glDisable(Batch& batch, uint32 paramOffset) {
    glDisable(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glEnableClientState(GLenum array) {
    ADD_COMMAND_GL(glEnableClientState);

    _params.push_back(array);

    DO_IT_NOW(_glEnableClientState, 1);
}
void GLBackend::do_glEnableClientState(Batch& batch, uint32 paramOffset) {
    glEnableClientState(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glDisableClientState(GLenum array) {
    ADD_COMMAND_GL(glDisableClientState);

    _params.push_back(array);

    DO_IT_NOW(_glDisableClientState, 1);
}
void GLBackend::do_glDisableClientState(Batch& batch, uint32 paramOffset) {
    glDisableClientState(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glCullFace(GLenum mode) {
    ADD_COMMAND_GL(glCullFace);

    _params.push_back(mode);

    DO_IT_NOW(_glCullFace, 1);
}
void GLBackend::do_glCullFace(Batch& batch, uint32 paramOffset) {
    glCullFace(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glAlphaFunc(GLenum func, GLclampf ref) {
    ADD_COMMAND_GL(glAlphaFunc);

    _params.push_back(ref);
    _params.push_back(func);

    DO_IT_NOW(_glAlphaFunc, 2);
}
void GLBackend::do_glAlphaFunc(Batch& batch, uint32 paramOffset) {
    glAlphaFunc(
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._float);
    (void) CHECK_GL_ERROR();
}

void Batch::_glDepthFunc(GLenum func) {
    ADD_COMMAND_GL(glDepthFunc);

    _params.push_back(func);

    DO_IT_NOW(_glDepthFunc, 1);
}
void GLBackend::do_glDepthFunc(Batch& batch, uint32 paramOffset) {
    glDepthFunc(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glDepthMask(GLboolean flag) {
    ADD_COMMAND_GL(glDepthMask);

    _params.push_back(flag);

    DO_IT_NOW(_glDepthMask, 1);
}
void GLBackend::do_glDepthMask(Batch& batch, uint32 paramOffset) {
    glDepthMask(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glDepthRange(GLfloat zNear, GLfloat zFar) {
    ADD_COMMAND_GL(glDepthRange);

    _params.push_back(zFar);
    _params.push_back(zNear);

    DO_IT_NOW(_glDepthRange, 2);
}
void GLBackend::do_glDepthRange(Batch& batch, uint32 paramOffset) {
    glDepthRange(
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void) CHECK_GL_ERROR();
}

void Batch::_glBindBuffer(GLenum target, GLuint buffer) {
    ADD_COMMAND_GL(glBindBuffer);

    _params.push_back(buffer);
    _params.push_back(target);

    DO_IT_NOW(_glBindBuffer, 2);
}
void GLBackend::do_glBindBuffer(Batch& batch, uint32 paramOffset) {
    glBindBuffer(
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glBindTexture(GLenum target, GLuint texture) {
    ADD_COMMAND_GL(glBindTexture);

    _params.push_back(texture);
    _params.push_back(target);

    DO_IT_NOW(_glBindTexture, 2);
}
void GLBackend::do_glBindTexture(Batch& batch, uint32 paramOffset) {
    glBindTexture(
        batch._params[paramOffset + 1]._uint,
        batch._params[paramOffset + 0]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glActiveTexture(GLenum texture) {
    ADD_COMMAND_GL(glActiveTexture);

    _params.push_back(texture);

    DO_IT_NOW(_glActiveTexture, 1);
}
void GLBackend::do_glActiveTexture(Batch& batch, uint32 paramOffset) {
    glActiveTexture(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glTexParameteri(GLenum target, GLenum pname, GLint param) {
    ADD_COMMAND_GL(glTexParameteri);
    
    _params.push_back(param);
    _params.push_back(pname);
    _params.push_back(target);
    
    DO_IT_NOW(glTexParameteri, 3);
}
void GLBackend::do_glTexParameteri(Batch& batch, uint32 paramOffset) {
    glTexParameteri(batch._params[paramOffset + 2]._uint,
                    batch._params[paramOffset + 1]._uint,
                    batch._params[paramOffset + 0]._int);
    (void) CHECK_GL_ERROR();
}

void Batch::_glDrawBuffers(GLsizei n, const GLenum* bufs) {
    ADD_COMMAND_GL(glDrawBuffers);

    _params.push_back(cacheData(n * sizeof(GLenum), bufs));
    _params.push_back(n);

    DO_IT_NOW(_glDrawBuffers, 2);
}
void GLBackend::do_glDrawBuffers(Batch& batch, uint32 paramOffset) {
    glDrawBuffers(
        batch._params[paramOffset + 1]._uint,
        (const GLenum*)batch.editData(batch._params[paramOffset + 0]._uint));
    (void) CHECK_GL_ERROR();
}

void Batch::_glUseProgram(GLuint program) {
    ADD_COMMAND_GL(glUseProgram);

    _params.push_back(program);

    DO_IT_NOW(_glUseProgram, 1);
}
void GLBackend::do_glUseProgram(Batch& batch, uint32 paramOffset) {
    
    _pipeline._program = batch._params[paramOffset]._uint;
    // for this call we still want to execute the glUseProgram in the order of the glCOmmand to avoid any issue
    _pipeline._invalidProgram = false;
    glUseProgram(_pipeline._program);

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

void Batch::_glEnableVertexAttribArray(GLint location) {
    ADD_COMMAND_GL(glEnableVertexAttribArray);

    _params.push_back(location);

    DO_IT_NOW(_glEnableVertexAttribArray, 1);
}
void GLBackend::do_glEnableVertexAttribArray(Batch& batch, uint32 paramOffset) {
    glEnableVertexAttribArray(batch._params[paramOffset]._uint);
    (void) CHECK_GL_ERROR();
}

void Batch::_glDisableVertexAttribArray(GLint location) {
    ADD_COMMAND_GL(glDisableVertexAttribArray);

    _params.push_back(location);

    DO_IT_NOW(_glDisableVertexAttribArray, 1);
}
void GLBackend::do_glDisableVertexAttribArray(Batch& batch, uint32 paramOffset) {
    glDisableVertexAttribArray(batch._params[paramOffset]._uint);
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
    glColor4f(
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void) CHECK_GL_ERROR();
}

void Batch::_glLineWidth(GLfloat width) {
    ADD_COMMAND_GL(glLineWidth);
    
    _params.push_back(width);
    
    DO_IT_NOW(_glLineWidth, 1);
}
void GLBackend::do_glLineWidth(Batch& batch, uint32 paramOffset) {
    glLineWidth(batch._params[paramOffset]._float);
    (void) CHECK_GL_ERROR();
}
