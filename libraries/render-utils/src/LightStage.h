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

#include <set>
#include <unordered_map>

#include <gpu/Framebuffer.h>

#include <model/Light.h>

#include <render/IndexedContainer.h>
#include <render/Stage.h>
#include <render/Engine.h>

class ViewFrustum;

// Light stage to set up light-related rendering tasks
class LightStage : public render::Stage {
public:
    static std::string _stageName;
    static const std::string& getName() { return _stageName; }

    using Index = render::indexed_container::Index;
    static const Index INVALID_INDEX { render::indexed_container::INVALID_INDEX };
    static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }
    
    using LightPointer = model::LightPointer;
    using Lights = render::indexed_container::IndexedPointerVector<model::Light>;
    using LightMap = std::unordered_map<LightPointer, Index>;

    using LightIndices = std::vector<Index>;

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
    using Shadows = render::indexed_container::IndexedPointerVector<Shadow>;

    struct Desc {
        Index shadowId { INVALID_INDEX };
    };
    using Descs = std::vector<Desc>;

    Index findLight(const LightPointer& light) const;
    Index addLight(const LightPointer& light);

    Index addShadow(Index lightIndex);

    LightPointer removeLight(Index index);
    
    bool checkLightId(Index index) const { return _lights.checkIndex(index); }

    Index getNumLights() const { return _lights.getNumElements(); }
    Index getNumFreeLights() const { return _lights.getNumFreeIndices(); }
    Index getNumAllocatedLights() const { return _lights.getNumAllocatedIndices(); }

    LightPointer getLight(Index lightId) const {
        return _lights.get(lightId);
    }

    Index getShadowId(Index lightId) const {
        if (checkLightId(lightId)) {
            return _descs[lightId].shadowId;
        } else {
            return INVALID_INDEX;
        }
    }
    ShadowPointer getShadow(Index lightId) const {
        return _shadows.get(getShadowId(lightId));
    }

    using LightAndShadow = std::pair<LightPointer, ShadowPointer>;
    LightAndShadow getLightAndShadow(Index lightId) const {
        return LightAndShadow(getLight(lightId), getShadow(lightId));
    }

    LightStage();
    Lights _lights;
    LightMap _lightMap;
    Descs _descs;

    class Frame {
    public:
        Frame() {}
        
        void clear() { _pointLights.clear(); _spotLights.clear(); _sunLights.clear(); _ambientLights.clear(); }
        void pushLight(LightStage::Index index, model::Light::Type type) {
            switch (type) {
                case model::Light::POINT: { pushPointLight(index); break; }
                case model::Light::SPOT: { pushSpotLight(index); break; }
                case model::Light::SUN: { pushSunLight(index); break; }
                case model::Light::AMBIENT: { pushAmbientLight(index); break; }
                default: { break; }
            }
        }
        void pushPointLight(LightStage::Index index) { _pointLights.emplace_back(index); }
        void pushSpotLight(LightStage::Index index) { _spotLights.emplace_back(index); }
        void pushSunLight(LightStage::Index index) { _sunLights.emplace_back(index); }
        void pushAmbientLight(LightStage::Index index) { _ambientLights.emplace_back(index); }

        LightStage::LightIndices _pointLights;
        LightStage::LightIndices _spotLights;
        LightStage::LightIndices _sunLights;
        LightStage::LightIndices _ambientLights;
    };
    
    Frame _currentFrame;
    
    gpu::BufferPointer _lightArrayBuffer;
    void updateLightArrayBuffer(Index lightId);

    Shadows _shadows;
};
using LightStagePointer = std::shared_ptr<LightStage>;



class LightStageSetup {
public:
    using JobModel = render::Job::Model<LightStageSetup>;

    LightStageSetup();
    void run(const render::RenderContextPointer& renderContext);

protected:
};


#endif
