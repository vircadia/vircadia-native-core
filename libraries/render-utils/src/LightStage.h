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
    static const Index INVALID_INDEX;
    static bool isIndexInvalid(Index index) { return index == INVALID_INDEX; }
    
    using LightPointer = model::LightPointer;
    using Lights = render::indexed_container::IndexedPointerVector<model::Light>;
    using LightMap = std::unordered_map<LightPointer, Index>;

    using LightIndices = std::vector<Index>;

    class Shadow {
    public:
        using UniformBufferView = gpu::BufferView;
        static const int MAP_SIZE;

        class Cascade {
            friend Shadow;
        public:

            Cascade();

            gpu::FramebufferPointer framebuffer;
            gpu::TexturePointer map;

            const std::shared_ptr<ViewFrustum>& getFrustum() const { return _frustum; }

            const glm::mat4& getView() const;
            const glm::mat4& getProjection() const;

            void setMinDistance(float value) { _minDistance = value; }
            void setMaxDistance(float value) { _maxDistance = value; }
            float getMinDistance() const { return _minDistance; }
            float getMaxDistance() const { return _maxDistance; }

        private:

            std::shared_ptr<ViewFrustum> _frustum;
            float _minDistance;
            float _maxDistance;

            float computeFarDistance(const ViewFrustum& viewFrustum, const Transform& shadowViewInverse,
                                     float left, float right, float bottom, float top, float viewMaxShadowDistance) const;
        };

        Shadow(model::LightPointer light, float maxDistance, unsigned int cascadeCount = 1);

        void setKeylightFrustum(const ViewFrustum& viewFrustum,
                                float nearDepth = 1.0f, float farDepth = 1000.0f);
        void setKeylightCascadeFrustum(unsigned int cascadeIndex, const ViewFrustum& viewFrustum,
                                float nearDepth = 1.0f, float farDepth = 1000.0f);
        void setCascadeFrustum(unsigned int cascadeIndex, const ViewFrustum& shadowFrustum);

        const UniformBufferView& getBuffer() const { return _schemaBuffer; }

        unsigned int getCascadeCount() const { return (unsigned int)_cascades.size(); }
        const Cascade& getCascade(unsigned int index) const { return _cascades[index]; }

        float getMaxDistance() const { return _maxDistance; }
        void setMaxDistance(float value);

        const model::LightPointer& getLight() const { return _light; }

    protected:

#include "Shadows_shared.slh"

        using Cascades = std::vector<Cascade>;

        static const glm::mat4 _biasMatrix;

        model::LightPointer _light;
        float _maxDistance;
        Cascades _cascades;

        class Schema : public ShadowParameters {
        public:

            Schema();

        };
        UniformBufferView _schemaBuffer = nullptr;
    };

    using ShadowPointer = std::shared_ptr<Shadow>;
    using Shadows = render::indexed_container::IndexedPointerVector<Shadow>;

    Index findLight(const LightPointer& light) const;
    Index addLight(const LightPointer& light);

    Index addShadow(Index lightIndex, float maxDistance = 20.0f, unsigned int cascadeCount = 1U);

    LightPointer removeLight(Index index);
    
    bool checkLightId(Index index) const { return _lights.checkIndex(index); }

    Index getNumLights() const { return _lights.getNumElements(); }
    Index getNumFreeLights() const { return _lights.getNumFreeIndices(); }
    Index getNumAllocatedLights() const { return _lights.getNumAllocatedIndices(); }

    LightPointer getLight(Index lightId) const {
        return _lights.get(lightId);
    }

    Index getShadowId(Index lightId) const;

    ShadowPointer getShadow(Index lightId) const {
        return _shadows.get(getShadowId(lightId));
    }

    using LightAndShadow = std::pair<LightPointer, ShadowPointer>;
    LightAndShadow getLightAndShadow(Index lightId) const {
        auto light = getLight(lightId);
        auto shadow = getShadow(lightId);
        assert(shadow == nullptr || shadow->getLight() == light);
        return LightAndShadow(light, shadow);
    }

    LightPointer getCurrentKeyLight() const;
    LightPointer getCurrentAmbientLight() const;
    ShadowPointer getCurrentKeyShadow() const;
    LightAndShadow getCurrentKeyLightAndShadow() const;

    LightStage();

    gpu::BufferPointer getLightArrayBuffer() const { return _lightArrayBuffer; }
    void updateLightArrayBuffer(Index lightId);

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
    
protected:

    struct Desc {
        Index shadowId{ INVALID_INDEX };
    };
    using Descs = std::vector<Desc>;

    gpu::BufferPointer _lightArrayBuffer;

    Lights _lights;
    Shadows _shadows;
    Descs _descs;
    LightMap _lightMap;

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
