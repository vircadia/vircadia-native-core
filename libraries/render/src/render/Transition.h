//
//  Transition.h

//  Created by Olivier Prat on 07/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Transition_h
#define hifi_render_Transition_h

#include "Item.h"

namespace render {

    // This holds the current state for all transition event types applied to a render item
    class Transition {
    public:

        enum Type {
            ELEMENT_ENTER_DOMAIN = 0,
            ELEMENT_LEAVE_DOMAIN,
            BUBBLE_ISECT_OWNER,
            BUBBLE_ISECT_TRESPASSER,
            USER_ENTER_DOMAIN,
            USER_LEAVE_DOMAIN,
            AVATAR_CHANGE,

            // Don't forget to modify Fade.slh to reflect the change in number of categories
            TYPE_COUNT,
            NONE = TYPE_COUNT
        };

        Type eventType{ ELEMENT_ENTER_DOMAIN };
        ItemID itemId{ Item::INVALID_ITEM_ID };
        ItemID boundItemId{ Item::INVALID_ITEM_ID };
        double time{ 0.0 };
        glm::vec3 noiseOffset{ 0.f, 0.f, 0.f };
        glm::vec3 baseOffset{ 0.f, 0.f, 0.f };
        glm::vec3 baseInvSize{ 1.f, 1.f, 1.f };
        float threshold{ 0.f };
        uint8_t isFinished{ 0 };
    };

    typedef std::shared_ptr<Transition> TransitionPointer;
    typedef std::vector<Transition::Type> TransitionTypes;
}

#endif // hifi_render_Transition_h