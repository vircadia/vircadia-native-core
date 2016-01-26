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

class JobConfig;

class RenderContext {
public:
    RenderArgs* args;
    std::shared_ptr<JobConfig> jobConfig{ nullptr };
};
using RenderContextPointer = std::shared_ptr<RenderContext>;

}

#endif // hifi_render_Context_h

