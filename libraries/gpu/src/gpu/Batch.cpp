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
#include "GPUConfig.h"

#include <QDebug>

#include <GLMHelpers.h>


#if defined(NSIGHT_FOUND)
#include "nvToolsExt.h"

ProfileRange::ProfileRange(const char *name) {
    nvtxRangePush(name);
}
ProfileRange::~ProfileRange() {
    nvtxRangePop();
}
#endif

#define ADD_COMMAND(call) _commands.push_back(COMMAND_##call); _commandOffsets.push_back(_params.size());

using namespace gpu;

Batch::Batch() :
    _commands(),
    _commandOffsets(),
    _params(),
    _data(),
    _buffers(),
    _textures(),
    _streamFormats(),
    _transforms(),
    _pipelines(),
    _framebuffers()
{
}

Batch::~Batch() {
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

uint32 Batch::cacheData(uint32 size, const void* data) {
    uint32 offset = _data.size();
    uint32 nbBytes = size;
    _data.resize(offset + nbBytes);
    memcpy(_data.data() + offset, data, size);

    return offset;
}

void Batch::draw(Primitive primitiveType, uint32 nbVertices, uint32 startVertex) {
    ADD_COMMAND(draw);

    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
}

void Batch::drawIndexed(Primitive primitiveType, uint32 nbIndices, uint32 startIndex) {
    ADD_COMMAND(drawIndexed);

    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
}

void Batch::drawInstanced(uint32 nbInstances, Primitive primitiveType, uint32 nbVertices, uint32 startVertex, uint32 startInstance) {
    ADD_COMMAND(drawInstanced);

    _params.push_back(startInstance);
    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}

void Batch::drawIndexedInstanced(uint32 nbInstances, Primitive primitiveType, uint32 nbIndices, uint32 startIndex, uint32 startInstance) {
    ADD_COMMAND(drawIndexedInstanced);

    _params.push_back(startInstance);
    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
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

void Batch::setModelTransform(const Transform& model) {
    ADD_COMMAND(setModelTransform);

    _params.push_back(_transforms.cache(model));
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

    _params.push_back(rect.x);
    _params.push_back(rect.y);
    _params.push_back(rect.z);
    _params.push_back(rect.w);
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

void push_back(Batch::Params& params, const vec3& v) {
    params.push_back(v.x);
    params.push_back(v.y);
    params.push_back(v.z);
}

void push_back(Batch::Params& params, const vec4& v) {
    params.push_back(v.x);
    params.push_back(v.y);
    params.push_back(v.z);
    params.push_back(v.a);
}
