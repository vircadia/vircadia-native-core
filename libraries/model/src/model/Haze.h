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
#include "NumericalConstants.h"

namespace model {

    const double p_005 = 0.05;

    // Derivation (d is distance, b is haze coefficient, f is attenuation, solve for f = 0.05
    //  f = exp(-d * b)
    //  ln(f) = -d * b
    //  b = -ln(f)/d
    inline float convertHazeRangeToHazeRangeFactor(const float hazeRange_m) { return (float)-log(p_005) / hazeRange_m; }

    inline float convertHazeAltitudeToHazeAltitudeFactor(const float hazeAltitude_m) {
        return (float)-log(p_005) / hazeAltitude_m;
    }

    // Derivation (s is th proportion of sun blend, a is the angle at which the blend is 50%, solve for m = 0.5
    //  s = dot(lookAngle, sunAngle) = cos(a)
    //  m = pow(s, p)
    //  log(m) = p * log(s)
    //  p = log(m) / log(s)
    inline float convertDirectionalLightAngleToPower(const float directionalLightAngle) {
        return log(0.5) / log(cos(RADIANS_PER_DEGREE * directionalLightAngle));
    }

    const glm::vec3 initialHazeColor{ 0.5, 0.6, 0.7 };
    const float initialDirectionalLightAngle_degs{ 30.0f };

    const glm::vec3 initialDirectionalLightColor{ 1.0, 0.9, 0.7 };
    const float initialHazeBaseReference{ 0.0f };

    // Haze range is defined here as the range the visibility is reduced by 95%
    // Haze altitude is defined here as the altitude (above 0) that the haze is reduced by 95%
    const float initialHazeRange_m = 150.0f;
    const float initialHazeAltitude_m = 150.0f;

    const float initialHazeRangeKeyLight_m = 150.0f;
    const float initialHazeAltitudeKeyLight_m = 150.0f;

    const glm::vec3 initialColorModulationFactor{
        convertHazeRangeToHazeRangeFactor(initialHazeRange_m),
        convertHazeRangeToHazeRangeFactor(initialHazeRange_m),
        convertHazeRangeToHazeRangeFactor(initialHazeRange_m)
    };

    class Haze {
    public:
        using UniformBufferView = gpu::BufferView;

        Haze();

        void setHazeColor(const glm::vec3 hazeColor);
        void setDirectionalLightBlend(const float directionalLightBlend);

        void setDirectionalLightColor(const glm::vec3 directionalLightColor);
        void setHazeBaseReference(const float hazeBaseReference);

        void setIsHazeActive(const bool isHazeActive);
        void setIsAltitudeBased(const bool isAltitudeBased);
        void setIsDirectionaLightAttenuationActive(const bool isDirectionaLightAttenuationActive);
        void setIsModulateColorActive(const bool isModulateColorActive);

        void setHazeRangeFactor(const float hazeRange);
        void setHazeAltitudeFactor(const float hazeAltitude);

        void setHazeRangeFactorKeyLight(const float hazeRangeKeyLight);
        void setHazeAltitudeFactorKeyLight(const float hazeAltitudeKeyLight);

        UniformBufferView getParametersBuffer() const { return _parametersBuffer; }

    protected:
        class Parameters {
        public:
            // DO NOT CHANGE ORDER HERE WITHOUT UNDERSTANDING THE std140 LAYOUT
            glm::vec3 hazeColor{ initialHazeColor };
            float directionalLightBlend{ convertDirectionalLightAngleToPower(initialDirectionalLightAngle_degs) };

            glm::vec3 directionalLightColor{ initialDirectionalLightColor };
            float hazeBaseReference{ initialHazeBaseReference };

            glm::vec3 colorModulationFactor{ initialColorModulationFactor };
            int hazeMode{ 0 };    // bit 0 - set to activate haze attenuation of fragment color
                                  // bit 1 - set to add the effect of altitude to the haze attenuation
                                  // bit 2 - set to activate directional light attenuation mode

            // The haze attenuation exponents used by both fragment and directional light attenuation
            float hazeRangeFactor{ convertHazeRangeToHazeRangeFactor(initialHazeRange_m) };
            float hazeAltitudeFactor{ convertHazeAltitudeToHazeAltitudeFactor(initialHazeAltitude_m) };

            float hazeRangeFactorKeyLight{ convertHazeRangeToHazeRangeFactor(initialHazeRangeKeyLight_m) };
            float hazeAltitudeFactorKeyLight{ convertHazeAltitudeToHazeAltitudeFactor(initialHazeAltitudeKeyLight_m) };

            Parameters() {}
        };

        UniformBufferView _parametersBuffer;
    };

    typedef std::shared_ptr<Haze> HazePointer;
}
#endif // hifi_model_Haze_h
