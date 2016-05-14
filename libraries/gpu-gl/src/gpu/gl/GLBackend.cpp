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
#include "GLBackend.h"

#include <mutex>
#include <queue>
#include <list>
#include <functional>
#include <glm/gtc/type_ptr.hpp>

#if defined(NSIGHT_FOUND)
#include "nvToolsExt.h"
#endif

#include <GPUIdent.h>
#include <NumericalConstants.h>
#include "GLBackendShared.h"

using namespace gpu;
using namespace gpu::gl;

GLBackend::CommandCall GLBackend::_commandCalls[Batch::NUM_COMMANDS] = 
{
    (&::gpu::gl::GLBackend::do_draw),
    (&::gpu::gl::GLBackend::do_drawIndexed),
    (&::gpu::gl::GLBackend::do_drawInstanced),
    (&::gpu::gl::GLBackend::do_drawIndexedInstanced),
    (&::gpu::gl::GLBackend::do_multiDrawIndirect),
    (&::gpu::gl::GLBackend::do_multiDrawIndexedIndirect),

    (&::gpu::gl::GLBackend::do_setInputFormat),
    (&::gpu::gl::GLBackend::do_setInputBuffer),
    (&::gpu::gl::GLBackend::do_setIndexBuffer),
    (&::gpu::gl::GLBackend::do_setIndirectBuffer),

    (&::gpu::gl::GLBackend::do_setModelTransform),
    (&::gpu::gl::GLBackend::do_setViewTransform),
    (&::gpu::gl::GLBackend::do_setProjectionTransform),
    (&::gpu::gl::GLBackend::do_setViewportTransform),
    (&::gpu::gl::GLBackend::do_setDepthRangeTransform),

    (&::gpu::gl::GLBackend::do_setPipeline),
    (&::gpu::gl::GLBackend::do_setStateBlendFactor),
    (&::gpu::gl::GLBackend::do_setStateScissorRect),

    (&::gpu::gl::GLBackend::do_setUniformBuffer),
    (&::gpu::gl::GLBackend::do_setResourceTexture),

    (&::gpu::gl::GLBackend::do_setFramebuffer),
    (&::gpu::gl::GLBackend::do_clearFramebuffer),
    (&::gpu::gl::GLBackend::do_blit),
    (&::gpu::gl::GLBackend::do_generateTextureMips),

    (&::gpu::gl::GLBackend::do_beginQuery),
    (&::gpu::gl::GLBackend::do_endQuery),
    (&::gpu::gl::GLBackend::do_getQuery),

    (&::gpu::gl::GLBackend::do_resetStages),

    (&::gpu::gl::GLBackend::do_runLambda),

    (&::gpu::gl::GLBackend::do_startNamedCall),
    (&::gpu::gl::GLBackend::do_stopNamedCall),

    (&::gpu::gl::GLBackend::do_glActiveBindTexture),

    (&::gpu::gl::GLBackend::do_glUniform1i),
    (&::gpu::gl::GLBackend::do_glUniform1f),
    (&::gpu::gl::GLBackend::do_glUniform2f),
    (&::gpu::gl::GLBackend::do_glUniform3f),
    (&::gpu::gl::GLBackend::do_glUniform4f),
    (&::gpu::gl::GLBackend::do_glUniform3fv),
    (&::gpu::gl::GLBackend::do_glUniform4fv),
    (&::gpu::gl::GLBackend::do_glUniform4iv),
    (&::gpu::gl::GLBackend::do_glUniformMatrix4fv),

    (&::gpu::gl::GLBackend::do_glColor4f),

    (&::gpu::gl::GLBackend::do_pushProfileRange),
    (&::gpu::gl::GLBackend::do_popProfileRange),
};

extern std::function<uint32(const Texture& texture)> TEXTURE_ID_RESOLVER;

void GLBackend::init() {
    static std::once_flag once;
    std::call_once(once, [] {

        TEXTURE_ID_RESOLVER = [](const Texture& texture)->uint32 {
            auto object = Backend::getGPUObject<GLBackend::GLTexture>(texture);
            if (!object) {
                return 0;
            }
            
            if (object->getSyncState() != GLTexture::Idle) {
                if (object->_downsampleSource) {
                    return object->_downsampleSource->_texture;
                }
                return 0;
            }
            return object->_texture;
        };

        QString vendor{ (const char*)glGetString(GL_VENDOR) };
        QString renderer{ (const char*)glGetString(GL_RENDERER) };
        qCDebug(gpugllogging) << "GL Version: " << QString((const char*) glGetString(GL_VERSION));
        qCDebug(gpugllogging) << "GL Shader Language Version: " << QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
        qCDebug(gpugllogging) << "GL Vendor: " << vendor;
        qCDebug(gpugllogging) << "GL Renderer: " << renderer;
        GPUIdent* gpu = GPUIdent::getInstance(vendor, renderer); 
        // From here on, GPUIdent::getInstance()->getMumble() should efficiently give the same answers.
        qCDebug(gpugllogging) << "GPU:";
        qCDebug(gpugllogging) << "\tcard:" << gpu->getName();
        qCDebug(gpugllogging) << "\tdriver:" << gpu->getDriver();
        qCDebug(gpugllogging) << "\tdedicated memory:" << gpu->getMemory() << "MB";

        glewExperimental = true;
        GLenum err = glewInit();
        glGetError(); // clear the potential error from glewExperimental
        if (GLEW_OK != err) {
            // glewInit failed, something is seriously wrong.
            qCDebug(gpugllogging, "Error: %s\n", glewGetErrorString(err));
        }
        qCDebug(gpugllogging, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

#if defined(Q_OS_WIN)
        if (wglewGetExtension("WGL_EXT_swap_control")) {
            int swapInterval = wglGetSwapIntervalEXT();
            qCDebug(gpugllogging, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
        }
#endif

#if defined(Q_OS_LINUX)
        // TODO: Write the correct  code for Linux...
        /* if (wglewGetExtension("WGL_EXT_swap_control")) {
            int swapInterval = wglGetSwapIntervalEXT();
            qCDebug(gpugllogging, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
        }*/
#endif
    });
}

Context::Size GLBackend::getDedicatedMemory() {
    static Context::Size dedicatedMemory { 0 };
    static std::once_flag once;
    std::call_once(once, [&] {
#ifdef Q_OS_WIN
        if (!dedicatedMemory && wglGetGPUIDsAMD && wglGetGPUInfoAMD) {
            UINT maxCount = wglGetGPUIDsAMD(0, 0);
            std::vector<UINT> ids;
            ids.resize(maxCount);
            wglGetGPUIDsAMD(maxCount, &ids[0]);
            GLuint memTotal;
            wglGetGPUInfoAMD(ids[0], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(GLuint), &memTotal);
            dedicatedMemory = MB_TO_BYTES(memTotal);
        }
#endif

        if (!dedicatedMemory) {
            GLint atiGpuMemory[4];
            // not really total memory, but close enough if called early enough in the application lifecycle
            glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, atiGpuMemory);
            if (GL_NO_ERROR == glGetError()) {
                dedicatedMemory = KB_TO_BYTES(atiGpuMemory[0]);
            }
        }

        if (!dedicatedMemory) {
            GLint nvGpuMemory { 0 };
            glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &nvGpuMemory);
            if (GL_NO_ERROR == glGetError()) {
                dedicatedMemory = KB_TO_BYTES(nvGpuMemory);
            }
        }

        if (!dedicatedMemory) {
            auto gpuIdent = GPUIdent::getInstance();
            if (gpuIdent && gpuIdent->isValid()) {
                dedicatedMemory = MB_TO_BYTES(gpuIdent->getMemory());
            }
        }
    });

    return dedicatedMemory;
}

Backend* GLBackend::createBackend() {
    return new GLBackend();
}

GLBackend::GLBackend() {
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &_uboAlignment);
    initInput();
    initTransform();
    initTextureTransferHelper();
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

    _inRenderTransferPass = true;
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
        _transform._cameras.clear();
        _transform._cameraOffsets.clear();

        for (_commandIndex = 0; _commandIndex < numCommands; ++_commandIndex) {
            switch (*command) {
                case Batch::COMMAND_draw:
                case Batch::COMMAND_drawIndexed:
                case Batch::COMMAND_drawInstanced:
                case Batch::COMMAND_drawIndexedInstanced:
                case Batch::COMMAND_multiDrawIndirect:
                case Batch::COMMAND_multiDrawIndexedIndirect:
                    _transform.preUpdate(_commandIndex, _stereo);
                    break;

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
        _transform.transfer(batch);
    }

    _inRenderTransferPass = false;
}

void GLBackend::renderPassDraw(Batch& batch) {
    _currentDraw = -1;
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

            case Batch::COMMAND_draw:
            case Batch::COMMAND_drawIndexed:
            case Batch::COMMAND_drawInstanced:
            case Batch::COMMAND_drawIndexedInstanced:
            case Batch::COMMAND_multiDrawIndirect:
            case Batch::COMMAND_multiDrawIndexedIndirect: {
                // updates for draw calls
                ++_currentDraw;
                updateInput();
                updateTransform(batch);
                updatePipeline();
                
                CommandCall call = _commandCalls[(*command)];
                (this->*(call))(batch, *offset);
                break;
            }
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
        PROFILE_RANGE(_stereo._enable ? "Render Stereo" : "Render");
        renderPassDraw(batch);
    }

    // Restore the saved stereo state for the next batch
    _stereo._enable = savedStereo;
}


void GLBackend::syncCache() {
    syncTransformStateCache();
    syncPipelineStateCache();
    syncInputStateCache();
    syncOutputStateCache();

    glEnable(GL_LINE_SMOOTH);
}

void GLBackend::setupStereoSide(int side) {
    ivec4 vp = _transform._viewport;
    vp.z /= 2;
    glViewport(vp.x + side * vp.z, vp.y, vp.z, vp.w);

    _transform.bindCurrentCamera(side);
}


void GLBackend::do_draw(Batch& batch, size_t paramOffset) {
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 1]._uint;
    uint32 startVertex = batch._params[paramOffset + 0]._uint;

    if (isStereo()) {
        setupStereoSide(0);
        glDrawArrays(mode, startVertex, numVertices);
        setupStereoSide(1);
        glDrawArrays(mode, startVertex, numVertices);

        _stats._DSNumTriangles += 2 * numVertices / 3;
        _stats._DSNumDrawcalls += 2;

    } else {
        glDrawArrays(mode, startVertex, numVertices);
        _stats._DSNumTriangles += numVertices / 3;
        _stats._DSNumDrawcalls++;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void GLBackend::do_drawIndexed(Batch& batch, size_t paramOffset) {
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numIndices = batch._params[paramOffset + 1]._uint;
    uint32 startIndex = batch._params[paramOffset + 0]._uint;

    GLenum glType = _elementTypeToGLType[_input._indexBufferType];
    
    auto typeByteSize = TYPE_SIZE[_input._indexBufferType];
    GLvoid* indexBufferByteOffset = reinterpret_cast<GLvoid*>(startIndex * typeByteSize + _input._indexBufferOffset);

    if (isStereo()) {
        setupStereoSide(0);
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
        setupStereoSide(1);
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);

        _stats._DSNumTriangles += 2 * numIndices / 3;
        _stats._DSNumDrawcalls += 2;
    } else {
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
        _stats._DSNumTriangles += numIndices / 3;
        _stats._DSNumDrawcalls++;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void GLBackend::do_drawInstanced(Batch& batch, size_t paramOffset) {
    GLint numInstances = batch._params[paramOffset + 4]._uint;
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 3]._uint;
    GLenum mode = _primitiveToGLmode[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 2]._uint;
    uint32 startVertex = batch._params[paramOffset + 1]._uint;


    if (isStereo()) {
        GLint trueNumInstances = 2 * numInstances;

        setupStereoSide(0);
        glDrawArraysInstancedARB(mode, startVertex, numVertices, numInstances);
        setupStereoSide(1);
        glDrawArraysInstancedARB(mode, startVertex, numVertices, numInstances);

        _stats._DSNumTriangles += (trueNumInstances * numVertices) / 3;
        _stats._DSNumDrawcalls += trueNumInstances;
    } else {
        glDrawArraysInstancedARB(mode, startVertex, numVertices, numInstances);
        _stats._DSNumTriangles += (numInstances * numVertices) / 3;
        _stats._DSNumDrawcalls += numInstances;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void glbackend_glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount, GLint basevertex, GLuint baseinstance) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, primcount, basevertex, baseinstance);
#else
    glDrawElementsInstanced(mode, count, type, indices, primcount);
#endif
}

void GLBackend::do_drawIndexedInstanced(Batch& batch, size_t paramOffset) {
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
 
    if (isStereo()) {
        GLint trueNumInstances = 2 * numInstances;

        setupStereoSide(0);
        glbackend_glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
        setupStereoSide(1);
        glbackend_glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);

        _stats._DSNumTriangles += (trueNumInstances * numIndices) / 3;
        _stats._DSNumDrawcalls += trueNumInstances;
    } else {
        glbackend_glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
        _stats._DSNumTriangles += (numInstances * numIndices) / 3;
        _stats._DSNumDrawcalls += numInstances;
    }

    _stats._DSNumAPIDrawcalls++;

    (void)CHECK_GL_ERROR();
}


void GLBackend::do_multiDrawIndirect(Batch& batch, size_t paramOffset) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = _primitiveToGLmode[(Primitive)batch._params[paramOffset + 1]._uint];

    glMultiDrawArraysIndirect(mode, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, (GLsizei)_input._indirectBufferStride);
    _stats._DSNumDrawcalls += commandCount;
    _stats._DSNumAPIDrawcalls++;

#else
    // FIXME implement the slow path
#endif
    (void)CHECK_GL_ERROR();

}

void GLBackend::do_multiDrawIndexedIndirect(Batch& batch, size_t paramOffset) {
#if (GPU_INPUT_PROFILE == GPU_CORE_43)
    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = _primitiveToGLmode[(Primitive)batch._params[paramOffset + 1]._uint];
    GLenum indexType = _elementTypeToGLType[_input._indexBufferType];
  
    glMultiDrawElementsIndirect(mode, indexType, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, (GLsizei)_input._indirectBufferStride);
    _stats._DSNumDrawcalls += commandCount;
    _stats._DSNumAPIDrawcalls++;
#else
    // FIXME implement the slow path
#endif
    (void)CHECK_GL_ERROR();
}


void GLBackend::do_resetStages(Batch& batch, size_t paramOffset) {
    resetStages();
}

void GLBackend::do_runLambda(Batch& batch, size_t paramOffset) {
    std::function<void()> f = batch._lambdas.get(batch._params[paramOffset]._uint);
    f();
}

void GLBackend::do_startNamedCall(Batch& batch, size_t paramOffset) {
    batch._currentNamedCall = batch._names.get(batch._params[paramOffset]._uint);
    _currentDraw = -1;
}

void GLBackend::do_stopNamedCall(Batch& batch, size_t paramOffset) {
    batch._currentNamedCall.clear();
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

// As long as we don;t use several versions of shaders we can avoid this more complex code path
// #define GET_UNIFORM_LOCATION(shaderUniformLoc) _pipeline._programShader->getUniformLocation(shaderUniformLoc, isStereo());
#define GET_UNIFORM_LOCATION(shaderUniformLoc) shaderUniformLoc

void Batch::_glActiveBindTexture(GLenum unit, GLenum target, GLuint texture) {
    // clean the cache on the texture unit we are going to use so the next call to setResourceTexture() at the same slot works fine
    setResourceTexture(unit - GL_TEXTURE0, nullptr);

    ADD_COMMAND_GL(glActiveBindTexture);
    _params.push_back(texture);
    _params.push_back(target);
    _params.push_back(unit);
}
void GLBackend::do_glActiveBindTexture(Batch& batch, size_t paramOffset) {
    glActiveTexture(batch._params[paramOffset + 2]._uint);
    glBindTexture(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 1]._uint),
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
}

void GLBackend::do_glUniform1i(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniform1f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 1]._int),
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
}
void GLBackend::do_glUniform1f(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniform1f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 1]._int),
        batch._params[paramOffset + 0]._float);
    (void) CHECK_GL_ERROR();
}

void Batch::_glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    ADD_COMMAND_GL(glUniform2f);

    _params.push_back(v1);
    _params.push_back(v0);
    _params.push_back(location);
}

void GLBackend::do_glUniform2f(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform2f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
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
}

void GLBackend::do_glUniform3f(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform3f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 3]._int),
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
}


void GLBackend::do_glUniform4f(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform4f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 4]._int),
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
}
void GLBackend::do_glUniform3fv(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform3fv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
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
}
void GLBackend::do_glUniform4fv(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    
    GLint location = GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int);
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
}
void GLBackend::do_glUniform4iv(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform4iv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
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
}
void GLBackend::do_glUniformMatrix4fv(Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniformMatrix4fv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 3]._int),
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
}
void GLBackend::do_glColor4f(Batch& batch, size_t paramOffset) {

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


void GLBackend::do_pushProfileRange(Batch& batch, size_t paramOffset) {
#if defined(NSIGHT_FOUND)
    auto name = batch._profileRanges.get(batch._params[paramOffset]._uint);
    nvtxRangePush(name.c_str());
#endif
}

void GLBackend::do_popProfileRange(Batch& batch, size_t paramOffset) {
#if defined(NSIGHT_FOUND)
    nvtxRangePop();
#endif
}
