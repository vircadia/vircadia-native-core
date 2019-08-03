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

#include <graphics/Light.h>

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
    
    using LightPointer = graphics::LightPointer;
    using Lights = render::indexed_container::IndexedPointerVector<graphics::Light>;
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

        Shadow(graphics::LightPointer light, unsigned int cascadeCount = 1);

        void setLight(graphics::LightPointer light);

        void setKeylightFrustum(const ViewFrustum& viewFrustum,
                                float nearDepth = 1.0f, float farDepth = 1000.0f);
        void setKeylightCascadeFrustum(unsigned int cascadeIndex, const ViewFrustum& viewFrustum,
                                float nearDepth = 1.0f, float farDepth = 1000.0f);
        void setKeylightCascadeBias(unsigned int cascadeIndex, float constantBias, float slopeBias);
        void setCascadeFrustum(unsigned int cascadeIndex, const ViewFrustum& shadowFrustum);

        const UniformBufferView& getBuffer() const { return _schemaBuffer; }

        unsigned int getCascadeCount() const { return (unsigned int)_cascades.size(); }
        const Cascade& getCascade(unsigned int index) const { return _cascades[index]; }

        float getMaxDistance() const { return _maxDistance; }
        void setMaxDistance(float value);

        const graphics::LightPointer& getLight() const { return _light; }

        gpu::TexturePointer map;
#include "Shadows_shared.slh"
        class Schema : public ShadowParameters {
        public:

            Schema();

        };
    protected:

        using Cascades = std::vector<Cascade>;

        static const glm::mat4 _biasMatrix;

        graphics::LightPointer _light;
        float _maxDistance{ 0.0f };
        Cascades _cascades;

        UniformBufferView _schemaBuffer = nullptr;
    };

    using ShadowPointer = std::shared_ptr<Shadow>;

    Index findLight(const LightPointer& light) const;
    Index addLight(const LightPointer& light, const bool shouldSetAsDefault = false);
    
    Index getDefaultLight() { return _defaultLightId; }

    LightPointer removeLight(Index index);
    
    bool checkLightId(Index index) const { return _lights.checkIndex(index); }

    Index getNumLights() const { return _lights.getNumElements(); }
    Index getNumFreeLights() const { return _lights.getNumFreeIndices(); }
    Index getNumAllocatedLights() const { return _lights.getNumAllocatedIndices(); }

    LightPointer getLight(Index lightId) const { return _lights.get(lightId); }

    LightStage();

    gpu::BufferPointer getLightArrayBuffer() const { return _lightArrayBuffer; }
    void updateLightArrayBuffer(Index lightId);

    class Frame {
    public:
        Frame() {}
        
        void clear() { _pointLights.clear(); _spotLights.clear(); _sunLights.clear(); _ambientLights.clear(); }
        void pushLight(LightStage::Index index, graphics::Light::Type type) {
            switch (type) {
                case graphics::Light::POINT: { pushPointLight(index); break; }
                case graphics::Light::SPOT: { pushSpotLight(index); break; }
                case graphics::Light::SUN: { pushSunLight(index); break; }
                case graphics::Light::AMBIENT: { pushAmbientLight(index); break; }
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
    using FramePointer = std::shared_ptr<Frame>;
    
    class ShadowFrame {
    public:
        ShadowFrame() {}
        
        void clear() {}
        
        using Object = ShadowPointer;
        using Objects = std::vector<Object>;

        void pushShadow(const ShadowPointer& shadow) {
            _objects.emplace_back(shadow);
        }


        Objects _objects;
    };
    using ShadowFramePointer = std::shared_ptr<ShadowFrame>;

    Frame _currentFrame;
    
    Index getAmbientOffLight() { return _ambientOffLightId; }
    Index getPointOffLight() { return _pointOffLightId; }
    Index getSpotOffLight() { return _spotOffLightId; }
    Index getSunOffLight() { return _sunOffLightId; }

    LightPointer getCurrentKeyLight(const LightStage::Frame& frame) const;
    LightPointer getCurrentAmbientLight(const LightStage::Frame& frame) const;

protected:

    struct Desc {
        Index shadowId{ INVALID_INDEX };
    };
    using Descs = std::vector<Desc>;

    gpu::BufferPointer _lightArrayBuffer;

    Lights _lights;
    Descs _descs;
    LightMap _lightMap;

    // define off lights
    Index _ambientOffLightId;
    Index _pointOffLightId;
    Index _spotOffLightId;
    Index _sunOffLightId;

    Index _defaultLightId;

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
