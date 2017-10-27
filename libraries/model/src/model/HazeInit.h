//
//  MakeHaze.h
//  libraries/model/src/model
//
//  Created by Nissim Hadar on 10/26/2017.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_HazeInit_h
#define hifi_model_HazeInit_h

namespace model {
    const float LOG_P_005 = logf(0.05f);
    const float LOG_P_05 = logf(0.5f);

    // Derivation (d is distance, b is haze coefficient, f is attenuation, solve for f = 0.05
    //  f = exp(-d * b)
    //  ln(f) = -d * b
    //  b = -ln(f)/d
    inline glm::vec3 convertHazeRangeToHazeRangeFactor(const glm::vec3 hazeRange_m) { 
        return glm::vec3(
            -LOG_P_005 / hazeRange_m.x,
            -LOG_P_005 / hazeRange_m.y,
            -LOG_P_005 / hazeRange_m.z);
    }

    // limit range and altitude to no less than 1.0 metres
    inline float convertHazeRangeToHazeRangeFactor(const float hazeRange_m) { return -LOG_P_005 / glm::max(hazeRange_m, 1.0f); }

    inline float convertHazeAltitudeToHazeAltitudeFactor(const float hazeHeight_m) { return -LOG_P_005 / glm::max(hazeHeight_m, 1.0f); }

    // Derivation (s is the proportion of sun blend, a is the angle at which the blend is 50%, solve for m = 0.5
    //  s = dot(lookAngle, sunAngle) = cos(a)
    //  m = pow(s, p)
    //  log(m) = p * log(s)
    //  p = log(m) / log(s)
    // limit to 0.1 degrees
    inline float convertGlareAngleToPower(const float hazeGlareAngle) {
        const float GLARE_ANGLE_LIMIT = 0.1f;
        return LOG_P_05 / logf(cosf(RADIANS_PER_DEGREE * glm::max(GLARE_ANGLE_LIMIT, hazeGlareAngle)));
    }

    const float initialHazeRange_m{ 1000.0f };
    const float initialHazeHeight_m{ 200.0f };

    const float initialHazeKeyLightRange_m{ 1000.0f };
    const float initialHazeKeyLightAltitude_m{ 200.0f };

    const float initialHazeBackgroundBlend{ 0.0f };

    const glm::vec3 initialColorModulationFactor{
      convertHazeRangeToHazeRangeFactor(initialHazeRange_m),
      convertHazeRangeToHazeRangeFactor(initialHazeRange_m),
      convertHazeRangeToHazeRangeFactor(initialHazeRange_m)
    };

    const glm::vec3 initialHazeColor{ 0.5f, 0.6f, 0.7f }; // Bluish
    const xColor initialHazeColorXcolor{ 128, 154, 179 };

    const float initialGlareAngle_degs{ 20.0f };

    const glm::vec3 initialHazeGlareColor{ 1.0f, 0.9f, 0.7f };
    const xColor initialHazeGlareColorXcolor{ 255, 229, 179 };

    const float initialHazeBaseReference_m{ 0.0f };
}
#endif // hifi_model_HazeInit_h
