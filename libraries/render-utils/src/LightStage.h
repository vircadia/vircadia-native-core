//
//  LightStage.h
//  render-utils/src
//
//  Created by Zach Pomerantz on 1/14/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_LightStage_h
#define hifi_render_utils_LightStage_h

#include "gpu/Framebuffer.h"

#include "model/Light.h"

class ViewFrustum;

// Light stage to set up light-related rendering tasks
class LightStage {
public:
    class Shadow {
    public:
        using UniformBufferView = gpu::BufferView;
        static const int MAP_SIZE = 1024;

        Shadow(model::LightPointer light);

        void setKeylightFrustum(const ViewFrustum& viewFrustum, float nearDepth, float farDepth);

        const std::shared_ptr<ViewFrustum> getFrustum() const { return _frustum; }

        const glm::mat4& getView() const;
        const glm::mat4& getProjection() const;

        const UniformBufferView& getBuffer() const { return _schemaBuffer; }

        gpu::FramebufferPointer framebuffer;
        gpu::TexturePointer map;
    protected:
        model::LightPointer _light;
        std::shared_ptr<ViewFrustum> _frustum;

        class Schema {
        public:
            glm::mat4 projection;
            glm::mat4 viewInverse;

            glm::float32 bias = 0.005f;
            glm::float32 scale = 1 / MAP_SIZE;
        };
        UniformBufferView _schemaBuffer = nullptr;
        
        friend class Light;
    };
    using ShadowPointer = std::shared_ptr<Shadow>;

    class Light {
    public:
        Light(Shadow&& shadow) : shadow{ shadow } {}

        model::LightPointer light;
        Shadow shadow;
    };
    using LightPointer = std::shared_ptr<Light>;
    using Lights = std::vector<LightPointer>;

    const LightPointer addLight(model::LightPointer light);
    // TODO: removeLight

    Lights lights;
};

#endif
