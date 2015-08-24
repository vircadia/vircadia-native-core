//
//  Created by Bradley Austin Davis on 2015/08/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_Forward_h
#define hifi_gpu_Forward_h

namespace gpu {
    class Batch;
    class Backend;
    class Context;
    typedef std::shared_ptr<Context> ContextPointer;
    class GPUObject;

    typedef int Stamp;
    typedef uint32_t uint32;
    typedef int32_t int32;
    typedef uint16_t uint16;
    typedef int16_t int16;
    typedef uint8_t uint8;
    typedef int8_t int8;

    typedef uint8 Byte;
    typedef uint32 Offset;
    typedef std::vector<Offset> Offsets;

    typedef glm::mat4 Mat4;
    typedef glm::mat3 Mat3;
    typedef glm::vec4 Vec4;
    typedef glm::ivec4 Vec4i;
    typedef glm::vec3 Vec3;
    typedef glm::vec2 Vec2;
    typedef glm::ivec2 Vec2i;
    typedef glm::uvec2 Vec2u;

    class Element;
    typedef Element Format;
    class Swapchain;
    typedef std::shared_ptr<Swapchain> SwapchainPointer;
    class Framebuffer;
    typedef std::shared_ptr<Framebuffer> FramebufferPointer;
    class Pipeline;
    typedef std::shared_ptr<Pipeline> PipelinePointer;
    typedef std::vector<PipelinePointer> Pipelines;
    class Query;
    typedef std::shared_ptr<Query> QueryPointer;
    typedef std::vector<QueryPointer> Queries;
    class Resource;
    class Buffer;
    typedef std::shared_ptr<Buffer> BufferPointer;
    typedef std::vector<BufferPointer> Buffers;
    class BufferView;
    class Shader;
    typedef Shader::Pointer ShaderPointer;
    typedef std::vector<ShaderPointer> Shaders;
    class State;
    typedef std::shared_ptr<State> StatePointer;
    typedef std::vector<StatePointer> States;
    class Stream;
    class BufferStream;
    typedef std::shared_ptr<BufferStream> BufferStreamPointer;
    class Texture;
    class SphericalHarmonics;
    typedef std::shared_ptr<SphericalHarmonics> SHPointer;
    class Sampler;
    class Texture;
    typedef std::shared_ptr<Texture> TexturePointer;
    typedef std::vector<TexturePointer> Textures;
    class TextureView;
    typedef std::vector<TextureView> TextureViews;
}

#endif
