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
#include <gpu/Resource.h>

#include "Transform.h"
#include "NumericalConstants.h"

namespace model {
    // Haze range is defined here as the range the visibility is reduced by 95%
    // Haze altitude is defined here as the altitude (above 0) that the haze is reduced by 95%

    class Haze {
    public:
        // Initial values
        static const float INITIAL_HAZE_RANGE;
        static const float INITIAL_HAZE_HEIGHT;

        static const float INITIAL_KEY_LIGHT_RANGE;
        static const float INITIAL_KEY_LIGHT_ALTITUDE;

        static const float INITIAL_HAZE_BACKGROUND_BLEND;

        static const glm::vec3 INITIAL_HAZE_COLOR;

        static const float INITIAL_HAZE_GLARE_ANGLE;

        static const glm::vec3 INITIAL_HAZE_GLARE_COLOR;

        static const float INITIAL_HAZE_BASE_REFERENCE;

        static const float LOG_P_005;
        static const float LOG_P_05;

        // Derivation (d is distance, b is haze coefficient, f is attenuation, solve for f = 0.05
        //  f = exp(-d * b)
        //  ln(f) = -d * b
        //  b = -ln(f)/d
        static inline glm::vec3 convertHazeRangeToHazeRangeFactor(const glm::vec3 hazeRange) {
            return glm::vec3(
                -LOG_P_005 / hazeRange.x,
                -LOG_P_005 / hazeRange.y,
                -LOG_P_005 / hazeRange.z);
        }

        // limit range and altitude to no less than 1.0 metres
        static inline float convertHazeRangeToHazeRangeFactor(const float hazeRange) { return -LOG_P_005 / glm::max(hazeRange, 1.0f); }

        static inline float convertHazeAltitudeToHazeAltitudeFactor(const float hazeHeight) { return -LOG_P_005 / glm::max(hazeHeight, 1.0f); }

        // Derivation (s is the proportion of sun blend, a is the angle at which the blend is 50%, solve for m = 0.5
        //  s = dot(lookAngle, sunAngle) = cos(a)
        //  m = pow(s, p)
        //  log(m) = p * log(s)
        //  p = log(m) / log(s)
        // limit to 0.1 degrees
        static inline float convertGlareAngleToPower(const float hazeGlareAngle) {
            const float GLARE_ANGLE_LIMIT = 0.1f;
            return LOG_P_05 / logf(cosf(RADIANS_PER_DEGREE * glm::max(GLARE_ANGLE_LIMIT, hazeGlareAngle)));
        }

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

        using UniformBufferView = gpu::BufferView;
        UniformBufferView getHazeParametersBuffer() const { return _hazeParametersBuffer; }

    protected:
        class Parameters {
        public:
            // DO NOT CHANGE ORDER HERE WITHOUT UNDERSTANDING THE std140 LAYOUT
            glm::vec3 hazeColor{ INITIAL_HAZE_COLOR };
            float hazeGlareBlend{ convertGlareAngleToPower(INITIAL_HAZE_GLARE_ANGLE) };

            glm::vec3 hazeGlareColor{ INITIAL_HAZE_GLARE_COLOR };
            float hazeBaseReference{ INITIAL_HAZE_BASE_REFERENCE };

            glm::vec3 colorModulationFactor;
            int hazeMode{ 0 };    // bit 0 - set to activate haze attenuation of fragment color
                                  // bit 1 - set to add the effect of altitude to the haze attenuation
                                  // bit 2 - set to activate directional light attenuation mode
                                  // bit 3 - set to blend between blend-in and blend-out colours

            glm::mat4 zoneTransform;

            // Amount of background (skybox) to display, overriding the haze effect for the background
            float hazeBackgroundBlend{ INITIAL_HAZE_BACKGROUND_BLEND };

            // The haze attenuation exponents used by both fragment and directional light attenuation
            float hazeRangeFactor{ convertHazeRangeToHazeRangeFactor(INITIAL_HAZE_RANGE) };
            float hazeHeightFactor{ convertHazeAltitudeToHazeAltitudeFactor(INITIAL_HAZE_HEIGHT) };

            float hazeKeyLightRangeFactor{ convertHazeRangeToHazeRangeFactor(INITIAL_KEY_LIGHT_RANGE) };
            float hazeKeyLightAltitudeFactor{ convertHazeAltitudeToHazeAltitudeFactor(INITIAL_KEY_LIGHT_ALTITUDE) };

            Parameters() {}
        };

        UniformBufferView _hazeParametersBuffer { nullptr };
    };

    using HazePointer = std::shared_ptr<Haze>;
}
#endif // hifi_model_Haze_h
