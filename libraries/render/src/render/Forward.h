//
//  Created by Bradley Austin Davis on 2017/06/15
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_render_Forward_h
#define hifi_render_Forward_h

#include <memory>

namespace render {
    class Scene;
    using ScenePointer = std::shared_ptr<Scene>;
    class ShapePipeline;
    class Args;
}

using RenderArgs = render::Args;


#endif // hifi_render_Forward_h
