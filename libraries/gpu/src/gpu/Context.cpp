//
//  Context.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Context.h"
#include "Frame.h"
using namespace gpu;

Context::CreateBackend Context::_createBackendCallback = nullptr;
Context::MakeProgram Context::_makeProgramCallback = nullptr;
std::once_flag Context::_initialized;

Context::Context() {
    if (_createBackendCallback) {
        _backend.reset(_createBackendCallback());
    }

    _frameHandler = [this](Frame& frame){
        for (size_t i = 0; i < frame.batches.size(); ++i) {
            _backend->_stereo = frame.stereoStates[i];
            _backend->render(frame.batches[i]);
        }
    };
}

Context::Context(const Context& context) {
}

Context::~Context() {
}

void Context::setFrameHandler(FrameHandler handler) {
    _frameHandler = handler;
}

#define DEFERRED_RENDERING

void Context::beginFrame(const FramebufferPointer& outputFramebuffer, const glm::mat4& renderPose) {
    _currentFrame = Frame();
    _currentFrame.framebuffer = outputFramebuffer;
    _currentFrame.pose = renderPose;
    _frameActive = true;
}

void Context::append(Batch& batch) {
    if (!_frameActive) {
        qWarning() << "Batch executed outside of frame boundaries";
    }
#ifdef DEFERRED_RENDERING
    _currentFrame.batches.emplace_back(batch);
    _currentFrame.stereoStates.emplace_back(_stereo);
#else
    _backend->_stereo = _stereo;
    _backend->render(batch);
#endif
}

void Context::endFrame() {
#ifdef DEFERRED_RENDERING
    if (_frameHandler) {
        _frameHandler(_currentFrame);
    }
#endif
    _currentFrame = Frame();
    _frameActive = false;
}


bool Context::makeProgram(Shader& shader, const Shader::BindingSet& bindings) {
    if (shader.isProgram() && _makeProgramCallback) {
        return _makeProgramCallback(shader, bindings);
    }
    return false;
}

void Context::enableStereo(bool enable) {
    _stereo._enable = enable;
}

bool Context::isStereo() {
    return _stereo._enable;
}

void Context::setStereoProjections(const mat4 eyeProjections[2]) {
    for (int i = 0; i < 2; ++i) {
        _stereo._eyeProjections[i] = eyeProjections[i];
    }
}

void Context::setStereoViews(const mat4 views[2]) {
    for (int i = 0; i < 2; ++i) {
        _stereo._eyeViews[i] = views[i];
    }
}

void Context::getStereoProjections(mat4* eyeProjections) const {
    for (int i = 0; i < 2; ++i) {
        eyeProjections[i] = _stereo._eyeProjections[i];
    }
}

void Context::getStereoViews(mat4* eyeViews) const {
    for (int i = 0; i < 2; ++i) {
        eyeViews[i] = _stereo._eyeViews[i];
    }
}

void Context::syncCache() {
    PROFILE_RANGE(__FUNCTION__);
    _backend->syncCache();
}

void Context::downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) {
    _backend->downloadFramebuffer(srcFramebuffer, region, destImage);
}


void Context::getStats(ContextStats& stats) const {
    _backend->getStats(stats);
}

const Backend::TransformCamera& Backend::TransformCamera::recomputeDerived(const Transform& xformView) const {
    _projectionInverse = glm::inverse(_projection);

    // Get the viewEyeToWorld matrix form the transformView as passed to the gpu::Batch
    // this is the "_viewInverse" fed to the shader
    // Genetrate the "_view" matrix as well from the xform
    xformView.getMatrix(_viewInverse);
    _view = glm::inverse(_viewInverse);

    Mat4 viewUntranslated = _view;
    viewUntranslated[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    _projectionViewUntranslated = _projection * viewUntranslated;

    _stereoInfo = Vec4(0.0f);

    return *this;
}

Backend::TransformCamera Backend::TransformCamera::getEyeCamera(int eye, const StereoState& _stereo, const Transform& xformView) const {
    TransformCamera result = *this;
    Transform offsetTransform = xformView;
    if (!_stereo._skybox) {
        offsetTransform.postTranslate(-Vec3(_stereo._eyeViews[eye][3]));
    } else {
        // FIXME: If "skybox" the ipd is set to 0 for now, let s try to propose a better solution for this in the future
    }
    result._projection = _stereo._eyeProjections[eye];
    result.recomputeDerived(offsetTransform);

    result._stereoInfo = Vec4(1.0f, (float)eye, 0.0f, 0.0f);

    return result;
}

// Counters for Buffer and Texture usage in GPU/Context
std::atomic<uint32_t> Context::_bufferGPUCount{ 0 };
std::atomic<Buffer::Size> Context::_bufferGPUMemoryUsage{ 0 };

std::atomic<uint32_t> Context::_textureGPUCount{ 0 };
std::atomic<Texture::Size> Context::_textureGPUMemoryUsage{ 0 };
std::atomic<Texture::Size> Context::_textureGPUVirtualMemoryUsage{ 0 };
std::atomic<uint32_t> Context::_textureGPUTransferCount{ 0 };

void Context::incrementBufferGPUCount() {
    _bufferGPUCount++;
}
void Context::decrementBufferGPUCount() {
    _bufferGPUCount--;
}
void Context::updateBufferGPUMemoryUsage(Size prevObjectSize, Size newObjectSize) {
    if (prevObjectSize == newObjectSize) {
        return;
    }
    if (newObjectSize > prevObjectSize) {
        _bufferGPUMemoryUsage.fetch_add(newObjectSize - prevObjectSize);
    } else {
        _bufferGPUMemoryUsage.fetch_sub(prevObjectSize - newObjectSize);
    }
}

void Context::incrementTextureGPUCount() {
    _textureGPUCount++;
}
void Context::decrementTextureGPUCount() {
    _textureGPUCount--;
}
void Context::updateTextureGPUMemoryUsage(Size prevObjectSize, Size newObjectSize) {
    if (prevObjectSize == newObjectSize) {
        return;
    }
    if (newObjectSize > prevObjectSize) {
        _textureGPUMemoryUsage.fetch_add(newObjectSize - prevObjectSize);
    } else {
        _textureGPUMemoryUsage.fetch_sub(prevObjectSize - newObjectSize);
    }
}

void Context::updateTextureGPUVirtualMemoryUsage(Size prevObjectSize, Size newObjectSize) {
    if (prevObjectSize == newObjectSize) {
        return;
    }
    if (newObjectSize > prevObjectSize) {
        _textureGPUVirtualMemoryUsage.fetch_add(newObjectSize - prevObjectSize);
    } else {
        _textureGPUVirtualMemoryUsage.fetch_sub(prevObjectSize - newObjectSize);
    }
}

void Context::incrementTextureGPUTransferCount() {
    _textureGPUTransferCount++;
}
void Context::decrementTextureGPUTransferCount() {
    _textureGPUTransferCount--;
}

uint32_t Context::getBufferGPUCount() {
    return _bufferGPUCount.load();
}

Context::Size Context::getBufferGPUMemoryUsage() {
    return _bufferGPUMemoryUsage.load();
}

uint32_t Context::getTextureGPUCount() {
    return _textureGPUCount.load();
}

Context::Size Context::getTextureGPUMemoryUsage() {
    return _textureGPUMemoryUsage.load();
}

Context::Size Context::getTextureGPUVirtualMemoryUsage() {
    return _textureGPUVirtualMemoryUsage.load();
}

uint32_t Context::getTextureGPUTransferCount() {
    return _textureGPUTransferCount.load();
}

void Backend::incrementBufferGPUCount() { Context::incrementBufferGPUCount(); }
void Backend::decrementBufferGPUCount() { Context::decrementBufferGPUCount(); }
void Backend::updateBufferGPUMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateBufferGPUMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::incrementTextureGPUCount() { Context::incrementTextureGPUCount(); }
void Backend::decrementTextureGPUCount() { Context::decrementTextureGPUCount(); }
void Backend::updateTextureGPUMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateTextureGPUMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::updateTextureGPUVirtualMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateTextureGPUVirtualMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::incrementTextureGPUTransferCount() { Context::incrementTextureGPUTransferCount(); }
void Backend::decrementTextureGPUTransferCount() { Context::decrementTextureGPUTransferCount(); }
