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

#include "../gl41/GL41Backend.h"
#include "../gl45/GL45Backend.h"

#if defined(NSIGHT_FOUND)
#include "nvToolsExt.h"
#endif

#include <shared/GlobalAppProperties.h>
#include <GPUIdent.h>
#include <gl/QOpenGLContextWrapper.h>
#include <QtCore/QProcessEnvironment>

#include "GLTexture.h"
#include "GLShader.h"

using namespace gpu;
using namespace gpu::gl;

static const QString DEBUG_FLAG("HIFI_DISABLE_OPENGL_45");
static bool disableOpenGL45 = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);

static GLBackend* INSTANCE{ nullptr };

BackendPointer GLBackend::createBackend() {
    // FIXME provide a mechanism to override the backend for testing
    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    auto version = QOpenGLContextWrapper::currentContextVersion();
    std::shared_ptr<GLBackend> result;
    if (!disableOpenGL45 && version >= 0x0405) {
        qCDebug(gpugllogging) << "Using OpenGL 4.5 backend";
        result = std::make_shared<gpu::gl45::GL45Backend>();
    } else {
        qCDebug(gpugllogging) << "Using OpenGL 4.1 backend";
        result = std::make_shared<gpu::gl41::GL41Backend>();
    }
    result->initInput();
    result->initTransform();
    result->initTextureManagementStage();

    INSTANCE = result.get();
    void* voidInstance = &(*result);
    qApp->setProperty(hifi::properties::gl::BACKEND, QVariant::fromValue(voidInstance));
    return result;
}

GLBackend& getBackend() {
    if (!INSTANCE) {
        INSTANCE = static_cast<GLBackend*>(qApp->property(hifi::properties::gl::BACKEND).value<void*>());
    }
    return *INSTANCE;
}

bool GLBackend::makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) {
    return GLShader::makeProgram(getBackend(), shader, slotBindings);
}

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
    (&::gpu::gl::GLBackend::do_setResourceBuffer),
    (&::gpu::gl::GLBackend::do_setResourceTexture),

    (&::gpu::gl::GLBackend::do_setFramebuffer),
    (&::gpu::gl::GLBackend::do_clearFramebuffer),
    (&::gpu::gl::GLBackend::do_blit),
    (&::gpu::gl::GLBackend::do_generateTextureMips),

    (&::gpu::gl::GLBackend::do_beginQuery),
    (&::gpu::gl::GLBackend::do_endQuery),
    (&::gpu::gl::GLBackend::do_getQuery),

    (&::gpu::gl::GLBackend::do_resetStages),
    
    (&::gpu::gl::GLBackend::do_disableContextViewCorrection),
    (&::gpu::gl::GLBackend::do_restoreContextViewCorrection),
    (&::gpu::gl::GLBackend::do_disableContextStereo),
    (&::gpu::gl::GLBackend::do_restoreContextStereo),

    (&::gpu::gl::GLBackend::do_runLambda),

    (&::gpu::gl::GLBackend::do_startNamedCall),
    (&::gpu::gl::GLBackend::do_stopNamedCall),

    (&::gpu::gl::GLBackend::do_glUniform1i),
    (&::gpu::gl::GLBackend::do_glUniform1f),
    (&::gpu::gl::GLBackend::do_glUniform2f),
    (&::gpu::gl::GLBackend::do_glUniform3f),
    (&::gpu::gl::GLBackend::do_glUniform4f),
    (&::gpu::gl::GLBackend::do_glUniform3fv),
    (&::gpu::gl::GLBackend::do_glUniform4fv),
    (&::gpu::gl::GLBackend::do_glUniform4iv),
    (&::gpu::gl::GLBackend::do_glUniformMatrix3fv),
    (&::gpu::gl::GLBackend::do_glUniformMatrix4fv),

    (&::gpu::gl::GLBackend::do_glColor4f),

    (&::gpu::gl::GLBackend::do_pushProfileRange),
    (&::gpu::gl::GLBackend::do_popProfileRange),
};

void GLBackend::init() {
    static std::once_flag once;
    std::call_once(once, [] {
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
#if THREADED_TEXTURE_BUFFERING
        // This has to happen on the main thread in order to give the thread 
        // pool a reasonable parent object
        GLVariableAllocationSupport::TransferJob::startBufferingThread();
#endif
    });
}

GLBackend::GLBackend() {
    _pipeline._cameraCorrectionBuffer._buffer->flush();
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &_uboAlignment);
}


GLBackend::~GLBackend() {
    killInput();
    killTransform();
}

void GLBackend::renderPassTransfer(const Batch& batch) {
    const size_t numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();

    _inRenderTransferPass = true;
    { // Sync all the buffers
        PROFILE_RANGE(render_gpu_gl_detail, "syncGPUBuffer");

        for (auto& cached : batch._buffers._items) {
            if (cached._data) {
                syncGPUObject(*cached._data);
            }
        }
    }

    { // Sync all the transform states
        PROFILE_RANGE(render_gpu_gl_detail, "syncCPUTransform");
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

                case Batch::COMMAND_disableContextStereo:
                    _stereo._contextDisable = true;
                    break;

                case Batch::COMMAND_restoreContextStereo:
                    _stereo._contextDisable = false;
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
        PROFILE_RANGE(render_gpu_gl_detail, "syncGPUTransform");
        transferTransformState(batch);
    }

    _inRenderTransferPass = false;
}

void GLBackend::renderPassDraw(const Batch& batch) {
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

void GLBackend::render(const Batch& batch) {
    _transform._skybox = _stereo._skybox = batch.isSkyboxEnabled();
    // Allow the batch to override the rendering stereo settings
    // for things like full framebuffer copy operations (deferred lighting passes)
    bool savedStereo = _stereo._enable;
    if (!batch.isStereoEnabled()) {
        _stereo._enable = false;
    }
    
    {
        PROFILE_RANGE(render_gpu_gl_detail, "Transfer");
        renderPassTransfer(batch);
    }

#ifdef GPU_STEREO_DRAWCALL_INSTANCED
    if (_stereo.isStereo()) {
        glEnable(GL_CLIP_DISTANCE0);
    }
#endif
    {
        PROFILE_RANGE(render_gpu_gl_detail, _stereo.isStereo() ? "Render Stereo" : "Render");
        renderPassDraw(batch);
    }
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
    if (_stereo.isStereo()) {
        glDisable(GL_CLIP_DISTANCE0);
    }
#endif
    // Restore the saved stereo state for the next batch
    _stereo._enable = savedStereo;
}


void GLBackend::syncCache() {
    PROFILE_RANGE(render_gpu_gl_detail, __FUNCTION__);

    syncTransformStateCache();
    syncPipelineStateCache();
    syncInputStateCache();
    syncOutputStateCache();
}

#ifdef GPU_STEREO_DRAWCALL_DOUBLED
void GLBackend::setupStereoSide(int side) {
    ivec4 vp = _transform._viewport;
    vp.z /= 2;
    glViewport(vp.x + side * vp.z, vp.y, vp.z, vp.w);


#ifdef GPU_STEREO_CAMERA_BUFFER
#ifdef GPU_STEREO_DRAWCALL_DOUBLED
    glVertexAttribI1i(14, side);
#endif
#else
    _transform.bindCurrentCamera(side);
#endif

}
#else
#endif

void GLBackend::do_resetStages(const Batch& batch, size_t paramOffset) {
    resetStages();
}

void GLBackend::do_disableContextViewCorrection(const Batch& batch, size_t paramOffset) {
    _transform._viewCorrectionEnabled = false;
}

void GLBackend::do_restoreContextViewCorrection(const Batch& batch, size_t paramOffset) {
    _transform._viewCorrectionEnabled = true;
}

void GLBackend::do_disableContextStereo(const Batch& batch, size_t paramOffset) {

}

void GLBackend::do_restoreContextStereo(const Batch& batch, size_t paramOffset) {

}

void GLBackend::do_runLambda(const Batch& batch, size_t paramOffset) {
    std::function<void()> f = batch._lambdas.get(batch._params[paramOffset]._uint);
    f();
}

void GLBackend::do_startNamedCall(const Batch& batch, size_t paramOffset) {
    batch._currentNamedCall = batch._names.get(batch._params[paramOffset]._uint);
    _currentDraw = -1;
}

void GLBackend::do_stopNamedCall(const Batch& batch, size_t paramOffset) {
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


void GLBackend::do_pushProfileRange(const Batch& batch, size_t paramOffset) {
    if (trace_render_gpu_gl_detail().isDebugEnabled()) {
        auto name = batch._profileRanges.get(batch._params[paramOffset]._uint);
        profileRanges.push_back(name);
#if defined(NSIGHT_FOUND)
        nvtxRangePush(name.c_str());
#endif
    }
}

void GLBackend::do_popProfileRange(const Batch& batch, size_t paramOffset) {
    if (trace_render_gpu_gl_detail().isDebugEnabled()) {
        profileRanges.pop_back();
#if defined(NSIGHT_FOUND)
        nvtxRangePop();
#endif
    }
}

// TODO: As long as we have gl calls explicitely issued from interface
// code, we need to be able to record and batch these calls. THe long 
// term strategy is to get rid of any GL calls in favor of the HIFI GPU API

// As long as we don;t use several versions of shaders we can avoid this more complex code path
#ifdef GPU_STEREO_CAMERA_BUFFER
#define GET_UNIFORM_LOCATION(shaderUniformLoc) ((_pipeline._programShader) ? _pipeline._programShader->getUniformLocation(shaderUniformLoc, (GLShader::Version) isStereo()) : -1)
#else
#define GET_UNIFORM_LOCATION(shaderUniformLoc) shaderUniformLoc
#endif

void GLBackend::do_glUniform1i(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniform1i(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 1]._int),
        batch._params[paramOffset + 0]._int);
    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniform1f(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniform1f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 1]._int),
        batch._params[paramOffset + 0]._float);
    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniform2f(const Batch& batch, size_t paramOffset) {
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
    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniform3f(const Batch& batch, size_t paramOffset) {
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
    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniform4f(const Batch& batch, size_t paramOffset) {
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

void GLBackend::do_glUniform3fv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform3fv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint));

    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniform4fv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    GLint location = GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int);
    GLsizei count = batch._params[paramOffset + 1]._uint;
    const GLfloat* value = (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint);
    glUniform4fv(location, count, value);

    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniform4iv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform4iv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
        batch._params[paramOffset + 1]._uint,
        (const GLint*)batch.readData(batch._params[paramOffset + 0]._uint));

    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniformMatrix3fv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniformMatrix3fv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 3]._int),
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint));
    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glUniformMatrix4fv(const Batch& batch, size_t paramOffset) {
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
        (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint));
    (void)CHECK_GL_ERROR();
}

void GLBackend::do_glColor4f(const Batch& batch, size_t paramOffset) {

    glm::vec4 newColor(
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);

    if (_input._colorAttribute != newColor) {
        _input._colorAttribute = newColor;
        glVertexAttrib4fv(gpu::Stream::COLOR, &_input._colorAttribute.r);
    }
    (void)CHECK_GL_ERROR();
}

void GLBackend::releaseBuffer(GLuint id, Size size) const {
    Lock lock(_trashMutex);
    _buffersTrash.push_back({ id, size });
}

void GLBackend::releaseExternalTexture(GLuint id, const Texture::ExternalRecycler& recycler) const {
    Lock lock(_trashMutex);
    _externalTexturesTrash.push_back({ id, recycler });
}

void GLBackend::releaseTexture(GLuint id, Size size) const {
    Lock lock(_trashMutex);
    _texturesTrash.push_back({ id, size });
}

void GLBackend::releaseFramebuffer(GLuint id) const {
    Lock lock(_trashMutex);
    _framebuffersTrash.push_back(id);
}

void GLBackend::releaseShader(GLuint id) const {
    Lock lock(_trashMutex);
    _shadersTrash.push_back(id);
}

void GLBackend::releaseProgram(GLuint id) const {
    Lock lock(_trashMutex);
    _programsTrash.push_back(id);
}

void GLBackend::releaseQuery(GLuint id) const {
    Lock lock(_trashMutex);
    _queriesTrash.push_back(id);
}

void GLBackend::queueLambda(const std::function<void()> lambda) const {
    Lock lock(_trashMutex);
    _lambdaQueue.push_back(lambda);
}

void GLBackend::recycle() const {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__)
    {
        std::list<std::function<void()>> lamdbasTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_lambdaQueue, lamdbasTrash);
        }
        for (auto lambda : lamdbasTrash) {
            lambda();
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<std::pair<GLuint, Size>> buffersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_buffersTrash, buffersTrash);
        }
        ids.reserve(buffersTrash.size());
        for (auto pair : buffersTrash) {
            ids.push_back(pair.first);
        }
        if (!ids.empty()) {
            glDeleteBuffers((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<GLuint> framebuffersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_framebuffersTrash, framebuffersTrash);
        }
        ids.reserve(framebuffersTrash.size());
        for (auto id : framebuffersTrash) {
            ids.push_back(id);
        }
        if (!ids.empty()) {
            glDeleteFramebuffers((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<std::pair<GLuint, Size>> texturesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_texturesTrash, texturesTrash);
        }
        ids.reserve(texturesTrash.size());
        for (auto pair : texturesTrash) {
            ids.push_back(pair.first);
        }
        if (!ids.empty()) {
            glDeleteTextures((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::list<std::pair<GLuint, Texture::ExternalRecycler>> externalTexturesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_externalTexturesTrash, externalTexturesTrash);
        }
        if (!externalTexturesTrash.empty()) {
            std::vector<GLsync> fences;  
            fences.resize(externalTexturesTrash.size());
            for (size_t i = 0; i < externalTexturesTrash.size(); ++i) {
                fences[i] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            }
            // External texture fences will be read in another thread/context, so we need a flush
            glFlush();
            size_t index = 0;
            for (auto pair : externalTexturesTrash) {
                auto fence = fences[index++];
                pair.second(pair.first, fence);
            }
        }
    }

    {
        std::list<GLuint> programsTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_programsTrash, programsTrash);
        }
        for (auto id : programsTrash) {
            glDeleteProgram(id);
        }
    }

    {
        std::list<GLuint> shadersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_shadersTrash, shadersTrash);
        }
        for (auto id : shadersTrash) {
            glDeleteShader(id);
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<GLuint> queriesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_queriesTrash, queriesTrash);
        }
        ids.reserve(queriesTrash.size());
        for (auto id : queriesTrash) {
            ids.push_back(id);
        }
        if (!ids.empty()) {
            glDeleteQueries((GLsizei)ids.size(), ids.data());
        }
    }

    GLVariableAllocationSupport::manageMemory();
    GLVariableAllocationSupport::_frameTexturesCreated = 0;
    Texture::KtxStorage::releaseOpenKtxFiles();
}

void GLBackend::setCameraCorrection(const Mat4& correction) {
    _transform._correction.correction = correction;
    _transform._correction.correctionInverse = glm::inverse(correction);
    _pipeline._cameraCorrectionBuffer._buffer->setSubData(0, _transform._correction);
    _pipeline._cameraCorrectionBuffer._buffer->flush();
}
