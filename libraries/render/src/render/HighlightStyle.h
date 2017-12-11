//
//  HighlightStyle.h

//  Created by Olivier Prat on 11/06/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_HighlightStyle_h
#define hifi_render_utils_HighlightStyle_h

#include <glm/vec3.hpp>

#include <string>

namespace render {

    // This holds the configuration for a particular outline style
    class HighlightStyle {
    public:
        struct RGBA {
            glm::vec3 color{ 1.0f, 0.7f, 0.2f };
            float alpha{ 0.9f };

            RGBA(const glm::vec3& c, float a) : color(c), alpha(a) {}
        };

        RGBA _outlineUnoccluded{ { 1.0f, 0.7f, 0.2f }, 0.9f };
        RGBA _outlineOccluded{ { 1.0f, 0.7f, 0.2f }, 0.9f };
        RGBA _fillUnoccluded{ { 0.2f, 0.7f, 1.0f }, 0.0f };
        RGBA _fillOccluded{ { 0.2f, 0.7f, 1.0f }, 0.0f };

        float _outlineWidth{ 2.0f };
        bool _isOutlineSmooth{ false };

        bool isFilled() const {
            return _fillUnoccluded.alpha > 5e-3f || _fillOccluded.alpha > 5e-3f;
        }
    };

}

#endif // hifi_render_utils_HighlightStyle_h