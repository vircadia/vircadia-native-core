//
//  RenderHifi.h
//  libraries/render-utils/src
//
//  Created by Sam Gateau on 5/30/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RenderHifi_h
#define hifi_RenderHifi_h

#include <render/Item.h>



// In the library render-utils we are specializing the generic components of the render library to create the custom hifi render engine
// Objects and types serving this goal are define in the namespace render.hifi
// TODO: extend the namespace to all the classes where it make sense in render-utils
namespace render {
    namespace hifi {

        // Tag is the alias names of render::ItemKey::Tag combinations used in the Hifi Render Engine
        enum Tag : uint8_t {
            TAG_NONE = render::ItemKey::TAG_BITS_NONE,          // No Tags at all
            TAG_MAIN_VIEW = render::ItemKey::TAG_BITS_0,        // Main view
            TAG_SECONDARY_VIEW = render::ItemKey::TAG_BITS_1,   // Secondary View
            TAG_ALL_VIEWS = TAG_MAIN_VIEW | TAG_SECONDARY_VIEW, // All views
        };

        // Layer is the alias names of the render::ItemKey::Layer used in the Hifi Render Engine
        enum Layer : uint8_t {
            LAYER_3D = render::ItemKey::LAYER_DEFAULT,
            LAYER_3D_FRONT = render::ItemKey::LAYER_1,
            LAYER_3D_HUD = render::ItemKey::LAYER_2,
            LAYER_2D = render::ItemKey::LAYER_3,
            LAYER_BACKGROUND = render::ItemKey::LAYER_BACKGROUND,
        };
    }
}

#endif // hifi_RenderHifi_h
