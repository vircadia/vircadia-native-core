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
#include <vector>
#include <set>
#include <cstdint>

namespace render {
    class Args;

    using ItemID = uint32_t;
    using ItemCell = int32_t;

    // A few typedefs for standard containers of ItemIDs
    using ItemIDs = std::vector<ItemID>;
    using ItemIDSet = std::set<ItemID>;

    class Scene;
    using ScenePointer = std::shared_ptr<Scene>;
    class ShapePipeline;
}

using RenderArgs = render::Args;


#endif // hifi_render_Forward_h
