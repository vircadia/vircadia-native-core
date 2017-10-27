//
//  MakeHaze.h
//  libraries/model/src/model
//
//  Created by Nissim Hadar on 9/13/2017.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Haze_h
#define hifi_model_Haze_h

#include <glm/glm.hpp>
#include "Transform.h"
#include "NumericalConstants.h"

#include "HazeInit.h"

namespace model {
    // Haze range is defined here as the range the visibility is reduced by 95%
    // Haze altitude is defined here as the altitude (above 0) that the haze is reduced by 95%

    class Haze {
    public:
        using UniformBufferView = gpu::BufferView;

        Haze();

        void setHazeColor(const glm::vec3 hazeColor);
        void setHazeGlareBlend(const float hazeGlareBlend);

        void setHazeGlareColor(const glm::vec3 hazeGlareColor);
        void setHazeBaseReference(const float hazeBaseReference);

        void setHazeActive(const bool isHazeActive);
        void setAltitudeBased(const bool isAltitudeBased);
        void setHazeAttenuateKeyLight(const bool isHazeAttenuateKeyLight);
        void setModulateColorActive(const bool isModulateColorActive);
        void setHazeEnableGlare(const bool isHazeEnableGlare);

        void setHazeRangeFactor(const float hazeRange);
        void setHazeAltitudeFactor(const float hazeAltitude);

        void setHazeKeyLightRangeFactor(const float hazeKeyLightRange);
        void setHazeKeyLightAltitudeFactor(const float hazeKeyLightAltitude);

        void setHazeBackgroundBlend(const float hazeBackgroundBlend);

        void setZoneTransform(const glm::mat4& zoneTransform);

        UniformBufferView getHazeParametersBuffer() const { return _hazeParametersBuffer; }

    protected:
        class Parameters {
        public:
            // DO NOT CHANGE ORDER HERE WITHOUT UNDERSTANDING THE std140 LAYOUT
            glm::vec3 hazeColor{ initialHazeColor };
            float hazeGlareBlend{ convertGlareAngleToPower(initialGlareAngle_degs) };

            glm::vec3 hazeGlareColor{ initialHazeGlareColor };
            float hazeBaseReference{ initialHazeBaseReference_m };

            glm::vec3 colorModulationFactor{ initialColorModulationFactor };
            int hazeMode{ 0 };    // bit 0 - set to activate haze attenuation of fragment color
                                  // bit 1 - set to add the effect of altitude to the haze attenuation
                                  // bit 2 - set to activate directional light attenuation mode
                                  // bit 3 - set to blend between blend-in and blend-out colours

            glm::mat4 zoneTransform;

            // Amount of background (skybox) to display, overriding the haze effect for the background
            float hazeBackgroundBlend{ initialHazeBackgroundBlend };

            // The haze attenuation exponents used by both fragment and directional light attenuation
            float hazeRangeFactor{ convertHazeRangeToHazeRangeFactor(initialHazeRange_m) };
            float hazeHeightFactor{ convertHazeAltitudeToHazeAltitudeFactor(initialHazeHeight_m) };

            float hazeKeyLightRangeFactor{ convertHazeRangeToHazeRangeFactor(initialHazeKeyLightRange_m) };
            float hazeKeyLightAltitudeFactor{ convertHazeAltitudeToHazeAltitudeFactor(initialHazeKeyLightAltitude_m) };

            Parameters() {}
        };

        UniformBufferView _hazeParametersBuffer;
    };

    using HazePointer = std::shared_ptr<Haze>;
}
#endif // hifi_model_Haze_h
