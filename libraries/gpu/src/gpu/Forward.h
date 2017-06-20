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
#include <mutex>
#include <vector>

#include <glm/glm.hpp>

namespace gpu {
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    class Batch;
    class Backend;
    using BackendPointer = std::shared_ptr<Backend>;
    class Context;
    using ContextPointer = std::shared_ptr<Context>;
    class GPUObject;
    class Frame;
    using FramePointer = std::shared_ptr<Frame>;

    using Stamp = int;
    using uint32 = uint32_t;
    using int32 = int32_t;
    using uint16 = uint16_t;
    using int16 = int16_t;
    using uint8 = uint8_t;
    using int8 = int8_t;

    using Byte = uint8;
    using Size = size_t;
    static const Size INVALID_SIZE = (Size)-1;

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
    using TextureWeakPointer = std::weak_ptr<Texture>;
    using Textures = std::vector<TexturePointer>;
    class TextureView;
    using TextureViews = std::vector<TextureView>;

    struct StereoState {
        bool isStereo() const {
            return _enable && !_contextDisable;
        }
        bool _enable{ false };
        bool _contextDisable { false };
        bool _skybox{ false };
        // 0 for left eye, 1 for right eye
        uint8 _pass{ 0 };
        Mat4 _eyeViews[2];
        Mat4 _eyeProjections[2];
    };

    class GPUObject {
    public:
        virtual ~GPUObject() = default;
    };

    class GPUObjectPointer {
    private:
        using GPUObjectUniquePointer = std::unique_ptr<GPUObject>;

        // This shouldn't be used by anything else than the Backend class with the proper casting.
        mutable GPUObjectUniquePointer _gpuObject;
        void setGPUObject(GPUObject* gpuObject) const { _gpuObject.reset(gpuObject); }
        GPUObject* getGPUObject() const { return _gpuObject.get(); }

        friend class Backend;
        friend class Texture;
    };

    namespace gl {
        class GLBackend;
        class GLBuffer;
    }

    namespace gl41 {
        class GL41Backend;
        class GL41Buffer;
    }

    namespace gl45 {
        class GL45Backend;
        class GL45Buffer;
    }
}

#endif
