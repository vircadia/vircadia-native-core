//
//  Batch.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Batch.h"

#include <QDebug>
#include <string.h>

#if defined(NSIGHT_FOUND)
#include "nvToolsExt.h"

ProfileRangeBatch::ProfileRangeBatch(gpu::Batch& batch, const char *name) : _batch(batch) {
    _batch.pushProfileRange(name);
}

ProfileRangeBatch::~ProfileRangeBatch() {
    _batch.popProfileRange();
}
#endif

#define ADD_COMMAND(call) _commands.push_back(COMMAND_##call); _commandOffsets.push_back(_params.size());

using namespace gpu;

Batch::Batch(const CacheState& cacheState) : Batch() {
    _commands.reserve(cacheState.commandsSize);
    _commandOffsets.reserve(cacheState.offsetsSize);
    _params.reserve(cacheState.paramsSize);
    _data.reserve(cacheState.dataSize);
}

Batch::CacheState Batch::getCacheState() {
    return CacheState(_commands.size(), _commandOffsets.size(), _params.size(), _data.size(),
                _buffers.size(), _textures.size(), _streamFormats.size(), _transforms.size(), _pipelines.size(), 
                _framebuffers.size(), _queries.size());
}

Batch::~Batch() {
    //qDebug() << "Batch::~Batch()... " << getCacheState();
}

void Batch::clear() {
    _commands.clear();
    _commandOffsets.clear();
    _params.clear();
    _data.clear();
    _buffers.clear();
    _textures.clear();
    _streamFormats.clear();
    _transforms.clear();
    _pipelines.clear();
    _framebuffers.clear();
}

size_t Batch::cacheData(size_t size, const void* data) {
    size_t offset = _data.size();
    size_t numBytes = size;
    _data.resize(offset + numBytes);
    memcpy(_data.data() + offset, data, size);

    return offset;
}

void Batch::draw(Primitive primitiveType, uint32 numVertices, uint32 startVertex) {
    ADD_COMMAND(draw);

    _params.push_back(startVertex);
    _params.push_back(numVertices);
    _params.push_back(primitiveType);

    captureDrawCallInfo();
}

void Batch::drawIndexed(Primitive primitiveType, uint32 numIndices, uint32 startIndex) {
    ADD_COMMAND(drawIndexed);

    _params.push_back(startIndex);
    _params.push_back(numIndices);
    _params.push_back(primitiveType);

    captureDrawCallInfo();
}

void Batch::drawInstanced(uint32 numInstances, Primitive primitiveType, uint32 numVertices, uint32 startVertex, uint32 startInstance) {
    ADD_COMMAND(drawInstanced);

    _params.push_back(startInstance);
    _params.push_back(startVertex);
    _params.push_back(numVertices);
    _params.push_back(primitiveType);
    _params.push_back(numInstances);

    captureDrawCallInfo();
}

void Batch::drawIndexedInstanced(uint32 numInstances, Primitive primitiveType, uint32 numIndices, uint32 startIndex, uint32 startInstance) {
    ADD_COMMAND(drawIndexedInstanced);

    _params.push_back(startInstance);
    _params.push_back(startIndex);
    _params.push_back(numIndices);
    _params.push_back(primitiveType);
    _params.push_back(numInstances);

    captureDrawCallInfo();
}


void Batch::multiDrawIndirect(uint32 numCommands, Primitive primitiveType) {
    ADD_COMMAND(multiDrawIndirect);
    _params.push_back(numCommands);
    _params.push_back(primitiveType);

    captureDrawCallInfo();
}

void Batch::multiDrawIndexedIndirect(uint32 nbCommands, Primitive primitiveType) {
    ADD_COMMAND(multiDrawIndexedIndirect);
    _params.push_back(nbCommands);
    _params.push_back(primitiveType);

    captureDrawCallInfo();
}

void Batch::setInputFormat(const Stream::FormatPointer& format) {
    ADD_COMMAND(setInputFormat);

    _params.push_back(_streamFormats.cache(format));
}

void Batch::setInputBuffer(Slot channel, const BufferPointer& buffer, Offset offset, Offset stride) {
    ADD_COMMAND(setInputBuffer);

    _params.push_back(stride);
    _params.push_back(offset);
    _params.push_back(_buffers.cache(buffer));
    _params.push_back(channel);
}

void Batch::setInputBuffer(Slot channel, const BufferView& view) {
    setInputBuffer(channel, view._buffer, view._offset, Offset(view._stride));
}

void Batch::setInputStream(Slot startChannel, const BufferStream& stream) {
    if (stream.getNumBuffers()) {
        const Buffers& buffers = stream.getBuffers();
        const Offsets& offsets = stream.getOffsets();
        const Offsets& strides = stream.getStrides();
        for (unsigned int i = 0; i < buffers.size(); i++) {
            setInputBuffer(startChannel + i, buffers[i], offsets[i], strides[i]);
        }
    }
}

void Batch::setIndexBuffer(Type type, const BufferPointer& buffer, Offset offset) {
    ADD_COMMAND(setIndexBuffer);

    _params.push_back(offset);
    _params.push_back(_buffers.cache(buffer));
    _params.push_back(type);
}

void Batch::setIndexBuffer(const BufferView& buffer) {
    setIndexBuffer(buffer._element.getType(), buffer._buffer, buffer._offset);
}

void Batch::setIndirectBuffer(const BufferPointer& buffer, Offset offset, Offset stride) {
    ADD_COMMAND(setIndirectBuffer);

    _params.push_back(_buffers.cache(buffer));
    _params.push_back(offset);
    _params.push_back(stride);
}


void Batch::setModelTransform(const Transform& model) {
    ADD_COMMAND(setModelTransform);

    _currentModel = model;
    _invalidModel = true;
}

void Batch::setViewTransform(const Transform& view) {
    ADD_COMMAND(setViewTransform);

    _params.push_back(_transforms.cache(view));
}

void Batch::setProjectionTransform(const Mat4& proj) {
    ADD_COMMAND(setProjectionTransform);

    _params.push_back(cacheData(sizeof(Mat4), &proj));
}

void Batch::setViewportTransform(const Vec4i& viewport) {
    ADD_COMMAND(setViewportTransform);

    _params.push_back(cacheData(sizeof(Vec4i), &viewport));
}

void Batch::setDepthRangeTransform(float nearDepth, float farDepth) {
    ADD_COMMAND(setDepthRangeTransform);

    _params.push_back(farDepth);
    _params.push_back(nearDepth);
}

void Batch::setPipeline(const PipelinePointer& pipeline) {
    ADD_COMMAND(setPipeline);

    _params.push_back(_pipelines.cache(pipeline));
}

void Batch::setStateBlendFactor(const Vec4& factor) {
    ADD_COMMAND(setStateBlendFactor);

    _params.push_back(factor.x);
    _params.push_back(factor.y);
    _params.push_back(factor.z);
    _params.push_back(factor.w);
}

void Batch::setStateScissorRect(const Vec4i& rect) {
    ADD_COMMAND(setStateScissorRect);

    _params.push_back(cacheData(sizeof(Vec4i), &rect));
}

void Batch::setUniformBuffer(uint32 slot, const BufferPointer& buffer, Offset offset, Offset size) {
    ADD_COMMAND(setUniformBuffer);

    _params.push_back(size);
    _params.push_back(offset);
    _params.push_back(_buffers.cache(buffer));
    _params.push_back(slot);
}

void Batch::setUniformBuffer(uint32 slot, const BufferView& view) {
    setUniformBuffer(slot, view._buffer, view._offset, view._size);
}


void Batch::setResourceTexture(uint32 slot, const TexturePointer& texture) {
    ADD_COMMAND(setResourceTexture);

    _params.push_back(_textures.cache(texture));
    _params.push_back(slot);
}

void Batch::setResourceTexture(uint32 slot, const TextureView& view) {
    setResourceTexture(slot, view._texture);
}

void Batch::setFramebuffer(const FramebufferPointer& framebuffer) {
    ADD_COMMAND(setFramebuffer);

    _params.push_back(_framebuffers.cache(framebuffer));

}

void Batch::clearFramebuffer(Framebuffer::Masks targets, const Vec4& color, float depth, int stencil, bool enableScissor) {
    ADD_COMMAND(clearFramebuffer);

    _params.push_back(enableScissor);
    _params.push_back(stencil);
    _params.push_back(depth);
    _params.push_back(color.w);
    _params.push_back(color.z);
    _params.push_back(color.y);
    _params.push_back(color.x);
    _params.push_back(targets);
}

void Batch::clearColorFramebuffer(Framebuffer::Masks targets, const Vec4& color, bool enableScissor) {
    clearFramebuffer(targets & Framebuffer::BUFFER_COLORS, color, 1.0f, 0, enableScissor);
}

void Batch::clearDepthFramebuffer(float depth, bool enableScissor) {
    clearFramebuffer(Framebuffer::BUFFER_DEPTH, Vec4(0.0f), depth, 0, enableScissor);
}

void Batch::clearStencilFramebuffer(int stencil, bool enableScissor) {
    clearFramebuffer(Framebuffer::BUFFER_STENCIL, Vec4(0.0f), 1.0f, stencil, enableScissor);
}

void Batch::clearDepthStencilFramebuffer(float depth, int stencil, bool enableScissor) {
    clearFramebuffer(Framebuffer::BUFFER_DEPTHSTENCIL, Vec4(0.0f), depth, stencil, enableScissor);
}

void Batch::blit(const FramebufferPointer& src, const Vec4i& srcViewport,
    const FramebufferPointer& dst, const Vec4i& dstViewport) {
    ADD_COMMAND(blit);

    _params.push_back(_framebuffers.cache(src));
    _params.push_back(srcViewport.x);
    _params.push_back(srcViewport.y);
    _params.push_back(srcViewport.z);
    _params.push_back(srcViewport.w);
    _params.push_back(_framebuffers.cache(dst));
    _params.push_back(dstViewport.x);
    _params.push_back(dstViewport.y);
    _params.push_back(dstViewport.z);
    _params.push_back(dstViewport.w);
}

void Batch::generateTextureMips(const TexturePointer& texture) {
    ADD_COMMAND(generateTextureMips);

    _params.push_back(_textures.cache(texture));
}

void Batch::beginQuery(const QueryPointer& query) {
    ADD_COMMAND(beginQuery);

    _params.push_back(_queries.cache(query));
}

void Batch::endQuery(const QueryPointer& query) {
    ADD_COMMAND(endQuery);

    _params.push_back(_queries.cache(query));
}

void Batch::getQuery(const QueryPointer& query) {
    ADD_COMMAND(getQuery);

    _params.push_back(_queries.cache(query));
}

void Batch::resetStages() {
    ADD_COMMAND(resetStages);
}

void Batch::runLambda(std::function<void()> f) {
    ADD_COMMAND(runLambda);
    _params.push_back(_lambdas.cache(f));
}

void Batch::startNamedCall(const std::string& name) {
    ADD_COMMAND(startNamedCall);
    _params.push_back(_names.cache(name));
    
    _currentNamedCall = name;
}

void Batch::stopNamedCall() {
    ADD_COMMAND(stopNamedCall);

    _currentNamedCall.clear();
}

void Batch::enableStereo(bool enable) {
    _enableStereo = enable;
}

bool Batch::isStereoEnabled() const {
    return _enableStereo;
}

void Batch::enableSkybox(bool enable) {
    _enableSkybox = enable;
}

bool Batch::isSkyboxEnabled() const {
    return _enableSkybox;
}

void Batch::setupNamedCalls(const std::string& instanceName, NamedBatchData::Function function) {
    NamedBatchData& instance = _namedData[instanceName];
    if (!instance.function) {
        instance.function = function;
    }

    captureNamedDrawCallInfo(instanceName);
}

BufferPointer Batch::getNamedBuffer(const std::string& instanceName, uint8_t index) {
    NamedBatchData& instance = _namedData[instanceName];
    if (instance.buffers.size() <= index) {
        instance.buffers.resize(index + 1);
    }
    if (!instance.buffers[index]) {
        instance.buffers[index] = std::make_shared<Buffer>();
    }
    return instance.buffers[index];
}

Batch::DrawCallInfoBuffer& Batch::getDrawCallInfoBuffer() {
    if (_currentNamedCall.empty()) {
        return _drawCallInfos;
    } else {
        return _namedData[_currentNamedCall].drawCallInfos;
    }
}

const Batch::DrawCallInfoBuffer& Batch::getDrawCallInfoBuffer() const {
    if (_currentNamedCall.empty()) {
        return _drawCallInfos;
    } else {
        auto it = _namedData.find(_currentNamedCall);
        Q_ASSERT_X(it != _namedData.end(), Q_FUNC_INFO, (_currentNamedCall + " not in _namedData.").data());
        return it->second.drawCallInfos;
    }
}

void Batch::captureDrawCallInfoImpl() {
    if (_invalidModel) {
        TransformObject object;
        _currentModel.getMatrix(object._model);

        // FIXME - we don't want to be using glm::inverse() here but it fixes the flickering issue we are 
        // seeing with planky blocks in toybox. Our implementation of getInverseMatrix() is buggy in cases
        // of non-uniform scale. We need to fix that. In the mean time, glm::inverse() works.
        //_model.getInverseMatrix(_object._modelInverse);
        object._modelInverse = glm::inverse(object._model);

        _objects.push_back(object);

        // Flag is clean
        _invalidModel = false;
    }

    auto& drawCallInfos = getDrawCallInfoBuffer();
    drawCallInfos.push_back((uint16)_objects.size() - 1);
}

void Batch::captureDrawCallInfo() {
    if (!_currentNamedCall.empty()) {
        // If we are processing a named call, we don't want to register the raw draw calls
        return;
    }

    captureDrawCallInfoImpl();
}

void Batch::captureNamedDrawCallInfo(std::string name) {
    std::swap(_currentNamedCall, name); // Set and save _currentNamedCall
    captureDrawCallInfoImpl();
    std::swap(_currentNamedCall, name); // Restore _currentNamedCall
}

void Batch::preExecute() {
    for (auto& mapItem : _namedData) {
        auto& name = mapItem.first;
        auto& instance = mapItem.second;

        startNamedCall(name);
        instance.process(*this);
        stopNamedCall();
    }
}

QDebug& operator<<(QDebug& debug, const Batch::CacheState& cacheState) {
    debug << "Batch::CacheState[ "
        << "commandsSize:" << cacheState.commandsSize
        << "offsetsSize:" << cacheState.offsetsSize
        << "paramsSize:" << cacheState.paramsSize
        << "dataSize:" << cacheState.dataSize
        << "]";

    return debug;
}

// Debugging
void Batch::pushProfileRange(const char* name) {
#if defined(NSIGHT_FOUND)
    ADD_COMMAND(pushProfileRange);
    _params.push_back(_profileRanges.cache(name));
#endif
}

void Batch::popProfileRange() {
#if defined(NSIGHT_FOUND)
    ADD_COMMAND(popProfileRange);
#endif
}
