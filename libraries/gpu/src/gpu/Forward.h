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

#include <stdint.h>
#include <memory>
#include <vector>
#include <functional>

#include <glm/glm.hpp>

namespace gpu {
    class Batch;
    class Backend;
    class Context;
    using ContextPointer = std::shared_ptr<Context>;
    class GPUObject;
    class Frame;
    using FramePointer = std::shared_ptr<Frame>;
    using FrameHandler = std::function<void(Frame& frame)>;

    using Stamp = int;
    using uint32 = uint32_t;
    using int32 = int32_t;
    using uint16 = uint16_t;
    using int16 = int16_t;
    using uint8 = uint8_t;
    using int8 = int8_t;

    using Byte = uint8;
    using Size = size_t;
    using Offset = size_t;
    using Offsets = std::vector<Offset>;

    using Mat4 = glm::mat4;
    using Mat3 = glm::mat3;
    using Vec4 = glm::vec4;
    using Vec4i = glm::ivec4;
    using Vec3 = glm::vec3;
    using Vec2 = glm::vec2;
    using Vec2i = glm::ivec2;
    using Vec2u = glm::uvec2;
    using Vec3u = glm::uvec3;
    using Vec3u = glm::uvec3;

    class Element;
    using Format = Element;
    class Swapchain;
    using SwapchainPointer = std::shared_ptr<Swapchain>;
    class Framebuffer;
    using FramebufferPointer = std::shared_ptr<Framebuffer>;
    class Pipeline;
    using PipelinePointer = std::shared_ptr<Pipeline>;
    using Pipelines = std::vector<PipelinePointer>;
    class Query;
    using QueryPointer = std::shared_ptr<Query>;
    using Queries = std::vector<QueryPointer>;
    class Resource;
    class Buffer;
    using BufferPointer = std::shared_ptr<Buffer>;
    using Buffers = std::vector<BufferPointer>;
    class BufferView;
    class Shader;
    using ShaderPointer = std::shared_ptr<Shader>;
    using Shaders = std::vector<ShaderPointer>;
    class State;
    using StatePointer = std::shared_ptr<State>;
    using States = std::vector<StatePointer>;
    class Stream;
    class BufferStream;
    using BufferStreamPointer = std::shared_ptr<BufferStream>;
    class Texture;
    class SphericalHarmonics;
    using SHPointer = std::shared_ptr<SphericalHarmonics>;
    class Sampler;
    class Texture;
    using TexturePointer = std::shared_ptr<Texture>;
    using Textures = std::vector<TexturePointer>;
    class TextureView;
    using TextureViews = std::vector<TextureView>;

    namespace gl {
        class GLBuffer;
    }

    namespace gl41 {
        class GL41Backend;
    }

    namespace gl45 {
        class GL45Backend;
    }
}

#endif
