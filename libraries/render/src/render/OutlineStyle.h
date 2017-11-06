//
//  OutlineStyle.h

//  Created by Olivier Prat on 11/06/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_OutlineStyle_h
#define hifi_render_utils_OutlineStyle_h

#include <glm/vec3.hpp>

#include <string>

namespace render {

    // This holds the configuration for a particular outline style
    class OutlineStyle {
    public:

        bool isFilled() const {
            return unoccludedFillOpacity > 5e-3f || occludedFillOpacity > 5e-3f;
        }

        glm::vec3 color{ 1.f, 0.7f, 0.2f };
        float width{ 2.0f };
        float intensity{ 0.9f };
        float unoccludedFillOpacity{ 0.0f };
        float occludedFillOpacity{ 0.0f };
        bool glow{ false };
        std::string selectionName;
    };

}

#endif // hifi_render_utils_OutlineStyle_h