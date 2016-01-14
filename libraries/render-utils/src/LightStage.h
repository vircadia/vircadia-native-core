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
        const int MAP_SIZE = 2048;

        Shadow(model::LightPointer light);

        void setKeylightFrustum(ViewFrustum* viewFrustum, float zBack, float zFront);

        const std::shared_ptr<ViewFrustum> getFrustum() const { return _frustum; }
        const glm::mat4& getProjection() const { return _projection; }
        const Transform& getView() const { return _view; }

        gpu::FramebufferPointer framebuffer;
        gpu::TexturePointer map;
    protected:
        model::LightPointer _light;
        std::shared_ptr<ViewFrustum> _frustum;
        glm::mat4 _projection;
        Transform _view;
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
