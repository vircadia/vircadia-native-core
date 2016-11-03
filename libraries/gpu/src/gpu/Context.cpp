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
#include "GPULogging.h"
using namespace gpu;

Context::CreateBackend Context::_createBackendCallback = nullptr;
Context::MakeProgram Context::_makeProgramCallback = nullptr;
std::once_flag Context::_initialized;

Context::Context() {
    if (_createBackendCallback) {
        _backend = _createBackendCallback();
    }
}

Context::Context(const Context& context) {
}

Context::~Context() {
}

void Context::beginFrame(const glm::mat4& renderPose) {
    assert(!_frameActive);
    _frameActive = true;
    _currentFrame = std::make_shared<Frame>();
    _currentFrame->pose = renderPose;
}

void Context::appendFrameBatch(Batch& batch) {
    if (!_frameActive) {
        qWarning() << "Batch executed outside of frame boundaries";
        return;
    }
    _currentFrame->batches.push_back(batch);
}

FramePointer Context::endFrame() {
    assert(_frameActive);
    auto result = _currentFrame;
    _currentFrame.reset();
    _frameActive = false;

    result->stereoState = _stereo;
    result->finish();
    return result;
}

void Context::executeBatch(Batch& batch) const {
    batch.flush();
    _backend->render(batch);
}

void Context::recycle() const {
    _backend->recycle();
}

void Context::consumeFrameUpdates(const FramePointer& frame) const {
    frame->preRender();
}

void Context::executeFrame(const FramePointer& frame) const {
    // FIXME? probably not necessary, but safe
    consumeFrameUpdates(frame);
    _backend->setStereoState(frame->stereoState);
    {
        // Execute the frame rendering commands
        for (auto& batch : frame->batches) {
            _backend->render(batch);
        }
    }
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
std::atomic<Size> Context::_freeGPUMemory { 0 };
std::atomic<uint32_t> Context::_fenceCount { 0 };
std::atomic<uint32_t> Context::_bufferGPUCount { 0 };
std::atomic<Buffer::Size> Context::_bufferGPUMemoryUsage { 0 };

std::atomic<uint32_t> Context::_textureGPUCount{ 0 };
std::atomic<uint32_t> Context::_textureGPUSparseCount { 0 };
std::atomic<Texture::Size> Context::_textureGPUMemoryUsage { 0 };
std::atomic<Texture::Size> Context::_textureGPUVirtualMemoryUsage { 0 };
std::atomic<Texture::Size> Context::_textureGPUFramebufferMemoryUsage { 0 };
std::atomic<Texture::Size> Context::_textureGPUSparseMemoryUsage { 0 };
std::atomic<uint32_t> Context::_textureGPUTransferCount { 0 };

void Context::setFreeGPUMemory(Size size) {
    _freeGPUMemory.store(size);
}

Size Context::getFreeGPUMemory() {
    return _freeGPUMemory.load();
}

void Context::incrementBufferGPUCount() {
    static std::atomic<uint32_t> max { 0 };
    auto total = ++_bufferGPUCount;
    if (total > max.load()) {
        max = total;
        qCDebug(gpulogging) << "New max GPU buffers " << total;
    }
}
void Context::decrementBufferGPUCount() {
    --_bufferGPUCount;
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

void Context::incrementFenceCount() {
    static std::atomic<uint32_t> max { 0 };
    auto total = ++_fenceCount;
    if (total > max.load()) {
        max = total;
        qCDebug(gpulogging) << "New max Fences " << total;
    }
}
void Context::decrementFenceCount() {
    --_fenceCount;
}

void Context::incrementTextureGPUCount() {
    static std::atomic<uint32_t> max { 0 };
    auto total = ++_textureGPUCount;
    if (total > max.load()) {
        max = total;
        qCDebug(gpulogging) << "New max GPU textures " << total;
    }
}
void Context::decrementTextureGPUCount() {
    --_textureGPUCount;
}

void Context::incrementTextureGPUSparseCount() {
    static std::atomic<uint32_t> max { 0 };
    auto total = ++_textureGPUSparseCount;
    if (total > max.load()) {
        max = total;
        qCDebug(gpulogging) << "New max GPU textures " << total;
    }
}
void Context::decrementTextureGPUSparseCount() {
    --_textureGPUSparseCount;
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

void Context::updateTextureGPUFramebufferMemoryUsage(Size prevObjectSize, Size newObjectSize) {
    if (prevObjectSize == newObjectSize) {
        return;
    }
    if (newObjectSize > prevObjectSize) {
        _textureGPUFramebufferMemoryUsage.fetch_add(newObjectSize - prevObjectSize);
    } else {
        _textureGPUFramebufferMemoryUsage.fetch_sub(prevObjectSize - newObjectSize);
    }
}

void Context::updateTextureGPUSparseMemoryUsage(Size prevObjectSize, Size newObjectSize) {
    if (prevObjectSize == newObjectSize) {
        return;
    }
    if (newObjectSize > prevObjectSize) {
        _textureGPUSparseMemoryUsage.fetch_add(newObjectSize - prevObjectSize);
    } else {
        _textureGPUSparseMemoryUsage.fetch_sub(prevObjectSize - newObjectSize);
    }
}

void Context::incrementTextureGPUTransferCount() {
    static std::atomic<uint32_t> max { 0 };
    auto total = ++_textureGPUTransferCount;
    if (total > max.load()) {
        max = total;
        qCDebug(gpulogging) << "New max GPU textures transfers" << total;
    }
}

void Context::decrementTextureGPUTransferCount() {
    --_textureGPUTransferCount;
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

uint32_t Context::getTextureGPUSparseCount() {
    return _textureGPUSparseCount.load();
}

Context::Size Context::getTextureGPUMemoryUsage() {
    return _textureGPUMemoryUsage.load();
}

Context::Size Context::getTextureGPUVirtualMemoryUsage() {
    return _textureGPUVirtualMemoryUsage.load();
}

Context::Size Context::getTextureGPUFramebufferMemoryUsage() {
    return _textureGPUFramebufferMemoryUsage.load();
}

Context::Size Context::getTextureGPUSparseMemoryUsage() {
    return _textureGPUSparseMemoryUsage.load();
}

uint32_t Context::getTextureGPUTransferCount() {
    return _textureGPUTransferCount.load();
}

void Backend::setFreeGPUMemory(Size size) { Context::setFreeGPUMemory(size); }
Resource::Size Backend::getFreeGPUMemory() { return Context::getFreeGPUMemory(); }
void Backend::incrementBufferGPUCount() { Context::incrementBufferGPUCount(); }
void Backend::decrementBufferGPUCount() { Context::decrementBufferGPUCount(); }
void Backend::updateBufferGPUMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateBufferGPUMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::incrementTextureGPUCount() { Context::incrementTextureGPUCount(); }
void Backend::decrementTextureGPUCount() { Context::decrementTextureGPUCount(); }
void Backend::incrementTextureGPUSparseCount() { Context::incrementTextureGPUSparseCount(); }
void Backend::decrementTextureGPUSparseCount() { Context::decrementTextureGPUSparseCount(); }
void Backend::updateTextureGPUMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateTextureGPUMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::updateTextureGPUVirtualMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateTextureGPUVirtualMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::updateTextureGPUFramebufferMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateTextureGPUFramebufferMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::updateTextureGPUSparseMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize) { Context::updateTextureGPUSparseMemoryUsage(prevObjectSize, newObjectSize); }
void Backend::incrementTextureGPUTransferCount() { Context::incrementTextureGPUTransferCount(); }
void Backend::decrementTextureGPUTransferCount() { Context::decrementTextureGPUTransferCount(); }
