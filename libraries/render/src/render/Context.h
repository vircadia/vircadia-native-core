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

class JobConfig;

class RenderContext {
public:
    RenderContext(int drawStatus);
    RenderContext() {};

    void setArgs(RenderArgs* args) { _args = args; }
    RenderArgs* getArgs() { return _args; }
    int getDrawStatus() { return _drawStatus; }
    bool getOcclusionStatus() { return _occlusionStatus; }
    bool getFxaaStatus() { return _fxaaStatus; }
    bool getShadowMapStatus() { return _shadowMapStatus; }
    void setOptions(bool occlusion, bool fxaa, bool showOwned, bool shadowMap);

    std::shared_ptr<JobConfig> jobConfig{ nullptr };
protected:
    RenderArgs* _args;

    // Options
    int _drawStatus; // bitflag
    bool _occlusionStatus { false };
    bool _fxaaStatus { false };
    bool _shadowMapStatus { false };
};
typedef std::shared_ptr<RenderContext> RenderContextPointer;

}

#endif // hifi_render_Context_h

