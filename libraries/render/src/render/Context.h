//
//  Context.h
//  render/src/render
//
//  Created by Zach Pomerantz on 1/6/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Context_h
#define hifi_render_Context_h

#include "Scene.h"

namespace render {

class SceneContext {
public:
    ScenePointer _scene;
  
    SceneContext() {}
};
using SceneContextPointer = std::shared_ptr<SceneContext>;

// see examples/utilities/tools/renderEngineDebug.js
const int showDisplayStatusFlag = 1;
const int showNetworkStatusFlag = 2;

class RenderContext {
public:
    class ItemsConfig {
    public:
        class Counter {
        public:
            Counter() {}
            Counter(const Counter& counter) : maxDrawn { counter.maxDrawn } {}

            void setCounts(const Counter& counter) {
                numFeed = counter.numFeed;
                numDrawn = counter.numDrawn;
            };

            int numFeed { 0 };
            int numDrawn { 0 };
            int maxDrawn { -1 };
        };

        class State : public Counter {
        public:
            bool render { true };
            bool cull { true };
            bool sort { true };

            Counter counter{};
        };

        ItemsConfig(State opaqueState, State transparentState, Counter overlay3DCounter)
            : opaque{ opaqueState }, transparent{ transparentState }, overlay3D{ overlay3DCounter } {}
        ItemsConfig() : ItemsConfig{ {}, {}, {} } {}

        // TODO: If member count increases, store counters in a map instead of multiple members
        State opaque{};
        State transparent{};
        Counter overlay3D{};
    };

    class Tone {
    public:
        int toneCurve = 1; // Means just Gamma 2.2 correction
        float exposure = 0.0;
    };
    
    class AmbientOcclusion {
    public:
        int resolutionLevel { 1 };
        float radius { 0.5f }; // radius in meters of the AO effect
        float level { 0.5f }; // Level of the obscrance value
        int numSamples { 11 }; // Num Samples per pixel
        float numSpiralTurns { 7.0f };
        bool ditheringEnabled { true };
        float falloffBias { 0.01f };
        float edgeSharpness { 1.0f };
        int blurRadius { 4 };
        float blurDeviation { 2.5f};

        double gpuTime { 0.0 };
    };

    RenderContext(ItemsConfig items, Tone tone, AmbientOcclusion ao, int drawStatus, bool drawHitEffect, glm::vec4 deferredDebugSize, int deferredDebugMode);
    RenderContext() {};

    void setArgs(RenderArgs* args) { _args = args; }
    RenderArgs* getArgs() { return _args; }
    ItemsConfig& getItemsConfig() { return _items; }
    Tone& getTone() { return _tone; }
    AmbientOcclusion& getAmbientOcclusion() { return _ambientOcclusion; }
    int getDrawStatus() { return _drawStatus; }
    bool getDrawHitEffect() { return _drawHitEffect; }
    bool getOcclusionStatus() { return _occlusionStatus; }
    bool getFxaaStatus() { return _fxaaStatus; }
    bool getShadowMapStatus() { return _shadowMapStatus; }
    void setOptions(bool occlusion, bool fxaa, bool showOwned, bool shadowMap);

    // Debugging
    int _deferredDebugMode;
    glm::vec4 _deferredDebugSize;

protected:
    RenderArgs* _args;

    // Options
    int _drawStatus; // bitflag
    bool _drawHitEffect;
    bool _occlusionStatus { false };
    bool _fxaaStatus { false };
    bool _shadowMapStatus { false };

    ItemsConfig _items;
    Tone _tone;
    AmbientOcclusion _ambientOcclusion;
};
typedef std::shared_ptr<RenderContext> RenderContextPointer;

}

#endif // hifi_render_Context_h

